/******************************************************************************
Este arquivo eh parte da implementacao do ambiente declarativo do middleware
Ginga (Ginga-NCL).

Direitos Autorais Reservados (c) 1989-2007 PUC-Rio/Laboratorio TeleMidia

Este programa eh software livre; voce pode redistribui-lo e/ou modificah-lo sob
os termos da Licenca Publica Geral GNU versao 2 conforme publicada pela Free
Software Foundation.

Este programa eh distribuido na expectativa de que seja util, porem, SEM
NENHUMA GARANTIA; nem mesmo a garantia implicita de COMERCIABILIDADE OU
ADEQUACAO A UMA FINALIDADE ESPECIFICA. Consulte a Licenca Publica Geral do
GNU versao 2 para mais detalhes.

Voce deve ter recebido uma copia da Licenca Publica Geral do GNU versao 2 junto
com este programa; se nao, escreva para a Free Software Foundation, Inc., no
endereco 59 Temple Street, Suite 330, Boston, MA 02111-1307 USA.

Para maiores informacoes:
ncl @ telemidia.puc-rio.br
http://www.ncl.org.br
http://www.ginga.org.br
http://www.telemidia.puc-rio.br
******************************************************************************
This file is part of the declarative environment of middleware Ginga (Ginga-NCL)

Copyright: 1989-2007 PUC-RIO/LABORATORIO TELEMIDIA, All Rights Reserved.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License version 2 as published by
the Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License version 2 for more
details.

You should have received a copy of the GNU General Public License version 2
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

For further information contact:
ncl @ telemidia.puc-rio.br
http://www.ncl.org.br
http://www.ginga.org.br
http://www.telemidia.puc-rio.br
*******************************************************************************/

#include "util/Color.h"

#include "mb/LocalScreenManager.h"
#include "mb/interface/dfb/output/DFBWindow.h"
#include "mb/interface/dfb/output/DFBSurface.h"

#include <stdlib.h>

/* macro for a safe call to DirectFB functions */
#ifdef DFBCHECK
#undef DFBCHECK
#endif

