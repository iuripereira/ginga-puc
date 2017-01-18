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

#ifndef _Metadata_H_
#define _Metadata_H_

#include "IMetadata.h"

#include "system/SystemCompat.h"
using namespace ::br::pucrio::telemidia::ginga::core::system::compat;


BR_PUCRIO_TELEMIDIA_GINGA_CORE_DATAPROCESSING_NCL_BEGIN

	class Metadata : public IMetadata {
		private:
			ofstream* mdFile;
			string name;
			double totalLength;
			double targetTotalLen;
			string baseUri;
			string mdUri;
			IDataFile* rootObject;
			map<int, IDataFile*>* dataFiles;

		public:
			Metadata(string name);
			virtual ~Metadata();

			string getRootUri();
			string getName();
			double getTotalLength();
			void setTargetTotalLength(double targetSize);
			bool isConsolidated();

			void setBaseUri(string uri);
			string getBaseUri();

			void setRootObject(IDataFile* rootObject);
			IDataFile* getRootObject();

			void addDataFile(IDataFile* dataObject);
			IDataFile* getDataFile(int structureId);
			map<int, IDataFile*>* getDataFiles();

		private:
			bool updateTotalLength(IDataFile* dataFile);

		public:
			vector<StreamData*>* createNCLSections();

		private:
			bool createMetadataFile();
			void closeMetadataFile();

			void openMetadataElement();
			void closeMetadataElement();

			void openBaseDataElement();
			void closeBaseDataElement();

			void writeRootElement(
					string sId, string uri, string size, string componentTag);

			void writeDataElement(
					string sId, string uri, string size, string componentTag);

			void copyContent(string uri, char* stream, int fileSize);

			StreamData* createStreamData(
					int structId,
					int structType,
					string uri,
					int fileSize);

		public:
			static int getFileSize(string uri);
	};

BR_PUCRIO_TELEMIDIA_GINGA_CORE_DATAPROCESSING_NCL_END
#endif //_Metadata_H_
