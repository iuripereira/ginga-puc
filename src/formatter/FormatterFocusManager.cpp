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
#include "FormatterFocusManager.h"

#include "FormatterConverter.h"
#include "FormatterScheduler.h"
#include "NclFormatterLayout.h"

#include "mb/Display.h"
using namespace ::ginga::mb;

#if defined WITH_MULTIDEVICE && WITH_MULTIDEVICE
#include "multidev/DeviceDomain.h"
using namespace ::ginga::multidev;
#endif

GINGA_FORMATTER_BEGIN

bool FormatterFocusManager::init = false;
set<FormatterFocusManager *> FormatterFocusManager::instances;
pthread_mutex_t FormatterFocusManager::iMutex;

FormatterFocusManager::FormatterFocusManager (
    AdapterPlayerManager *playerManager, PresentationContext *presContext,
    FormatterMultiDevice *multiDevice,
    INclLinkActionListener *settingActions, void *converter)
{
  string strValue;

  Ginga_Display->registerKeyEventListener(this);
  Ginga_Display->registerMouseEventListener(this);

  focusTable = new map<string, set<NclExecutionObject *> *>;
  currentFocus = "";
  objectToSelect = "";
  isHandler = false;
  parentManager = NULL;
  xOffset = 0;
  yOffset = 0;
  width = 0;
  height = 0;
  focusHandlerTS = 0;

  strValue = presContext->getPropertyValue (DEFAULT_FOCUS_BORDER_COLOR);
  if (strValue == "")
    {
      defaultFocusBorderColor = new SDL_Color();
      ginga_hex_to_sdl_color( "#00F" , defaultFocusBorderColor); //blue
    }
  else
    {
      defaultFocusBorderColor = new SDL_Color();
      ginga_color_input_to_sdl_color( strValue, defaultFocusBorderColor);
    }

  strValue = presContext->getPropertyValue (DEFAULT_FOCUS_BORDER_WIDTH);
  if (strValue == "")
    {
      defaultFocusBorderWidth = 1;
    }
  else
    {
      defaultFocusBorderWidth = xstrto_int (strValue);
    }

  strValue = presContext->getPropertyValue (DEFAULT_SEL_BORDER_COLOR);
  if (strValue == "")
    {
      defaultSelBorderColor = new SDL_Color();
      ginga_hex_to_sdl_color( "#0F0" ,defaultSelBorderColor); //green
    }
  else
    {
      defaultSelBorderColor = new SDL_Color();
      ginga_color_input_to_sdl_color( strValue, defaultSelBorderColor);
    }

  this->presContext = presContext;
  this->selectedObject = NULL;
  this->playerManager = playerManager;
  this->multiDevice = multiDevice;
  this->converter = converter;
  this->settingActions = settingActions;

  Thread::mutexInit (&mutexFocus);
  Thread::mutexInit (&mutexTable);

  checkInit ();

  Thread::mutexLock (&iMutex);
  instances.insert (this);
  Thread::mutexUnlock (&iMutex);
}

FormatterFocusManager::~FormatterFocusManager ()
{
  Thread::mutexLock (&mutexFocus);
  Thread::mutexLock (&mutexTable);

  hasInstance (this, true);

  if (hasInstance (parentManager, false))
    {
    /*  if (isHandler && !parentManager->setKeyHandler (true))
        {
      
        }  */
    }
  selectedObject = NULL;

  if (defaultFocusBorderColor != NULL)
    {
      delete defaultFocusBorderColor;
      defaultFocusBorderColor = NULL;
    }

  if (defaultSelBorderColor != NULL)
    {
      delete defaultSelBorderColor;
      defaultSelBorderColor = NULL;
    }

  if (focusTable != NULL)
    {
      delete focusTable;
      focusTable = NULL;
    }

  playerManager = NULL;
  presContext = NULL;
  Thread::mutexUnlock (&mutexFocus);
  Thread::mutexDestroy (&mutexFocus);
  Thread::mutexUnlock (&mutexTable);
  Thread::mutexDestroy (&mutexTable);
}

void
FormatterFocusManager::checkInit ()
{
  if (!init)
    {
      init = true;
      Thread::mutexInit (&iMutex, false);
    }
}

