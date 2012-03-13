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

#include "../config.h"

#include "tuner/ITuner.h"
using namespace ::br::pucrio::telemidia::ginga::core::tuning;

#include "tsparser/PipeFilter.h"
#include "tsparser/Demuxer.h"
using namespace ::br::pucrio::telemidia::ginga::core::tsparser;

#include "tsparser/AIT.h"
using namespace ::br::pucrio::telemidia::ginga::core::tsparser::si;

#if HAVE_COMPSUPPORT
#include "cm/IComponentManager.h"
using namespace ::br::pucrio::telemidia::ginga::core::cm;
#else
#include "tuner/Tuner.h"
#endif

#include <sys/types.h>
#include <fcntl.h>

int main(int argc, char** argv, char** envp) {
	IAIT* ait = new AIT();
	IDemuxer* demuxer = NULL;
	ITSFilter* pipeFilter = NULL;
	ITuner* tuner = NULL;
	int fdPipe = -1, fd = -1, ret;
	string pipeName, fileName;
	char buffer[188];

	pipeName = "dtv_channel.ts";
	fileName = "/tmp/TSPARSERTEST.ts";

#if HAVE_COMPSUPPORT
	IComponentManager* cm = IComponentManager::getCMInstance();
	tuner = ((TunerCreator*)(cm->getObject("Tuner")))();
#else
	tuner = new Tuner();
#endif

	demuxer = new Demuxer(tuner);

	((ITuner*)tuner)->tune();
	((IDemuxer*)demuxer)->waitProgramInformation();

	pipeFilter = new PipeFilter(0);
	pipeFilter->setDestName(pipeName);
	((IDemuxer*)demuxer)->addPesFilter(PFT_DEFAULTTS, pipeFilter);

	if (argc == 2 && strcmp(argv[1], "save-pipe") == 0) {
		fdPipe = open(pipeName.c_str(), O_RDONLY);
		fd = open(fileName.c_str(), O_CREAT | O_LARGEFILE | O_WRONLY, 0644);

		while (true) {
			ret = read(fdPipe, buffer, ITSPacket::TS_PACKET_SIZE);
			if (ret == ITSPacket::TS_PACKET_SIZE) {
				write(fd, buffer, ITSPacket::TS_PACKET_SIZE);

			} else {
				break;
			}
		}
	}
	//TODO: tests

	clog << "gingacc-tsparser test all done. press enter to exit" << endl;
	getchar();
	return 0;
}
