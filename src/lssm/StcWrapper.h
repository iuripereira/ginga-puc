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

#ifndef STC_WRAPPER_H
#define STC_WRAPPER_H

#include "ginga.h"

#include "isdbt-tuner/ISTCProvider.h"
using namespace ::ginga::tuner;

#include "player/IPlayer.h"
using namespace ::ginga::player;

GINGA_LSSM_BEGIN

class StcWrapper : public ISTCProvider
{
private:
  IPlayer *realStcProvider;

public:
  StcWrapper (IPlayer *realStcProvider);
  virtual ~StcWrapper ();
  bool getSTCValue (guint64 *stc, int *valueType);
};

GINGA_LSSM_END

#endif /* STC_WRAPPER_H */
