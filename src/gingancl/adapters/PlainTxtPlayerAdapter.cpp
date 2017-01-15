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

#include "config.h"
#include "PlainTxtPlayerAdapter.h"
#include "player/PlainTxtPlayer.h"

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace ncl {
namespace adapters {
namespace text {
	PlainTxtPlayerAdapter::PlainTxtPlayerAdapter() : FormatterPlayerAdapter() {

	}

	void PlainTxtPlayerAdapter::createPlayer() {
		string paramValue;
		CascadingDescriptor* descriptor;

		if (fileExists(mrl)) {
			player = new PlainTxtPlayer(myScreen, mrl.c_str());
		} else {
			return;
		}

		player->setPropertyValue("x-setFile", mrl);
		descriptor = object->getDescriptor();
		if (descriptor != NULL) {
			paramValue = trim(descriptor->getParameterValue("fontUri"));
			if (paramValue == "") {
				paramValue = SystemCompat::appendGingaFilesPrefix("font/vera.ttf");
			}
			player->setPropertyValue("fontUri", paramValue);

			paramValue = trim(descriptor->getParameterValue("fontSize"));
			if (paramValue == "") {
				paramValue = "10";
			}
			player->setPropertyValue("fontSize", paramValue);

			paramValue = trim(descriptor->getParameterValue("x-rgbBgColor"));
			if (paramValue != "") {
				player->setPropertyValue("x-rgbBgColor", paramValue);
			}

			paramValue = trim(descriptor->getParameterValue("fontColor"));
			if (paramValue == "") {
				paramValue = trim(
						descriptor->getParameterValue("x-rgbFontColor"));

				if (paramValue == "") {
					paramValue = "255,255,255";
				}
				player->setPropertyValue("x-rgbFontColor", paramValue);

			} else {
				player->setPropertyValue("fontColor", paramValue);
			}
		}

		FormatterPlayerAdapter::createPlayer();
	}
}
}
}
}
}
}
}
