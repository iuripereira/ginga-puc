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
#include "AdapterChannelPlayer.h"
#include "player/AVPlayer.h"
#include "player/ChannelPlayer.h"

GINGA_FORMATTER_BEGIN

AdapterChannelPlayer::AdapterChannelPlayer () : AdapterFormatterPlayer ()
{
  typeSet.insert ("AdapterChannelPlayer");
}

void
AdapterChannelPlayer::createPlayer ()
{
  IPlayer *childPlayer = NULL;
  NclCompositeExecutionObject *cpExObj;
  NclExecutionObject *childObj;
  map<string, NclExecutionObject *> *objects;
  map<string, NclExecutionObject *>::iterator i;
  map<string, IPlayer *> *objectMap;
  Content *content;
  string mrlPlayer;
  string selectedObject = "";

  cpExObj = (NclCompositeExecutionObject *)object;
  objectMap = new map<string, IPlayer *>;
  objects = cpExObj->getExecutionObjects ();

  if (objects != NULL && cpExObj->getDescriptor () != NULL)
    {
      i = objects->begin ();
      while (i != objects->end ())
        {
          childObj = i->second;
          if (childObj != NULL && childObj->getDataObject () != NULL
              && childObj->getDataObject ()->getDataEntity () != NULL
              && ((NodeEntity *)(childObj->getDataObject ()
                                     ->getDataEntity ()))
                         ->getContent ()
                     != NULL)
            {
              content = ((NodeEntity *)(childObj->getDataObject ()
                                            ->getDataEntity ()))
                            ->getContent ();

              if (content->instanceOf ("ReferenceContent"))
                {
                  mrlPlayer = ((ReferenceContent *)content)
                                  ->getCompleteReferenceUrl ();

                  childPlayer = new AVPlayer (mrlPlayer);

                  if (childPlayer != NULL)
                    {
                      (*objectMap)[childObj->getDataObject ()->getId ()]
                          = childPlayer;
                    }
                }
            }
          ++i;
        }

      clog << "AdapterChannelPlayer::createPlayer objMap->size = ";
      clog << objectMap->size () << endl;
      selectedObject
          = cpExObj->getDescriptor ()->getParameterValue ("selectedObject");
    }

  if (objects != NULL)
    {
      delete objects;
      objects = NULL;
    }

  player = new ChannelPlayer ();

  if (player != NULL)
    {
      player->setPlayerMap (objectMap);
      if (selectedObject != "")
        {
          clog << "AdapterChannelPlayer::createPlayer selecting '";
          clog << selectedObject << "'" << endl;

          childPlayer = player->getPlayer (selectedObject);
          player->select (childPlayer);
        }
    }

  AdapterFormatterPlayer::createPlayer ();
}

bool
AdapterChannelPlayer::setPropertyValue (NclAttributionEvent *event,
                                        const string &value)
{
  if (value == "")
    {
      event->stop ();
      return false;
    }

  NclCascadingDescriptor *descriptor;
  string propName = "";
  string paramValue = "FALSE";

  descriptor = object->getDescriptor ();
  propName = event->getAnchor ()->getPropertyName ();
  if (propName == "selectedObject")
    {
      IPlayer *oldPlayer = NULL;
      IPlayer *newPlayer = NULL;
      double oldPlayerMediaTime = 0;

      oldPlayer = player->getSelectedPlayer ();
      if (oldPlayer == NULL)
        {
          clog << "AdapterChannelPlayer::setPropertyValue Warning!";
          clog << " cant find oldPlayer to new '" << value << "'";
          clog << endl;
          event->stop ();
          return false;
        }

      newPlayer = player->getPlayer (value);
      if (newPlayer == NULL)
        {
          clog << "AdapterChannelPlayer::setPropertyValue Warning!";
          clog << " cant find newPlayer for '" << value << "'";
          clog << ". Did you selected the same option twice?" << endl;
          event->stop ();
          return false;
        }

      if (descriptor->getParameterValue ("x-entryInstant") == "begin")
        {
          newPlayer->setMediaTime (0);
        }
      else
        {
          oldPlayerMediaTime = oldPlayer->getMediaTime () + 0.3;
          newPlayer->setMediaTime (oldPlayerMediaTime);
        }

      oldPlayer->stop ();
      newPlayer->play ();

      player->select (newPlayer);
      // oldPlayer->stop();
      event->stop ();
      return true;
    }
  else
    {
      return AdapterFormatterPlayer::setPropertyValue (event, value);
    }
}

GINGA_FORMATTER_END