#ifndef DFBCHECK
/*
#define DFBCHECK(x...)                                            \
{                                                                 \
	DFBResult err = x;                                            \
	if (err != DFB_OK) {                                          \
		fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ );    \
		DirectFBErrorFatal( #x, err );                            \
	}                                                             \
}
*/
#define DFBCHECK(x...)                                            \
{                                                                 \
	DFBResult err = x;                                            \
	if (err != DFB_OK) {                                          \
		fprintf( stderr, "%s <%d>: \n\t", __FILE__, __LINE__ );   \
		DirectFBError( #x, err );                                 \
	}                                                             \
}
#endif /*DFBCHECK*/

struct DynamicRender {
	IDirectFBSurface* destination;
	IDirectFBSurface* frame;
};

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace mb {
	DFBWindow::DFBWindow(
			GingaWindowID underlyingWindowID,
			GingaWindowID parentWindowID,
			GingaScreenID screenId,
			int x, int y, int width, int height) {

		initialize(
				underlyingWindowID,
				parentWindowID,
				screenId,
				x, y, width, height);

		if (underlyingWindowID != NULL) {
			draw();
		}
	}

	DFBWindow::~DFBWindow() {
		ISurface* surface;
		vector<ISurface*>::iterator i;

		lock();
		lockChilds();
		if (releaseListener != NULL) {
			releaseListener->setParent(NULL);
		}

		if (childSurfaces != NULL) {
			i = childSurfaces->begin();
			while (i != childSurfaces->end()) {
				surface = *i;
				if (surface != NULL) {
					surface->setParent(NULL);
				}
				++i;
			}
			delete childSurfaces;
			childSurfaces = NULL;
		}
		unlockChilds();

		if (winSur != NULL) {
			LocalScreenManager::getInstance()->releaseSurface(myScreen, winSur);
			winSur = NULL;
		}

		if (win != NULL) {
			LocalScreenManager::getInstance()->releaseWindow(myScreen, win);
			win = NULL;
		}
		unlock();

		pthread_mutex_destroy(&mutex);
		pthread_mutex_destroy(&mutexC);

		clog << "DFBWindow::~DFBWindow(" << this << ") all done" << endl;
	}

	void DFBWindow::initialize(
			GingaWindowID underlyingWindowID,
			GingaWindowID parentWindowID,
			GingaScreenID screenId,
			int x, int y, int w, int h) {

		if (underlyingWindowID != NULL) {
			this->windowId    = (DFBWindowID)(unsigned long)underlyingWindowID;

		} else {
			this->windowId    = -1;
		}

		if (parentWindowID != NULL) {
			this->parentId    = (DFBWindowID)(unsigned long)parentWindowID;

		} else {
			this->parentId    = -1;
		}

		this->win             = NULL;
		this->winSur          = NULL;
		this->myScreen        = screenId;

		this->x               = x;
		this->y               = y;
		this->width           = w;
		this->height          = h;
		this->ghost           = false;
		this->visible         = false;
		this->r               = -1;
		this->g               = -1;
		this->b               = -1;
		this->alpha           = 0xFF;
		this->childSurfaces   = new vector<ISurface*>;
		this->releaseListener = NULL;
		this->fit             = true;
		this->stretch         = true;
		this->caps            = DWCAPS_NODECORATION;
		transparencyValue     = 0x00;

		pthread_mutex_init(&mutex, NULL);
		pthread_mutex_init(&mutexC, NULL);
	}

	GingaScreenID DFBWindow::getScreen() {
		return myScreen;
	}

	void DFBWindow::revertContent() {
		lock();
		win    = NULL;
		winSur = NULL;
		unlock();
	}

	void DFBWindow::setReleaseListener(ISurface* listener) {
		this->releaseListener = listener;
	}

	int DFBWindow::getCap(string cap) {
		if (cap == "ALL") {
			return DWCAPS_ALL;

		} else if (cap == "NOSTRUCTURE") {
			return DWCAPS_NODECORATION;

		} else if (cap == "ALPHACHANNEL") {
			return DWCAPS_ALPHACHANNEL;

		} else if (cap == "DOUBLEBUFFER") {
			return DWCAPS_DOUBLEBUFFER;

		} else {
			return DWCAPS_NONE;
		}
	}

	void DFBWindow::setCaps(int caps) {
		this->caps = caps;
	}

	void DFBWindow::addCaps(int capability) {
		this->caps = (this->caps | capability);
	}

	int DFBWindow::getCaps() {
		return caps;
	}

	void DFBWindow::draw() {
		if (win != NULL) {
			clog << "DFBWindow::draw Warning! Requesting redraw" << endl;

		} else if (windowId < 0) {
			DFBWindowDescription dsc;

			dsc.flags  = (DFBWindowDescriptionFlags)(
				    DWDESC_POSX |
				    DWDESC_POSY |
				    DWDESC_WIDTH |
				    DWDESC_HEIGHT);

			dsc.posx   = x;
			dsc.posy   = y;
			dsc.width  = width;
			dsc.height = height;

			if (caps > 0) {
				dsc.flags = (DFBWindowDescriptionFlags)(
						dsc.flags | DWDESC_CAPS);

				dsc.caps  = (DFBWindowCapabilities)(caps);
			}

			win = (IDirectFBWindow*)(LocalScreenManager::getInstance()->
					createWindow(myScreen, &dsc));

			DFBCHECK(win->SetOpacity(win, 0x00));
			DFBCHECK(win->GetSurface(win, &winSur));
			DFBCHECK(win->GetID(win, (DFBWindowID*)&windowId));

		} else {
			win = (IDirectFBWindow*)(LocalScreenManager::getInstance()->
					getWindow(
							myScreen,
							(GingaWindowID)(unsigned long)windowId));

			win->GetPosition(win, &x, &y);
			win->GetSize(win, &width, &height);
			DFBCHECK(win->GetSurface(win, &winSur));
			return;
		}

		if (caps & DWCAPS_ALPHACHANNEL) {
			DFBCHECK(win->SetOptions(win, (DFBWindowOptions)DWOP_ALPHACHANNEL));
		}

		setBackgroundColor(r, g, b, alpha);
	}

	void DFBWindow::setBounds(int posX, int posY, int w, int h) {
		this->x      = posX;
		this->y      = posY;
		this->width  = w;
		this->height = h;

		lock();
		if (win != NULL) {
			DFBCHECK(win->SetBounds(win, x, y, width, height));
			unprotectedValidate();
		}
		unlock();
	}

	void DFBWindow::setBackgroundColor(int r, int g, int b, int alpha) {
		//clog << this << ">> DFBWindow::setBackgroundColor" << endl;
		if (win == NULL) {
			return;
		}

		if (winSur != NULL) {
			this->r = r;
			this->g = g;
			this->b = b;
			this->alpha = alpha;

			if (r < 0 || g < 0 || b < 0) {
				if (caps & DWCAPS_ALPHACHANNEL) {
					DFBCHECK(win->SetOptions(win, (DFBWindowOptions)
						    (DWOP_ALPHACHANNEL)));
				}
				DFBCHECK(winSur->SetColor(winSur, 0x00, 0x00, 0x00, 0x00));
				DFBCHECK(winSur->Clear(winSur, 0x00, 0x00, 0x00, 0x00));

			} else {
				//DFBCHECK(win->SetOptions(win, (DFBWindowOptions)(DWOP_OPAQUE_REGION)));
				DFBCHECK(winSur->Clear(winSur, r, g, b, alpha));
				DFBCHECK(winSur->SetColor(winSur, r, g, b, alpha));
				DFBCHECK(winSur->FillRectangle(winSur, 0, 0, width, height));
			}

			DFBCHECK(winSur->FillRectangle(winSur, 0, 0, width, height));
			DFBCHECK(winSur->Flip(winSur, NULL, (DFBSurfaceFlipFlags)0));
		}
	}

	IColor* DFBWindow::getBgColor() {
		return new Color(r, g, b, alpha);
	}

	void DFBWindow::setColorKey(int r, int g, int b) {
		//clog << this << ">> DFBWindow::setColorKey" << endl;
		//lock();
		if (win == NULL) {
			return;
		}

		DFBCHECK(win->SetColorKey(win, r, g, b));
		if (caps & DWCAPS_ALPHACHANNEL) {
			DFBCHECK(win->SetOptions(win, (DFBWindowOptions)
				    (DWOP_COLORKEYING | DWOP_ALPHACHANNEL)));

		} else {
			DFBCHECK(win->SetOptions(
					win, (DFBWindowOptions)(DWOP_COLORKEYING)));
		}

		//unlock();
		//setBackgroundColor(r, g, b, alpha);
	}

	void DFBWindow::moveTo(int posX, int posY) {
		this->x = posX;
		this->y = posY;

		lock();
		if (win != NULL) {
			DFBCHECK(win->MoveTo(win, x, y));
		}
		unlock();
	}

	void DFBWindow::resize(int width, int height) {
		this->width = width;
		this->height = height;

		lock();
		if (win != NULL) {
			DFBCHECK(win->Resize(win, width, height));
		}
		unlock();
	}

	void DFBWindow::setZBoundaries(GingaWindowID lower, GingaWindowID upper) {
		IDirectFBWindow* bWin;

		lock();
		if (win == NULL) {
			unlock();
			return;
		}

		if (upper >= 0) {
			bWin = (IDirectFBWindow*)(LocalScreenManager::getInstance()->
					getWindow(myScreen, upper));

			DFBCHECK(win->PutBelow(win, bWin));
		}

		if (lower >= 0) {
			bWin = (IDirectFBWindow*)(LocalScreenManager::getInstance()->
					getWindow(myScreen, lower));

			DFBCHECK(win->PutAtop(win, bWin));
		}

		unprotectedValidate();
		unlock();
	}

	void DFBWindow::raise() {
		lock();
		if (win != NULL) {
			win->Raise(win);
			unprotectedValidate();
		}
		unlock();
	}

	void DFBWindow::lower() {
		lock();
		if (win != NULL) {
			win->Lower(win);
			unprotectedValidate();
		}
		unlock();
	}

	void DFBWindow::raiseToTop() {
		lock();
		if (win != NULL) {
			win->RaiseToTop(win);
			unprotectedValidate();
		}
		unlock();
	}

	void DFBWindow::lowerToBottom() {
		lock();
		if (win != NULL) {
			win->LowerToBottom(win);
			unprotectedValidate();
		}
		unlock();
	}

	void DFBWindow::setCurrentTransparency(int alpha) {
		if (alpha != 255) {
			this->visible = true;

		} else {
			this->visible = false;
		}

		lock();
		transparencyValue = alpha;
		if (win != NULL) {
			if (alpha == 255 || !ghost) {
				win->SetOpacity(win, (255 - alpha));
			}
		}
		unlock();
	}

	void DFBWindow::setOpaqueRegion(int x1, int y1, int x2, int y2) {
		lock();
		if (win != NULL) {
			win->SetOpaqueRegion(win, x1, y1, x2, y2);
		}
		unlock();
	}

	int DFBWindow::getTransparencyValue() {
		return this->transparencyValue;
	}

	GingaWindowID DFBWindow::getId() {
		return (GingaWindowID)(unsigned long)windowId;
	}

	void DFBWindow::show() {
		this->visible = true;

		if (win != NULL) {
			if (!ghost) {
				win->SetOpacity(win, (255 - transparencyValue));
			}
		}
	}

	void DFBWindow::hide() {
		this->visible = false;

		lock();
		if (win != NULL) {
			win->SetOpacity(win, 0x00);
		}
		unlock();
	}

	int DFBWindow::getX() {
		return this->x;
	}

	int DFBWindow::getY() {
		return this->y;
	}

	int DFBWindow::getW() {
		return this->width;
	}

	int DFBWindow::getH() {
		return this->height;
	}

	void DFBWindow::setX(int x) {
		lock();
		if (win != NULL) {
			DFBCHECK(win->MoveTo(win, x, y));
		}
		unlock();
	}

	void DFBWindow::setY(int y) {
		lock();
		if (win != NULL) {
			DFBCHECK(win->MoveTo(win, x, y));
		}
		unlock();
	}

	void DFBWindow::setW(int w) {
		lock();
		if (win != NULL) {
			DFBCHECK(win->Resize(win, w, height));
		}
		unlock();
	}

	void DFBWindow::setH(int h) {
		lock();
		if (win != NULL) {
			DFBCHECK(win->Resize(win, width, h));
		}
		unlock();
	}

	void* DFBWindow::getContent() {
		return win;
	}
/*
	IDirectFBSurface* DFBWindow::getContentSurface() {
		if (win != NULL) {
			IDirectFBSurface* s;
			DFBCHECK(win->GetSurface(win, &s));
			return s;

		} else {
			return NULL;
		}
	}
*/
	void DFBWindow::setColor(int r, int g, int b, int alpha) {
		if (win != NULL && winSur != NULL) {
			DFBCHECK(winSur->SetColor(winSur, r, g, b, alpha));
		}
	}

	void DFBWindow::setBorder(int r, int g, int b, int alpha, int bWidth) {
		int i;

		//lock();
		if (win != NULL && winSur != NULL) {
			DFBCHECK(winSur->SetColor(winSur, r, g, b, alpha));
			if (bWidth < 0) {
				bWidth = bWidth * -1;
			}

			for (i=0; i < bWidth; i++) {
				DFBCHECK(winSur->DrawRectangle(
						winSur, i, i, width - (2*i), height - (2*i)));
			}

			DFBCHECK(winSur->Flip(winSur, NULL, (DFBSurfaceFlipFlags)0));
		}
		//unlock();
	}

	void DFBWindow::setBorder(IColor* color, int bWidth) {
		setBorder(
			    color->getR(),
			    color->getG(),
			    color->getB(),
			    color->getAlpha(),
			    bWidth);
	}

	void DFBWindow::setGhostWindow(bool ghost) {
		this->ghost = ghost;
	}

	bool DFBWindow::isVisible() {
		return this->visible;
	}

	void DFBWindow::validate() {
		lock();
		unprotectedValidate();
		unlock();
	}

	void DFBWindow::unprotectedValidate() {
		ISurface* surface;

		if (win != NULL && winSur != NULL) {
			if (winSur != NULL) {
				lockChilds();
				if (childSurfaces != NULL && !childSurfaces->empty()) {
					surface = childSurfaces->at(0);
					if (surface != NULL) {
						renderFrom(surface);
					}

				} else {
					DFBCHECK(winSur->Flip(
							winSur, NULL, (DFBSurfaceFlipFlags)0));
				}
				unlockChilds();
			}
		}
	}

	void DFBWindow::addChildSurface(ISurface* s) {
		unsigned int i;
		ISurface* surface;

		lockChilds();
		for (i = 0; i < childSurfaces->size(); i++) {
			surface = childSurfaces->at(i);
			if (surface == s) {
				unlockChilds();
				return;
			}
		}
		childSurfaces->push_back(s);
		unlockChilds();
	}

	bool DFBWindow::removeChildSurface(ISurface* s) {
		unsigned int i;
		vector<ISurface*>::iterator j;
		ISurface* surface;

		lockChilds();
		if (releaseListener == s) {
			releaseListener = NULL;
		}

		if (childSurfaces == NULL) {
			unlockChilds();
			return false;
		}

		for (i = 0; i < childSurfaces->size(); i++) {
			surface = childSurfaces->at(i);
			if (surface == s) {
				j = childSurfaces->begin() + i;
				childSurfaces->erase(j);
				unlockChilds();
				return true;
			}
		}
		unlockChilds();
		return false;
	}

	void DFBWindow::setStretch(bool stretchTo) {
		this->stretch = stretchTo;
	}

	bool DFBWindow::getStretch() {
		return this->stretch;
	}

	void DFBWindow::setFit(bool fitTo) {
		this->fit = fitTo;
	}

	bool DFBWindow::getFit() {
		return this->fit;
	}

	void DFBWindow::clearContent() {
		if (winSur != NULL) {
			DFBCHECK(winSur->Clear(winSur, 0, 0, 0, 0));
		}
	}

	bool DFBWindow::isMine(ISurface* surface) {
		IDirectFBSurface* contentSurface;

		if (win == NULL || winSur == NULL || surface == NULL) {
			return false;
		}

		contentSurface = (IDirectFBSurface*)(surface->getContent());
		if (contentSurface == winSur) {
			return true;
		}
		return false;
	}

	void DFBWindow::renderFrom(string serializedImageUrl) {
		IDirectFB* dfb;
		IDirectFBImageProvider* ip;
		DFBImageDescription imgDsc;
		IDirectFBSurface* destination = NULL;
		DFBSurfaceDescription surDsc;
		IColor* chromaKey = NULL;

		dfb = (IDirectFB*)(LocalScreenManager::getInstance()->getGfxRoot(
				myScreen));

		DFBCHECK(dfb->CreateImageProvider(
				dfb, serializedImageUrl.c_str(), &ip));

		if ((ip->GetImageDescription(ip, &imgDsc) == DFB_OK) &&
			 (ip->GetSurfaceDescription(ip, &surDsc) == DFB_OK)) {

			destination = (IDirectFBSurface*)(
					LocalScreenManager::getInstance()->createSurface(
							myScreen, &surDsc));

			if (imgDsc.caps & DICAPS_ALPHACHANNEL) {
				DFBCHECK(destination->SetBlittingFlags(destination,
					 (DFBSurfaceBlittingFlags)(DSBLIT_BLEND_ALPHACHANNEL)));
			}

			if (imgDsc.caps & DICAPS_COLORKEY) {
				DFBCHECK(destination->SetSrcColorKey(
						destination,
					    imgDsc.colorkey_r,
					    imgDsc.colorkey_g,
					    imgDsc.colorkey_b));

				DFBCHECK(destination->SetBlittingFlags(destination,
					    (DFBSurfaceBlittingFlags)(
					    DSBLIT_BLEND_ALPHACHANNEL |
					    DSBLIT_SRC_COLORKEY)));

			} else if (imgDsc.caps & DICAPS_NONE) {
				DFBCHECK(destination->SetBlittingFlags(destination,
					    (DFBSurfaceBlittingFlags)DSBLIT_NOFX));
			}
		}

		if (destination != NULL) {
			DFBCHECK(ip->RenderTo(ip, (IDirectFBSurface*)(destination), NULL));
			renderFrom(destination);
			LocalScreenManager::getInstance()->releaseSurface(
					myScreen, destination);
		}
	}

	void DFBWindow::renderFrom(ISurface* surface) {
		IDirectFBSurface* contentSurface;

		if (win != NULL && !isMine(surface)) {
			contentSurface = (IDirectFBSurface*)(surface->getContent());
			if (contentSurface == NULL) {
				return;
			}

			renderFrom(contentSurface);
		}
	}

	void DFBWindow::renderFrom(IDirectFBSurface* contentSurface) {
		int w, h;
		ISurface* sur;
		IDirectFBSurface* s2;

		DFBCHECK(contentSurface->GetSize(contentSurface, &w, &h));
		if (winSur != NULL && winSur != contentSurface) {
			DFBCHECK(winSur->Clear(winSur, 0, 0, 0, 0));
			if ((w != width || h != height) && fit) {
				if (stretch) {
					DFBCHECK(winSur->StretchBlit(
							winSur, contentSurface, NULL, NULL));

				} else {
					sur = new DFBSurface(myScreen, width, height);
					s2 = (IDirectFBSurface*)(sur->getContent());

					DFBCHECK(s2->StretchBlit(
						    s2,
						    contentSurface,
						    NULL,
						    NULL));

					DFBCHECK(s2->Flip(
							s2, NULL, (DFBSurfaceFlipFlags)DSFLIP_BLIT));

					DFBCHECK(winSur->Blit(winSur, s2, NULL, 0, 0));
					DFBCHECK(winSur->Flip(
							winSur,
							NULL, (DFBSurfaceFlipFlags) DSFLIP_BLIT));

					delete sur;
				}

			} else {
				DFBCHECK(winSur->Blit(winSur, contentSurface, NULL, 0, 0));
			}
			DFBCHECK(winSur->Flip(winSur, NULL, (DFBSurfaceFlipFlags)0));
		}
	}

	void DFBWindow::blit(IWindow* src) {
		//DFBRectangle rect, *r = NULL;
		IDirectFBWindow* srcWin;
		IDirectFBSurface* srcSur;

		lock();
		if (src != NULL) {
			src->lock();
			srcWin = (IDirectFBWindow*)(src->getContent());
			srcWin->GetSurface(srcWin, &srcSur);

			/*rect.x = src->getX();
			rect.y = src->getY();
			rect.w = src->getW();
			rect.h = src->getH();
			r = &rect;*/

			if (winSur != NULL) {
				DFBCHECK(winSur->SetBlittingFlags(
						winSur,
						(DFBSurfaceBlittingFlags)DSBLIT_BLEND_ALPHACHANNEL));

				DFBCHECK(winSur->Blit(
						winSur, srcSur, NULL, src->getX(), src->getY()));
			}
			src->unlock();
		}
		unlock();
	}

	void DFBWindow::stretchBlit(IWindow* src) {
		//DFBRectangle rect, *r = NULL;
		IDirectFBWindow* srcWin;
		IDirectFBSurface* srcSur;

		lock();
		if (src != NULL) {
			src->lock();
			srcWin = (IDirectFBWindow*)(src->getContent());
			srcWin->GetSurface(srcWin, &srcSur);

			/*rect.x = src->getX();
			rect.y = src->getY();
			rect.w = src->getW();
			rect.h = src->getH();
			r = &rect;*/

			if (winSur != NULL) {
				DFBCHECK(winSur->SetBlittingFlags(
						winSur,
						(DFBSurfaceBlittingFlags)DSBLIT_BLEND_ALPHACHANNEL));

				DFBCHECK(winSur->StretchBlit(winSur, srcSur, NULL, NULL));
			}
			src->unlock();
		}
		unlock();
	}

	string DFBWindow::getDumpFileUri(int quality, int dumpW, int dumpH) {
		string uri, strCmd;

		lock();
		if (winSur == NULL) {
			uri = "";

		} else {
			uri = "/tmp/dump_0000";
			remove((char*)((uri + ".ppm").c_str()));
			remove((char*)((uri + ".jpg").c_str()));
			remove((char*)((uri + ".pgm").c_str()));
			winSur->Dump(winSur, "/tmp", "dump");

			if (fileExists(uri + ".ppm")) {
				strCmd = ("convert -quality " + itos(quality) + " " +
						"-resize " + itos(dumpW) + "x" + itos(dumpH) + " " +
						//"-colors 32 " +
						"-depth 4 " +
						"-interlace None " +
						"-compress BZip " +
						"-colorspace RGB " +
						//"-mask " + uri + ".pgm" + " " +
						uri + ".ppm " + uri + ".jpg" + " 2> /dev/null");

				if (::system(strCmd.c_str()) < 0) {
					::usleep(10000);
					remove((char*)((uri + ".jpg").c_str()));
					if (::system(strCmd.c_str()) < 0) {
						clog << "DFBWindow::getDumpFileUri Warning!!! ";
						clog << " Can't create JPEG file" << endl;
						unlock();
						return uri + ".ppm";
					}
				}

				unlock();
				return uri + ".jpg";
			}
		}

		unlock();
		return "";
	}

	void DFBWindow::lock() {
		pthread_mutex_lock(&mutex);
	}

	void DFBWindow::unlock() {
		pthread_mutex_unlock(&mutex);
	}

	void DFBWindow::lockChilds() {
		pthread_mutex_lock(&mutexC);
	}

	void DFBWindow::unlockChilds() {
		pthread_mutex_unlock(&mutexC);
	}
}
}
}
}
}
}

extern "C" ::br::pucrio::telemidia::ginga::core::mb::IWindow*
		createDFBWindow(
				GingaWindowID underlyingWindowID,
				GingaWindowID parentWindowID,
				GingaScreenID screenID,
				int x, int y, int w, int h) {

	return new ::br::pucrio::telemidia::ginga::core::mb::DFBWindow(
			underlyingWindowID, parentWindowID, screenID, x, y, w, h);
}

extern "C" void destroyDFBWindow(
		::br::pucrio::telemidia::ginga::core::mb::IWindow* w) {

	delete w;
}