bool
FormatterFocusManager::hasInstance (FormatterFocusManager *instance,
                                    bool remove)
{
  set<FormatterFocusManager *>::iterator i;
  bool find = false;

  if (!init)
    {
      return false;
    }

  Thread::mutexLock (&iMutex);
  i = instances.find (instance);
  if (i != instances.end ())
    {
      find = true;
      if (remove)
        {
          instances.erase (i);
        }
    }
  Thread::mutexUnlock (&iMutex);

  return find;
}

void
FormatterFocusManager::setParent (FormatterFocusManager *parent)
{
  parentManager = parent;

  if (parentManager != NULL)
    {
      multiDevice->setParent (parentManager->multiDevice);
    }
}

bool
FormatterFocusManager::isKeyHandler ()
{
  return isHandler;
}

bool
FormatterFocusManager::setKeyHandler (bool isHandler)
{
  NclExecutionObject *focusedObj;
  NclCascadingDescriptor *dc;
  NclFormatterRegion *fr;
  string ix;

  /*clog << "FormatterFocusManager::setKeyHandler(" << this << ")";
  clog << " isHandler '" << isHandler << "'";
  clog << " this->isHandler '" << this->isHandler << "'";
  clog << " parentManager '" << parentManager << "'";
  clog << endl;*/

  if (this->isHandler == isHandler)
    {
      return false;
    }

  if (isHandler && parentManager != NULL && !parentManager->isKeyHandler ())
    {
      clog << "FormatterFocusManager::setKeyHandler(" << this << ")";
      clog << " can't set handler because parent manager is not ";
      clog << "handling";
      clog << endl;
      return false;
    }

  focusedObj = getObjectFromFocusIndex (currentFocus);

  this->isHandler = isHandler;
  setHandlingObjects (isHandler);
  if (isHandler)
    {
      Thread::mutexLock (&mutexTable);
      if (focusedObj == NULL)
        {
          if (!focusTable->empty ())
            {
              ix = focusTable->begin ()->first;
              Thread::mutexUnlock (&mutexTable);
              currentFocus = "";
              setFocus (ix);
            }
          else
            {
              Thread::mutexUnlock (&mutexTable);
            }
        }
      else
        {
          Thread::mutexUnlock (&mutexTable);
          ix = currentFocus;
          currentFocus = "";
          setFocus (ix);
        }
      
    }
  else
    {
      if (focusedObj != NULL)
        {
          dc = focusedObj->getDescriptor ();
          if (dc != NULL)
            {
              fr = dc->getFormatterRegion ();
              if (fr != NULL)
                {
                  fr->setFocus (false);
                }
            }
          recoveryDefaultState (focusedObj);
        }
   
    }

  return isHandler;
}

void
FormatterFocusManager::setHandlingObjects (bool isHandling)
{
  ((FormatterConverter *)converter)->setHandlingStatus (isHandling);
}

NclExecutionObject *
FormatterFocusManager::getObjectFromFocusIndex (const string &focusIndex)
{
  map<string, set<NclExecutionObject *> *>::iterator i;
  set<NclExecutionObject *>::iterator j;
  NclCascadingDescriptor *desc;
  bool visible;

  i = focusTable->find (focusIndex);
  if (i == focusTable->end ())
    {
      return NULL;
    }

  j = i->second->begin ();
  while (j != i->second->end ())
    {
      desc = (*j)->getDescriptor ();
      if (desc != NULL && desc->getFormatterRegion () != NULL)
        {
          visible = desc->getFormatterRegion ()->isVisible ();
          if (visible)
            {
              return *j;
            }
        }
      ++j;
    }

  delete i->second;
  focusTable->erase (i);

  return NULL;
}

void
FormatterFocusManager::insertObject (NclExecutionObject *obj,
                                     const string &focusIndex)
{
  string auxIndex;
  map<string, set<NclExecutionObject *> *>::iterator i;
  vector<string>::iterator j;
  set<NclExecutionObject *> *objs;

  i = focusTable->find (focusIndex);
  if (i != focusTable->end ())
    {
      objs = i->second;
    }
  else
    {
      objs = new set<NclExecutionObject *>;
      (*focusTable)[focusIndex] = objs;
    }

  objs->insert (obj);
}

