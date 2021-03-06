/* Copyright (C) 2006-2017 PUC-Rio/Laboratorio TeleMidia

This file is part of Ginga (Ginga-NCL).

Ginga is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Ginga is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
License for more details.

You should have received a copy of the GNU General Public License
along with Ginga.  If not, see <http://www.gnu.org/licenses/>.  */

#include "ginga.h"
#include "ginga-color-table.h"
#include "NclFormatterRegion.h"

#include "NclCascadingDescriptor.h"
#include "NclFormatterLayout.h"

#include "mb/Display.h"
using namespace ::ginga::mb;

//#include "util/Color.h"
//using namespace ::ginga::util;

GINGA_FORMATTER_BEGIN

NclFormatterRegion::NclFormatterRegion (const string &objectId, void *descriptor,
                                        void *layoutManager)
{
  this->layoutManager = layoutManager;
  this->objectId = objectId;
  this->descriptor = descriptor;

  initializeNCMRegion ();

  this->outputDisplay = 0;
  this->renderedSurface = 0;
  this->imVisible = false;
  this->externHandler = false;
  this->focusState = NclFormatterRegion::UNSELECTED;
  this->focusBorderColor = NULL;
  this->focusBorderWidth = 0;
  this->focusComponentSrc = "";
  this->selBorderColor = NULL;
  this->selBorderWidth = 0;
  this->selComponentSrc = "";
  this->chromaKey = NULL;
  this->bgColor = NULL;
  this->transitionIn = "";
  this->transitionOut = "";
  this->abortTransitionIn = false;
  this->abortTransitionOut = false;
  this->focusIndex = "";
  this->moveUp = "";
  this->moveDown = "";
  this->moveLeft = "";
  this->moveRight = "";
  this->plan = "";
  this->zIndex = -1;

  Thread::mutexInit (&mutex);
  Thread::mutexInit (&mutexT);
  Thread::mutexInit (&mutexFI);

  // TODO: look for descriptor parameters overriding region attributes
  string value;
  value = ((NclCascadingDescriptor *)descriptor)
              ->getParameterValue ("transparency");

  this->setTransparency (value);

  value = ((NclCascadingDescriptor *)descriptor)
              ->getParameterValue ("background");

  if (xstrchomp (value) != "")
    {
      if (value.find (",") == std::string::npos)
        {
          this->setBackgroundColor (value);
        }
      else
        {
          SDL_Color *bg = NULL;
          vector<string> *params = NULL;

          params = split (xstrchomp (value), ",");
          if (params->size () == 3)
            {
              bg = new SDL_Color();
              bg->r =  xstrto_uint8 ((*params)[0]);
              bg->g =  xstrto_uint8 ((*params)[1]);
              bg->b =  xstrto_uint8 ((*params)[2]); 
              bg->a = 255;              
              setBackgroundColor (bg);
            }
          else if (params->size () == 4)
            {
              bg = new SDL_Color();
              bg->r =  xstrto_uint8 ((*params)[0]);
              bg->g =  xstrto_uint8 ((*params)[1]);
              bg->b =  xstrto_uint8 ((*params)[2]); 
              bg->a =  xstrto_uint8 ((*params)[3]);      
              setBackgroundColor (bg);
            }
          delete params;
        }
    }

  value = ((NclCascadingDescriptor *)descriptor)->getParameterValue ("fit");

  this->setFit (value);

  value = ((NclCascadingDescriptor *)descriptor)
              ->getParameterValue ("scroll");

  this->setScroll (value);

  value = ((NclCascadingDescriptor *)descriptor)
              ->getParameterValue ("chromakey");

  this->setChromaKey (value);

  value = ((NclCascadingDescriptor *)descriptor)
              ->getParameterValue ("rgbChromakey");

  if (value == "")
    {
      value = ((NclCascadingDescriptor *)descriptor)
                  ->getParameterValue ("x-rgbChromakey");
    }

  this->setRgbChromaKey (value);

  // TODO: methods setTransIn and setTransOut
  value = ((NclCascadingDescriptor *)descriptor)
              ->getParameterValue ("transitionIn");

  if (value != "")
    {
      transitionIn = value;
    }

  value = ((NclCascadingDescriptor *)descriptor)
              ->getParameterValue ("transitionOut");

  if (value != "")
    {
      transitionOut = value;
    }

  // TODO: fit and scroll
}

NclFormatterRegion::~NclFormatterRegion ()
{
  lock ();
  lockFocusInfo ();
  layoutManager = NULL;
  descriptor = NULL;

  originalRegion = NULL;
  if (ncmRegion != NULL)
    {
      // delete ncmRegion;
      ncmRegion = NULL;
    }

  disposeOutputDisplay ();

  if (focusBorderColor != NULL)
    {
      delete focusBorderColor;
      focusBorderColor = NULL;
    }

  if (selBorderColor != NULL)
    {
      delete selBorderColor;
      selBorderColor = NULL;
    }

  if (bgColor != NULL)
    {
      delete bgColor;
      bgColor = NULL;
    }

  if (chromaKey != NULL)
    {
      delete chromaKey;
      chromaKey = NULL;
    }

  unlock ();
  Thread::mutexDestroy (&mutex);
  lockTransition ();
  unlockTransition ();
  Thread::mutexDestroy (&mutexT);
  unlockFocusInfo ();
  Thread::mutexDestroy (&mutexFI);
}

