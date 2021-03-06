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

#ifndef VALUEASSESSMENT_H_
#define VALUEASSESSMENT_H_

#include "Assessment.h"

GINGA_NCL_BEGIN

class ValueAssessment : public Assessment
{
private:
  string value;

public:
  ValueAssessment (const string &value);
  string getValue ();
  void setValue (const string &newValue);
  bool
  instanceOf (const string &type)
  {
    return Assessment::instanceOf (type);
  }
};

GINGA_NCL_END

#endif /*VALUEASSESSMENT_H_*/
