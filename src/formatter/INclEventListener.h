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

#ifndef _IEVENTLISTENER_H_
#define _IEVENTLISTENER_H_

GINGA_FORMATTER_BEGIN

class INclEventListener
{
public:
  // PRIORITY TYPE FOR NOTIFICATIONS
  static const short PT_CORE = 0;
  static const short PT_LINK = 1;
  static const short PT_OBJECT = 2;

  virtual ~INclEventListener (){};
  virtual void eventStateChanged (void *event, short transition,
                                  short previousState)
      = 0;

  virtual short getPriorityType () = 0;
};

GINGA_FORMATTER_END

#endif //_IEVENTLISTENER_H_
