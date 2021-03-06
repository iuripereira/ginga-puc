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

#ifndef _MulticastSocketService_H_
#define _MulticastSocketService_H_

#include "ISocketService.h"

#include "system/SystemCompat.h"
#include "system/PracticalSocket.h"
using namespace ::ginga::system;

#include "system/Thread.h"
using namespace ::ginga::system;

GINGA_MULTIDEV_BEGIN

class MulticastSocketService : public ISocketService
{
private:
  unsigned int port;

  char *groupAddress;

  unsigned int interfaceIP;

  UDPSocket *readSocket;
  UDPSocket *writeSocket;

  pthread_mutex_t mutexBuffer;
  vector<struct frame *> *outputBuffer;

public:
  MulticastSocketService (char *groupAddr, unsigned int portNumber);
  ~MulticastSocketService ();

private:
  int createMulticastGroup ();
  bool createSocket ();
  bool addToGroup ();
  bool setSocketOptions ();
  bool tryToBind ();

public:
  unsigned int
  getInterfaceIPAddress ()
  {
    return 0;
  };
  int getServicePort ();
  void dataRequest (char *data, int taskSize, bool repeat = true);

private:
  bool sendData (struct frame *f);

public:
  bool checkOutputBuffer ();
  bool checkInputBuffer (char *data, int *size);
};

GINGA_MULTIDEV_END

#endif /*_MulticastSocketService_H_*/