void
NclFormatterRegion::initializeNCMRegion ()
{
  originalRegion = NULL;

  if (descriptor != NULL)
    {
      originalRegion = ((NclCascadingDescriptor *)descriptor)->getRegion ();
    }

  if (originalRegion != NULL)
    {
      ncmRegion = originalRegion->cloneRegion ();

      /*if (originalRegion->isLeftPercentual() ||
                      originalRegion->isTopPercentual() ||
                      originalRegion->isWidthPercentual() ||
                      originalRegion->isHeightPercentual()) {
              left   = originalRegion->getAbsoluteLeft();
              top    = originalRegion->getAbsoluteTop();
              width  = originalRegion->getWidthInPixels();
              height = originalRegion->getHeightInPixels();

              originalRegion->resetLeft();
              originalRegion->resetTop();
              originalRegion->resetWidth();
              originalRegion->resetHeight();

              originalRegion->setLeft(left, false);
              originalRegion->setTop(top, false);
              originalRegion->setWidth(width, false);
              originalRegion->setHeight(height, false);
      }*/
    }
  else
    {
      ncmRegion = NULL;
    }
}

void
NclFormatterRegion::setRenderedSurface (SDLSurface* iSur)
{
  this->renderedSurface = iSur;
}

void
NclFormatterRegion::setZIndex (int zIndex)
{
  string layoutId;
  double cvtZIndex;

  this->zIndex = zIndex;

  if (ncmRegion != NULL)
    {
      ncmRegion->setZIndex (zIndex);
    }

  if (originalRegion != NULL && layoutManager != NULL)
    {
      layoutId = originalRegion->getId ();

      cvtZIndex = ((NclFormatterLayout *)layoutManager)
                      ->refreshZIndex (this, layoutId, zIndex, plan,
                                       renderedSurface);

      if (outputDisplay != 0)
        {
          outputDisplay->setZ (cvtZIndex);
        }

      toFront ();
    }
}

int
NclFormatterRegion::getZIndex ()
{
  return zIndex;
}

void
NclFormatterRegion::setPlan (const string &plan)
{
  if (this->plan != plan)
    {
      this->plan = plan;

      if (zIndex < 0)
        {
          zIndex = ncmRegion->getZIndexValue ();
        }

      setZIndex (this->zIndex);
    }
}

string
NclFormatterRegion::getPlan ()
{
  return plan;
}

void
NclFormatterRegion::setFocusIndex (const string &focusIndex)
{
  this->focusIndex = focusIndex;
}

string
NclFormatterRegion::getFocusIndex ()
{
  if (focusIndex != "")
    {
      return focusIndex;
    }
  else if (descriptor != NULL)
    {
      return ((NclCascadingDescriptor *)descriptor)->getFocusIndex ();
    }

  return "";
}

void
NclFormatterRegion::setMoveUp (const string &moveUp)
{
  this->moveUp = moveUp;
}

string
NclFormatterRegion::getMoveUp ()
{
  if (moveUp != "")
    {
      return moveUp;
    }
  else if (descriptor != NULL)
    {
      return ((NclCascadingDescriptor *)descriptor)->getMoveUp ();
    }

  return "";
}

void
NclFormatterRegion::setMoveDown (const string &moveDown)
{
  this->moveDown = moveDown;
}

string
NclFormatterRegion::getMoveDown ()
{
  if (moveDown != "")
    {
      return moveDown;
    }
  else if (descriptor != NULL)
    {
      return ((NclCascadingDescriptor *)descriptor)->getMoveDown ();
    }

  return "";
}

void
NclFormatterRegion::setMoveLeft (const string &moveLeft)
{
  this->moveLeft = moveLeft;
}

string
NclFormatterRegion::getMoveLeft ()
{
  if (moveLeft != "")
    {
      return moveLeft;
    }
  else if (descriptor != NULL)
    {
      return ((NclCascadingDescriptor *)descriptor)->getMoveLeft ();
    }

  return "";
}

void
NclFormatterRegion::setMoveRight (const string &moveRight)
{
  this->moveRight = moveRight;
}

string
NclFormatterRegion::getMoveRight ()
{
  if (moveRight != "")
    {
      return moveRight;
    }
  else if (descriptor != NULL)
    {
      return ((NclCascadingDescriptor *)descriptor)->getMoveRight ();
    }

  return "";
}

void
NclFormatterRegion::setFocusBorderColor (SDL_Color *focusBorderColor)
{
  lockFocusInfo ();
  if (this->focusBorderColor == focusBorderColor)
    {
      unlockFocusInfo ();
      return;
    }

  if (this->focusBorderColor != NULL)
    {
      delete this->focusBorderColor;
      this->focusBorderColor = NULL;
    }

  if (focusBorderColor != NULL)
    {
      this->focusBorderColor = focusBorderColor;
    }
  unlockFocusInfo ();
}

SDL_Color *
NclFormatterRegion::getFocusBorderColor ()
{
  SDL_Color *bColor = NULL;

  lockFocusInfo ();
  if (focusBorderColor != NULL)
    {
      bColor = focusBorderColor;
    }
  else if (descriptor != NULL)
    {
      bColor
          = ((NclCascadingDescriptor *)descriptor)->getFocusBorderColor ();
    }
  unlockFocusInfo ();

  return bColor;
}

void
NclFormatterRegion::setFocusBorderWidth (int focusBorderWidth)
{
  this->focusBorderWidth = focusBorderWidth;
}

int
NclFormatterRegion::getFocusBorderWidth ()
{
  if (focusBorderWidth != 0)
    {
      return focusBorderWidth;
    }
  else if (descriptor != NULL)
    {
      return ((NclCascadingDescriptor *)descriptor)->getFocusBorderWidth ();
    }

  return 0;
}

void
NclFormatterRegion::setFocusComponentSrc (const string &focusComponentSrc)
{
  this->focusComponentSrc = focusComponentSrc;
}

string
NclFormatterRegion::getFocusComponentSrc ()
{
  if (focusComponentSrc != "")
    {
      return focusComponentSrc;
    }
  else if (descriptor != NULL)
    {
      return ((NclCascadingDescriptor *)descriptor)->getFocusSrc ();
    }

  return "";
}

