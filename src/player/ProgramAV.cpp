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
#include "ProgramAV.h"
#include "AVPlayer.h"

#include "util/functions.h"
using namespace ::ginga::util;

GINGA_PLAYER_BEGIN

ProgramAV::ProgramAV () : Player ("")
{
  currentPid = -1;
  currentPlayer = NULL;
  fullScreenBounds = "";
}

ProgramAV::~ProgramAV ()
{
  map<int, IPlayer *>::iterator i;

  i = players.begin ();
  while (i != players.end ())
    {
      i->second->stop ();
      if (i->second == currentPlayer)
        {
          currentPlayer = NULL;
        }
      delete i->second;
      ++i;
    }

  players.clear ();
  namePids.clear ();
  playerBounds.clear ();

  if (currentPlayer != NULL)
    {
      delete currentPlayer;
      currentPlayer = NULL;
    }
}

ProgramAV *ProgramAV::_instance = 0;

ProgramAV *
ProgramAV::getInstance ()
{
  if (ProgramAV::_instance == NULL)
    {
      ProgramAV::_instance = new ProgramAV ();
    }
  return ProgramAV::_instance;
}

void
ProgramAV::release ()
{
  if (currentPlayer != NULL)
    {
      currentPlayer->stop ();
    }

  if (_instance != NULL)
    {
      delete _instance;
      _instance = NULL;
    }
}

int64_t
ProgramAV::getVPts ()
{
  if (currentPlayer != NULL)
    {
      return currentPlayer->getVPts ();
    }

  return 0;
}

SDLSurface*
ProgramAV::getSurface ()
{
  SDLSurface* pSur = 0;

  if (currentPlayer != NULL)
    {
      pSur = currentPlayer->getSurface ();
      if (pSur != 0)
        {
          pSur->setExternalHandler (true);
        }
    }

  return pSur;
}

void
ProgramAV::addPidName (const string &name, int pid)
{
  clog << "ProgramAV::addPidName '" << name << "' = '" << pid;
  clog << "'" << endl;
  namePids[name] = pid;
}

int
ProgramAV::getPidByName (const string &name)
{
  map<string, int>::iterator i;

  i = namePids.find (name);
  if (i != namePids.end ())
    {
      return i->second;
    }

  return -1;
}

void
ProgramAV::forcePids (const string &pValue)
{
  vector<string> *vals;
  string name;

  vals = split (pValue, ",");
  if (vals->size () == 3)
    {
      name = getNameFromMrl ((*vals)[0]);
      setAVPid (name, xstrto_int ((*vals)[1]),
                xstrto_int ((*vals)[2]));
    }

  delete vals;
}

void
ProgramAV::setAVPid (const string &name, int aPid, int vPid)
{
  IPlayer *p;
  int pid;

  pid = getPidByName (name);
  if (pid < 0)
    {
      return;
    }

  p = getPlayer (pid);
  if (p != NULL)
    {
      clog << "ProgramAV::setAVPid";
      clog << " aPid = '" << aPid << "'";
      clog << " vPid = '" << vPid << "'";
      clog << endl;

      ((AVPlayer *)p)->setAVPid (aPid, vPid);
    }
  else
    {
      clog << "ProgramAV::setAVPid Warning! Can't find name '";
      clog << name << "' to set '" << aPid << "' and '" << vPid << "'";
      clog << endl;
    }
}

string
ProgramAV::getNameFromMrl (const string &mrl)
{
  if (mrl.substr (0, 11) == "sbtvd-ts://")
    {
      if (mrl.find ("dtv_channel.ts") != std::string::npos)
        {
          return "0";
        }
      return mrl.substr (11, mrl.length () - 11);
    }

  return mrl;
}

void
ProgramAV::showPlayer (const string &mrl)
{
  IPlayer *player;

  player = getPlayer (mrl);
  if (player != NULL)
    {
      player->setPropertyValue ("show", "0xFF");
    }
}

void
ProgramAV::hidePlayer (const string &mrl)
{
  IPlayer *player;

  player = getPlayer (mrl);
  if (player != NULL)
    {
      player->setPropertyValue ("hide", "0x00");
    }
}

void
ProgramAV::createPlayer (const string &mrl)
{
  string name;
  gint64 x;
  int pid;

  name = getNameFromMrl (mrl);
  if (_xstrtoll (name.c_str (), &x))
    {
      pid = (int) x;
    }
  else
    {
      pid = getPidByName (name);
    }

  if (pid < 0)
    {
      clog << "ProgramAV::createPlayer Warning! Can't create player '";
      clog << mrl << "' with name '" << name << "': pid '" << pid;
      clog << "' not found!" << endl;
      return;
    }

  currentPid = pid;
  currentPlayer = new AVPlayer (mrl);

  if (fullScreenBounds != "")
    {
      playerBounds[pid] = fullScreenBounds;
      currentPlayer->setPropertyValue ("createWindow", fullScreenBounds);
      fullScreenBounds = "";
    }

  setPlayer (pid, currentPlayer);

  clog << "ProgramAV::createPlayer for '" << mrl << "' all done" << endl;
}

void
ProgramAV::setPlayer (int pid, IPlayer *player)
{
  map<int, IPlayer *>::iterator i;
  IPlayer *ePlayer;

  i = players.find (pid);
  if (i == players.end ())
    {
      players[pid] = player;
    }
  else
    {
      ePlayer = players[pid];
      players[pid] = player;
      delete ePlayer;
      ePlayer = NULL;
    }
}

IPlayer *
ProgramAV::getPlayer (const string &mrl)
{
  string name;
  gint64 x;
  int pid;

  name = getNameFromMrl (mrl);
  if (_xstrtoll (name.c_str (), &x))
    {
      pid = (int) x;
    }
  else
    {
      pid = getPidByName (name);
    }

  return getPlayer (pid);
}

IPlayer *
ProgramAV::getPlayer (int pid)
{
  map<int, IPlayer *>::iterator i;

  i = players.find (pid);
  if (i != players.end ())
    {
      return i->second;
    }

  return NULL;
}

string
ProgramAV::getPropertyValue (const string &name)
{
  string value = "";

  if (currentPlayer != NULL)
    {
      value = currentPlayer->getPropertyValue (name);
    }

  if (value == "")
    {
      value = Player::getPropertyValue (name);
    }

  return value;
}

void
ProgramAV::setPropertyValue (const string &name, const string &value)
{
  clog << "ProgramAV::setPropertyValue '" << name << "' = '";
  clog << value << "'" << endl;

  if (name.substr (0, 11) == "sbtvd-ts://")
    {
      addPidName (getNameFromMrl (name), xstrto_int (value));
    }
  else if (name == "createPlayer")
    {
      createPlayer (value);
    }
  else if (name == "showPlayer")
    {
      showPlayer (value);
    }
  else if (name == "hidePlayer")
    {
      hidePlayer (value);
    }
  else if (name == "setBoundaries")
    {
      fullScreenBounds = value;
    }
  else if (name == "forcePids")
    {
      forcePids (value);
    }
  else if (currentPlayer != NULL)
    {
      if (name == "bounds")
        {
          if (value == "")
            {
              if (playerBounds.count (currentPid) != 0)
                {
                  currentPlayer->setPropertyValue (
                      name, playerBounds[currentPid]);
                }
            }
          else
            {
              currentPlayer->setPropertyValue (name, value);
            }
        }
      else
        {
          currentPlayer->setPropertyValue (name, value);
          Player::setPropertyValue (name, value);
        }
    }
}

GINGA_PLAYER_END
