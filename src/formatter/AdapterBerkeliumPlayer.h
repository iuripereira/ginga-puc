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

#ifndef AdapterBerkeliumPlayer_H_
#define AdapterBerkeliumPlayer_H_

#include "../../../../../model/NclExecutionObject.h"
using namespace ::ginga::formatter;

#include "../../../../../model/NclPresentationEvent.h"
using namespace ::ginga::formatter;

#include "../../../../AdapterFormatterPlayer.h"
using namespace ::ginga::formatter;

#include "mb/LocalScreenManager.h"
using namespace ::ginga::mb;

GINGA_FORMATTER_BEGIN

class AdapterBerkeliumPlayer : public AdapterFormatterPlayer
{
public:
  AdapterBerkeliumPlayer ();
  virtual ~AdapterBerkeliumPlayer (){};

protected:
  void rebase ();
  void updateProperties ();
  void createPlayer ();

public:
  bool setPropertyValue (NclAttributionEvent *event, string value);
};

GINGA_FORMATTER_END
#endif /*AdapterBerkeliumPlayer_H_*/