void
NclFormatterRegion::setSelBorderColor (SDL_Color *selBorderColor)
{
  lockFocusInfo ();
  if (this->selBorderColor == selBorderColor)
    {
      unlockFocusInfo ();
      return;
    }

  if (this->selBorderColor != NULL)
    {
      delete this->selBorderColor;
      this->selBorderColor = NULL;
    }

  if (selBorderColor != NULL)
    {
      this->selBorderColor = selBorderColor;
    }

  unlockFocusInfo ();
}

SDL_Color *
NclFormatterRegion::getSelBorderColor ()
{
  SDL_Color *sColor = NULL;

  lockFocusInfo ();
  if (selBorderColor != NULL)
    {
      sColor = selBorderColor;
    }
  else if (descriptor != NULL)
    {
      sColor = ((NclCascadingDescriptor *)descriptor)->getSelBorderColor ();
    }

  unlockFocusInfo ();

  return sColor;
}

void
NclFormatterRegion::setSelBorderWidth (int selBorderWidth)
{
  this->selBorderWidth = selBorderWidth;
}

int
NclFormatterRegion::getSelBorderWidth ()
{
  if (selBorderWidth != 0)
    {
      return selBorderWidth;
    }
  else if (descriptor != NULL)
    {
      return ((NclCascadingDescriptor *)descriptor)->getSelBorderWidth ();
    }

  return 0;
}

void
NclFormatterRegion::setSelComponentSrc (const string &selComponentSrc)
{
  this->selComponentSrc = selComponentSrc;
}

string
NclFormatterRegion::getSelComponentSrc ()
{
  if (selComponentSrc != "")
    {
      return selComponentSrc;
    }
  else if (descriptor != NULL)
    {
      return ((NclCascadingDescriptor *)descriptor)->getSelectionSrc ();
    }

  return "";
}

void
NclFormatterRegion::setFocusInfo (SDL_Color *focusBorderColor,
                                  int focusBorderWidth,
                                  const string &focusComponentSrc,
                                  SDL_Color *selBorderColor, int selBorderWidth,
                                  const string &selComponentSrc)
{
  setFocusBorderColor (focusBorderColor);
  setFocusBorderWidth (focusBorderWidth);
  setFocusComponentSrc (focusComponentSrc);
  setSelBorderColor (selBorderColor);
  setSelBorderWidth (selBorderWidth);
  setSelComponentSrc (selComponentSrc);
}

void *
NclFormatterRegion::getLayoutManager ()
{
  return layoutManager;
}

SDLWindow*
NclFormatterRegion::getOutputId ()
{
  return outputDisplay;
}

void
NclFormatterRegion::meetComponent (arg_unused (int width), arg_unused (int height), arg_unused (int prefWidth),
                                   arg_unused (int prefHeight), arg_unused (SDLSurface* component))
{
}

void
NclFormatterRegion::sliceComponent (arg_unused (int width), arg_unused (int height), arg_unused (int prefWidth),
                                    arg_unused (int prefHeight),
                                    arg_unused (SDLSurface* component))
{
}

void
NclFormatterRegion::updateCurrentComponentSize ()
{
}

void
NclFormatterRegion::updateRegionBounds ()
{
  sizeRegion ();
  if (focusState == NclFormatterRegion::UNSELECTED)
    {
      unselect ();
    }
  else
    { // is focused (at least)
      // if is focused and selected
      if (focusState == NclFormatterRegion::SELECTED)
        {
          setSelection (true);
        }
      else if (focusState == NclFormatterRegion::FOCUSED)
        {
          setFocus (true);
        }
    }
}

void
NclFormatterRegion::sizeRegion ()
{
  int left = 0;
  int top = 0;
  int width = 0;
  int height = 0;

  if (ncmRegion != NULL)
    {
      left = ncmRegion->getLeftInPixels ();
      top = ncmRegion->getTopInPixels ();
      width = ncmRegion->getWidthInPixels ();
      height = ncmRegion->getHeightInPixels ();
    }

  /*clog << "NclFormatterRegion::sizeRegion windowAdd = '" << outputDisplay;
  clog << "' x = '" << left;
  clog << "' y = '" << top;
  clog << "' w = '" << width;
  clog << "' h = '" << height << "'" << endl;*/

  if (left < 0)
    left = 0;

  if (top < 0)
    top = 0;

  if (width <= 0)
    width = 1;

  if (height <= 0)
    height = 1;

  lock ();
  if (outputDisplay != 0)
    {
      outputDisplay->setBounds (left, top, width, height);
    }
  unlock ();
}

bool
NclFormatterRegion::intersects (int x, int y)
{
  if (ncmRegion != NULL)
    {
      return ncmRegion->intersects (x, y);
    }

  return false;
}

LayoutRegion *
NclFormatterRegion::getLayoutRegion ()
{
  return ncmRegion;
}

LayoutRegion *
NclFormatterRegion::getOriginalRegion ()
{
  return originalRegion;
}

