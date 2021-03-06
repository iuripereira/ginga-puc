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

#ifndef TRANSPORTPROTOCOLDESCRIPTOR_H_
#define TRANSPORTPROTOCOLDESCRIPTOR_H_

#include "IMpegDescriptor.h"

GINGA_TSPARSER_BEGIN

class TransportProtocolDescriptor : public IMpegDescriptor
{
private:
  unsigned short protocolId;
  unsigned char transportProtocolLabel;
  unsigned char selectorByteLength;
  char *selectorByte;

public:
  TransportProtocolDescriptor ();
  virtual ~TransportProtocolDescriptor ();
  unsigned char getDescriptorTag ();
  unsigned int getDescriptorLength ();
  size_t process (char *data, size_t pos);
  void print ();
  unsigned int getSelectorByteLength ();
  unsigned short getProtocolId ();
  unsigned char getTransportProtocolLabel ();
  char *getSelectorByte ();
};

GINGA_TSPARSER_END

#endif /* TRANSPORTPROTOCOLDESCRIPTOR_H_ */
