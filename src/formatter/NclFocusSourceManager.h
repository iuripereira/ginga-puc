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

#ifndef FOCUS_SOURCE_MANAGER_H
#define FOCUS_SOURCE_MANAGER_H

#include "ginga.h"

#include "mb/SDLSurface.h"
using namespace ::ginga::mb;

GINGA_FORMATTER_BEGIN

class NclFocusSourceManager
{
public:
  static SDLSurface* getFocusSourceComponent (const string &src);
};

GINGA_FORMATTER_END

#endif /* FOCUS_SOURCE_MANAGER_H */
