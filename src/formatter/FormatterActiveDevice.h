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

#ifndef FORMATTER_ACTIVE_DEVICE_H
#define FORMATTER_ACTIVE_DEVICE_H

#include "ginga.h"

#include "FormatterMultiDevice.h"

#include "util/functions.h"
#include "util/Base64.h"
using namespace ::ginga::util;

#include "player/INCLPlayer.h"
using namespace ::ginga::player;

#include "system/SystemCompat.h"
#include "system/PracticalSocket.h"
using namespace ::ginga::system;

#include "PrivateBaseManager.h"

#include "mb/IKeyInputEventListener.h"

GINGA_FORMATTER_BEGIN

class FormatterActiveDevice : public FormatterMultiDevice, IKeyInputEventListener
{
public:
  static const unsigned int ADD_DOCUMENT = 10081;
  static const unsigned int REMOVE_DOCUMENT = 10082;
  static const unsigned int START_DOCUMENT = 10083;
  static const unsigned int STOP_DOCUMENT = 10084;
  static const unsigned int PAUSE_DOCUMENT = 10085;
  static const unsigned int RESUME_DOCUMENT = 10086;
  static const unsigned int SET_VAR = 10087;
  static const unsigned int SELECTION = 10088;
  static const unsigned int RCVBUFSIZE = 100;
  string img_dir;
  string img_dev;
  string img_reset;
  string tmp_dir;

private:
  TCPSocket *tcpSocket;
  int deviceServicePort;
  SDLSurface* s;
  map<string, string> *contentsInfo;
  map<string, string> initVars;
  string currentDocUri;
  PrivateBaseManager *privateBaseManager;
  INCLPlayer *formatter;
  bool listening;

public:
  FormatterActiveDevice (DeviceLayout *deviceLayout,
                         int x, int y, int w, int h, bool useMulticast,
                         int srvPort);
  virtual ~FormatterActiveDevice ();

  void keyInputCallback (SDL_EventType evtType, SDL_Keycode key);

protected:
  bool
  newDeviceConnected (arg_unused (int newDevClass), arg_unused (int w),
                      arg_unused (int h))
  {
    return false;
  };
  bool socketSend (TCPSocket *sock, const string &payload);
  void connectedToBaseDevice (unsigned int domainAddr);
  bool receiveRemoteEvent (int remoteDevClass, int eventType,
                           const string &eventContent);
  bool
  receiveRemoteContent (arg_unused (int remoteDevClass),
                        arg_unused (char *stream),
                        arg_unused (int streamSize))
  {
    return false;
  };
  bool receiveRemoteContent (int remoteDevClass, const string &contentUri);
  bool receiveRemoteContentInfo (const string &contentId, const string &contentUri);
  bool openDocument (const string &contentUri);
  INCLPlayer *createNCLPlayer ();
  NclPlayerData *createNclPlayerData ();
  int getCommandCode (const string &com);
  void handleTCPClient (TCPSocket *sock);
  bool handleTCPCommand (const string &sid,
                         const string &snpt,
                         const string &scommand,
                         const string &spayload_desc,
                         const string &payload);
};

GINGA_FORMATTER_END

#endif /* FORMATTER_ACTIVE_DEVICE_H */
