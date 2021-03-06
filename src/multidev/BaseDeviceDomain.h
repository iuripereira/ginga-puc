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

#ifndef BASE_DEVICE_DOMAIN_H
#define BASE_DEVICE_DOMAIN_H

#include "system/Thread.h"
using namespace ::ginga::system;

#include "BroadcastSocketService.h"
#include "BroadcastDualSocketService.h"
#include "BaseDeviceService.h"

#include "DeviceDomain.h"

GINGA_MULTIDEV_BEGIN

typedef struct
{
  char *data;
  int size;
  double timestamp;
} RemoteTask;

class BaseDeviceDomain : public DeviceDomain
{
protected:
  pthread_mutex_t pMutex;
  RemoteTask lastMediaContentTask;
  int timerCount;

public:
  BaseDeviceDomain (bool useMulticast, int srvPort);
  virtual ~BaseDeviceDomain ();

protected:
  virtual bool taskRequest (int destDevClass, char *data, int taskSize);
  virtual bool activeTaskRequest (char *data, int taskSize);
  virtual void postConnectionRequestTask (arg_unused (int width),
                                          arg_unused (int height)){};
  virtual void receiveConnectionRequest (char *task);
  virtual void postAnswerTask (int reqDeviceClass, int answer);
  virtual void receiveAnswerTask (arg_unused (char *answerTask)){};

public:
  virtual bool postMediaContentTask (int destDevClass, const string &url);

protected:
  virtual bool
  receiveMediaContentTask (arg_unused (char *task))
  {
    return false;
  };
  virtual bool receiveEventTask (char *task);

public:
  virtual void setDeviceInfo (int width, int height,
                              const string &base_device_ncl_path);

protected:
  virtual bool runControlTask ();
  virtual bool runDataTask ();
  virtual void checkDomainTasks ();
};

GINGA_MULTIDEV_END

#endif /* BASE_DEVICE_DOMAIN_H */
