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

#ifndef PRESENTATION_ENGINE_MANAGER_H
#define PRESENTATION_ENGINE_MANAGER_H

#include "system/ITimeBaseProvider.h"
using namespace ::ginga::system;

#include "player/INCLPlayer.h"
using namespace ::ginga::player;

#include "system/Thread.h"
using namespace ::ginga::system;

#include "mb/Display.h"
#include "mb/ICmdEventListener.h"
using namespace ::ginga::mb;

#include "player/ShowButton.h"
#include "player/IPlayerListener.h"
using namespace ::ginga::player;

#include "formatter/PrivateBaseManager.h"
using namespace ::ginga::formatter;

GINGA_LSSM_BEGIN

class PresentationEngineManager : public IPlayerListener,
                                  public ICmdEventListener,
                                  public Thread
{
public:
  bool quit;
  GMutex quit_mutex;
  GCond quit_cond;

private:
  static const short UC_BACKGROUND = 0;
  static const short UC_PRINTSCREEN = 1;
  static const short UC_STOP = 2;
  static const short UC_PAUSE = 3;
  static const short UC_RESUME = 4;
  static const short UC_SHIFT = 5;
  int devClass;
  int x;
  int y;
  int w;
  int h;
  bool enableGfx;
  PrivateBaseManager *privateBaseManager;
  map<string, INCLPlayer *> formatters;
  set<INCLPlayer *> formattersToRelease;
  bool paused;
  string iconPath;
  bool isEmbedded;
  bool standAloneApp;
  bool isLocalNcl;
  void *dsmccListener;
  void *tuner;
  bool closed;
  bool hasTMPNotification;
  bool hasInteractivity;
  bool enableMulticast;
  bool exitOnEnd;
  bool disableFKeys;
  ITimeBaseProvider *timeBaseProvider;
  int currentPrivateBaseId;
  static bool autoProcess;
  vector<string> commands;
//  InputManager *im;
  ShowButton *sb;
  string nclFile;
  string interfId;

  static gpointer startPresentationThreadWrapper (gpointer data);

public:
  PresentationEngineManager (int devClass, int xOffset, int yOffset,
                             int width, int height, bool disableGfx,
                             bool useMulticast);
  virtual ~PresentationEngineManager ();
  void setExitOnEnd (bool exitOnEnd);
  void setDisableFKeys (bool disableFKeys);
  set<string> *createPortIdList (const string &nclFile);
  short getMappedInterfaceType (const string &nclFile, const string &portId);
  void autoMountOC (bool autoMountIt);
  void setCurrentPrivateBaseId (unsigned int baseId);
  void setTimeBaseProvider (ITimeBaseProvider *tmp);

  void startPresentationThread(void);

private:
  void printGingaWindows ();
  bool nclEdit (const string &nclEditApi);
  bool editingCommand (const string &commandTag, const string &commandPayload);

public:
  bool editingCommand (const string &editingCmd);

private:
  void close ();
  void registerKeys ();
  void setTimeBaseInfo (INCLPlayer *nclPlayer);

public:
  void getScreenShot ();
  bool getIsLocalNcl ();
  void setEmbedApp (bool isEmbedded);
  void setIsLocalNcl (bool isLocal, void *tuner = NULL);
  void setInteractivityInfo (bool hasInt);

private:
  INCLPlayer *createNclPlayer (const string &baseId, const string &fname);

public:
  NclPlayerData *createNclPlayerData ();
  void addPlayerListener (const string &nclFile, IPlayerListener *listener);
  void removePlayerListener (const string &nclFile, IPlayerListener *listener);
  bool openNclFile (const string &nclFile);
  bool startPresentation (const string &nclFile, const string &interfId);
  bool pausePresentation (const string &nclFile);
  bool resumePresentation (const string &nclFile);
  bool stopPresentation (const string &nclFile);
  bool setPropertyValue (const string &nclFile,
                         const string &interfaceId,
                         const string &value);
  string getPropertyValue (const string &nclFile, const string &interfaceId);
  bool stopAllPresentations ();
  bool abortPresentation (const string &nclFile);

private:
  void openNclDocument (const string &docUri, int x, int y, int w, int h);
  void pausePressed ();

public:
  void *getDsmccListener ();
  void setCmdFile (const string &cmdFile);
  static void *processAutoCmd (void *ptr);
  void waitUnlockCondition ();

private:
  void presentationCompleted (const string &formatterId);
  void releaseFormatter (const string &formatterId);
  bool checkStatus ();
  void updateStatus (short code,
                     const string &parameter,
                     short type,
                     const string &value);

  bool cmdEventReceived (const string &command, const string &args);
  static void *eventReceived (void *ptr);
  void readCommand (const string &command);
  bool getNclPlayer (const string &docLocation, INCLPlayer **player);
  bool getNclPlayer (const string &baseId, const string &docId, INCLPlayer **p);
  void updateFormatters (short command, const string &parameter = "");
  void run ();
};

GINGA_LSSM_END

#endif /* PRESENTATION_ENGINE_MANAGER_H */
