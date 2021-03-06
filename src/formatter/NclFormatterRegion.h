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

#ifndef FORMATTERREGION_H_
#define FORMATTERREGION_H_

//#include "util/Color.h"
#include "util/functions.h"
using namespace ::ginga::util;

#include "ncl/LayoutRegion.h"
using namespace ::ginga::ncl;

#include "ncl/Descriptor.h"
#include "ncl/DescriptorUtil.h"
using namespace ::ginga::ncl;

#include "ncl/Transition.h"
#include "ncl/TransitionUtil.h"
using namespace ::ginga::ncl;

#include "mb/SDLWindow.h"
using namespace ::ginga::mb;

#include "NclFocusSourceManager.h"

GINGA_FORMATTER_BEGIN

class NclFormatterRegion
{
private:
  void *layoutManager; // NclFormatterLayout
  void *descriptor;    // NclCascadingDescriptor

  string objectId;
  LayoutRegion *ncmRegion;
  LayoutRegion *originalRegion;
  bool externHandler;
  SDLWindow* outputDisplay;

  SDLSurface* renderedSurface;

  bool imVisible;

  short focusState;
  SDL_Color *focusBorderColor;
  int focusBorderWidth;
  string focusComponentSrc;
  SDL_Color *selBorderColor;
  string selComponentSrc;
  int selBorderWidth;

  int zIndex;

  SDL_Color *bgColor;
  double transparency;
  short fit;
  short scroll;
  SDL_Color *chromaKey;
  string transitionIn;
  string transitionOut;
  bool abortTransitionIn;
  bool abortTransitionOut;
  pthread_mutex_t mutex;
  pthread_mutex_t mutexT;
  pthread_mutex_t mutexFI;

  string focusIndex;
  string moveUp;
  string moveDown;
  string moveLeft;
  string moveRight;

  string plan;

public:
  static const short UNSELECTED = 0;
  static const short FOCUSED = 1;
  static const short SELECTED = 2;

  NclFormatterRegion (const string &objectId, void *descriptor,
                      void *layoutManager);

  virtual ~NclFormatterRegion ();

private:
  void initializeNCMRegion ();

public:
  void setRenderedSurface (SDLSurface* iSur);

  void setZIndex (int zIndex);
  int getZIndex ();

  void setPlan (const string &plan);
  string getPlan ();

  void setFocusIndex (const string &focusIndex);
  string getFocusIndex ();

  void setMoveUp (const string &moveUp);
  string getMoveUp ();

  void setMoveDown (const string &moveDown);
  string getMoveDown ();

  void setMoveLeft (const string &moveLeft);
  string getMoveLeft ();

  void setMoveRight (const string &moveRight);
  string getMoveRight ();

  void setFocusBorderColor (SDL_Color *focusBorderColor);
  SDL_Color *getFocusBorderColor ();

  void setFocusBorderWidth (int focusBorderWidth);
  int getFocusBorderWidth ();

  void setFocusComponentSrc (const string &focusComponentSrc);
  string getFocusComponentSrc ();

  void setSelBorderColor (SDL_Color *selBorderColor);
  SDL_Color *getSelBorderColor ();

  void setSelBorderWidth (int selBorderWidth);
  int getSelBorderWidth ();

  void setSelComponentSrc (const string &selComponentSrc);
  string getSelComponentSrc ();

  void setFocusInfo (SDL_Color *focusBorderColor, int focusBorderWidth,
                     const string &focusComponentSrc, SDL_Color *selBorderColor,
                     int selBorderWidth, const string &selComponentSrc);

  void *getLayoutManager ();
  SDLWindow* getOutputId ();

private:
  void meetComponent (int width, int height, int prefWidth, int prefHeight,
                      SDLSurface* component);

  void sliceComponent (int width, int height, int prefWidth, int prefHeight,
                       SDLSurface* component);

  void updateCurrentComponentSize ();

public:
  void updateRegionBounds ();

private:
  void sizeRegion ();

public:
  bool intersects (int x, int y);
  LayoutRegion *getLayoutRegion ();
  LayoutRegion *getOriginalRegion ();

  SDLWindow* prepareOutputDisplay (SDLSurface* renderedSurface,
                                      double cvtIndex);

  void showContent ();
  void hideContent ();

  void performOutTrans ();
  double getOutTransDur ();

  void setRegionVisibility (bool visible);

private:
  void disposeOutputDisplay ();

public:
  void toFront ();

private:
  void bringChildrenToFront (LayoutRegion *parentRegion);
  void traverseFormatterRegions (LayoutRegion *region,
                                 LayoutRegion *baseRegion);

  void bringHideWindowToFront (LayoutRegion *baseRegion,
                               LayoutRegion *hideRegion);

  void bringSiblingToFront (NclFormatterRegion *region);

public:
  void setGhostRegion (bool ghost);
  bool isVisible ();
  short getFocusState ();
  bool setSelection (bool selOn);
  void setFocus (bool focusOn);
  void unselect ();

  SDL_Color *getBackgroundColor ();

  double getTransparency ();
  void setTransparency (const string &strTrans);
  void setTransparency (double transparency);
  void setBackgroundColor (const string &color);
  void setBackgroundColor (SDL_Color *color);
  void setChromaKey (const string &value);
  void setRgbChromaKey (const string &value);
  void setFit (const string &value);
  void setFit (short value);
  void setScroll (const string &value);
  void setScroll (short value);

private:
  void barWipe (Transition *transition, bool isShowEffect);
  static void *barWipeT (void *ptr);
  void fade (Transition *transition, bool isShowEffect);
  static void *fadeT (void *ptr);
  void lock ();
  void unlock ();
  void lockTransition ();
  void unlockTransition ();
  void lockFocusInfo ();
  void unlockFocusInfo ();
};

typedef struct
{
  NclFormatterRegion *fr;
  Transition *t;
} TransInfo;

GINGA_FORMATTER_END

#endif /*FORMATTERREGION_H_*/
