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

#include "isdbt-dataproc/ncl/Metadata.h"

#include "util/functions.h"
using namespace ::br::pucrio::telemidia::util;

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace dataprocessing {
namespace ncl {
	Metadata::Metadata(string name) {
		this->name           = name;
		this->targetTotalLen = 0;
		this->totalLength    = 0;
		this->baseUri        = "";
		this->mdUri          = "";
		this->rootObject     = NULL;
		this->dataFiles      = new map<int, IDataFile*>;
	}

	Metadata::~Metadata() {
		map<int, IDataFile*>::iterator i;

		if (dataFiles != NULL) {
			i = dataFiles->begin();
			while (i != dataFiles->end()) {
				delete i->second;
				++i;
			}

			delete dataFiles;
			dataFiles = NULL;
		}
	}

	string Metadata::getRootUri() {
		string rootUri = "";

		if (rootObject != NULL) {
			rootUri = rootObject->getUri();
			if (!SystemCompat::isAbsolutePath(rootUri)) {
				rootUri = SystemCompat::updatePath(baseUri + SystemCompat::getIUriD() + rootUri);
			}
		}

		return rootUri;
	}

	string Metadata::getName() {
		return this->name;
	}

	double Metadata::getTotalLength() {
		return this->totalLength;
	}

	void Metadata::setTargetTotalLength(double targetSize) {
		this->targetTotalLen = targetSize;
	}

	bool Metadata::isConsolidated() {
		bool consolidated = false;

		if (targetTotalLen == totalLength) {
			consolidated = true;
		}

		return consolidated;
	}

	void Metadata::setBaseUri(string uri) {
		this->baseUri = uri;
		this->mdUri   = SystemCompat::updatePath(baseUri + "/metadata.xml");
	}

	string Metadata::getBaseUri() {
		return baseUri;
	}

	void Metadata::setRootObject(IDataFile* rootObject) {
		if (updateTotalLength(rootObject)) {
			this->rootObject = rootObject;
		}
	}

	IDataFile* Metadata::getRootObject() {
		return rootObject;
	}

	void Metadata::addDataFile(IDataFile* dataFile) {
		updateTotalLength(dataFile);
		(*dataFiles)[dataFile->getId()] = dataFile;
	}

	IDataFile* Metadata::getDataFile(int structureId) {
		map<int, IDataFile*>::iterator i;
		IDataFile* dataFile = NULL;

		i = dataFiles->find(structureId);
		if (i != dataFiles->end()) {
			dataFile = i->second;
		}

		return dataFile;
	}

	map<int, IDataFile*>* Metadata::getDataFiles() {
		return dataFiles;
	}

	bool Metadata::updateTotalLength(IDataFile* dataFile) {
		if (rootObject != dataFile &&
				dataFiles->count(dataFile->getId()) == 0) {

			totalLength = totalLength + dataFile->getSize();
			return true;
		}
		return false;
	}

	vector<StreamData*>* Metadata::createNCLSections() {
		map<int, IDataFile*>::iterator i;
		vector<StreamData*>* streams;
		string fileUri, cTag;
		int structId, fileSize;
		IDataFile* dataFile;
		StreamData* streamData;

		streams = new vector<StreamData*>;

		createMetadataFile();
		openMetadataElement();
		openBaseDataElement();

		structId   = rootObject->getId();
		fileUri    = rootObject->getUri();
		fileSize   = rootObject->getSize();
		cTag       = rootObject->getCopmonentTag();

		writeDataElement(itos(structId), fileUri, itos(fileSize), cTag);
		streamData = createStreamData(
				structId, ST_DATAFILE, fileUri, fileSize);

		streams->push_back(streamData);

		i = dataFiles->begin();
		while (i != dataFiles->end()) {
			structId   = i->first;
			dataFile   = i->second;
			fileUri    = dataFile->getUri();
			fileSize   = dataFile->getSize();
			cTag       = dataFile->getCopmonentTag();

			writeDataElement(itos(structId), fileUri, itos(fileSize), cTag);

			streamData = createStreamData(
					structId, ST_DATAFILE, fileUri, fileSize);

			streams->push_back(streamData);
			++i;
		}

		closeBaseDataElement();
		closeMetadataElement();
		closeMetadataFile();

		fileSize = getFileSize(mdUri);
		streamData = createStreamData(0, ST_METADA, mdUri, fileSize);
		streams->insert(streams->begin(), streamData);

		return streams;
	}

	bool Metadata::createMetadataFile() {
		remove(mdUri.c_str());

		mdFile = new ofstream(mdUri.c_str(), ios::out | ios::binary);

		if (!mdFile) {
			clog << "Metadata::createMetadataFile warning: can't create '";
			clog << mdUri << "'" << endl;
			return false;

		} else {
			mdFile->seekp(ofstream::end);
			if (mdFile->tellp() > 2) {
				clog << "ComponentDescription::createFile warning: opening ";
				clog << " existent file '" << mdFile << "' pos = '";
				clog << mdFile->tellp() << "'" << endl;
				return false;
			}
			mdFile->seekp(0);
		}

		return true;
	}

	void Metadata::closeMetadataFile() {
		mdFile->close();
		delete mdFile;
		mdFile = NULL;
	}

	void Metadata::openMetadataElement() {
		*mdFile << "<metadata name=\"" << name << "\" size=\"";
		*mdFile << totalLength << "\">" << endl;
	}

	void Metadata::closeMetadataElement() {
		*mdFile << "</metadata>" << endl;
	}

	void Metadata::openBaseDataElement() {
		*mdFile << "  <baseData uri=\"" << baseUri << "\">" << endl;
	}

	void Metadata::closeBaseDataElement() {
		*mdFile << "  </baseData>" << endl;
	}

	void Metadata::writeRootElement(
			string sId, string uri, string size, string componentTag) {

		*mdFile << "    <pushedRoot structureId=\"" << sId << "\" uri=\"";
		*mdFile << uri << "\" size=\"" << size << "\" component_tag=\"";
		*mdFile << componentTag << "\"/>" << endl;
	}

	void Metadata::writeDataElement(
			string sId, string uri, string size, string componentTag) {

		*mdFile << "    <pushedData structureId=\"" << sId << "\" uri=\"";
		*mdFile << uri << "\" size=\"" << size << "\" component_tag=\"";
		*mdFile << componentTag << "\"/>" << endl;
	}

	void Metadata::copyContent(string uri, char* stream, int fileSize) {
		FILE* fd;
		int bytes;
		string absUri;

		if (SystemCompat::isAbsolutePath(uri)) {
			absUri = uri;

		} else {
			absUri = SystemCompat::updatePath(baseUri + SystemCompat::getIUriD() + uri);
		}

		fd = fopen(absUri.c_str(), "rb");
		if (fd != NULL) {
			bytes = fread(stream, 1, fileSize, fd);
			if (bytes != fileSize) {
				clog << "Metadata::copyContent Warning! Can't read '";
				clog << fileSize << "' from file '" << absUri << "' (";
				clog << bytes << " bytes read)" << endl;
			}
			fclose(fd);

		} else {
			clog << "Metadata::copyContent Warning! Can't open file '";
			clog << absUri << "'" << endl;
		}
	}

	StreamData* Metadata::createStreamData(
			int structId,
			int structType,
			string uri,
			int fileSize) {

		StreamData* streamData;
		char* stream = new char[fileSize + 2];

		stream[0]    = (((unsigned int)structType) & 0xFF);
		stream[1]    = (((unsigned int)structId) & 0xFF);

		copyContent(uri, stream + 2, fileSize);

		streamData = new StreamData;
		streamData->stream = stream;
		streamData->size   = fileSize + 2;

		return streamData;
	}

	int Metadata::getFileSize(string uri) {
		FILE* fd;
		int bytes = 0;

		fd = fopen(uri.c_str(), "rb");
		if (fd > 0) {
			fseek(fd, 0L, SEEK_END);
			bytes = ftell(fd);
			fclose(fd);
		}

		return bytes;
	}
}
}
}
}
}
}
}