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

#ifndef _LINKTRIGGERLISTENER_H_
#define _LINKTRIGGERLISTENER_H_

GINGA_FORMATTER_BEGIN

class NclLinkTriggerListener
{
public:
  static const short CONDITION_SATISFIED = 0;
  static const short EVALUATION_STARTED = 1;
  static const short EVALUATION_ENDED = 2;
  virtual ~NclLinkTriggerListener (){};
  virtual void conditionSatisfied (void *condition) = 0;
  virtual void evaluationStarted () = 0;
  virtual void evaluationEnded () = 0;
};

GINGA_FORMATTER_END

#endif //_LINKTRIGGERLISTENER_H_
