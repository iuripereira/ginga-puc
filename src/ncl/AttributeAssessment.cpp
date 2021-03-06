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
#include "AttributeAssessment.h"

GINGA_NCL_BEGIN

AttributeAssessment::AttributeAssessment (const string &role)
    : Assessment (), Role ()
{
  Role::setLabel (role);
  eventType = EventUtil::EVT_ATTRIBUTION;
  attributeType = EventUtil::ATT_NODE_PROPERTY;
  key = "";
  offset = "";
  maxCon = 1;
  typeSet.insert ("AttributeAssessment");
  typeSet.insert ("Role");
}

void
AttributeAssessment::setMaxCon (int max)
{
  maxCon = max;
}

void
AttributeAssessment::setMinCon (int min)
{
  minCon = min;
}

short
AttributeAssessment::getAttributeType ()
{
  return attributeType;
}

void
AttributeAssessment::setAttributeType (short attribute)
{
  attributeType = attribute;
}

string
AttributeAssessment::getKey ()
{
  return key;
}

void
AttributeAssessment::setKey (const string &key)
{
  this->key = key;
}

string
AttributeAssessment::getOffset ()
{
  return offset;
}

void
AttributeAssessment::setOffset (const string &offset)
{
  this->offset = offset;
}

string
AttributeAssessment::toString ()
{
  return xstrbuild ("%d", attributeType);
}

GINGA_NCL_END
