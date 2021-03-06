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

#ifndef FORMATTER_FOCUS_MANAGER_H
#define FORMATTER_FOCUS_MANAGER_H

#include "ginga.h"

#include "util/functions.h"

#include "NclCascadingDescriptor.h"
#include "NclExecutionObject.h"

#include "NclSelectionEvent.h"

#include "INclLinkActionListener.h"
#include "NclLinkSimpleAction.h"

#include "NclFormatterRegion.h"

#include "PresentationContext.h"

#include "AdapterFormatterPlayer.h"
#include "AdapterPlayerManager.h"

#include "FormatterMultiDevice.h"

#include "mb/IMouseEventListener.h"
#include "mb/IKeyInputEventListener.h"

GINGA_FORMATTER_BEGIN

class FormatterFocusManager : public IMouseEventListener, IKeyInputEventListener
{
private:
//  InputManager *im;
  PresentationContext *presContext;
  map<string, set<NclExecutionObject *> *> *focusTable;
  bool isHandler;
  string currentFocus;
  string objectToSelect;
  NclExecutionObject *selectedObject;

  SDL_Color *defaultFocusBorderColor;
  int defaultFocusBorderWidth;
  SDL_Color *defaultSelBorderColor;
  AdapterPlayerManager *playerManager;
  FormatterFocusManager *parentManager;
  FormatterMultiDevice *multiDevice;
  INclLinkActionListener *settingActions;

  int xOffset;
  int yOffset;
  int width;
  int height;

  double focusHandlerTS;

  void *converter;
  pthread_mutex_t mutexFocus;
  pthread_mutex_t mutexTable;

  static bool init;
  static set<FormatterFocusManager *> instances;
  static pthread_mutex_t iMutex;

public:
  FormatterFocusManager (AdapterPlayerManager *playerManager,
                         PresentationContext *presContext,
                         FormatterMultiDevice *multiDevice,
                         INclLinkActionListener *settingActions,
                         void *converter);

  virtual ~FormatterFocusManager ();

private:
  static void checkInit ();
  static bool hasInstance (FormatterFocusManager *instance, bool remove);

public:
  void setParent (FormatterFocusManager *parent);
  bool isKeyHandler ();
  bool setKeyHandler (bool isHandler);

private:
  void setHandlingObjects (bool isHandling);
  void resetKeyMaster ();

public:
  void tapObject (void *executionObject);
  void setKeyMaster (const string &mediaId);
  void setFocus (const string &focusIndex);

private:
  void setFocus (NclCascadingDescriptor *descriptor);
  void recoveryDefaultState (NclExecutionObject *object);

public:
  void showObject (NclExecutionObject *object);
  void hideObject (NclExecutionObject *object);

private:
  NclExecutionObject *getObjectFromFocusIndex (const string &focusIndex);
  void insertObject (NclExecutionObject *object, const string &focusIndex);
  void removeObject (NclExecutionObject *object, const string &focusIndex);

  bool keyCodeOk (NclExecutionObject *currentObject);
  bool keyCodeBack ();
  bool enterSelection (AdapterFormatterPlayer *player);
  void exitSelection (AdapterFormatterPlayer *player);

public:
  void setDefaultFocusBorderColor (SDL_Color *color);
  void setDefaultFocusBorderWidth (int width);
  void setDefaultSelBorderColor (SDL_Color *color);

  void setMotionBoundaries (int x, int y, int w, int h);

private:
  void changeSettingState (const string &name, const string &act);

public:
  void keyInputCallback (SDL_EventType evtType, SDL_Keycode key);
  void mouseInputCallback (arg_unused (SDL_EventType evtType), int x, int y);
//  bool motionEventReceived (int x, int y, int z);
};

GINGA_FORMATTER_END

#endif /* FORMATTER_FOCUS_MANAGER_H */