SDLWindow*
NclFormatterRegion::prepareOutputDisplay (SDLSurface* renderedSurface,
                                          double cvtIndex)
{
  lock ();

  if (outputDisplay == 0)
    {
      string title;
      int left = 0;
      int top = 0;
      int width = 0;
      int height = 0;

      if (ncmRegion == NULL)
        {
          title = objectId;
        }
      else
        {
          if (ncmRegion->getTitle () == "")
            {
              title = objectId;
            }
          else
            {
              title = ncmRegion->getTitle ();
            }

          left = ncmRegion->getAbsoluteLeft ();
          top = ncmRegion->getAbsoluteTop ();
          width = ncmRegion->getWidthInPixels ();
          height = ncmRegion->getHeightInPixels ();
        }

      if (left < 0)
        left = 0;

      if (top < 0)
        top = 0;

      if (width <= 0)
        width = 1;

      if (height <= 0)
        height = 1;

      this->renderedSurface = renderedSurface;

      if (renderedSurface != 0
          && renderedSurface->hasExternalHandler ())
        {
          externHandler = true;
          outputDisplay = renderedSurface->getParentWindow ();
        }

      if (!externHandler)
        {
          outputDisplay = Ginga_Display
            ->createWindow (left, top, width, height, (int)cvtIndex);
        }

      clog << "NclFormatterRegion::prepareOutputDisplay '" << outputDisplay;
      clog << "' created with ";
      clog << "left   = '" << left << "' ";
      clog << "top    = '" << top << "' ";
      clog << "width  = '" << width << "' ";
      clog << "height = '" << height << "' ";
      clog << endl;

      lockFocusInfo ();
      if (bgColor != NULL)
        {
          clog << "NclFormatterRegion::prepareOutputDisplay bg color ";

          clog << "r = '" << bgColor->r << "', ";
          clog << "g = '" << bgColor->g << "' and ";
          clog << "b = '" << bgColor->b << "' ";
          clog << endl;

          outputDisplay->setBgColor (*bgColor);
        }
      unlockFocusInfo ();

      if (!externHandler)
        {
          outputDisplay->setCurrentTransparency ((guint8)(transparency * 255));
        }

      int caps = outputDisplay->getCap ("ALPHACHANNEL");
      if (!externHandler && renderedSurface != 0
          && (caps & renderedSurface->getCaps ()))
        {
          outputDisplay->addCaps (caps);
        }

      if (chromaKey != NULL)
        {
          caps = outputDisplay->getCap ("NOSTRUCTURE");
          outputDisplay->setCaps (caps);
          outputDisplay->draw ();

          outputDisplay->setColorKey (*chromaKey);
        }
      else if (!externHandler)
        {
          outputDisplay->draw ();
        }
    }
  else
    {
      clog << "NclFormatterRegion::prepareOutputDisplay Warning!";
      clog << "window != NULL" << endl;
    }

  if (renderedSurface != 0 && !externHandler)
    {
      if (renderedSurface->setParentWindow (
                                      outputDisplay))
        {
          outputDisplay->renderFrom (renderedSurface);
        }
    }

  unlock ();

  return outputDisplay;
}

void
NclFormatterRegion::showContent ()
{
  string value;
  NclCascadingDescriptor *desc;
  int transitionType;
  unsigned int i;
  vector<Transition *> *transitions;
  Transition *transition;
  TransInfo *t;
  pthread_t threadId_;

  lockTransition ();
  desc = ((NclCascadingDescriptor *)descriptor);
  value = desc->getParameterValue ("visible");
  abortTransitionIn = false;
  abortTransitionOut = true;
  if (value != "false")
    {
      imVisible = true;
      transitions = desc->getInputTransitions ();

      if (!transitions->empty ())
        {
          for (i = 0; i < transitions->size (); i++)
            {
              transition = (*transitions)[i];
              transitionType = transition->getType ();
              if (transitionType == Transition::TYPE_FADE)
                {
                  toFront ();

                  // fade(transition, true);
                  t = new TransInfo;
                  t->fr = this;
                  t->t = transition;

                  // show with fade transition type
                  pthread_create (&threadId_, 0, NclFormatterRegion::fadeT,
                                  (void *)t);

                  pthread_detach (threadId_);
                  unlockTransition ();
                  return;
                }
              else if (transitionType == Transition::TYPE_BARWIPE)
                {
                  toFront ();

                  // barWipe(transition, true);
                  t = new TransInfo;
                  t->fr = this;
                  t->t = transition;

                  // show with barwipe transition type
                  pthread_create (&threadId_, 0,
                                  NclFormatterRegion::barWipeT, (void *)t);

                  pthread_detach (threadId_);
                  unlockTransition ();
                  return;
                }
            }
        }

      toFront ();
      unlockTransition ();
      setRegionVisibility (true);
      /*clog << "NclFormatterRegion::showContent '" << desc->getId();
      clog << "'" << endl;*/
    }
  else
    {
      unlockTransition ();
    }
}

void
NclFormatterRegion::hideContent ()
{
  setRegionVisibility (false);
  abortTransitionIn = true;
  abortTransitionOut = true;

  lock ();
  disposeOutputDisplay ();
  unlock ();
}

void
NclFormatterRegion::performOutTrans ()
{
  vector<Transition *> *transitions;
  unsigned int i;
  Transition *transition;
  int transitionType;
  bool currentVisibility;

  abortTransitionIn = true;
  abortTransitionOut = false;
  focusState = NclFormatterRegion::UNSELECTED;

  lockTransition ();

  currentVisibility = imVisible;
  if (currentVisibility)
    {
      transitions
          = ((NclCascadingDescriptor *)descriptor)->getOutputTransitions ();

      if (!transitions->empty ())
        {
          for (i = 0; i < transitions->size (); i++)
            {
              transition = (*transitions)[i];
              transitionType = transition->getType ();
              if (transitionType == Transition::TYPE_FADE)
                {
                  // hide with fade transition type
                  fade (transition, false);
                  unlockTransition ();
                  return;
                }
              else if (transitionType == Transition::TYPE_BARWIPE)
                {
                  // hide with bar wipe transition type
                  barWipe (transition, false);
                  unlockTransition ();
                  return;
                }
            }
        }
    }

  unlockTransition ();
}

