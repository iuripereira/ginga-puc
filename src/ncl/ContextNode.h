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

#ifndef _CONTEXTNODE_H_
#define _CONTEXTNODE_H_

#include "GenericDescriptor.h"

#include "Link.h"
#include "LinkComposition.h"

#include "NodeEntity.h"
#include "CompositeNode.h"

GINGA_NCL_BEGIN

class ContextNode : public CompositeNode, public LinkComposition
{
protected:
  map<string, GenericDescriptor *> descriptorCollection;

private:
  set<Link *> linkSet;

public:
  ContextNode (const string &id);
  virtual ~ContextNode ();

  bool
  instanceOf (const string &className)
  {
    return Entity::instanceOf (className);
  };

  bool addLink (Link *link);
  bool addNode (Node *node);

  void clearLinks ();
  bool containsLink (Link *link);
  virtual set<Link *> *getLinks ();
  Link *getLink (const string &linkId);
  GenericDescriptor *getNodeDescriptor (Node *node);
  int getNumLinks ();
  bool removeLink (const string &linkId);
  bool removeLink (Link *link);
  bool setNodeDescriptor (const string &nodeId, GenericDescriptor *descriptor);
};

GINGA_NCL_END

#endif //_CONTEXTNODE_H_