void
FormatterFocusManager::removeObject (NclExecutionObject *obj,
                                     const string &focusIndex)
{
  map<string, set<NclExecutionObject *> *>::iterator i;
  set<NclExecutionObject *>::iterator j;
  vector<string>::iterator k;
  set<NclExecutionObject *> *objs;

  i = focusTable->find (focusIndex);
  if (i != focusTable->end ())
    {
      objs = i->second;
      j = objs->find (obj);
      if (j != objs->end ())
        {
          objs->erase (j);
        }
      if (objs->empty ())
        {
          delete objs;
          focusTable->erase (i);
        }
    }
}

void
FormatterFocusManager::resetKeyMaster ()
{
  NclCascadingDescriptor *desc;
  AdapterFormatterPlayer *player = NULL;

  if (selectedObject != NULL)
    {
      objectToSelect = "";
      selectedObject->setHandler (false);
      desc = selectedObject->getDescriptor ();
      if (desc != NULL)
        {
          desc->getFormatterRegion ()->setSelection (false);
        }

      recoveryDefaultState (selectedObject);

      player = (AdapterFormatterPlayer *)playerManager->getObjectPlayer (
          selectedObject);
      exitSelection (player);
    }
}

void
FormatterFocusManager::tapObject (void *executionObject)
{
  NclExecutionObject *object = (NclExecutionObject *)executionObject;
  NclCascadingDescriptor *ds = NULL;
  NclFormatterRegion *fr = NULL;

  if (object->isSleeping ())
    {
      object = this->getObjectFromFocusIndex (currentFocus);
      if (object == NULL)
        {
          clog << "FormatterFocusManager::tapObject invalid object";
          clog << endl;
          return;
        }
    }

  ds = object->getDescriptor ();
  if (ds != NULL)
    {
      fr = ds->getFormatterRegion ();
    }

  clog << "FormatterFocusManager::tapObject " << endl;

  if (isHandler)
    {
      if (fr != NULL && fr->isVisible ())
        {
          if (fr->getFocusIndex () != "")
            {
              setFocus (fr->getFocusIndex ());
            }

          if (fr->setSelection (true))
            {
              if (!keyCodeOk (object))
                {
                   Ginga_Display->postKeyInputEventListener(SDLK_BACKSPACE);
                }
            }
        }
      else
        {
          clog << "FormatterFocusManager::tapObject can't tap '";
          clog << object->getId ();
          clog << "': !visible (FR = " << fr << ")" << endl;
        }
    }
  else
    {
      clog << "FormatterFocusManager::tapObject can't ";
      clog << " select '";
      clog << object->getId ();
      clog << "' focus manager is handling = '" << isHandler;
      clog << "'" << endl;
    }
}

void
FormatterFocusManager::setKeyMaster (const string &mediaId)
{
  NclExecutionObject *nextObject = NULL;
  NclCascadingDescriptor *nextDescriptor = NULL;
  NclFormatterRegion *fr = NULL;
  AdapterFormatterPlayer *player = NULL;
  bool isFRVisible = false;
  bool abortKeyMaster = false;
  string lastFocus = "";

  Thread::mutexLock (&mutexTable);
  Thread::mutexLock (&mutexFocus);

  if (mediaId == "" && selectedObject != NULL)
    {
      resetKeyMaster ();
      Thread::mutexUnlock (&mutexTable);
      Thread::mutexUnlock (&mutexFocus);
      return;
    }

  nextObject
      = ((FormatterConverter *)converter)->getObjectFromNodeId (mediaId);

  if (nextObject == NULL)
    {
      clog << "FormatterFocusManager::setKeyMaster can't set '";
      clog << mediaId << "' as master: object is not available.";
      clog << endl;

      objectToSelect = mediaId;
      Thread::mutexUnlock (&mutexTable);
      Thread::mutexUnlock (&mutexFocus);
      return;
    }

  if (selectedObject != NULL && selectedObject != nextObject)
    {
      resetKeyMaster ();
    }

  Thread::mutexUnlock (&mutexTable);

  nextDescriptor = nextObject->getDescriptor ();
  if (nextDescriptor != NULL)
    {
      fr = nextDescriptor->getFormatterRegion ();
    }

  if (fr != NULL && fr->getFocusIndex () != "")
    {
      lastFocus = currentFocus;
      currentFocus = fr->getFocusIndex ();
    }

  if (fr != NULL)
    {
      isFRVisible = fr->isVisible ();
      if (isFRVisible)
        {
          fr->setSelection (true);
        }
      else
        {
          abortKeyMaster = true;
        }
    }
  else
    {
      abortKeyMaster = true;
    }

  if (abortKeyMaster && fr != NULL && fr->getFocusIndex () != "")
    {
      currentFocus = lastFocus;
      Thread::mutexUnlock (&mutexFocus);
      return;
    }

  // selecting new object
  selectedObject = nextObject;
  selectedObject->setHandler (true);
  player = (AdapterFormatterPlayer *)playerManager->getObjectPlayer (
      selectedObject);

  enterSelection (player);
  nextObject->selectionEvent (0,
                              player->getMediaTime () * 1000);

  Thread::mutexUnlock (&mutexFocus);
}