double
NclFormatterRegion::getOutTransDur ()
{
  vector<Transition *> *transitions;
  unsigned int i;
  Transition *transition;
  int transitionType;

  lockTransition ();

  transitions
      = ((NclCascadingDescriptor *)descriptor)->getOutputTransitions ();

  if (!transitions->empty ())
    {
      for (i = 0; i < transitions->size (); i++)
        {
          transition = (*transitions)[i];
          transitionType = transition->getType ();

          switch (transitionType)
            {
            case Transition::TYPE_FADE:
              unlockTransition ();
              return transition->getDur ();

            case Transition::TYPE_BARWIPE:
              unlockTransition ();
              return transition->getDur ();

            default:
              g_assert_not_reached ();
            }
        }
    }

  unlockTransition ();
  return 0.0;
}

void
NclFormatterRegion::setRegionVisibility (bool visible)
{
  lock ();
  if (externHandler)
    {
      unlock ();
      return;
    }

  if (outputDisplay != 0)
    {
      if (!visible)
        {
          clog << "NclFormatterRegion::setRegionVisibility (" << this;
          clog << ") object '" << objectId << "' display '";
          clog << outputDisplay;
          clog << "' HIDE" << endl;

          outputDisplay->hide ();
        }
      else
        {
          clog << "NclFormatterRegion::setRegionVisibility (" << this;
          clog << ") object '" << objectId << "' display '";
          clog << outputDisplay;
          clog << "' SHOW" << endl;

          outputDisplay->show ();
        }
    }
  imVisible = visible;
  unlock ();
}

void
NclFormatterRegion::disposeOutputDisplay ()
{
  if (outputDisplay != 0)
    {
      if (!externHandler)
        {
          Ginga_Display->destroyWindow (outputDisplay);
        }
      outputDisplay = 0;
    }

  // rendered surface is deleted by player
  renderedSurface = 0;
}

void
NclFormatterRegion::toFront ()
{
  lock ();
  if (outputDisplay != 0 && !externHandler)
    {
      outputDisplay->raiseToTop ();
      unlock ();
      if (ncmRegion != NULL)
        {
          bringChildrenToFront (ncmRegion);
        }
      bringSiblingToFront (this);
    }
  else
    {
      unlock ();
    }
}

void
NclFormatterRegion::bringChildrenToFront (LayoutRegion *parentRegion)
{
  vector<LayoutRegion *> *regions = NULL;
  vector<LayoutRegion *>::iterator i;
  set<NclFormatterRegion *> *formRegions = NULL;
  set<NclFormatterRegion *>::iterator j;
  LayoutRegion *layoutRegion;
  NclFormatterRegion *region;

  if (parentRegion != NULL)
    {
      regions = parentRegion->getRegionsSortedByZIndex ();
    }

  // clog << endl << endl << endl;
  // clog << "DEBUG REGIONS SORTED BY ZINDEX parentID: ";
  // clog << parentRegion->getId() << endl;
  // i = regions->begin();
  // while (i != regions->end()) {
  //	clog << "region: " << (*i)->getId();
  //	clog << "zindex: " << (*i)->getZIndex();
  //	++i;
  //}
  // clog << endl << endl << endl;

  if (regions != NULL)
    {
      i = regions->begin ();
      while (i != regions->end ())
        {
          layoutRegion = *i;

          if (layoutRegion != NULL)
            {
              bringChildrenToFront (layoutRegion);
              formRegions = ((NclFormatterLayout *)layoutManager)
                                ->getFormatterRegionsFromNcmRegion (
                                    layoutRegion->getId ());

              if (formRegions != NULL)
                {
                  j = formRegions->begin ();
                  while (j != formRegions->end ())
                    {
                      region = *j;
                      if (region != NULL)
                        {
                          region->toFront ();
                        }
                      else
                        {
                          clog << "NclFormatterRegion::";
                          clog << "bringChildrenToFront";
                          clog << " Warning! region == NULL";
                        }

                      ++j;
                    }

                  delete formRegions;
                  formRegions = NULL;
                }
            }
          ++i;
        }

      delete regions;
    }
}

void
NclFormatterRegion::traverseFormatterRegions (LayoutRegion *region,
                                              LayoutRegion *baseRegion)
{
  LayoutRegion *auxRegion;
  set<NclFormatterRegion *> *formRegions;
  NclFormatterRegion *formRegion;
  set<NclFormatterRegion *>::iterator it;

  formRegions = ((NclFormatterLayout *)layoutManager)
                    ->getFormatterRegionsFromNcmRegion (region->getId ());

  if (formRegions != NULL)
    {
      it = formRegions->begin ();
      while (it != formRegions->end ())
        {
          formRegion = *it;
          if (formRegion != NULL)
            {
              auxRegion = formRegion->getLayoutRegion ();
              if (ncmRegion != NULL)
                {
                  if (ncmRegion->intersects (auxRegion)
                      && ncmRegion != auxRegion)
                    {
                      formRegion->toFront ();
                    }
                }
            }
          else
            {
              clog << "NclFormatterRegion::traverseFormatterRegion";
              clog << " Warning! formRegion == NULL" << endl;
            }

          // clog << "NclFormatterRegion::traverseFormatterRegion toFront =
          // ";
          // clog << "'" << formRegion->getLayoutRegion()->getId();
          // clog << "'" << endl;

          ++it;
        }
      delete formRegions;
      formRegions = NULL;
    }
  else
    {
      bringHideWindowToFront (baseRegion, region);
    }
}

void
NclFormatterRegion::bringHideWindowToFront (LayoutRegion *baseRegion,
                                            LayoutRegion *hideRegion)
{
  vector<LayoutRegion *> *regions;
  LayoutRegion *region;
  vector<LayoutRegion *>::iterator it;

  if (ncmRegion->intersects (hideRegion) && ncmRegion != hideRegion)
    {
      regions = hideRegion->getRegions ();
      if (regions != NULL)
        {
          for (it = regions->begin (); it != regions->end (); ++it)
            {
              region = *it;
              traverseFormatterRegions (region, baseRegion);
            }
        }
      delete regions;
      regions = NULL;
    }
}

