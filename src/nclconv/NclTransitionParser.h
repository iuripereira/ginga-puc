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

#ifndef NCLTRANSITIONPARSER_H_
#define NCLTRANSITIONPARSER_H_

#include "ModuleParser.h"
#include "DocumentParser.h"
using namespace ::br::pucrio::telemidia::converter::framework;

#include <xercesc/dom/DOM.hpp>
XERCES_CPP_NAMESPACE_USE

#include <iostream>
#include <string>
using namespace std;

#include "NclImportParser.h"

namespace br {
namespace pucrio {
namespace telemidia {
namespace converter {
namespace framework {
namespace ncl {
	class NclTransitionParser : public ModuleParser {
		private:
			NclImportParser *importParser;
			void *presentationControlParser;

		public:
			NclTransitionParser(DocumentParser *documentParser);

			void *parseTransitionBase(
				    DOMElement *parentElement, void *objGrandParent);

			virtual void* createTransitionBase(
				    DOMElement *parentElement, void *objGrandParent)=0;

			virtual void addImportBaseToTransitionBase(
				    void *parentObject, void *childObject)=0;

			virtual void addTransitionToTransitionBase(
				    void *parentObject, void *childObject)=0;

			void* parseTransition(
				    DOMElement *parentElement, void *objGrandParent);

			virtual void* createTransition(
				    DOMElement *parentElement, void *objGrandParent)=0;

			NclImportParser *getImportParser();
			void setImportParser(NclImportParser *importParser);
	};
}
}
}
}
}
}

#endif //NCLTRANSITIONPARSER_H_