void
FormatterFocusManager::setFocus (const string &focusIndex)
{
  NclExecutionObject *nextObject = NULL;
  NclExecutionObject *currentObject = NULL;
  NclCascadingDescriptor *currentDescriptor = NULL;
  NclCascadingDescriptor *nextDescriptor = NULL;

  if ((focusIndex == currentFocus && currentFocus != "") || !isHandler)
    {
      if (!isHandler)
        {
          clog << "FormatterFocusManager::setFocus(" << this << ")";
          clog << " can't set focus index because I'm not ";
          clog << "handling";
          clog << endl;
        }
      return;
    }

  Thread::mutexLock (&mutexTable);
  Thread::mutexLock (&mutexFocus);

  nextObject = getObjectFromFocusIndex (focusIndex);
  if (nextObject == NULL)
    {
      Thread::mutexUnlock (&mutexFocus);
      Thread::mutexUnlock (&mutexTable);
      return;
    }

  currentObject = getObjectFromFocusIndex (currentFocus);
  if (currentObject != NULL)
    {
      Thread::mutexUnlock (&mutexTable);
      currentDescriptor = currentObject->getDescriptor ();
    }
  else
    {
      currentDescriptor = NULL;
      clog << "FormatterFocusManager::setFocus index '";
      clog << focusIndex << "' is not in focus Table." << endl;
      Thread::mutexUnlock (&mutexTable);
    }

  currentFocus = focusIndex;
  presContext->setPropertyValue ("service.currentFocus", currentFocus);

  if (currentDescriptor != NULL)
    {
      recoveryDefaultState (currentObject);
      currentDescriptor->getFormatterRegion ()->setFocus (false);
    }
  else if (currentObject != NULL)
    {
      clog << "FormatterFocusManager::setFocus Warning can't recover '";
      clog << currentObject->getId () << "'" << endl;
    }

  nextDescriptor = nextObject->getDescriptor ();
  if (nextDescriptor != NULL)
    {
      setFocus (nextDescriptor);
    }

  Thread::mutexUnlock (&mutexFocus);
}

void
FormatterFocusManager::setFocus (NclCascadingDescriptor *descriptor)
{
  double borderAlpha;
  bool canDelFocusColor = false;
  bool canDelSelColor = false;
  SDL_Color *focusColor = NULL;
  SDL_Color *selColor = NULL;
  int borderWidth = 1;
  int width;
  NclFormatterRegion *fr = NULL;

  if (!isHandler)
    {
      clog << "FormatterFocusManager::setFocus(" << this << ")";
      clog << " can't set focus because I'm not ";
      clog << "handling";
      clog << endl;
      return;
    }

  fr = descriptor->getFormatterRegion ();
  if (fr == NULL)
    {
      clog << "FormatterFocusManager::setFocus(" << this << ")";
      clog << " can't set focus because FR is NULL";
      clog << endl;
      return;
    }

  focusColor = fr->getFocusBorderColor ();
  borderWidth = fr->getFocusBorderWidth ();
  selColor = fr->getSelBorderColor ();

  if (focusColor == NULL)
    {
      focusColor = defaultFocusBorderColor;
    }

  borderAlpha = descriptor->getFocusBorderTransparency ();
  if (!isnan (borderAlpha))
    {
      canDelFocusColor = true;
       if(focusColor==NULL)focusColor = new SDL_Color();
       focusColor->a = (guint8)(borderAlpha * 255);
    }

  width = borderWidth;

  if (selColor == NULL)
    {
      selColor = defaultSelBorderColor;
    }

  if (!isnan (borderAlpha))
    {
      canDelSelColor = true;
      if(selColor==NULL)selColor = new SDL_Color();
      selColor->a = (guint8)(borderAlpha * 255);
    }

  if (fr != NULL)
    {
      fr->setFocusInfo (focusColor, width, descriptor->getFocusSrc (),
                        selColor, width, descriptor->getSelectionSrc ());

      fr->setFocus (true);
    }

  if (canDelFocusColor)
    {
      delete focusColor;
      focusColor = NULL;
    }

  if (canDelSelColor)
    {
      delete selColor;
      selColor = NULL;
    }
}

