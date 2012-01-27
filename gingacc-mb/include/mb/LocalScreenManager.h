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

#ifndef LocalScreenManager_H_
#define LocalScreenManager_H_

#ifdef _WIN32
#if	_EXP_LOCALSCREENHANDLER_API == 0
#define LOCALSCREENHANDLER_API	__declspec(dllexport)
#else
#define LOCALSCREENHANDLER_API	__declspec(dllimport)
#endif
#else
#define LOCALSCREENHANDLER_API
#endif

#include "interface/IDeviceScreen.h"
#include "ILocalScreenManager.h"

#include <pthread.h>

#include <string>
#include <iostream>
#include <map>
using namespace std;

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace mb {
	class LOCALSCREENHANDLER_API LocalScreenManager :
			public ILocalScreenManager {

		private:
			map<GingaScreenID, IDeviceScreen*>* screens;
			pthread_mutex_t scrMutex;

			static LocalScreenManager* _instance;

			LocalScreenManager();
			virtual ~LocalScreenManager();

		public:
			void releaseHandler();

			void setParentScreen(GingaScreenID screenId, void* parentId);

			void setBackgroundImage(GingaScreenID screenId, string uri);

			static LocalScreenManager* getInstance();

			int getDeviceWidth(GingaScreenID screenId);

			int getDeviceHeight(GingaScreenID screenId);

			void* getGfxRoot(GingaScreenID screenId);

			void clearWidgetPools(GingaScreenID screenId);

			unsigned long createScreen(int numArgs, char** args);

			void mergeIds(
					GingaScreenID screenId,
					GingaWindowID destId,
					vector<GingaWindowID>* srcIds);

			void* getWindow(GingaScreenID screenId, GingaWindowID windowId);
			void* createWindow(GingaScreenID screenId, void* windowDesc);
			void releaseWindow(GingaScreenID screenId, void* window);
			void* createSurface(GingaScreenID screenId, void* surfaceDesc);
			void releaseSurface(GingaScreenID screenId, void* sur);

		private:
			void addScreen(GingaScreenID screenId, IDeviceScreen* screen);
			short getNumOfScreens();
			bool getScreen(GingaScreenID screenId, IDeviceScreen** screen);
			void lockScreens();
			void unlockScreens();
	};
}
}
}
}
}
}

#endif /*LocalScreenManager_H_*/