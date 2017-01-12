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

#ifndef NclDocumentConverter_H
#define NclDocumentConverter_H

#include "ncl/components/Node.h"
using namespace ::br::pucrio::telemidia::ncl::components;

#include "DocumentParser.h"
#include "ModuleParser.h"
using namespace ::br::pucrio::telemidia::converter::framework;

#include "NclDocumentParser.h"
using namespace ::br::pucrio::telemidia::converter::framework::ncl;

#include "ncl/NclDocument.h"
#include "ncl/IPrivateBaseContext.h"
using namespace ::br::pucrio::telemidia::ncl;

#include "IDocumentConverter.h"

#include <string>
using namespace std;

#include <xercesc/dom/DOM.hpp>
XERCES_CPP_NAMESPACE_USE

namespace br {
namespace pucrio {
namespace telemidia {
namespace converter {
namespace ncl {
  class NclDocumentConverter : public IDocumentConverter,
		  public NclDocumentParser {

	private:
		IPrivateBaseContext* privateBaseContext;
		bool ownManager;

		bool parseEntityVar;
		void* parentObject;

	public:
		NclDocumentConverter();
		virtual ~NclDocumentConverter();

		void setConverterInfo(
				IPrivateBaseContext* pbc, IDeviceLayout* deviceLayout);

	private:
		virtual void initializeInstance(std::string& data, short scenario);
		virtual void testInstance(std::string& data, short scenario);

	protected:
		void checkManager();
		virtual void initialize();
		virtual void* parseRootElement(DOMElement *rootElement);

	public:
		string getAttribute(void* element, string attribute);
		Node *getNode(string id);
		bool removeNode(Node *node);
		IPrivateBaseContext* getPrivateBaseContext();
		NclDocument* importDocument(string* docLocation);
		void* parseEntity(string entityLocation,
			    NclDocument* document,
			    void* parent);

		void* getObject(string tableName, string key) {
			return DocumentParser::getObject(tableName, key);
		}

		void* parse(string uri, string iUriD, string fUriD) {
			return DocumentParser::parse(uri, iUriD, fUriD);
		}
  };
}
}
}
}
}

#endif