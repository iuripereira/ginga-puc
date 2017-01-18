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

#ifndef FILESYSTEMPROVIDER_H_
#define FILESYSTEMPROVIDER_H_

#include "system/SystemCompat.h"
using namespace ::br::pucrio::telemidia::ginga::core::system::compat;

#include "IDataProvider.h"


BR_PUCRIO_TELEMIDIA_GINGA_CORE_TUNING_BEGIN

	class FileSystemProvider : public IDataProvider {
		protected:
			string fileName;
			FILE* fileDescriptor;
			short capabilities;
			ITProviderListener* listener;

			bool checkPossiblePacket(char *buff, const int &pos);
			int nextPacket(char *buff);
			int synchBuffer(char *buff, int diff);

		public:
			FileSystemProvider(string fileName);
			virtual ~FileSystemProvider();

			void setListener(ITProviderListener* listener);
			void attachFilter(IFrontendFilter* filter){};
			void removeFilter(IFrontendFilter* filter){};

			short getCaps();
			bool tune();

			Channel* getCurrentChannel();
			bool getSTCValue(uint64_t* stc, int* valueType);
			bool changeChannel(int factor);
			bool setChannel(string channelValue);
			int createPesFilter(int pid, int pesType, bool compositeFiler);
			string getPesFilterOutput();
			void close();

			char* receiveData(int* len);
	};

BR_PUCRIO_TELEMIDIA_GINGA_CORE_TUNING_END
#endif /*FILESYSTEMPROVIDER_H_*/