void
NclFormatterRegion::bringSiblingToFront (NclFormatterRegion *region)
{
  LayoutRegion *layoutRegion, *parentRegion, *baseRegion, *siblingRegion;
  vector<LayoutRegion *> *regions;
  vector<LayoutRegion *>::iterator it;

  layoutRegion = region->getOriginalRegion ();
  if (layoutRegion == NULL)
    {
      return;
    }
  parentRegion = layoutRegion->getParent ();
  baseRegion = layoutRegion;

  while (parentRegion != NULL)
    {
      regions = parentRegion->getRegionsOverRegion (baseRegion);
      for (it = regions->begin (); it != regions->end (); ++it)
        {
          siblingRegion = *it;
          traverseFormatterRegions (siblingRegion, layoutRegion);
        }
      baseRegion = parentRegion;
      delete regions;
      regions = NULL;
      parentRegion = parentRegion->getParent ();
    }
}

void
NclFormatterRegion::setGhostRegion (bool ghost)
{
  lock ();
  if (outputDisplay != 0 && !externHandler)
    {
      outputDisplay->setGhostWindow (ghost);
    }
  unlock ();
}

bool
NclFormatterRegion::isVisible ()
{
  return imVisible;
}

short
NclFormatterRegion::getFocusState ()
{
  return focusState;
}

bool
NclFormatterRegion::setSelection (bool selOn)
{
  SDLSurface* selSurface;

  if (selOn && focusState == NclFormatterRegion::SELECTED)
    {
      return false;
    }

  if (selOn)
    {
      focusState = NclFormatterRegion::SELECTED;
      if (selComponentSrc != "")
        {
          lock ();

          selSurface = NclFocusSourceManager::getFocusSourceComponent (selComponentSrc);

          if (selSurface != 0)
            {
              if (outputDisplay != 0 && !externHandler)
                {
                  outputDisplay->renderFrom (
                                        selSurface);
                }

              delete selSurface;
            }
          unlock ();
        }

      lock ();
      if (outputDisplay != 0 && !externHandler)
        {
          lockFocusInfo ();
          if (selComponentSrc == "")
            {
              outputDisplay->validate ();
            }

          if (selBorderColor != NULL)
            {
              outputDisplay->setBorder (*selBorderColor,
                  selBorderWidth);
            }

          unlockFocusInfo ();
        }
      unlock ();
    }
  else
    {
      unselect ();
    }

  return selOn;
}

void
NclFormatterRegion::setFocus (bool focusOn)
{
  SDLSurface* focusSurface;

  if (focusOn)
    {
      focusState = NclFormatterRegion::FOCUSED;

      if (focusComponentSrc != "")
        {
          lock ();

          focusSurface = NclFocusSourceManager::getFocusSourceComponent (
              focusComponentSrc);

          if (focusSurface != 0)
            {
              if (outputDisplay != 0 && !externHandler)
                {
                  outputDisplay->renderFrom (
                                        focusSurface);
                }
              delete focusSurface;
            }
          unlock ();
        }

      lock ();
      if (outputDisplay != 0 && !externHandler)
        {
          lockFocusInfo ();
          if (focusComponentSrc == "")
            {
              outputDisplay->validate ();
            }

          if (focusBorderColor != NULL)
            {
              outputDisplay->setBorder (*focusBorderColor,
                                        focusBorderWidth);
            }

          unlockFocusInfo ();
        }
      unlock ();
    }
  else
    {
      unselect ();
    }
}

void
NclFormatterRegion::unselect ()
{
  focusState = NclFormatterRegion::UNSELECTED;

  lock ();
  if (outputDisplay != 0 && !externHandler)
    {
      SDL_Color c = {0, 0, 0, 0};
      outputDisplay->setBorder (c, 0);
      if (renderedSurface != 0)
        {
          renderedSurface->setParentWindow (outputDisplay);
          outputDisplay->renderFrom (renderedSurface);
        }
      outputDisplay->validate ();
    }
  unlock ();
}

SDL_Color *
NclFormatterRegion::getBackgroundColor ()
{
  return bgColor;
}

