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

#ifndef NCLPLAYERADAPTER_H_
#define NCLPLAYERADAPTER_H_

#include "AdapterApplicationPlayer.h"

GINGA_FORMATTER_BEGIN

class AdapterNCLPlayer : public AdapterApplicationPlayer
{
public:
  AdapterNCLPlayer ();
  virtual ~AdapterNCLPlayer (){};

protected:
  void createPlayer ();

public:
  bool setAndLockCurrentEvent (NclFormatterEvent *event);
  void unlockCurrentEvent (NclFormatterEvent *event);
  void flip ();
};

GINGA_FORMATTER_END

#endif /* NCLPLAYERADAPTER_H_ */