void
FormatterFocusManager::recoveryDefaultState (NclExecutionObject *object)
{
  SDLWindow* wId;
  AdapterFormatterPlayer *player;
  NclFormatterRegion *fRegion;

  if (object == NULL || object->getDescriptor () == NULL
      || object->getDescriptor ()->getFormatterRegion () == NULL)
    {
      return;
    }

  player
      = (AdapterFormatterPlayer *)playerManager->getObjectPlayer (object);
  if (player != NULL)
    {
      fRegion = object->getDescriptor ()->getFormatterRegion ();
      fRegion->setRenderedSurface (player->getPlayer ()->getSurface ());
      fRegion->setFocus (false);
      wId = fRegion->getOutputId ();
      player->setOutputWindow (wId);
      player->flip ();
    }
}

void
FormatterFocusManager::showObject (NclExecutionObject *object)
{
  NclCascadingDescriptor *descriptor;
  NclFormatterRegion *fr = NULL;
  string focusIndex, auxIndex;
  string paramValue, mediaId;
  AdapterFormatterPlayer *player;
  Node *ncmNode;
  vector<string>::iterator i;

  if (object == NULL)
    {
      clog << "FormatterFocusManager::showObject Warning! object ";
      clog << "is null." << endl;
      return;
    }

  descriptor = object->getDescriptor ();
  if (descriptor == NULL)
    {
      clog << "FormatterFocusManager::showObject Warning! ";
      clog << " descriptor is null." << endl;
      return;
    }

  fr = descriptor->getFormatterRegion ();
  if (fr == NULL)
    {
      clog << "FormatterFocusManager::showObject Warning! ";
      clog << " FR is null." << endl;
      return;
    }

  focusIndex = fr->getFocusIndex ();

  ncmNode = object->getDataObject ();
  mediaId = ncmNode->getDataEntity ()->getId ();

  if (ncmNode->instanceOf ("ReferNode"))
    {
      if (((ReferNode *)ncmNode)->getInstanceType () == "new")
        {
          mediaId = ncmNode->getId ();
        }
    }

  clog << "FormatterFocusManager::showObject '" << mediaId << "'";
  clog << " with focus index = '" << focusIndex << "'";
  clog << " current focus = '" << currentFocus << "'";
  clog << endl;

  if (focusIndex != "")
    {
      Thread::mutexLock (&mutexTable);
      insertObject (object, focusIndex);
      Thread::mutexUnlock (&mutexTable);
    }

  if (currentFocus == "")
    {
      paramValue
          = presContext->getPropertyValue ("service.currentKeyMaster");

      if (paramValue == mediaId || objectToSelect == mediaId)
        {
          objectToSelect = "";
          setKeyMaster (mediaId);
        }
      else if (focusIndex != "")
        {
          setFocus (focusIndex);
        }
    }
  else
    {
      paramValue = presContext->getPropertyValue ("service.currentFocus");
      if (paramValue != "" && paramValue == focusIndex && fr->isVisible ())
        {
          /*if (focusTable->count(currentFocus) != 0) {
                  currentObject = (*focusTable)[currentFocus];
                  currentObject->getDescriptor()->
                              getFormatterRegion()->setFocus(false);

                  recoveryDefaultState(currentObject);
          }

          currentFocus = focusIndex;*/
          setFocus (focusIndex);
        }

      paramValue
          = presContext->getPropertyValue ("service.currentKeyMaster");

      if ((paramValue == mediaId || objectToSelect == mediaId)
          && fr->isVisible ())
        {
          objectToSelect = "";

          if (focusIndex != "")
            {
              setFocus (focusIndex);
            }

          // then set as selected
          if (fr->setSelection (true))
            {
              // unselect the previous selected object, if exists
              if (selectedObject != NULL)
                {
                  selectedObject->setHandler (false);
                  selectedObject->getDescriptor ()
                      ->getFormatterRegion ()
                      ->setSelection (false);

                  recoveryDefaultState (selectedObject);
                }

              selectedObject = object;
              selectedObject->setHandler (true);
              player = (AdapterFormatterPlayer *)
                           playerManager->getObjectPlayer (selectedObject);
              enterSelection (player);
            }
        }
    }
}

