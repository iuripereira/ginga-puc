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
#include "Role.h"

GINGA_NCL_BEGIN

Role::Role ()
{
  this->minCon = 1;
  this->maxCon = 1;
}

Role::~Role () {}

short
Role::getEventType ()
{
  return eventType;
}

string
Role::getLabel ()
{
  return label;
}

void
Role::setEventType (short type)
{
  eventType = type;
}

void
Role::setLabel (const string &id)
{
  label = id;
}

int
Role::getMinCon ()
{
  return minCon;
}

int
Role::getMaxCon ()
{
  return maxCon;
}

void
Role::setMinCon (int minCon)
{
  this->minCon = minCon;
}

void
Role::setMaxCon (int maxCon)
{
  this->maxCon = maxCon;
}

GINGA_NCL_END
