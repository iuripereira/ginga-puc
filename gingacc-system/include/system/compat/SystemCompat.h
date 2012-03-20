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

#ifndef SystemCompat_H_
#define SystemCompat_H_

extern "C" {
#ifdef WIN32
	#include <sys/timeb.h>
	#include <sys/types.h>
	#include <time.h>
	#include <direct.h>
	#include <io.h>
	#include <windows.h>
	#include <winsock2.h>
#ifdef WINSTRUCTS
	#include <Ws2tcpip.h>
#endif
	#pragma comment(lib,"ws2_32.lib")
#else
	#include <dlfcn.h>
	#include <fcntl.h>
	#include <sys/param.h>
	#include <unistd.h>
	#include <sys/resource.h>
	#include <signal.h>
	#include <sys/utsname.h>
	#include <sys/sysinfo.h>

	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/ioctl.h>
	#include <sys/time.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <net/if.h>
#endif
}

#include "util/functions.h"
using namespace ::br::pucrio::telemidia::util;

#include <string>
using namespace std;

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace system {
namespace compat {
	class SystemCompat {
		private:
			static string filesPref;
			static string installPref;
			static string userCurrentPath;
			static string gingaCurrentPath;
			static string pathD;
			static string iUriD;
			static string fUriD;
			static bool initialized;

			static void checkValues();
			static void initializeGingaPath();
			static void initializeUserCurrentPath();
			static void initializeGingaConfigFile();

			static void sigpipeHandler(int x) throw(const char*);

		public:
			/********
			 * URIs *
			 ********/
			static string updatePath(string dir);
			static bool isXmlStr(string location);
			static bool isAbsolutePath(string path);

			static string getIUriD();
			static string getFUriD();

			static string getPath(string filename);

			static string getUserCurrentPath();

			/*
			 * updates the URL with specific system delimiter
			 * and append with ginga config files prefix
			 */
			static string appendGingaFilesPrefix(string relUrl);

			/*
			 * updates the URL with specific system delimiter
			 * and append with ginga config install prefix
			 */
			static string appendGingaInstallPrefix(string relUrl);


			/******************************
			 * Handling Dynamic libraries *
			 ******************************/
			static void* loadComponent(string libName, string symName);

			/****************
			 * SIG Handlers *
			 ****************/
			static void initializeSigpipeHandler();


			/*****************
			 * Embedded Info *
			 *****************/
			static string getOperatingSystem();
			static float getClockSpeed();
			static float getMemorySize();


			/**********************
			 * Specific Functions *
			 **********************/
			static void makeDir(const char* dirName, unsigned int mode);
			static void uSleep(unsigned int sleepTime);
	};
}
}
}
}
}
}
}

#endif /*SystemCompat_H_*/