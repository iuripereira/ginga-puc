/******************************************************************************
Este arquivo eh parte da implementacao do ambiente declarativo do middleware
Ginga (Ginga-NCL).

Direitos Autorais Reservados (c) 1989-2007 PUC-Rio/Laboratorio TeleMidia

Este programa eh software livre; voce pode redistribui-lo e/ou modificah-lo sob
os termos da Licenca Publica Geral GNU versao 2 conforme publicada pela Free
Software Foundation.

Este programa eh distribuido na expectativa de que seja util, porem, SEM
NENHUMA GARANTIA; nem mesmo a garantia implicita de COMERCIABILIDADE OU
ADEQUACAO A UMA FINALIDADE ESPECIFICA. Consulte a Licenca Publica Geral do
GNU versao 2 para mais detalhes.

Voce deve ter recebido uma copia da Licenca Publica Geral do GNU versao 2 junto
com este programa; se nao, escreva para a Free Software Foundation, Inc., no
endereco 59 Temple Street, Suite 330, Boston, MA 02111-1307 USA.

Para maiores informacoes:
ncl @ telemidia.puc-rio.br
http://www.ncl.org.br
http://www.ginga.org.br
http://www.telemidia.puc-rio.br
******************************************************************************
This file is part of the declarative environment of middleware Ginga (Ginga-NCL)

Copyright: 1989-2007 PUC-RIO/LABORATORIO TELEMIDIA, All Rights Reserved.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License version 2 as published by
the Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License version 2 for more
details.

You should have received a copy of the GNU General Public License version 2
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

For further information contact:
ncl @ telemidia.puc-rio.br
http://www.ncl.org.br
http://www.ginga.org.br
http://www.telemidia.puc-rio.br
*******************************************************************************/

#include "../../../include/DataContentDescriptor.h"

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace tsparser {
namespace si{
namespace descriptors{
	DataContentDescriptor::DataContentDescriptor() {
		descriptorTag    = 0xC7;
		componentRef     = NULL;
		dataComponentId  = 0;
		descriptorLength = 0;
		selectorByte     = NULL;
		selectorLength   = 0;
		textChar         = NULL;
		textLength       = 0;
	}

	DataContentDescriptor::~DataContentDescriptor() {
		if(selectorByte != NULL){
			delete selectorByte;
			selectorByte = NULL;
		}
		if(textChar != NULL){
			delete textChar;
			textChar = NULL;
		}

	}
	unsigned int DataContentDescriptor::getDescriptorLength(){
		return (unsigned int) descriptorLength;
	}
	unsigned char DataContentDescriptor::getDescriptorTag(){
		return descriptorTag;
	}
	string DataContentDescriptor::getTextChar() {
		string str;

		str.append(textChar, textLength);
		return str;
	}
	string DataContentDescriptor::getLanguageCode() {
		string str;

		if (languageCode == NULL){
			return "";
		}
		str.append(languageCode, 3);
		return str;
	}
	void DataContentDescriptor::print() {
		cout << "DataContentDescriptor::print printing..." << endl;
		cout << " -descriptorLenght = "  << getDescriptorLength() << endl;
		cout << " -languageCode = "      << getLanguageCode()     << endl;
		cout << " -textChar = "          << getTextChar()         << endl;
	}
	size_t DataContentDescriptor::process (char* data, size_t pos){
		//cout << "DataContentDescriptor process with pos = " << pos;

		descriptorLength = data[pos+1];
		pos += 2;
		//cout << " and length = " << (descriptorLength & 0xFF) << endl;

		dataComponentId = (((data[pos] << 8) & 0x00FF)| (data[pos+1] & 0xFF));
		//cout << "Data dataComponentId = " << (dataComponentId & 0xFF) << endl;
		pos += 2;

		entryComponent = data[pos];
		pos ++;
		selectorLength = data[pos];
		//cout << "Data Contents entryComponent = " << (entryComponent & 0xFF) << " and";
		//cout << " selectorLength = " << (selectorLength & 0xFF) << endl;
		if(selectorLength > 0){
			selectorByte = new char[selectorLength];
			memset(selectorByte, 0 , selectorLength);
			memcpy(selectorByte, data+pos+1 , selectorLength);
			/*
			cout << "Data selectorByte = " << endl;
			for(int i = 0; i < selectorLength; i++){
				cout << selectorByte[i];
			}
			cout << endl;
			*/
		}
		pos += selectorLength + 1;
		numOfComponentRef = data[pos];
		if (numOfComponentRef > 0){
			componentRef = new char[numOfComponentRef];
			memset(componentRef, 0, numOfComponentRef);
			memcpy(componentRef, data+pos+1, numOfComponentRef);
			/*
			cout << "Data Comp Ref = " << endl;
			for(int i = 0; i < numOfComponentRef; i++){
				cout << componentRef[i];
			}
			cout << endl;
			*/
		}
		pos += numOfComponentRef + 1;

		memcpy(languageCode, data+pos, 3);
		pos += 3;
		/*
		cout << "Data languageCode = ";
		for (int i = 0; i < 3; i++){
			cout << languageCode[i];
		}
		cout << endl;
		*/
		textLength =  data[pos];
		if(textLength > 0){
			textChar = new char[textLength];
			memset(textChar, 0, textLength);
			memcpy(textChar, data+pos+1, textLength);
			/*
			cout << "Data textLength = " << (textLength & 0xFF) << endl;
			cout << "Data textChar = ";
			for(int i = 0; i < textLength; i++){
				cout << textChar[i];
			}
			cout << endl;
			*/
		}
		pos += textLength;
		return pos;
	}
}
}
}
}
}
}
}
}