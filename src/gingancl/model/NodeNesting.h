/* Copyright (C) 1989-2017 PUC-Rio/Laboratorio TeleMidia

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

#ifndef NODENESTING_H_
#define NODENESTING_H_

#include <vector>
using namespace std;

#include "system/Thread.h"
using namespace ::br::pucrio::telemidia::ginga::core::system::thread;

#include "ncl/components/Node.h"
using namespace ::br::pucrio::telemidia::ncl::components;

#include "system/SystemCompat.h"
using namespace ::br::pucrio::telemidia::ginga::core::system::compat;

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace ncl {
namespace model {
namespace components {
  class NodeNesting {
	private:
		string id;
		vector<Node*>* nodes;
		set<string> typeSet;
		pthread_mutex_t mutexNodes;

	public:
		NodeNesting();
		NodeNesting(Node* node);
		NodeNesting(NodeNesting* seq);
		NodeNesting(vector<Node*>* seq);
		virtual ~NodeNesting();

	private:
		void initialize();

	public:
		bool instanceOf(string s);
		void append(NodeNesting* otherSeq);
		void append(vector<Node*>* otherSeq);
		Node* getAnchorNode();
		Node* getHeadNode();
		Node* getNode(int index);
		int getNumNodes();
		void insertAnchorNode(Node *node);
		void insertHeadNode(Node *node);
		bool removeAnchorNode();
		bool removeHeadNode();
		string getId();
		NodeNesting* copy();
		string toString();
  };
}
}
}
}
}
}
}

#endif /*NODENESTING_H_*/