void
NclFormatterRegion::barWipe (Transition *transition, bool isShowEffect)
{
  int i, factor = 1, x, y, width, height;
  double time, initTime;
  int transitionSubType, initValue, endValue;
  double transitionDur, startProgress, endProgress;
  guint8 transparencyValue;
  short transitionDir;

  lock ();
  if (outputDisplay == 0 || externHandler)
    {
      clog << "NclFormatterRegion::barWipe(" << this << ")";
      clog << "Warning! return cause ";
      clog << "abortIn = '" << abortTransitionIn << "' and ";
      clog << "abortOut = '" << abortTransitionOut << "' and ";
      clog << "display = '" << outputDisplay << "' and ";
      clog << "isShow = '" << isShowEffect << "'" << endl;
      unlock ();
      return;
    }

  x = outputDisplay->getX ();
  y = outputDisplay->getY ();
  width = outputDisplay->getW ();
  height = outputDisplay->getH ();
  unlock ();

  transitionDur = transition->getDur ();
  transitionSubType = transition->getSubtype ();
  transitionDir = transition->getDirection ();
  startProgress = transition->getStartProgress ();
  endProgress = transition->getEndProgress ();

  transparencyValue
      = outputDisplay->getTransparencyValue ();
  initTime = xruntime_ms ();

  if (transitionSubType == Transition::SUBTYPE_BARWIPE_LEFTTORIGHT)
    {
      if (isShowEffect)
        {
          lock ();
          if (outputDisplay != 0)
            {
              outputDisplay->setCurrentTransparency (
                                                transparencyValue);
              outputDisplay->resize (1, height);
            }
          unlock ();

          initValue = (int)(width * startProgress);
          i = initValue + 1;
          endValue = (int)(width * endProgress);
        }
      else
        {
          initValue = (int)(width * endProgress);
          endValue = (int)(width * startProgress);
          i = initValue - 1;
        }

      while (true)
        {
          time = xruntime_ms ();
          if (time >= initTime + transitionDur)
            {
              break;
            }

          lock ();
          if (outputDisplay != 0)
            {
              if (transitionDir == Transition::DIRECTION_REVERSE)
                {
                  /*clog << "outDisplay =" << outputDisplay << " x=" << x;
                  clog << " w=" << width << " y=" << y << " h=" << height;
                  clog << " i=" << i << endl; */

                  if (x >= 0 && x + (width - i) >= 0 && i > 0 && height > 0)
                    {
                      outputDisplay->setBounds (
                                           x + (width - i), y, i, height);
                    }
                }
              else if (i > 0 && height > 0)
                {
                  outputDisplay->resize (i, height);
                }
            }
          else
            {
              unlock ();
              return;
            }

          unlock ();

          i = (int) getNextStepValue (initValue, endValue, factor, time, initTime,
                                      transitionDur, 0);

          lock ();
          if (outputDisplay != 0)
            {
              outputDisplay->validate ();
            }
          unlock ();

          if ((abortTransitionIn && isShowEffect)
              || (abortTransitionOut && !isShowEffect))
            {
              lock ();
              if (outputDisplay != 0)
                {
                  if (x < 0 || y < 0 || width <= 0 || height <= 0)
                    {
                      clog << "NclFormatterRegion::barWipe Warning! ";
                      clog << "invalid dimensions: ";
                      clog << "x = '" << x << "' ";
                      clog << "y = '" << y << "' ";
                      clog << "w = '" << width << "' ";
                      clog << "h = '" << height << "' ";
                      clog << endl;
                    }
                  else
                    {
                      outputDisplay->setBounds (x, y,
                                           width, height);
                    }

                  unlock ();
                  setRegionVisibility (isShowEffect);
                  // outputDisplay->setStretch(true);
                  if (!isShowEffect)
                    {
                      lock ();
                      disposeOutputDisplay ();
                      unlock ();
                    }
                }
              else
                {
                  unlock ();
                }
              return;
            }
        }
    }
  else if (transitionSubType == Transition::SUBTYPE_BARWIPE_TOPTOBOTTOM)
    {
      if (isShowEffect)
        {
          lock ();
          if (outputDisplay != 0)
            {
              outputDisplay->setCurrentTransparency (
                                                transparencyValue);
              if (width > 0)
                {
                  outputDisplay->resize (width, 1);
                }
            }
          unlock ();
          initValue = (int)(height * startProgress);
          i = initValue + 1;
          endValue = (int)(height * endProgress);
        }
      else
        {
          initValue = (int)(height * endProgress);
          endValue = (int)(height * startProgress);
          i = initValue - 1;
        }

      /*clog << "i = " << i << " startProgress = ";
      clog << startProgress << " endProgress = " << endProgress << endl;
      clog << " initValue = " << initValue << " endValue = " << endValue;
      clog << endl;*/

      while (true)
        {
          time = xruntime_ms ();
          if (time >= initTime + transitionDur)
            {
              break;
            }

          lock ();
          if (outputDisplay != 0)
            {
              if (transitionDir == Transition::DIRECTION_REVERSE)
                {
                  if (x >= 0 && y + (height - i) >= 0 && width > 0 && i > 0)
                    {
                      outputDisplay->setBounds (x,
                                           y + (height - i), width, i);
                    }
                }
              else if (width > 0 && i > 0)
                {
                  outputDisplay->resize (width, i);
                }
            }
          else
            {
              unlock ();
              return;
            }
          unlock ();

          i = (int) getNextStepValue (initValue, endValue, factor, time, initTime,
                                      transitionDur, 0);

          lock ();
          if (outputDisplay != 0)
            {
              outputDisplay->validate ();
            }
          unlock ();

          if ((abortTransitionIn && isShowEffect)
              || (abortTransitionOut && !isShowEffect))
            {
              lock ();
              if (outputDisplay != 0)
                {
                  if (x < 0 || y < 0 || width <= 0 || height <= 0)
                    {
                      clog << "NclFormatterRegion::barWipe Warning! ";
                      clog << "invalid dimensions: ";
                      clog << "x = '" << x << "' ";
                      clog << "y = '" << y << "' ";
                      clog << "w = '" << width << "' ";
                      clog << "h = '" << height << "' ";
                      clog << endl;
                    }
                  else
                    {
                      outputDisplay->setBounds (x, y,
                                           width, height);
                    }

                  // outputDisplay->setStretch(true);
                  if (!isShowEffect)
                    {
                      disposeOutputDisplay ();
                    }
                }
              unlock ();
              setRegionVisibility (isShowEffect);
              return;
            }
        }
    }

  lock ();
  if (!isShowEffect)
    {
      disposeOutputDisplay ();
    }
  else
    {
      if (outputDisplay != 0)
        {
          // outputDisplay->setStretch(true);
          outputDisplay->setBounds (x, y, width,
                               height);
          outputDisplay->validate ();
        }
    }
  unlock ();
}

void *
NclFormatterRegion::barWipeT (void *ptr)
{
  TransInfo *trans;
  Transition *t;
  NclFormatterRegion *fr;

  trans = (TransInfo *)ptr;
  fr = trans->fr;
  t = trans->t;

  delete trans;

  fr->barWipe (t, true);
  return NULL;
}

