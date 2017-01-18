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

#ifndef _LINKTRANSITIONTRIGGERCONDITION_H_
#define _LINKTRANSITIONTRIGGERCONDITION_H_

#include "ncl/Bind.h"
using namespace ::ginga::ncl;

#include "NclFormatterEvent.h"
#include "INclEventListener.h"
using namespace ::ginga::formatter;

#include "NclLinkTriggerCondition.h"
#include "NclLinkTriggerListener.h"

GINGA_FORMATTER_BEGIN

class NclLinkTransitionTriggerCondition : public NclLinkTriggerCondition,
                                       public INclEventListener
{

protected:
  NclFormatterEvent *event;
  short transition;
  Bind *bind;

public:
  NclLinkTransitionTriggerCondition (NclFormatterEvent *event, short transition,
                                  Bind *bind);

  virtual ~NclLinkTransitionTriggerCondition ();

  Bind *getBind ();

  virtual void eventStateChanged (void *event, short transition,
                                  short previousState);

  short getPriorityType ();
  NclFormatterEvent *getEvent ();
  short getTransition ();
  virtual vector<NclFormatterEvent *> *getEvents ();
};

GINGA_FORMATTER_END
#endif //_LINKTRANSITIONTRIGGERCONDITION_H_