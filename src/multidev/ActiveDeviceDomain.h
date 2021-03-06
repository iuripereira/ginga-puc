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

#ifndef _ActiveDeviceDomain_H_
#define _ActiveDeviceDomain_H_

#include "DeviceDomain.h"

GINGA_MULTIDEV_BEGIN

class ActiveDeviceDomain : public DeviceDomain
{
public:
  ActiveDeviceDomain (bool useMulticast, int srvPort);
  virtual ~ActiveDeviceDomain ();

protected:
  bool
  taskRequest (arg_unused (int destDevClass), arg_unused (char *data),
               arg_unused (int taskSize))
  {
    return false;
  };
  bool
  activeTaskRequest (arg_unused (char *data), arg_unused (int taskSize))
  {
    return false;
  };
  void postConnectionRequestTask (int width, int height);
  void receiveConnectionRequest (arg_unused (char *task)){};
  void postAnswerTask (arg_unused (int reqDeviceClass),
                       arg_unused (int answer)){};
  void receiveAnswerTask (char *answerTask);
  bool
  postMediaContentTask (arg_unused (int destDevClass),
                        arg_unused (const string &url))
  {
    return false;
  };
  bool receiveMediaContentTask (char *task);
  bool
  receiveEventTask (arg_unused (char *task))
  {
    return false;
  };
  bool runControlTask ();
  bool
  runDataTask ()
  {
    return false;
  };
  void checkDomainTasks ();
};

GINGA_MULTIDEV_END

#endif /*_ActiveDeviceDomain_H_*/