void
NclFormatterRegion::fade (Transition *transition, bool isShowEffect)
{
  int i, factor = 1;
  double time, initTime, initValue, endValue, startProgress, endProgress;
  int opacityValue;
  double transitionDur;

  lock ();
  if (outputDisplay == 0 || externHandler)
    {
      clog << "NclFormatterRegion::fade(" << this << ")";
      clog << "Warning! return cause ";
      clog << "abortIn = '" << abortTransitionIn << "' and ";
      clog << "abortOut = '" << abortTransitionOut << "' and ";
      clog << "display = '" << outputDisplay << "' and ";
      clog << "isShow = '" << isShowEffect << "'" << endl;
      unlock ();
      return;
    }

  opacityValue
      = (255 - outputDisplay->getTransparencyValue ());
  unlock ();

  transitionDur = transition->getDur ();
  startProgress = transition->getStartProgress ();
  endProgress = transition->getEndProgress ();

  if (isShowEffect)
    {
      initValue = opacityValue * startProgress;
      endValue = opacityValue * endProgress;
      i = (int)(initValue + 1);
    }
  else
    {
      initValue = opacityValue * endProgress;
      endValue = opacityValue * startProgress;
      i = (int)(initValue + 1);
    }

  initTime = xruntime_ms ();
  while (true)
    {
      time = xruntime_ms ();
      if (time >= initTime + transitionDur)
        {
          break;
        }

      lock ();
      if (outputDisplay != 0)
        {
          outputDisplay->setCurrentTransparency ((guint8) (255 - i));
        }
      else
        {
          unlock ();
          return;
        }
      unlock ();

      i = (int) getNextStepValue (initValue, endValue, factor, time, initTime,
                                  transitionDur, 0);

      if ((abortTransitionIn && isShowEffect)
          || (abortTransitionOut && !isShowEffect))
        {
          break;
        }
    }

  if (!isShowEffect)
    {
      disposeOutputDisplay ();
    }
  else
    {
      setRegionVisibility (isShowEffect);
    }
}

void *
NclFormatterRegion::fadeT (void *ptr)
{
  TransInfo *trans;
  Transition *t;
  NclFormatterRegion *fr;

  trans = (TransInfo *)ptr;
  fr = trans->fr;
  t = trans->t;

  delete trans;

  fr->fade (t, true);
  return NULL;
}

/*void changeCurrentComponent(Component newComponent) {
        if (newComponent != null && currentComponent != null &&
                        outputDisplay != null) {
                newComponent.setBounds(currentComponent.getBounds());
                outputDisplay.remove(currentComponent);
                outputDisplay.add(newComponent);
                currentComponent = newComponent;
                outputDisplay.validate();
        }
}*/

void
NclFormatterRegion::lock ()
{
  Thread::mutexLock (&mutex);
}

void
NclFormatterRegion::unlock ()
{
  Thread::mutexUnlock (&mutex);
}

void
NclFormatterRegion::lockTransition ()
{
  Thread::mutexLock (&mutexT);
}

void
NclFormatterRegion::unlockTransition ()
{
  Thread::mutexUnlock (&mutexT);
}

void
NclFormatterRegion::lockFocusInfo ()
{
  Thread::mutexLock (&mutexFI);
}

void
NclFormatterRegion::unlockFocusInfo ()
{
  Thread::mutexUnlock (&mutexFI);
}

void
NclFormatterRegion::setTransparency (const string &strTrans)
{
  double trans;
  if (strTrans == "")
    {
      trans = 0.0;
    }
  else
    {
      trans = xstrtod (strTrans);
    }

  setTransparency (trans);
}

double
NclFormatterRegion::getTransparency ()
{
  return this->transparency;
}

void
NclFormatterRegion::setTransparency (double transparency)
{
  lock ();
  if (transparency < 0.0)
    {
      this->transparency = 0.0;
    }
  else if (transparency > 1.0)
    {
      this->transparency = 1.0;
    }
  else
    {
      this->transparency = transparency;
    }

  if (outputDisplay != 0 && !externHandler)
    {
      outputDisplay->setCurrentTransparency ((guint8)(this->transparency * 255));
    }

  unlock ();
}

void
NclFormatterRegion::setBackgroundColor (const string &c)
{
    SDL_Color *bg = new SDL_Color();
    ginga_color_input_to_sdl_color(c,bg);
    bg->a = (guint8)(transparency * 255);
    setBackgroundColor (bg);
}

void
NclFormatterRegion::setBackgroundColor (SDL_Color *color)
{
  lockFocusInfo ();
  if (color != bgColor && bgColor != NULL)
    {
      delete bgColor;
      bgColor = NULL;
    }

  this->bgColor = color;
  unlockFocusInfo ();
}

void
NclFormatterRegion::setChromaKey (const string &value)
{
  if (value != "")
    {
      if (value == "black")
        {
             ginga_color_input_to_sdl_color( "#0", this->chromaKey);
        }
      else
        {
             ginga_color_input_to_sdl_color( value, this->chromaKey);
        }
    }
}

void
NclFormatterRegion::setRgbChromaKey (const string &value)
{
  if (value != "")
    {
      this->chromaKey = new SDL_Color();
      ginga_color_input_to_sdl_color(value, this->chromaKey);   
    }
}

void
NclFormatterRegion::setFit (const string &value)
{
  if (value != "")
    {
      setFit (DescriptorUtil::getFitCode (value));
    }
  else
    {
      setFit (Descriptor::FIT_FILL);
    }
}

void
NclFormatterRegion::setFit (short value)
{
  if (value < 0)
    {
      fit = Descriptor::FIT_FILL;
    }
  else
    {
      fit = value;
    }
}

void
NclFormatterRegion::setScroll (const string &value)
{
  if (value != "")
    {
      setScroll (DescriptorUtil::getScrollCode (value));
    }
  else
    {
      setScroll (Descriptor::SCROLL_NONE);
    }
}

void
NclFormatterRegion::setScroll (short value)
{
  if (value < 0)
    {
      scroll = Descriptor::SCROLL_NONE;
    }
  else
    {
      scroll = value;
    }
}

GINGA_FORMATTER_END