void
FormatterFocusManager::hideObject (NclExecutionObject *object)
{
  string focusIndex = "", ix;
  NclFormatterRegion *fr;
  AdapterFormatterPlayer *player;
  map<string, set<NclExecutionObject *> *>::iterator i;

  if (object == NULL || object->getDescriptor () == NULL)
    {
      return;
    }

  fr = object->getDescriptor ()->getFormatterRegion ();
  if (fr != NULL)
    {
      focusIndex = fr->getFocusIndex ();
    }

  if (focusIndex != "")
    {
      Thread::mutexLock (&mutexTable);
      removeObject (object, focusIndex);
      Thread::mutexUnlock (&mutexTable);

      if (fr != NULL && fr->getFocusState () == NclFormatterRegion::SELECTED
          && selectedObject == object)
        {
          player
              = (AdapterFormatterPlayer *)playerManager->getObjectPlayer (
                  selectedObject);
          exitSelection (player);
          // region->setSelection(false);
          // recoveryDefaultState(selectedObject);
          selectedObject = NULL;
        }

      if (currentFocus == focusIndex)
        {
          // region->setFocus(false);
          // recoveryDefaultState(object);

          Thread::mutexLock (&mutexTable);
          if (focusTable->empty ())
            {
              Thread::mutexUnlock (&mutexTable);
              currentFocus = "";
            }
          else
            {
              ix = focusTable->begin ()->first;
              Thread::mutexUnlock (&mutexTable);
              setFocus (ix);
            }
        }
    }
}

bool
FormatterFocusManager::keyCodeOk (NclExecutionObject *currentObject)
{
  AdapterFormatterPlayer *player;
  bool isHandling = false;
  /*
                  clog << "FormatterFocusManager::keyCodeOk(" << this <<
     ")";
                  clog << " this->isHandler '" << this->isHandler << "'";
                  clog << " parentManager '" << parentManager << "'";
                  clog << " lastHandler '" << lastHandler << "'";
                  clog << endl;
 */

  if (currentObject != selectedObject)
    {
      if (selectedObject != NULL)
        {
          clog << "FormatterFocusManager::keyCodeOk Warning! ";
          clog << "selecting an object with another selected." << endl;
          selectedObject->setHandler (false);
        }
    }

  selectedObject = currentObject;
  selectedObject->setHandler (true);
  player = (AdapterFormatterPlayer *)playerManager->getObjectPlayer (
      selectedObject);

  changeSettingState ("service.currentKeyMaster", "start");
  isHandling = enterSelection (player);

  if (selectedObject != NULL)
    {
      clog << "FormatterFocusManager::keyCodeOk ";
      clog << "selecting '" << selectedObject->getId () << "'" << endl;
      selectedObject->selectionEvent (0,
                                      player->getMediaTime () * 1000);
    }

  changeSettingState ("service.currentKeyMaster", "stop");

  return isHandling;
}

