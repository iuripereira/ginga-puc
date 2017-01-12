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

#ifndef _LINKASSESSMENTSTATEMENT_H_
#define _LINKASSESSMENTSTATEMENT_H_

#include <vector>
#include <set>
using namespace std;

#include "FormatterEvent.h"
using namespace ::br::pucrio::telemidia::ginga::ncl::model::event;

#include "ncl/util/Comparator.h"
using namespace ::br::pucrio::telemidia::ncl::util;

#include "LinkStatement.h"
#include "LinkAssessment.h"
#include "LinkAttributeAssessment.h"

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace ncl {
namespace model {
namespace link {
	class LinkAssessmentStatement : public LinkStatement {
		protected:
			short comparator;
			LinkAssessment* otherAssessment;

		private:
			LinkAssessment *mainAssessment;

		public:
			LinkAssessmentStatement(short comparator,
				    LinkAttributeAssessment *mainAssessment,
				    LinkAssessment *otherAssessment);

			virtual ~LinkAssessmentStatement();
			vector<FormatterEvent*> *getEvents();
			short getComparator();
			void setComparator(short comp);
			LinkAssessment *getMainAssessment();
			void setMainAssessment(LinkAssessment *assessment);
			LinkAssessment* getOtherAssessment();
			void setOtherAssessment(LinkAssessment* assessment);
			virtual bool evaluate();
	};
}
}
}
}
}
}
}

#endif //_LINKASSESSMENTSTATEMENT_H_