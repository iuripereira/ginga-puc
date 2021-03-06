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
#include "NclLinkAssignmentAction.h"

GINGA_FORMATTER_BEGIN

NclLinkAssignmentAction::NclLinkAssignmentAction (NclFormatterEvent *event,
                                                  short actionType,
                                                  string value)
    : NclLinkRepeatAction (event, actionType)
{
  this->value = value;
  this->animation = NULL;
  typeSet.insert ("NclLinkAssignmentAction");
}

NclLinkAssignmentAction::~NclLinkAssignmentAction ()
{
  isDeleting = true;

  if (animation != NULL)
    {
      delete animation;
      animation = NULL;
    }
}

string
NclLinkAssignmentAction::getValue ()
{
  return value;
}

void
NclLinkAssignmentAction::setValue (const string &value)
{
  this->value = value;
}

Animation *
NclLinkAssignmentAction::getAnimation ()
{
  return animation;
}

void
NclLinkAssignmentAction::setAnimation (Animation *animation)
{
  this->animation = animation;
}

GINGA_FORMATTER_END