bool
FormatterFocusManager::keyCodeBack ()
{
  NclCascadingDescriptor *selectedDescriptor;
  NclFormatterRegion *fr = NULL;
  AdapterFormatterPlayer *player;
  string ix;
  /*
                  clog << "FormatterFocusManager::keyCodeBack(" << this <<
     ")";
                  clog << " this->isHandler '" << this->isHandler << "'";
                  clog << " parentManager '" << parentManager << "'";
                  clog << " lastHandler '" << lastHandler << "'";
                  clog << endl;
    }
  */
  if (selectedObject == NULL)
    {
      clog << "FormatterFocusManager::keyCodeBack NULL selObject";
      clog << endl;
      return false;
    }

  selectedObject->setHandler (false);
  selectedDescriptor = selectedObject->getDescriptor ();
  if (selectedDescriptor == NULL)
    {
      clog << "FormatterFocusManager::keyCodeBack NULL selDescriptor";
      clog << endl;
      return false;
    }

  fr = selectedDescriptor->getFormatterRegion ();
  if (fr == NULL)
    {
      clog << "FormatterFocusManager::keyCodeBack NULL formatterRegion";
      clog << endl;
      return false;
    }

  ix = fr->getFocusIndex ();
  fr->setSelection (false);

  Thread::mutexLock (&mutexFocus);
  if (ix != "" && ix == currentFocus)
    {
      fr->setFocus (true);
    }

  if (selectedObject != NULL)
    {
      player = (AdapterFormatterPlayer *)playerManager->getObjectPlayer (
          selectedObject);
      changeSettingState ("service.currentKeyMaster", "start");
      exitSelection (player);
      changeSettingState ("service.currentKeyMaster", "stop");
      selectedObject = NULL;
    }
  Thread::mutexUnlock (&mutexFocus);

  return false;
}

bool
FormatterFocusManager::enterSelection (AdapterFormatterPlayer *player)
{
  bool newHandler = false;
  string keyMaster;

  if (player != NULL && selectedObject != NULL)
    {
      keyMaster
          = (selectedObject->getDataObject ()->getDataEntity ()->getId ());

      presContext->setPropertyValue ("service.currentKeyMaster", keyMaster);

    //  newHandler = player->setKeyHandler (true);

      clog << "FormatterFocusManager::enterSelection(" << this << "): '";
      clog << keyMaster << "'" << endl;
    }
  else
    {
      clog << "FormatterFocusManager::enterSelection(";
      clog << this << ") can't enter selection" << endl;
    }

    return false;
  //return newHandler;
}

void
FormatterFocusManager::exitSelection (AdapterFormatterPlayer *player)
{
  clog << "FormatterFocusManager::exitSelection(" << this << ")" << endl;

  if (player != NULL)
    {
    //  player->setKeyHandler (false);

      presContext->setPropertyValue ("service.currentKeyMaster", "");
    }
}
 

void
FormatterFocusManager::setDefaultFocusBorderColor (SDL_Color *color)
{
  if (defaultFocusBorderColor != NULL)
    {
      delete defaultFocusBorderColor;
    }
  defaultFocusBorderColor = color;
}

void
FormatterFocusManager::setDefaultFocusBorderWidth (int width)
{
  defaultFocusBorderWidth = width;
}

void
FormatterFocusManager::setDefaultSelBorderColor (SDL_Color *color)
{
  if (defaultSelBorderColor != NULL)
    {
      delete defaultSelBorderColor;
    }
  defaultSelBorderColor = color;
}

void
FormatterFocusManager::setMotionBoundaries (int x, int y, int w, int h)
{
  xOffset = x;
  yOffset = y;
  width = w;
  height = h;
}

void
FormatterFocusManager::changeSettingState (const string &name, const string &act)
{
  set<NclExecutionObject *> *settingObjects;
  set<NclExecutionObject *>::iterator i;
  NclFormatterEvent *event;
  string keyM;

  settingObjects
      = ((FormatterConverter *)converter)->getSettingNodeObjects ();

  /*clog << "FormatterFocusManager::changeSettingState number of ";
  clog << "settings objects: '" << settingObjects->size() << "'";
  clog << endl;*/

  i = settingObjects->begin ();
  while (i != settingObjects->end ())
    {
      event = (*i)->getEventFromAnchorId (name);
      if (event != NULL && event->instanceOf ("NclAttributionEvent"))
        {
          if (act == "start")
            {
              event->start ();
            }
          else if (act == "stop")
            {
              if (name == "service.currentFocus")
                {
                  ((NclAttributionEvent *)(event))->setValue (currentFocus);
                }
              else if (name == "service.currentKeyMaster")
                {
                  if (selectedObject != NULL)
                    {
                      keyM = (selectedObject->getDataObject ()
                                  ->getDataEntity ()
                                  ->getId ());

                      ((NclAttributionEvent *)event)->setValue (keyM);
                    }
                }

              event->stop ();
            }
        }
      ++i;
    }

  delete settingObjects;
}

void 
FormatterFocusManager::keyInputCallback (SDL_EventType evtType, SDL_Keycode key){
  
  if (key == SDLK_ESCAPE || evtType == SDL_KEYDOWN)
        return ;


  NclExecutionObject *currentObject;
  NclCascadingDescriptor *currentDescriptor;
  NclFormatterRegion *fr;
  string nextIndex;
  map<string, set<NclExecutionObject *> *>::iterator i;

  if (!isHandler)
    {
     // return true;
     return;
    }

  Thread::mutexLock (&mutexTable);

  // if (xruntime_ms () - focusHandlerTS < 300
  //     && key != SDLK_BACKSPACE)
  //   {
  //     Thread::mutexUnlock (&mutexTable);
  //    // return true;
  //    return;
  //   }

  // focusHandlerTS = xruntime_ms ();

  i = focusTable->find (currentFocus);
  if (i == focusTable->end ())
    {
      if (currentFocus != "")
        {
          clog << "FormatterFocusManager::keyEventReceived ";
          clog << "currentFocus not found which is '" << currentFocus;
          clog << "'" << endl;
        }

      if (selectedObject != NULL && key == SDLK_BACKSPACE){
            keyCodeBack ();
            Thread::mutexUnlock (&mutexTable);
            return;
        }

      if (!focusTable->empty ())
        {
          nextIndex = focusTable->begin ()->first;
          Thread::mutexUnlock (&mutexTable);
          setFocus (nextIndex);
        }
      else
        {
          Thread::mutexUnlock (&mutexTable);
        }

      return;
    }

  currentObject = getObjectFromFocusIndex (currentFocus);
  if (currentObject == NULL)
    {
      Thread::mutexUnlock (&mutexTable);
      return;
    }
  Thread::mutexUnlock (&mutexTable);

  currentDescriptor = currentObject->getDescriptor ();
  if (currentDescriptor == NULL)
    {
      return;
    }

  fr = currentDescriptor->getFormatterRegion ();
  nextIndex = "";
  if (selectedObject != NULL)
    {
      if (key == SDLK_BACKSPACE)
        {
           keyCodeBack ();
         // return canItBack;
           return;
        }
    }
  else if (key == SDLK_UP)
    {
      if (fr != NULL)
        {
          nextIndex = fr->getMoveUp ();
        }
    }
  else if (key == SDLK_DOWN)
    {
      if (fr != NULL)
        {
          nextIndex = fr->getMoveDown ();
        }
    }
  else if (key == SDLK_LEFT)
    {
      if (fr != NULL)
        {
          nextIndex = fr->getMoveLeft ();
        }
    }
  else if (key == SDLK_RIGHT)
    {
      if (fr != NULL)
        {
          nextIndex = fr->getMoveRight ();
        }
    }
  else if (key == SDLK_RETURN)
    {
    //  userEvent->setKeyCode (Key::KEY_NULL);
      tapObject (currentObject);

      return ;
    }

  if (nextIndex != "")
    {
      changeSettingState ("service.currentFocus", "start");
      setFocus (nextIndex);
      changeSettingState ("service.currentFocus", "stop");
    }

}

void 
FormatterFocusManager::mouseInputCallback (arg_unused (SDL_EventType evtType), int x, int y){

  NclFormatterLayout *formatterLayout;
  NclExecutionObject *object;
  string objectFocusIndex;

  if (isHandler)
    {
      formatterLayout
          = (NclFormatterLayout *)(multiDevice->getMainLayout ());
      if (formatterLayout != NULL)
        {
          if ((x < xOffset || x > xOffset + width) || y < yOffset
              || y > yOffset + height)
            {
              return;
            }
  
          object = formatterLayout->getObject (x, y);
          if (object != NULL && object->getDescriptor () != NULL)
            {
              NclFormatterRegion *fr;
              fr = object->getDescriptor ()->getFormatterRegion ();

              if (fr != NULL)
                {
                  objectFocusIndex = fr->getFocusIndex ();
                  if (objectFocusIndex != ""
                      && objectFocusIndex != currentFocus)
                    {
                       setFocus (objectFocusIndex);
                       tapObject (object);
                    }
                }
            }
        }
    }
}

GINGA_FORMATTER_END
