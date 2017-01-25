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

#include "ginga.h"
#include "SDLDisplay.h"

#include "DisplayManager.h"
#include "InputManager.h"
#include "SDLAudioProvider.h"
#include "SDLEventBuffer.h"
#include "SDLFontProvider.h"
#include "SDLImageProvider.h"
#include "SDLInputEvent.h"
#include "SDLSurface.h"
#include "SDLVideoProvider.h"
#include "SDLWindow.h"

GINGA_MB_BEGIN

// Global display; initialized by main().
SDLDisplay *_Ginga_Display = NULL;

map<SDLDisplay *, short> SDLDisplay::sdlScreens;
bool SDLDisplay::hasRenderer = false;
bool SDLDisplay::hasERC = false;
bool SDLDisplay::mutexInit = false;

map<int, int> SDLDisplay::gingaToSDLCodeMap;
map<int, int> SDLDisplay::sdlToGingaCodeMap;
map<string, int> SDLDisplay::sdlStrToSdlCode;

set<SDL_Texture *> SDLDisplay::uTexPool;
set<SDL_Surface *> SDLDisplay::uSurPool;
vector<ReleaseContainer *> SDLDisplay::releaseList;
map<int, map<double, set<SDLWindow *> *> *>
    SDLDisplay::renderMap;
set<IContinuousMediaProvider *> SDLDisplay::cmpRenderList;

const unsigned int SDLDisplay::DSA_UNKNOWN = 0;
const unsigned int SDLDisplay::DSA_4x3 = 1;
const unsigned int SDLDisplay::DSA_16x9 = 2;

pthread_mutex_t SDLDisplay::sdlMutex;
pthread_mutex_t SDLDisplay::sieMutex;
pthread_mutex_t SDLDisplay::renMutex;
pthread_mutex_t SDLDisplay::scrMutex;
pthread_mutex_t SDLDisplay::recMutex;
pthread_mutex_t SDLDisplay::winMutex;
pthread_mutex_t SDLDisplay::surMutex;
pthread_mutex_t SDLDisplay::proMutex;
pthread_mutex_t SDLDisplay::cstMutex;

SDLDisplay::SDLDisplay (int argc, char **args,
                                  UnderlyingWindowID embedId,
                                  bool externalRenderer)
{
  string parentCoords = "";
  int i;

  pthread_t tId;
  pthread_attr_t tattr;

  pthread_attr_init (&tattr);
  pthread_attr_setdetachstate (&tattr, PTHREAD_CREATE_DETACHED);
  pthread_attr_setscope (&tattr, PTHREAD_SCOPE_SYSTEM);

  hRes = 0;
  wRes = 0;
  im = NULL;
  useStdin = false;
  uParentId = NULL;
  uEmbedId = embedId;
  uEmbedFocused = false;
  mustGainFocus = false;
  renderer = NULL;
  mbMode = "";
  mbSubSystem = "";
  screen = NULL;
  sdlId = 0;
  backgroundLayer = NULL;
  fullScreen = false;
  winIdRefCounter = 1;

  if (externalRenderer)
    {
      hasERC = externalRenderer;
    }

  for (i = 0; i < argc; i++)
    {
      if ((strcmp (args[i], "subsystem") == 0) && ((i + 1) < argc))
        {
          mbSubSystem.assign (args[i + 1]);
        }
      else if ((strcmp (args[i], "parent") == 0) && ((i + 1) < argc))
        {
          parentCoords.assign (args[i + 1]);
        }
      else if ((strcmp (args[i], "mode") == 0) && ((i + 1) < argc))
        {
          mbMode.assign (args[i + 1]);
          if (mbMode == "fullscreen")
            {
              fullScreen = true;
            }
        }
      else if ((strcmp (args[i], "poll-stdin") == 0))
        {
          useStdin = true;
        }
    }

#ifdef _MSC_VER
  putenv ("SDL_AUDIODRIVER=DirectSound");
#endif

  waitingCreator = false;
  Thread::mutexInit (&condMutex);
  Thread::condInit (&cond, NULL);

  checkMutexInit ();

  if (!hasRenderer)
    {
      hasRenderer = true;

      if (!hasERC)
        {
          setInitScreenFlag ();

          pthread_create (&tId, &tattr, SDLDisplay::rendererT, this);
          pthread_detach (tId);
        }
      else
        {
          setInitScreenFlag ();
          rendererT (this);
        }
    }
  else
    {
      setInitScreenFlag ();

      if (hasERC)
        {
          rendererT (this);
        }
    }

  if (useStdin)
    {
      pthread_create (&tId, &tattr, SDLDisplay::checkStdin, this);
      pthread_detach (tId);
    }
}

SDLDisplay::~SDLDisplay ()
{
  map<SDLDisplay *, short>::iterator i;
  map<int, map<double, set<SDLWindow *> *> *>::iterator j;
  map<double, set<SDLWindow *> *>::iterator k;

  useStdin = false;

  waitingCreator = false;
  Thread::mutexDestroy (&condMutex);
  Thread::condDestroy (&cond);

  Thread::mutexLock (&renMutex);
  j = renderMap.find (0);
  if (j != renderMap.end ())
    {
      k = j->second->begin ();
      while (k != j->second->end ())
        {
          delete k->second;
          ++k;
        }
      delete j->second;
      renderMap.erase (j);
    }
  Thread::mutexUnlock (&renMutex);

  if (im != NULL)
    {
      delete im;
      im = NULL;
    }

  releaseScreen ();

  while (this->renderer != NULL)
    {
      g_usleep (10000);
    }

  Thread::mutexLock (&scrMutex);
  i = sdlScreens.find (this);
  if (i != sdlScreens.end ())
    {
      sdlScreens.erase (i);
    }

  if (sdlScreens.empty ())
    {
      hasRenderer = false;
      sdlQuit ();
    }
  Thread::mutexUnlock (&scrMutex);

  clog << "SDLDisplay::~SDLDisplay all done" << endl;
}

void
SDLDisplay::checkMutexInit ()
{
  if (!mutexInit)
    {
      mutexInit = true;

      Thread::mutexInit (&sdlMutex, true);
      Thread::mutexInit (&sieMutex, true);
      Thread::mutexInit (&renMutex, true);
      Thread::mutexInit (&scrMutex, true);
      Thread::mutexInit (&recMutex, true);
      Thread::mutexInit (&winMutex, true);
      Thread::mutexInit (&surMutex, true);
      Thread::mutexInit (&proMutex, true);
      Thread::mutexInit (&cstMutex, true);
    }
}

void
SDLDisplay::lockSDL ()
{
  checkMutexInit ();

  Thread::mutexLock (&sdlMutex);
}

void
SDLDisplay::unlockSDL ()
{
  checkMutexInit ();

  Thread::mutexUnlock (&sdlMutex);
}

void
SDLDisplay::updateRenderMap (SDLWindow *window,
                             double oldZIndex, double newZIndex)
{
  checkMutexInit ();

  renderMapRemoveWindow (window, oldZIndex);
  renderMapInsertWindow (window, newZIndex);
}

void
SDLDisplay::releaseScreen ()
{
  Thread::mutexLock (&scrMutex);
  sdlScreens[this] = SPT_RELEASE;
  Thread::mutexUnlock (&scrMutex);
}

int
SDLDisplay::getWidthResolution ()
{
  while (wRes <= 0)
    {
      g_usleep (uSleepTime);
    }
  return wRes;
}

void
SDLDisplay::setWidthResolution (int wRes)
{
  this->wRes = wRes;

  lockSDL ();
  if (screen != NULL)
    {
      SDL_SetWindowSize (screen, this->wRes, this->hRes);
    }
  unlockSDL ();
}

int
SDLDisplay::getHeightResolution ()
{
  /*
   * hRes == 0 is an initial state. So pthread_cond_t
   * is not necessary here.
   */
  while (hRes <= 0)
    {
      g_usleep (uSleepTime);
    }

  clog << "SDLDisplay::getHeightResolution returns '";
  clog << hRes << "'";
  clog << endl;

  return hRes;
}

void
SDLDisplay::setHeightResolution (int hRes)
{
  this->hRes = hRes;

  lockSDL ();
  if (screen != NULL)
    {
      SDL_SetWindowSize (screen, this->wRes, this->hRes);
    }
  unlockSDL ();

  clog << "SDLDisplay::setHeightResolution to '";
  clog << hRes << "'";
  clog << endl;
}

SDLWindow *
SDLDisplay::getIWindowFromId (GingaWindowID winId)
{
  map<GingaWindowID, SDLWindow *>::iterator i;
  SDLWindow *iWin = NULL;

  Thread::mutexLock (&winMutex);
  i = windowRefs.find (winId);
  if (i != windowRefs.end ())
    {
      iWin = i->second;
    }
  Thread::mutexUnlock (&winMutex);

  return iWin;
}

bool
SDLDisplay::mergeIds (GingaWindowID destId,
                           vector<GingaWindowID> *srcIds)
{
  map<GingaWindowID, SDLWindow *>::iterator i;
  vector<GingaWindowID>::iterator j;
  SDLWindow *destWin;
  SDL_Surface *destSur;
  bool merged = false;
  int w, h;

  lockSDL ();

  Thread::mutexLock (&winMutex);
  i = windowRefs.find (destId);
  if (i != windowRefs.end ())
    {
      destWin = (SDLWindow *)(i->second);

      w = destWin->getW ();
      h = destWin->getH ();

      destSur = createUnderlyingSurface (w, h);

      j = srcIds->begin ();
      while (j != srcIds->end ())
        {
          i = windowRefs.find (*j);
          if (i != windowRefs.end ())
            {
              if (blitFromWindow (i->second, destSur))
                {
                  clog << "SDLDisplay::mergeIds merged '";
                  clog << (unsigned long)(*j) << "' on destination '";
                  clog << (unsigned long)destId << "'" << endl;

                  merged = true;
                }
              else
                {
                  clog << "SDLDisplay::mergeIds can't merge '";
                  clog << (unsigned long)(*j) << "' on destination '";
                  clog << (unsigned long)destId << "'" << endl;
                }
            }
          ++j;
        }

      Thread::mutexUnlock (&winMutex);
      unlockSDL ();

      destWin->setRenderedSurface (destSur);
    }
  else
    {
      clog << "SDLDisplay::mergeIds can't find destination window '";
      clog << (unsigned long)destId << "'" << endl;

      Thread::mutexUnlock (&winMutex);
      unlockSDL ();
    }

  return merged;
}

void
SDLDisplay::blitScreen (SDLSurface *destination)
{
  SDL_Surface *dest;

  lockSDL ();
  dest = (SDL_Surface *)(destination->getSurfaceContent ());
  if (dest == NULL)
    {
      dest = createUnderlyingSurface (wRes, hRes);
      destination->setSurfaceContent (dest);
    }

  blitScreen (dest);

  unlockSDL ();
}

void
SDLDisplay::blitScreen (string fileUri)
{
  SDL_Surface *dest;

  lockSDL ();
  dest = createUnderlyingSurface (wRes, hRes);
  blitScreen (dest);

  if (SDL_SaveBMP_RW (dest, SDL_RWFromFile (fileUri.c_str (), "wb"), 1) < 0)
    {
      clog << "SDLDisplay::blitScreen SDL error: '";
      clog << SDL_GetError () << "'" << endl;
    }
  unlockSDL ();
}

void
SDLDisplay::blitScreen (SDL_Surface *dest)
{
  map<int, map<double, set<SDLWindow *> *> *>::iterator i;
  map<double, set<SDLWindow *> *>::iterator j;
  set<SDLWindow *>::iterator k;

  Thread::mutexLock (&renMutex);
  i = renderMap.find (0);
  if (i != renderMap.end ())
    {
      j = i->second->begin ();
      while (j != i->second->end ())
        {
          k = j->second->begin ();
          while (k != j->second->end ())
            {
              blitFromWindow ((*k), dest);
              ++k;
            }
          ++j;
        }
    }
  Thread::mutexUnlock (&renMutex);
}

void
SDLDisplay::setInitScreenFlag ()
{
  Thread::mutexLock (&scrMutex);
  sdlScreens[this] = SPT_INIT;
  Thread::mutexUnlock (&scrMutex);
}

/* interfacing output */

SDLWindow *
SDLDisplay::createWindow (int x, int y, int w, int h, double z)
{
  SDLWindow *iWin;

  Thread::mutexLock (&winMutex);

  iWin = new SDLWindow ((GingaWindowID)winIdRefCounter, 0, x, y, w, h,
                        z);

  windowRefs[(GingaWindowID)winIdRefCounter] = iWin;

  winIdRefCounter++;
  windowPool.insert (iWin);
  renderMapInsertWindow (iWin, z);

  Thread::mutexUnlock (&winMutex);

  return iWin;
}

bool
SDLDisplay::hasWindow (SDLWindow *win)
{
  set<SDLWindow *>::iterator i;
  bool hasWin = false;

  Thread::mutexLock (&winMutex);

  i = windowPool.find (win);
  if (i != windowPool.end ())
    {
      hasWin = true;
    }

  Thread::mutexUnlock (&winMutex);

  return hasWin;
}

void
SDLDisplay::releaseWindow (SDLWindow *win)
{
  set<SDLWindow *>::iterator i;
  map<GingaWindowID, SDLWindow *>::iterator j;
  SDLWindow *iWin;
  SDL_Texture *uTex = NULL;
  bool uTexOwn;

  Thread::mutexLock (&winMutex);
  i = windowPool.find (win);
  if (i != windowPool.end ())
    {
      iWin = (SDLWindow *)(*i);

      j = windowRefs.find (iWin->getId ());
      if (j != windowRefs.end ())
        {
          windowRefs.erase (j);
        }

      renderMapRemoveWindow (iWin, iWin->getZ ());

      windowPool.erase (i);

      uTex = iWin->getTexture (NULL);
      uTexOwn = iWin->isTextureOwner (uTex);

      iWin->clearContent ();
      iWin->setTexture (NULL);

      Thread::mutexUnlock (&winMutex);

      if (uTexOwn)
        {
          createReleaseContainer (NULL, uTex, NULL);
        }
    }
  else
    {
      Thread::mutexUnlock (&winMutex);
    }
}

SDLSurface *
SDLDisplay::createSurface ()
{
  return createSurfaceFrom (NULL);
}

SDLSurface *
SDLDisplay::createSurface (int w, int h)
{
  SDLSurface *iSur = NULL;
  SDL_Surface *uSur = NULL;

  lockSDL ();

  uSur = createUnderlyingSurface (w, h);

  iSur = new SDLSurface (uSur);

  unlockSDL ();

  Thread::mutexLock (&surMutex);
  surfacePool.insert (iSur);
  Thread::mutexUnlock (&surMutex);

  return iSur;
}

SDLSurface *
SDLDisplay::createSurfaceFrom (void *uSur)
{
  SDLSurface *iSur = NULL;

  lockSDL ();
  if (uSur != NULL)
    {
      iSur = new SDLSurface (uSur);
    }
  else
    {
      iSur = new SDLSurface ();
    }
  unlockSDL ();

  Thread::mutexLock (&surMutex);
  surfacePool.insert (iSur);
  Thread::mutexUnlock (&surMutex);

  return iSur;
}

bool
SDLDisplay::hasSurface (SDLSurface *s)
{
  set<SDLSurface *>::iterator i;
  bool hasSur = false;

  Thread::mutexLock (&surMutex);
  i = surfacePool.find (s);
  if (i != surfacePool.end ())
    {
      hasSur = true;
    }
  Thread::mutexUnlock (&surMutex);

  return hasSur;
}

bool
SDLDisplay::releaseSurface (SDLSurface *s)
{
  set<SDLSurface *>::iterator i;
  bool released = false;

  Thread::mutexLock (&surMutex);
  i = surfacePool.find (s);
  if (i != surfacePool.end ())
    {
      surfacePool.erase (i);
      released = true;
    }
  Thread::mutexUnlock (&surMutex);

  return released;
}

/* interfacing content */
IContinuousMediaProvider *
SDLDisplay::createContinuousMediaProvider (const char *mrl,
                                                arg_unused (bool isRemote))
{
  IContinuousMediaProvider *provider;
  string strSym;

  lockSDL ();
  strSym = "SDLVideoProvider";
  provider = new SDLVideoProvider (mrl);
  unlockSDL ();

  Thread::mutexLock (&proMutex);
  cmpPool.insert (provider);
  Thread::mutexUnlock (&proMutex);

  return provider;
}

void
SDLDisplay::releaseContinuousMediaProvider (
    IContinuousMediaProvider *provider)
{
  set<IContinuousMediaProvider *>::iterator i;
  IContinuousMediaProvider *cmp;

  Thread::mutexLock (&proMutex);
  i = cmpPool.find (provider);
  if (i != cmpPool.end ())
    {
      cmp = (*i);
      cmpPool.erase (i);
      cmp->stop ();

      Thread::mutexUnlock (&proMutex);
      createReleaseContainer (NULL, NULL, cmp);
    }
  else
    {
      Thread::mutexUnlock (&proMutex);
    }
}

IFontProvider *
SDLDisplay::createFontProvider (const char *mrl, int fontSize)
{
  IFontProvider *provider = NULL;

  lockSDL ();
  provider = new SDLFontProvider (mrl, fontSize);
  unlockSDL ();

  Thread::mutexLock (&proMutex);
  dmpPool.insert (provider);
  Thread::mutexUnlock (&proMutex);

  return provider;
}

void
SDLDisplay::releaseFontProvider (IFontProvider *provider)
{
  set<IDiscreteMediaProvider *>::iterator i;
  IDiscreteMediaProvider *dmp;

  Thread::mutexLock (&proMutex);
  i = dmpPool.find (provider);
  if (i != dmpPool.end ())
    {
      dmp = (*i);
      dmpPool.erase (i);

      Thread::mutexUnlock (&proMutex);
      createReleaseContainer (NULL, NULL, dmp);
    }
  else
    {
      Thread::mutexUnlock (&proMutex);
    }
}

IImageProvider *
SDLDisplay::createImageProvider (const char *mrl)
{
  IImageProvider *provider = NULL;

  provider = new SDLImageProvider (mrl);

  Thread::mutexLock (&proMutex);
  dmpPool.insert (provider);
  Thread::mutexUnlock (&proMutex);

  return provider;
}

void
SDLDisplay::releaseImageProvider (IImageProvider *provider)
{
  set<IDiscreteMediaProvider *>::iterator i;
  IDiscreteMediaProvider *dmp;

  Thread::mutexLock (&proMutex);
  i = dmpPool.find (provider);
  if (i != dmpPool.end ())
    {
      dmp = (*i);
      dmpPool.erase (i);

      Thread::mutexUnlock (&proMutex);
      createReleaseContainer (NULL, NULL, dmp);
    }
  else
    {
      Thread::mutexUnlock (&proMutex);
    }
}

SDLSurface *
SDLDisplay::createRenderedSurfaceFromImageFile (const char *mrl)
{
  SDLSurface *iSur = NULL;
  IImageProvider *provider = NULL;

  provider = createImageProvider (mrl);
  if (provider == NULL)
    {
      return NULL;
    }
  iSur = createSurfaceFrom (NULL);
  Ginga_Display_M->registerSurface (iSur);
  provider->playOver (iSur->getId ());

  releaseImageProvider (provider);
  return iSur;
}

void
SDLDisplay::addCMPToRendererList (IContinuousMediaProvider *cmp)
{
  checkMutexInit ();

  Thread::mutexLock (&proMutex);
  if (cmp->getHasVisual ())
    {
      cmpRenderList.insert (cmp);
    }
  Thread::mutexUnlock (&proMutex);
}

void
SDLDisplay::removeCMPToRendererList (IContinuousMediaProvider *cmp)
{
  set<IContinuousMediaProvider *>::iterator i;

  checkMutexInit ();

  Thread::mutexLock (&proMutex);
  i = cmpRenderList.find (cmp);
  if (i != cmpRenderList.end ())
    {
      cmpRenderList.erase (i);
    }
  Thread::mutexUnlock (&proMutex);
}

void
SDLDisplay::createReleaseContainer (SDL_Surface *uSur,
                                         SDL_Texture *uTex,
                                         IMediaProvider *iDec)
{
  ReleaseContainer *rc;

  checkMutexInit ();

  Thread::mutexLock (&recMutex);

  rc = new ReleaseContainer;
  rc->iDec = iDec;
  rc->uSur = uSur;
  rc->uTex = uTex;

  releaseList.push_back (rc);

  Thread::mutexUnlock (&recMutex);
}

void
SDLDisplay::checkSDLInit ()
{
  Uint32 subsystem_init = SDL_WasInit (0);

  if (subsystem_init == 0)
    {
      if (SDL_Init ((Uint32) (SDL_INIT_AUDIO | SDL_INIT_VIDEO
                              | SDL_INIT_TIMER | SDL_INIT_NOPARACHUTE))
          != 0)
        {
          clog << "SDLDisplay::checkSDLInit ";
          clog << "Warning! " << SDL_GetError ();
          clog << endl;
        }
    }
  else
    {
      if ((subsystem_init & SDL_INIT_AUDIO) == 0)
        {
          SDL_InitSubSystem (SDL_INIT_AUDIO);
        }

      if ((subsystem_init & SDL_INIT_VIDEO) == 0)
        {
          SDL_InitSubSystem (SDL_INIT_VIDEO);
        }

      if ((subsystem_init & SDL_INIT_TIMER) == 0)
        {
          SDL_InitSubSystem (SDL_INIT_TIMER);
        }
    }
}

void
SDLDisplay::notifyQuit ()
{
  map<SDLDisplay *, short>::iterator i;
  SDLDisplay *s;

  Thread::mutexLock (&scrMutex);

  i = sdlScreens.begin ();
  while (i != sdlScreens.end ())
    {
      s = i->first;
      if (s->im != NULL)
        s->im->postInputEvent (CodeMap::KEY_QUIT);
      ++i;
    }
  Thread::mutexUnlock (&scrMutex);

  clog << "SDLDisplay::notifyQuit all done!" << endl;
}

void
SDLDisplay::sdlQuit ()
{
  if (SDL_GetAudioStatus () != SDL_AUDIO_STOPPED)
    {
      SDL_PauseAudio (1);
      SDL_CloseAudio ();
      SDL_AudioQuit ();
    }
  SDL_Quit ();
  clog << "SDLDisplay::sdlQuit all done!" << endl;
}

void
SDLDisplay::checkWindowFocus (SDLDisplay *s, SDL_Event *event)
{
  if (s->uEmbedId != NULL)
    {
      if (event->type == SDL_WINDOWEVENT
          && event->window.windowID == s->sdlId)
        {
          switch (event->window.event)
            {
            case SDL_WINDOWEVENT_SHOWN:
              clog << "SDLDisplay::checkWindowFocus ";
              clog << "Window '" << event->window.windowID;
              clog << "' shown" << endl;
              break;

            case SDL_WINDOWEVENT_HIDDEN:
              clog << "SDLDisplay::checkWindowFocus ";
              clog << "Window '" << event->window.windowID;
              clog << "' hidden" << endl;
              break;

            case SDL_WINDOWEVENT_EXPOSED:
              clog << "SDLDisplay::checkWindowFocus ";
              clog << "Window '" << event->window.windowID;
              clog << "' exposed" << endl;

              s->mustGainFocus = true;
              break;

            case SDL_WINDOWEVENT_MOVED:
              clog << "SDLDisplay::checkWindowFocus ";
              clog << "Window '" << event->window.windowID;
              clog << "' moved to '";
              clog << event->window.data1;
              clog << "," << event->window.data2 << "'";
              clog << endl;
              break;

            case SDL_WINDOWEVENT_RESIZED:
              clog << "SDLDisplay::checkWindowFocus ";
              clog << "Window '" << event->window.windowID;
              clog << "' resized to '";
              clog << event->window.data1;
              clog << "," << event->window.data2 << "'";
              clog << endl;

              s->wRes = event->window.data1;
              s->hRes = event->window.data2;

              if (s->im != NULL)
                {
                  s->im->setAxisBoundaries (s->wRes, s->hRes, 0);
                }

              break;

            case SDL_WINDOWEVENT_MINIMIZED:
              clog << "SDLDisplay::checkWindowFocus ";
              clog << "Window '" << event->window.windowID;
              clog << "' minimized" << endl;
              break;

            case SDL_WINDOWEVENT_MAXIMIZED:
              clog << "SDLDisplay::checkWindowFocus ";
              clog << "Window '" << event->window.windowID;
              clog << "' maximized" << endl;
              break;

            case SDL_WINDOWEVENT_RESTORED:
              clog << "SDLDisplay::checkWindowFocus ";
              clog << "Window '" << event->window.windowID;
              clog << "' restored to '";
              clog << event->window.data1;
              clog << ", " << event->window.data2 << "'";
              clog << endl;
              break;

            case SDL_WINDOWEVENT_ENTER:
              clog << "SDLDisplay::checkWindowFocus ";
              clog << "Window '" << event->window.windowID;
              clog << "' Mouse entered" << endl;

              s->mustGainFocus = true;
              break;

            case SDL_WINDOWEVENT_LEAVE:
              clog << "SDLDisplay::checkWindowFocus ";
              clog << "Window '" << event->window.windowID;
              clog << "' Mouse left" << endl;
              break;

            case SDL_WINDOWEVENT_FOCUS_GAINED:
              clog << "SDLDisplay::checkWindowFocus ";
              clog << "Window '" << event->window.windowID;
              clog << "' gained keyboard focus" << endl;

              s->uEmbedFocused = true;
              break;

            case SDL_WINDOWEVENT_FOCUS_LOST:
              clog << "SDLDisplay::checkWindowFocus ";
              clog << "Window '" << event->window.windowID;
              clog << "' lost keyboard focus" << endl;

              s->uEmbedFocused = false;
              break;

            case SDL_WINDOWEVENT_CLOSE:
              clog << "SDLDisplay::checkWindowFocus ";
              clog << "Window '" << event->window.windowID;
              clog << "' closed" << endl;
              break;

            default:
              clog << "SDLDisplay::checkWindowFocus ";
              clog << "Window '" << event->window.windowID;
              clog << "' got unknown event '";
              clog << event->window.event << "'" << endl;
              break;
            }
        }
    }
}

bool
SDLDisplay::notifyEvent (SDLDisplay *s, SDL_Event *event,
                              bool capsOn, bool shiftOn)
{
  SDLEventBuffer *eventBuffer = NULL;
  checkWindowFocus (s, event);

  if (s->im != NULL)
    {
      eventBuffer = (SDLEventBuffer *)(s->im->getEventBuffer ());
      if (((SDLEventBuffer::checkEvent (s->sdlId, *event)
            && s->uEmbedId == NULL)
           || checkEventFocus (s)))
        {
          eventBuffer->feed (*event, capsOn, shiftOn);
          return true;
        }
    }

  return false;
}

void
SDLDisplay::processCmd (SDLDisplay *s, string cmd, string type,
                             string args)
{
  int intEvent;
  SDL_Event ie;
  size_t token;

  assert (s->useStdin);
  if (s->uEmbedId != NULL)
    {
      ie.window.windowID = (Uint32) (unsigned long)s->uEmbedId;
    }

  if (type == "GIEK")
    {
      intEvent = convertEventCodeStrToInt (cmd);
      if (intEvent >= 0)
        {
          if (cmd != "GIEK:QUIT")
            {
              ie.type = SDL_KEYDOWN;
              ie.key.type = SDL_KEYDOWN;
              ie.key.state = SDL_PRESSED;
              ie.key.repeat = 0;
              ie.key.keysym.sym = intEvent;
            }
          else
            {
              ie.type = SDL_QUIT;
            }
          clog << "SDLDisplay::processCmd pushing keyboard '";
          clog << cmd << "'" << endl;
          SDL_PushEvent (&ie);
        }
    }
  else if (type == "GIEC")
    {
      token = args.find_first_of (",");
      if (token != std::string::npos)
        {
          int x = atoi (args.substr (0, token).c_str ());
          int y
              = atoi (args.substr (token + 1, args.length () - (token + 1))
                          .c_str ());
          ie.type = SDL_MOUSEBUTTONUP;
          ie.button.x = x;
          ie.button.y = y;

          clog << "SDLDisplay::processCmd pushing click '";
          clog << cmd << "' on '" << args << "'" << endl;
          SDL_PushEvent (&ie);
        }
    }
  else if (type == "GCMD")
    {
      string nCmd = args;
      string nArgs = "";

      token = args.find_first_of (",");
      if (token != std::string::npos)
        {
          nCmd = args.substr (0, token);
          nArgs = args.substr (token + 1, args.length () - (token + 1));
        }

      if (s->im != NULL)
        {
          clog << "SDLDisplay::processCmd calling postCommand" << endl;
          clog << "nCmd: " << nCmd << endl;
          clog << "nArgs: " << nArgs << endl;
          s->im->postCommand (nCmd, nArgs);
        }
    }
}

void *
SDLDisplay::checkStdin (void *ptr)
{
  SDLDisplay *s;
  string strCmd;
  string cmdType;

  clog << "SDLDisplay::checkStdin calling cin" << endl;

  Thread::mutexLock (&scrMutex);
  s = (SDLDisplay *)ptr;
  Thread::mutexUnlock (&scrMutex);

  while (true)
    {
      size_t token;

      std::getline (std::cin, strCmd);
      token = strCmd.find_first_of (":");
      if (token != std::string::npos && token < strCmd.length ())
        {
          processCmd (
              s, strCmd, strCmd.substr (0, token),
              strCmd.substr (token + 1, strCmd.length () - (token + 1)));
        }

      if (strCmd == "GIEK:QUIT")
        {
          clog << "SDLDisplay::checkStdin QUIT";
          clog << endl;
          break;
        }

      strCmd = "";
    }

  clog << "SDLDisplay::checkStdin all done" << endl;

  return NULL;
}

bool
SDLDisplay::checkEvents ()
{
  map<SDLDisplay *, short>::iterator i;
  SDLDisplay *s;
  SDL_Event event;
  bool shiftOn = false;
  bool capsOn = false;
  bool hasEvent;

  hasEvent = SDL_PollEvent (&event);

  while (hasEvent)
    {
      // clog << "SDLDisplay::checkEvents poll event";
      // clog << " type '" << event.type << "'" << endl;

      if (event.type == SDL_KEYDOWN)
        {
          clog << "SDLDisplay::checkEvents poll event '";
          clog << event.key.keysym.sym << "'" << endl;

          if (event.key.keysym.sym == SDLK_LSHIFT
              || event.key.keysym.sym == SDLK_RSHIFT)
            {
              shiftOn = true;
            }
        }
      else if (event.type == SDL_KEYUP)
        {
          if (event.key.keysym.sym == SDLK_CAPSLOCK)
            {
              capsOn = !capsOn;
            }
          else if (event.key.keysym.sym == SDLK_LSHIFT
                   || event.key.keysym.sym == SDLK_RSHIFT)
            {
              shiftOn = false;
            }
        }
      else if (event.type == SDL_QUIT)
        {
          notifyQuit ();
          sdlQuit ();
          hasRenderer = false;
          clog << "SDLDisplay::checkEvents QUIT" << endl;

          return false;
        }

      Thread::mutexLock (&scrMutex);
      i = sdlScreens.begin ();
      while (i != sdlScreens.end ())
        {
          s = i->first;
          notifyEvent (s, &event, capsOn, shiftOn);
          ++i;
        }
      Thread::mutexUnlock (&scrMutex);

      hasEvent = SDL_PollEvent (&event);
    }

  return true;
}

void *
SDLDisplay::rendererT (arg_unused (void *ptr))
{
  map<SDLDisplay *, short>::iterator i;
  SDLDisplay *s;
  pthread_mutex_t mutex;
  pthread_cond_t cond;

  checkSDLInit ();

  Thread::mutexInit (&mutex, false);
  Thread::condInit (&cond, NULL);

  while (hasRenderer)
    {
      if (!checkEvents ())
        {
          return NULL;
        }

      Thread::mutexLock (&scrMutex);
      i = sdlScreens.begin ();
      while (i != sdlScreens.end ())
        {
          s = i->first;

          switch (i->second)
            {
            case SPT_NONE:

              refreshCMP (s);
              refreshWin (s);

              if (s->mustGainFocus)
                {
                  if (!s->uEmbedFocused)
                    {
                      forceInputFocus (s, s->uEmbedId);
                    }
                  s->mustGainFocus = false;
                }

              refreshRC (s);
              ++i;
              break;

            case SPT_INIT:
              initScreen (s);
              sdlScreens[s] = SPT_NONE;
              i = sdlScreens.begin ();
              break;

            case SPT_CLEAR:
              clearScreen (s);
              sdlScreens[s] = SPT_NONE;
              i = sdlScreens.begin ();
              break;

            case SPT_RELEASE:
              releaseScreen (s);
              sdlScreens.erase (i);
              i = sdlScreens.begin ();
              break;

            default:
              i = sdlScreens.end ();
              break;
            }
        }
      Thread::mutexUnlock (&scrMutex);
      if (hasERC)
        {
          break;
        }
    }

  clog << "SDLDisplay::rendererT ALL DONE" << endl;
  return NULL;
}

void
SDLDisplay::refreshRC (SDLDisplay *s)
{
  vector<ReleaseContainer *>::iterator i;
  ReleaseContainer *rc;
  IMediaProvider *dec;
  SDL_Surface *sur;
  SDL_Texture *tex;
  IContinuousMediaProvider *cmp;
  IDiscreteMediaProvider *dmp;
  string strSym = "";

  set<IDiscreteMediaProvider *>::iterator j;

  Thread::mutexLock (&recMutex);

  if (s->releaseList.empty ())
    {
      Thread::mutexUnlock (&recMutex);
      return;
    }

  i = s->releaseList.begin ();
  while (i != s->releaseList.end ())
    {
      rc = (*i);

      dec = rc->iDec;
      sur = rc->uSur;
      tex = rc->uTex;

      delete rc;

      s->releaseList.erase (i);
      Thread::mutexUnlock (&recMutex);

      if (sur != NULL)
        {
          releaseUnderlyingSurface (sur);
        }

      if (tex != NULL)
        {
          releaseTexture (tex);
        }

      if (dec != NULL)
        {
          strSym = "";

          cmp = dynamic_cast<IContinuousMediaProvider *> (dec);

          if (cmp != NULL)
            {
              strSym = cmp->getLoadSymbol ();
              delete cmp;
            }
          else
            {
              dmp = dynamic_cast<IDiscreteMediaProvider *> (dec);

              if (dmp != NULL)
                {
                  Thread::mutexLock (&proMutex);
                  j = s->dmpPool.find (dmp);
                  if (j != s->dmpPool.end ())
                    {
                      s->dmpPool.erase (j);
                    }
                  Thread::mutexUnlock (&proMutex);

                  strSym = dmp->getLoadSymbol ();
                  delete dmp;
                }
            }
        }

      Thread::mutexLock (&recMutex);
      i = s->releaseList.begin ();
    }

  s->releaseList.clear ();
  Thread::mutexUnlock (&recMutex);
}

int
SDLDisplay::refreshCMP (SDLDisplay *s)
{
  set<IContinuousMediaProvider *>::iterator i;
  set<IContinuousMediaProvider *>::iterator j;
  IContinuousMediaProvider *cmp;
  int size;

  Thread::mutexLock (&proMutex);
  size = (int) cmpRenderList.size ();
  i = cmpRenderList.begin ();
  while (i != cmpRenderList.end ())
    {
      cmp = (*i);
      j = s->cmpPool.find (cmp);
      if (j != s->cmpPool.end ())
        {
          if (cmp->getProviderContent () == NULL)
            {
              initCMP (s, cmp);
            }
          else
            {
              cmp->refreshDR (NULL);
            }
        }
      ++i;
    }
  Thread::mutexUnlock (&proMutex);

  return size;
}

void
SDLDisplay::refreshWin (SDLDisplay *s)
{
  SDL_Texture *uTex;
  SDLWindow *dstWin;
  SDLWindow *mirrorSrc;

  map<int, map<double, set<SDLWindow *> *> *>::iterator i;
  map<double, set<SDLWindow *> *>::iterator j;
  set<SDLWindow *>::iterator k;

  Thread::mutexLock (&renMutex);
  if (s->renderer != NULL && !renderMap.empty ())
    {
      lockSDL ();
      SDL_RenderClear (s->renderer);
      unlockSDL ();

      i = renderMap.find (0);
      if (i != renderMap.end ())
        {
          j = i->second->begin ();
          while (j != i->second->end ())
            {
              k = j->second->begin ();
              while (k != j->second->end ())
                {
                  dstWin = (SDLWindow *)(*k);

                  if (dstWin->isVisible () && !dstWin->isGhostWindow ())
                    {
                      mirrorSrc = (SDLWindow *)dstWin->getMirrorSrc ();
                      if (mirrorSrc != NULL)
                        {
                          while (mirrorSrc->getMirrorSrc () != NULL)
                            {
                              mirrorSrc
                                  = (SDLWindow *)mirrorSrc->getMirrorSrc ();
                            }
                          uTex = mirrorSrc->getTexture (s->renderer);
                        }
                      else
                        {
                          uTex = dstWin->getTexture (s->renderer);
                        }

                      drawSDLWindow (s->renderer, uTex, dstWin);
                      dstWin->rendered ();
                    }

                  ++k;
                }
              ++j;
            }
        }

      lockSDL ();
      SDL_RenderPresent (s->renderer);
      unlockSDL ();
    }
  Thread::mutexUnlock (&renMutex);
}

void
SDLDisplay::initEmbed (SDLDisplay *s, arg_unused (UnderlyingWindowID uWin))
{
  SDL_SysWMinfo info;

  lockSDL ();
  SDL_VERSION (&info.version);
  SDL_GetWindowWMInfo (s->screen, &info);

#if defined(SDL_VIDEO_DRIVER_X11)
// XSetWindowAttributes attributes;

// if (info.info.x11.display == NULL) {
// 	info.info.x11.display = XOpenDisplay(getenv("DISPLAY"));
// }

// if (info.info.x11.display != NULL) {
// 	attributes.event_mask = (
// 			FocusChangeMask       |
// 			EnterWindowMask       |
// 			LeaveWindowMask       |
// 			ExposureMask          |
// 			ButtonPressMask       |
// 			ButtonReleaseMask     |
// 			PointerMotionMask     |
// 			KeyPressMask          |
// 			KeyReleaseMask        |
// 			PropertyChangeMask    |
// 			StructureNotifyMask   |
// 			KeymapStateMask);

// 	attributes.override_redirect = False;

// 	XChangeWindowAttributes(
// 			info.info.x11.display,
// 			(Window)uWin,
// 			(CWEventMask | CWOverrideRedirect),
// 			&attributes);

// 	XFlush(info.info.x11.display);

// 	XSync(info.info.x11.display, 1);

// 	clog << "SDLDisplay::initEmbed set attributes for '";
// 	clog << uWin << "'" << endl;

// } else {
// 	clog << "SDLDisplay::initEmbed Warning! ";
// 	clog << "Can't set input event mask for embedded ";
// 	clog << "window '" << uWin << "'" << endl;
// }

#elif defined(SDL_VIDEO_DRIVER_WINDOWS)
// TODO: Windows input event configuration
#endif

  unlockSDL ();
}

void
SDLDisplay::forceInputFocus (arg_unused (SDLDisplay *s),
                                  arg_unused (UnderlyingWindowID uWin))
{
  lockSDL ();

#if defined(SDL_VIDEO_DRIVER_X11)
// Window focusedWindow;
// int revert;
// SDL_SysWMinfo info;

// SDL_VERSION(&info.version);
// SDL_GetWindowWMInfo(s->screen, &info);

// if (info.info.x11.display == NULL) {
// 	info.info.x11.display = XOpenDisplay(getenv("DISPLAY"));
// }

// if (info.info.x11.display != NULL) {
// 	XGetInputFocus(
// 			info.info.x11.display,
// 			&focusedWindow,
// 			&revert);

// 	if (focusedWindow != (Window)uWin) {
// 		XSetInputFocus(
// 				info.info.x11.display,
// 				(Window)uWin,
// 				RevertToParent,
// 				CurrentTime);

// 		XCirculateSubwindowsUp(
// 				info.info.x11.display,
// 				(Window)uWin);

// 		XFlush(info.info.x11.display);

// 		XSync(info.info.x11.display, 1);

// 		clog << "SDLDisplay::forceInputFocus set input for '";
// 		clog << uWin << "'" << endl;
// 	}

// } else {
// 	clog << "SDLDisplay::forceInputFocus Warning! ";
// 	clog << "Can't set input event mask for embedded ";
// 	clog << "window '" << uWin << "'" << endl;
// }
#endif
  unlockSDL ();
}

void
SDLDisplay::initScreen (SDLDisplay *s)
{
  SDL_Rect rect;
  int i, numOfDrivers, x, y;
  string title = "";

  lockSDL ();

  if (s->mbSubSystem != "")
    {
      numOfDrivers = SDL_GetNumVideoDrivers ();
      for (i = 0; i < numOfDrivers; i++)
        {
          if (strcmp (SDL_GetVideoDriver (i), s->mbSubSystem.c_str ()) == 0)
            {
              SDL_VideoInit (s->mbSubSystem.c_str ());
              break;
            }
        }
    }
  else
    {
      SDL_VideoInit (NULL);
    }

  if (SDL_GetDisplayBounds (0, &rect) != 0)
    {
      rect.x = 0;
      rect.y = 0;
      rect.w = 0;
      rect.h = 0;
    }

  if (s->uEmbedId != NULL)
    {
      s->screen = SDL_CreateWindowFrom (s->uEmbedId);
      if (s->screen != NULL)
        {
          SDL_GetWindowSize (s->screen, &s->wRes, &s->hRes);
          s->sdlId = SDL_GetWindowID (s->screen);
          initEmbed (s, s->uEmbedId);
        }
    }
  else
    {
      title = string (PACKAGE_NAME) + " " + PACKAGE_VERSION;
      if (s->mbMode != "" && s->mbMode.find ("x") != std::string::npos)
        {
          s->wRes = xstrto_int (
              s->mbMode.substr (0, s->mbMode.find_first_of ("x")));

          s->hRes = xstrto_int (s->mbMode.substr (
              s->mbMode.find_first_of ("x") + 1,
              (s->mbMode.length () - (s->mbMode.find_first_of ("x")) + 1)));
        }
      else if (rect.w > 0 && rect.h > 0)
        {
          s->wRes = rect.w;
          s->hRes = rect.h;
        }
      else
        {
          s->wRes = 1920;
          s->hRes = 1080;
        }

      if (rect.w > 0 && (s->wRes <= 0 || s->wRes > rect.w))
        {
          s->wRes = rect.w;
        }

      if (rect.h > 0 && (s->hRes <= 0 || s->hRes > rect.h))
        {
          s->hRes = rect.h;
        }

      x = 0;
      y = 0;

      if (rect.w > 0)
        {
          x = (rect.w - s->wRes) / 2;
        }

      if (rect.h > 0)
        {
          y = (rect.h - s->hRes) / 2;
        }

      Uint32 winFlags;

      if (s->fullScreen)
        {
          winFlags = (Uint32) (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_SHOWN);
        }
      else
        {
          winFlags = (Uint32) (SDL_WINDOW_SHOWN);
        }

      s->screen = SDL_CreateWindow (title.c_str (), x, y, s->wRes, s->hRes,
                                    winFlags);

      s->sdlId = SDL_GetWindowID (s->screen);
    }

  assert (s->screen != NULL);

  SDL_RendererInfo info;
  int d = SDL_GetNumRenderDrivers ();
  for (int i = 0; i < d; i++)
    {
      SDL_GetRenderDriverInfo (i, &info);
      clog << "SDLDisplay::initScreen renderer driver: (" << i
           << ") = '" << info.name << "'";
      clog << endl;
    }

  s->renderer
      = SDL_CreateRenderer (s->screen, -1, SDL_RENDERER_ACCELERATED);

  if (s->renderer == NULL)
    {
      clog << "SDLDisplay::initScreen Warning! Can't create "
              "accelerated "
              "renderer";
      clog << endl;

      s->renderer
          = SDL_CreateRenderer (s->screen, -1, SDL_RENDERER_TARGETTEXTURE);

      if (s->renderer == NULL)
        {
          clog
              << "SDLDisplay::initScreen Warning! Can't create target "
                 "renderer";
          clog << endl;
          s->renderer
              = SDL_CreateRenderer (s->screen, -1, SDL_RENDERER_SOFTWARE);
        }
    }

  assert (s->renderer != NULL);

  initCodeMaps ();

  s->im = new InputManager ();

  if (s->im != NULL)
    s->im->setAxisBoundaries (s->wRes, s->hRes, 0);

  unlockSDL ();
}

void
SDLDisplay::clearScreen (SDLDisplay *s)
{
  SDLWindow *iWin;
  SDLSurface *iSur;
  IContinuousMediaProvider *iCmp;
  IDiscreteMediaProvider *iDmp;

  set<SDLWindow *>::iterator i;
  set<SDLSurface *>::iterator j;
  set<IContinuousMediaProvider *>::iterator k;
  set<IDiscreteMediaProvider *>::iterator l;

  Thread::mutexLock (&winMutex);

  // Releasing remaining Window objects in Window Pool
  if (!s->windowPool.empty ())
    {
      i = s->windowPool.begin ();
      while (i != s->windowPool.end ())
        {
          iWin = (*i);
          if (iWin != NULL)
            {
              delete iWin;
            }
          ++i;
        }
      s->windowPool.clear ();
    }
  Thread::mutexUnlock (&winMutex);

  Thread::mutexLock (&surMutex);
  // Releasing remaining Surface objects in Surface Pool
  if (!s->surfacePool.empty ())
    {
      j = s->surfacePool.begin ();
      while (j != s->surfacePool.end ())
        {
          iSur = (*j);
          if (iSur != NULL)
            {
              delete iSur;
            }
          ++j;
        }
      s->surfacePool.clear ();
    }

  Thread::mutexUnlock (&surMutex);

  Thread::mutexLock (&proMutex);

  // Releasing remaining CMP objects in CMP Pool
  if (!s->cmpPool.empty ())
    {
      k = s->cmpPool.begin ();
      while (k != s->cmpPool.end ())
        {
          iCmp = (*k);

          if (iCmp != NULL)
            {
              iCmp->stop ();
              delete iCmp;
            }
          ++k;
        }
      s->cmpPool.clear ();
    }

  // Releasing remaining DMP objects in DMP Pool
  if (!s->dmpPool.empty ())
    {
      l = s->dmpPool.begin ();
      while (l != s->dmpPool.end ())
        {
          iDmp = *l;

          if (iDmp != NULL)
            {
              delete iDmp;
            }
          ++l;
        }
      s->dmpPool.clear ();
    }

  Thread::mutexUnlock (&proMutex);
}

void
SDLDisplay::releaseScreen (SDLDisplay *s)
{
  lockSDL ();

  clearScreen (s);

  if (s->uEmbedId == NULL)
    {
      if (s->screen != NULL)
        {
          SDL_HideWindow (s->screen);
        }

      if (s->renderer != NULL)
        {
          SDL_DestroyRenderer (s->renderer);
          s->renderer = NULL;
        }

      if (s->screen != NULL)
        {
          SDL_DestroyWindow (s->screen);
          s->screen = NULL;
        }
    }

  unlockSDL ();
}

void
SDLDisplay::releaseAll ()
{
  map<SDLDisplay *, short>::iterator i;

  Thread::mutexLock (&scrMutex);

  i = sdlScreens.begin ();
  while (i != sdlScreens.begin ())
    {
      releaseScreen (i->first);
      ++i;
    }
  sdlScreens.clear ();

  Thread::mutexUnlock (&scrMutex);
}

void
SDLDisplay::initCMP (SDLDisplay *s, IContinuousMediaProvider *cmp)
{
  SDL_Texture *texture;
  int w, h;

  cmp->getOriginalResolution (&w, &h);

  /*clog << "SDLDisplay::initCMP creating texture with w = '";
  clog << w << "' and h = '" << h << "'" << endl;*/

  texture = createTexture (s->renderer, w, h);
  cmp->setProviderContent ((void *)texture);
}

bool
SDLDisplay::blitFromWindow (SDLWindow *iWin, SDL_Surface *dest)
{
  SDL_Surface *tmpSur;
  SDL_Texture *tmpTex;
  SDL_Rect rect;

  bool blitted = false;
  bool freeSurface = false;

  lockSDL ();
  iWin->lock ();
  tmpSur = (SDL_Surface *)(iWin->getContent ());

  if (tmpSur == NULL)
    {
      tmpTex = ((SDLWindow *)iWin)->getTexture (NULL);
      if (hasTexture (tmpTex))
        {
          tmpSur = createUnderlyingSurfaceFromTexture (tmpTex);
          freeSurface = true;
        }
    }

  if (tmpSur != NULL)
    {
      rect.x = iWin->getX ();
      rect.y = iWin->getY ();
      rect.w = iWin->getW ();
      rect.h = iWin->getH ();

      if (SDL_UpperBlitScaled (tmpSur, NULL, dest, &rect) < 0)
        {
          SDL_Surface *tmpSur2;

          clog << "SDLDisplay::blitFromWindow SDL error: '";
          clog << SDL_GetError () << "'! Trying to convert source surface";
          clog << endl;

          tmpSur2 = SDL_ConvertSurface (tmpSur, dest->format, 0);

          if (tmpSur2 != NULL)
            {
              if (SDL_UpperBlitScaled (tmpSur2, NULL, dest, &rect) < 0)
                {
                  clog << "SDLDisplay::blitFromWindow ";
                  clog << "BLIT from converted surface SDL error: '";
                  clog << SDL_GetError () << "'";
                  clog << endl;
                }
              else
                {
                  blitted = true;
                }
              createReleaseContainer (tmpSur2, NULL, NULL);
            }
          else
            {
              clog << "SDLDisplay::blitFromWindow convert surface";
              clog << " SDL error: '";
              clog << SDL_GetError () << "'" << endl;
            }
        }
      else
        {
          blitted = true;
        }
    }
  else
    {
      clog << "SDLDisplay::blitFromWindow can't blit from '";
      clog << (unsigned long)iWin->getId () << "' null texture" << endl;
    }

  if (freeSurface)
    {
      freeSurface = false;
      releaseUnderlyingSurface (tmpSur);
    }

  iWin->unlock ();
  unlockSDL ();

  return blitted;
}

/* interfacing input */

InputManager *
SDLDisplay::getInputManager ()
{
  /*
   * im == NULL is an initial state. So pthread_cond_t
   * is not necessary here.
   */
  while (im == NULL)
    {
      g_usleep (1000000 / SDS_FPS);
    }
  return im;
}

SDLEventBuffer *
SDLDisplay::createEventBuffer ()
{
  return new SDLEventBuffer ();
}

SDLInputEvent *
SDLDisplay::createInputEvent (void *event, const int symbol)
{
  SDLInputEvent *ie = NULL;

  if (event != NULL)
    {
      ie = new SDLInputEvent (*(SDL_Event *)event);
    }

  if (symbol >= 0)
    {
      ie = new SDLInputEvent (symbol);
    }

  return ie;
}

SDLInputEvent *
SDLDisplay::createApplicationEvent (int type, void *data)
{
  return new SDLInputEvent (type, data);
}

int
SDLDisplay::fromMBToGinga (int keyCode)
{
  map<int, int>::iterator i;
  int translated;

  checkMutexInit ();

  Thread::mutexLock (&sieMutex);

  translated = CodeMap::KEY_NULL;
  i = sdlToGingaCodeMap.find (keyCode);
  if (i != sdlToGingaCodeMap.end ())
    {
      translated = i->second;
    }
  else
    {
      clog << "SDLDisplay::fromMBToGinga can't find code '";
      clog << keyCode << "' returning KEY_NULL" << endl;
    }

  Thread::mutexUnlock (&sieMutex);

  return translated;
}

int
SDLDisplay::fromGingaToMB (int keyCode)
{
  map<int, int>::iterator i;
  int translated;

  checkMutexInit ();

  Thread::mutexLock (&sieMutex);

  translated = CodeMap::KEY_NULL;
  i = gingaToSDLCodeMap.find (keyCode);
  if (i != gingaToSDLCodeMap.end ())
    {
      translated = i->second;
    }
  else
    {
      clog << "SDLDisplay::fromGingaToMB can't find code '";
      clog << keyCode << "' returning KEY_NULL" << endl;
    }

  Thread::mutexUnlock (&sieMutex);

  return translated;
}

/* libgingaccmbsdl internal use*/

/* input */
int
SDLDisplay::convertEventCodeStrToInt (string strEvent)
{
  int intEvent = -1;
  map<string, int>::iterator i;

  i = sdlStrToSdlCode.find (strEvent);
  if (i != sdlStrToSdlCode.end ())
    {
      intEvent = i->second;
    }

  return intEvent;
}

void
SDLDisplay::initCodeMaps ()
{
  checkMutexInit ();

  Thread::mutexLock (&sieMutex);
  if (!gingaToSDLCodeMap.empty ())
    {
      Thread::mutexUnlock (&sieMutex);
      return;
    }

  // sdlStrToSdlCode
  sdlStrToSdlCode["GIEK:QUIT"] = SDL_QUIT;
  sdlStrToSdlCode["GIEK:UNKNOWN"] = SDLK_UNKNOWN;
  sdlStrToSdlCode["GIEK:0"] = SDLK_0;
  sdlStrToSdlCode["GIEK:1"] = SDLK_1;
  sdlStrToSdlCode["GIEK:2"] = SDLK_2;
  sdlStrToSdlCode["GIEK:3"] = SDLK_3;
  sdlStrToSdlCode["GIEK:4"] = SDLK_4;
  sdlStrToSdlCode["GIEK:5"] = SDLK_5;
  sdlStrToSdlCode["GIEK:6"] = SDLK_6;
  sdlStrToSdlCode["GIEK:7"] = SDLK_7;
  sdlStrToSdlCode["GIEK:8"] = SDLK_8;
  sdlStrToSdlCode["GIEK:9"] = SDLK_9;

  sdlStrToSdlCode["GIEK:a"] = SDLK_a;
  sdlStrToSdlCode["GIEK:b"] = SDLK_b;
  sdlStrToSdlCode["GIEK:c"] = SDLK_c;
  sdlStrToSdlCode["GIEK:d"] = SDLK_d;
  sdlStrToSdlCode["GIEK:e"] = SDLK_e;
  sdlStrToSdlCode["GIEK:f"] = SDLK_f;
  sdlStrToSdlCode["GIEK:g"] = SDLK_g;
  sdlStrToSdlCode["GIEK:h"] = SDLK_h;
  sdlStrToSdlCode["GIEK:i"] = SDLK_i;
  sdlStrToSdlCode["GIEK:j"] = SDLK_j;
  sdlStrToSdlCode["GIEK:k"] = SDLK_k;
  sdlStrToSdlCode["GIEK:l"] = SDLK_l;
  sdlStrToSdlCode["GIEK:m"] = SDLK_m;
  sdlStrToSdlCode["GIEK:n"] = SDLK_n;
  sdlStrToSdlCode["GIEK:o"] = SDLK_o;
  sdlStrToSdlCode["GIEK:p"] = SDLK_p;
  sdlStrToSdlCode["GIEK:q"] = SDLK_q;
  sdlStrToSdlCode["GIEK:r"] = SDLK_r;
  sdlStrToSdlCode["GIEK:s"] = SDLK_s;
  sdlStrToSdlCode["GIEK:t"] = SDLK_t;
  sdlStrToSdlCode["GIEK:u"] = SDLK_u;
  sdlStrToSdlCode["GIEK:v"] = SDLK_v;
  sdlStrToSdlCode["GIEK:w"] = SDLK_w;
  sdlStrToSdlCode["GIEK:x"] = SDLK_x;
  sdlStrToSdlCode["GIEK:y"] = SDLK_y;
  sdlStrToSdlCode["GIEK:z"] = SDLK_z;

  sdlStrToSdlCode["GIEK:A"] = SDLK_a + 5000;
  sdlStrToSdlCode["GIEK:B"] = SDLK_b + 5000;
  sdlStrToSdlCode["GIEK:C"] = SDLK_c + 5000;
  sdlStrToSdlCode["GIEK:D"] = SDLK_d + 5000;
  sdlStrToSdlCode["GIEK:E"] = SDLK_e + 5000;
  sdlStrToSdlCode["GIEK:F"] = SDLK_f + 5000;
  sdlStrToSdlCode["GIEK:G"] = SDLK_g + 5000;
  sdlStrToSdlCode["GIEK:H"] = SDLK_h + 5000;
  sdlStrToSdlCode["GIEK:I"] = SDLK_i + 5000;
  sdlStrToSdlCode["GIEK:J"] = SDLK_j + 5000;
  sdlStrToSdlCode["GIEK:K"] = SDLK_k + 5000;
  sdlStrToSdlCode["GIEK:L"] = SDLK_l + 5000;
  sdlStrToSdlCode["GIEK:M"] = SDLK_m + 5000;
  sdlStrToSdlCode["GIEK:N"] = SDLK_n + 5000;
  sdlStrToSdlCode["GIEK:O"] = SDLK_o + 5000;
  sdlStrToSdlCode["GIEK:P"] = SDLK_p + 5000;
  sdlStrToSdlCode["GIEK:Q"] = SDLK_q + 5000;
  sdlStrToSdlCode["GIEK:R"] = SDLK_r + 5000;
  sdlStrToSdlCode["GIEK:S"] = SDLK_s + 5000;
  sdlStrToSdlCode["GIEK:T"] = SDLK_t + 5000;
  sdlStrToSdlCode["GIEK:U"] = SDLK_u + 5000;
  sdlStrToSdlCode["GIEK:V"] = SDLK_v + 5000;
  sdlStrToSdlCode["GIEK:W"] = SDLK_w + 5000;
  sdlStrToSdlCode["GIEK:X"] = SDLK_x + 5000;
  sdlStrToSdlCode["GIEK:Y"] = SDLK_y + 5000;
  sdlStrToSdlCode["GIEK:Z"] = SDLK_z + 5000;

  sdlStrToSdlCode["GIEK:PAGEDOWN"] = SDLK_PAGEDOWN;
  sdlStrToSdlCode["GIEK:PAGEUP"] = SDLK_PAGEUP;

  sdlStrToSdlCode["GIEK:F1"] = SDLK_F1;
  sdlStrToSdlCode["GIEK:F2"] = SDLK_F2;
  sdlStrToSdlCode["GIEK:F3"] = SDLK_F3;
  sdlStrToSdlCode["GIEK:F4"] = SDLK_F4;
  sdlStrToSdlCode["GIEK:F5"] = SDLK_F5;
  sdlStrToSdlCode["GIEK:F6"] = SDLK_F6;
  sdlStrToSdlCode["GIEK:F7"] = SDLK_F7;
  sdlStrToSdlCode["GIEK:F8"] = SDLK_F8;
  sdlStrToSdlCode["GIEK:F9"] = SDLK_F9;
  sdlStrToSdlCode["GIEK:F10"] = SDLK_F10;
  sdlStrToSdlCode["GIEK:F11"] = SDLK_F11;
  sdlStrToSdlCode["GIEK:F12"] = SDLK_F12;

  sdlStrToSdlCode["GIEK:PLUS"] = SDLK_PLUS;
  sdlStrToSdlCode["GIEK:MINUS"] = SDLK_MINUS;

  sdlStrToSdlCode["GIEK:ASTERISK"] = SDLK_ASTERISK;
  sdlStrToSdlCode["GIEK:HASH"] = SDLK_HASH;

  sdlStrToSdlCode["GIEK:PERIOD"] = SDLK_PERIOD;

  sdlStrToSdlCode["GIEK:CAPSLOCK"] = SDLK_CAPSLOCK;
  sdlStrToSdlCode["GIEK:PRINTSCREEN"] = SDLK_PRINTSCREEN;
  sdlStrToSdlCode["GIEK:MENU"] = SDLK_MENU;
  sdlStrToSdlCode["GIEK:F14"] = SDLK_F14;
  sdlStrToSdlCode["GIEK:QUESTION"] = SDLK_QUESTION;

  sdlStrToSdlCode["GIEK:DOWN"] = SDLK_DOWN;
  sdlStrToSdlCode["GIEK:LEFT"] = SDLK_LEFT;
  sdlStrToSdlCode["GIEK:RIGHT"] = SDLK_RIGHT;
  sdlStrToSdlCode["GIEK:UP"] = SDLK_UP;

  sdlStrToSdlCode["GIEK:F15"] = SDLK_F15;
  sdlStrToSdlCode["GIEK:F16"] = SDLK_F16;

  sdlStrToSdlCode["GIEK:VOLUMEDOWN"] = SDLK_VOLUMEDOWN;
  sdlStrToSdlCode["GIEK:VOLUMEUP"] = SDLK_VOLUMEUP;

  sdlStrToSdlCode["GIEK:RETURN"] = SDLK_RETURN;
  sdlStrToSdlCode["GIEK:RETURN2"] = SDLK_RETURN2;

  sdlStrToSdlCode["GIEK:F17"] = SDLK_F17;
  sdlStrToSdlCode["GIEK:F18"] = SDLK_F18;
  sdlStrToSdlCode["GIEK:F19"] = SDLK_F19;
  sdlStrToSdlCode["GIEK:F20"] = SDLK_F20;

  sdlStrToSdlCode["GIEK:SPACE"] = SDLK_SPACE;
  sdlStrToSdlCode["GIEK:BACKSPACE"] = SDLK_BACKSPACE;
  sdlStrToSdlCode["GIEK:AC_BACK"] = SDLK_AC_BACK;
  sdlStrToSdlCode["GIEK:ESCAPE"] = SDLK_ESCAPE;
  sdlStrToSdlCode["GIEK:OUT"] = SDLK_OUT;

  sdlStrToSdlCode["GIEK:POWER"] = SDLK_POWER;
  sdlStrToSdlCode["GIEK:F21"] = SDLK_F21;
  sdlStrToSdlCode["GIEK:STOP"] = SDLK_STOP;
  sdlStrToSdlCode["GIEK:EJECT"] = SDLK_EJECT;
  sdlStrToSdlCode["GIEK:EXECUTE"] = SDLK_EXECUTE;
  sdlStrToSdlCode["GIEK:F22"] = SDLK_F22;
  sdlStrToSdlCode["GIEK:PAUSE"] = SDLK_PAUSE;

  sdlStrToSdlCode["GIEK:GREATER"] = SDLK_GREATER;
  sdlStrToSdlCode["GIEK:LESS"] = SDLK_LESS;

  sdlStrToSdlCode["GIEK:TAB"] = SDLK_TAB;
  sdlStrToSdlCode["GIEK:F23"] = SDLK_F23;

  // gingaToSDLCodeMap
  gingaToSDLCodeMap[CodeMap::KEY_QUIT] = SDL_QUIT;
  gingaToSDLCodeMap[CodeMap::KEY_NULL] = SDLK_UNKNOWN;
  gingaToSDLCodeMap[CodeMap::KEY_0] = SDLK_0;
  gingaToSDLCodeMap[CodeMap::KEY_1] = SDLK_1;
  gingaToSDLCodeMap[CodeMap::KEY_2] = SDLK_2;
  gingaToSDLCodeMap[CodeMap::KEY_3] = SDLK_3;
  gingaToSDLCodeMap[CodeMap::KEY_4] = SDLK_4;
  gingaToSDLCodeMap[CodeMap::KEY_5] = SDLK_5;
  gingaToSDLCodeMap[CodeMap::KEY_6] = SDLK_6;
  gingaToSDLCodeMap[CodeMap::KEY_7] = SDLK_7;
  gingaToSDLCodeMap[CodeMap::KEY_8] = SDLK_8;
  gingaToSDLCodeMap[CodeMap::KEY_9] = SDLK_9;

  gingaToSDLCodeMap[CodeMap::KEY_SMALL_A] = SDLK_a;
  gingaToSDLCodeMap[CodeMap::KEY_SMALL_B] = SDLK_b;
  gingaToSDLCodeMap[CodeMap::KEY_SMALL_C] = SDLK_c;
  gingaToSDLCodeMap[CodeMap::KEY_SMALL_D] = SDLK_d;
  gingaToSDLCodeMap[CodeMap::KEY_SMALL_E] = SDLK_e;
  gingaToSDLCodeMap[CodeMap::KEY_SMALL_F] = SDLK_f;
  gingaToSDLCodeMap[CodeMap::KEY_SMALL_G] = SDLK_g;
  gingaToSDLCodeMap[CodeMap::KEY_SMALL_H] = SDLK_h;
  gingaToSDLCodeMap[CodeMap::KEY_SMALL_I] = SDLK_i;
  gingaToSDLCodeMap[CodeMap::KEY_SMALL_J] = SDLK_j;
  gingaToSDLCodeMap[CodeMap::KEY_SMALL_K] = SDLK_k;
  gingaToSDLCodeMap[CodeMap::KEY_SMALL_L] = SDLK_l;
  gingaToSDLCodeMap[CodeMap::KEY_SMALL_M] = SDLK_m;
  gingaToSDLCodeMap[CodeMap::KEY_SMALL_N] = SDLK_n;
  gingaToSDLCodeMap[CodeMap::KEY_SMALL_O] = SDLK_o;
  gingaToSDLCodeMap[CodeMap::KEY_SMALL_P] = SDLK_p;
  gingaToSDLCodeMap[CodeMap::KEY_SMALL_Q] = SDLK_q;
  gingaToSDLCodeMap[CodeMap::KEY_SMALL_R] = SDLK_r;
  gingaToSDLCodeMap[CodeMap::KEY_SMALL_S] = SDLK_s;
  gingaToSDLCodeMap[CodeMap::KEY_SMALL_T] = SDLK_t;
  gingaToSDLCodeMap[CodeMap::KEY_SMALL_U] = SDLK_u;
  gingaToSDLCodeMap[CodeMap::KEY_SMALL_V] = SDLK_v;
  gingaToSDLCodeMap[CodeMap::KEY_SMALL_W] = SDLK_w;
  gingaToSDLCodeMap[CodeMap::KEY_SMALL_X] = SDLK_x;
  gingaToSDLCodeMap[CodeMap::KEY_SMALL_Y] = SDLK_y;
  gingaToSDLCodeMap[CodeMap::KEY_SMALL_Z] = SDLK_z;

  gingaToSDLCodeMap[CodeMap::KEY_CAPITAL_A] = SDLK_a + 5000;
  gingaToSDLCodeMap[CodeMap::KEY_CAPITAL_B] = SDLK_b + 5000;
  gingaToSDLCodeMap[CodeMap::KEY_CAPITAL_C] = SDLK_c + 5000;
  gingaToSDLCodeMap[CodeMap::KEY_CAPITAL_D] = SDLK_d + 5000;
  gingaToSDLCodeMap[CodeMap::KEY_CAPITAL_E] = SDLK_e + 5000;
  gingaToSDLCodeMap[CodeMap::KEY_CAPITAL_F] = SDLK_f + 5000;
  gingaToSDLCodeMap[CodeMap::KEY_CAPITAL_G] = SDLK_g + 5000;
  gingaToSDLCodeMap[CodeMap::KEY_CAPITAL_H] = SDLK_h + 5000;
  gingaToSDLCodeMap[CodeMap::KEY_CAPITAL_I] = SDLK_i + 5000;
  gingaToSDLCodeMap[CodeMap::KEY_CAPITAL_J] = SDLK_j + 5000;
  gingaToSDLCodeMap[CodeMap::KEY_CAPITAL_K] = SDLK_k + 5000;
  gingaToSDLCodeMap[CodeMap::KEY_CAPITAL_L] = SDLK_l + 5000;
  gingaToSDLCodeMap[CodeMap::KEY_CAPITAL_M] = SDLK_m + 5000;
  gingaToSDLCodeMap[CodeMap::KEY_CAPITAL_N] = SDLK_n + 5000;
  gingaToSDLCodeMap[CodeMap::KEY_CAPITAL_O] = SDLK_o + 5000;
  gingaToSDLCodeMap[CodeMap::KEY_CAPITAL_P] = SDLK_p + 5000;
  gingaToSDLCodeMap[CodeMap::KEY_CAPITAL_Q] = SDLK_q + 5000;
  gingaToSDLCodeMap[CodeMap::KEY_CAPITAL_R] = SDLK_r + 5000;
  gingaToSDLCodeMap[CodeMap::KEY_CAPITAL_S] = SDLK_s + 5000;
  gingaToSDLCodeMap[CodeMap::KEY_CAPITAL_T] = SDLK_t + 5000;
  gingaToSDLCodeMap[CodeMap::KEY_CAPITAL_U] = SDLK_u + 5000;
  gingaToSDLCodeMap[CodeMap::KEY_CAPITAL_V] = SDLK_v + 5000;
  gingaToSDLCodeMap[CodeMap::KEY_CAPITAL_W] = SDLK_w + 5000;
  gingaToSDLCodeMap[CodeMap::KEY_CAPITAL_X] = SDLK_x + 5000;
  gingaToSDLCodeMap[CodeMap::KEY_CAPITAL_Y] = SDLK_y + 5000;
  gingaToSDLCodeMap[CodeMap::KEY_CAPITAL_Z] = SDLK_z + 5000;

  gingaToSDLCodeMap[CodeMap::KEY_PAGE_DOWN] = SDLK_PAGEDOWN;
  gingaToSDLCodeMap[CodeMap::KEY_PAGE_UP] = SDLK_PAGEUP;

  gingaToSDLCodeMap[CodeMap::KEY_F1] = SDLK_F1;
  gingaToSDLCodeMap[CodeMap::KEY_F2] = SDLK_F2;
  gingaToSDLCodeMap[CodeMap::KEY_F3] = SDLK_F3;
  gingaToSDLCodeMap[CodeMap::KEY_F4] = SDLK_F4;
  gingaToSDLCodeMap[CodeMap::KEY_F5] = SDLK_F5;
  gingaToSDLCodeMap[CodeMap::KEY_F6] = SDLK_F6;
  gingaToSDLCodeMap[CodeMap::KEY_F7] = SDLK_F7;
  gingaToSDLCodeMap[CodeMap::KEY_F8] = SDLK_F8;
  gingaToSDLCodeMap[CodeMap::KEY_F9] = SDLK_F9;
  gingaToSDLCodeMap[CodeMap::KEY_F10] = SDLK_F10;
  gingaToSDLCodeMap[CodeMap::KEY_F11] = SDLK_F11;
  gingaToSDLCodeMap[CodeMap::KEY_F12] = SDLK_F12;

  gingaToSDLCodeMap[CodeMap::KEY_PLUS_SIGN] = SDLK_PLUS;
  gingaToSDLCodeMap[CodeMap::KEY_MINUS_SIGN] = SDLK_MINUS;

  gingaToSDLCodeMap[CodeMap::KEY_ASTERISK] = SDLK_ASTERISK;
  gingaToSDLCodeMap[CodeMap::KEY_NUMBER_SIGN] = SDLK_HASH;

  gingaToSDLCodeMap[CodeMap::KEY_PERIOD] = SDLK_PERIOD;

  gingaToSDLCodeMap[CodeMap::KEY_SUPER] = SDLK_CAPSLOCK;
  gingaToSDLCodeMap[CodeMap::KEY_PRINTSCREEN] = SDLK_PRINTSCREEN;
  gingaToSDLCodeMap[CodeMap::KEY_MENU] = SDLK_MENU;
  gingaToSDLCodeMap[CodeMap::KEY_INFO] = SDLK_F14;
  gingaToSDLCodeMap[CodeMap::KEY_EPG] = SDLK_QUESTION;

  gingaToSDLCodeMap[CodeMap::KEY_CURSOR_DOWN] = SDLK_DOWN;
  gingaToSDLCodeMap[CodeMap::KEY_CURSOR_LEFT] = SDLK_LEFT;
  gingaToSDLCodeMap[CodeMap::KEY_CURSOR_RIGHT] = SDLK_RIGHT;
  gingaToSDLCodeMap[CodeMap::KEY_CURSOR_UP] = SDLK_UP;

  gingaToSDLCodeMap[CodeMap::KEY_CHANNEL_DOWN] = SDLK_F15;
  gingaToSDLCodeMap[CodeMap::KEY_CHANNEL_UP] = SDLK_F16;

  gingaToSDLCodeMap[CodeMap::KEY_VOLUME_DOWN] = SDLK_VOLUMEDOWN;
  gingaToSDLCodeMap[CodeMap::KEY_VOLUME_UP] = SDLK_VOLUMEUP;

  gingaToSDLCodeMap[CodeMap::KEY_ENTER] = SDLK_RETURN;

  gingaToSDLCodeMap[CodeMap::KEY_RED] = SDLK_F17;
  gingaToSDLCodeMap[CodeMap::KEY_GREEN] = SDLK_F18;
  gingaToSDLCodeMap[CodeMap::KEY_YELLOW] = SDLK_F19;
  gingaToSDLCodeMap[CodeMap::KEY_BLUE] = SDLK_F20;

  gingaToSDLCodeMap[CodeMap::KEY_SPACE] = SDLK_SPACE;
  gingaToSDLCodeMap[CodeMap::KEY_BACKSPACE] = SDLK_BACKSPACE;
  gingaToSDLCodeMap[CodeMap::KEY_BACK] = SDLK_AC_BACK;
  gingaToSDLCodeMap[CodeMap::KEY_ESCAPE] = SDLK_ESCAPE;
  gingaToSDLCodeMap[CodeMap::KEY_EXIT] = SDLK_OUT;

  gingaToSDLCodeMap[CodeMap::KEY_POWER] = SDLK_POWER;
  gingaToSDLCodeMap[CodeMap::KEY_REWIND] = SDLK_F21;
  gingaToSDLCodeMap[CodeMap::KEY_STOP] = SDLK_STOP;
  gingaToSDLCodeMap[CodeMap::KEY_EJECT] = SDLK_EJECT;
  gingaToSDLCodeMap[CodeMap::KEY_PLAY] = SDLK_EXECUTE;
  gingaToSDLCodeMap[CodeMap::KEY_RECORD] = SDLK_F22;
  gingaToSDLCodeMap[CodeMap::KEY_PAUSE] = SDLK_PAUSE;

  gingaToSDLCodeMap[CodeMap::KEY_GREATER_THAN_SIGN] = SDLK_GREATER;
  gingaToSDLCodeMap[CodeMap::KEY_LESS_THAN_SIGN] = SDLK_LESS;

  gingaToSDLCodeMap[CodeMap::KEY_TAB] = SDLK_TAB;
  gingaToSDLCodeMap[CodeMap::KEY_TAP] = SDLK_F23;

  // sdlToGingaCodeMap
  map<int, int>::iterator i;
  i = gingaToSDLCodeMap.begin ();
  while (i != gingaToSDLCodeMap.end ())
    {
      sdlToGingaCodeMap[i->second] = i->first;
      ++i;
    }

  Thread::mutexUnlock (&sieMutex);
}

bool
SDLDisplay::checkEventFocus (SDLDisplay *s)
{
  bool hasFocus = false;

  if (s->uEmbedId != NULL)
    {
      /*
       * NOTE: in this case, the focus is defined by the application
       *       that is embedding Ginga, through keyHandler specs.
       */
      hasFocus = true;
    }

  return hasFocus;
}

/* output */
void
SDLDisplay::renderMapInsertWindow (SDLWindow *iWin, double z)
{
  map<int, map<double, set<SDLWindow *> *> *>::iterator i;
  map<double, set<SDLWindow *> *>::iterator j;

  map<double, set<SDLWindow *> *> *sortedMap;
  set<SDLWindow *> *windows;

  checkMutexInit ();

  Thread::mutexLock (&renMutex);
  i = renderMap.find (0);
  if (i != renderMap.end ())
    {
      sortedMap = i->second;
    }
  else
    {
      sortedMap = new map<double, set<SDLWindow *> *>;
      renderMap[0] = sortedMap;
    }

  j = sortedMap->find (z);
  if (j != sortedMap->end ())
    {
      windows = j->second;
    }
  else
    {
      windows = new set<SDLWindow *>;
      (*sortedMap)[z] = windows;
    }

  windows->insert (iWin);
  Thread::mutexUnlock (&renMutex);
}

void
SDLDisplay::renderMapRemoveWindow (SDLWindow *iWin, double z)
{
  map<int, map<double, set<SDLWindow *> *> *>::iterator i;
  map<double, set<SDLWindow *> *>::iterator j;
  set<SDLWindow *>::iterator k;

  map<double, set<SDLWindow *> *> *sortedMap;
  set<SDLWindow *> *windows;

  checkMutexInit ();

  Thread::mutexLock (&renMutex);
  i = renderMap.find (0);
  if (i != renderMap.end ())
    {
      sortedMap = i->second;
      j = sortedMap->find (z);
      if (j != sortedMap->end ())
        {
          windows = j->second;
          k = windows->find (iWin);
          if (k != windows->end ())
            {
              windows->erase (k);
            }
          if (windows->empty ())
            {
              delete windows;
              sortedMap->erase (j);
            }
        }
    }
  Thread::mutexUnlock (&renMutex);
}

bool
SDLDisplay::drawSDLWindow (SDL_Renderer *renderer,
                                SDL_Texture *srcTxtr, SDLWindow *dstWin)
{
  SDL_Rect dstRect;
  Color *bgColor;
  Uint8 rr, rg, rb, ra;
  int i, bw;
  guint8 r, g, b, a;
  int alpha = 0;

  bool drawing = false;

  DrawData *dd;
  SDL_Rect dr;
  vector<DrawData *> *drawData;
  vector<DrawData *>::iterator it;

  lockSDL ();

  if (dstWin != NULL)
    {
      /* getting renderer previous state */
      SDL_GetRenderDrawColor (renderer, &rr, &rg, &rb, &ra);

      dstRect.x = dstWin->getX ();
      dstRect.y = dstWin->getY ();
      dstRect.w = dstWin->getW ();
      dstRect.h = dstWin->getH ();

      alpha = dstWin->getTransparencyValue ();
      if (srcTxtr != NULL)
        {
          SDL_SetTextureAlphaMod (srcTxtr, (unsigned char) (255 - (unsigned char) (CLAMP (alpha, 0, 255))));
        }

      /* setting window background */
      bgColor = dstWin->getBgColor ();
      if (bgColor != NULL)
        {
          drawing = true;
          if (alpha == 0)
            {
              alpha = 255 - bgColor->getAlpha ();
            }

          r = bgColor->getR ();
          g = bgColor->getG ();
          b = bgColor->getB ();

          SDL_SetRenderDrawColor (renderer, (gint8) bgColor->getR (),
                                  (gint8) bgColor->getG (), (gint8) bgColor->getB (),
                                  (gint8)(255 - (gint8) alpha));

          if (SDL_RenderFillRect (renderer, &dstRect) < 0)
            {
              clog << "SDLDisplay::drawWindow ";
              clog << "Warning! Can't use render to fill rect ";
              clog << SDL_GetError ();
              clog << endl;
            }
        }

      /* geometric figures (lua only) */
      drawData = ((SDLWindow *)dstWin)->createDrawDataList ();
      if (drawData != NULL)
        {
          drawing = true;
          it = drawData->begin ();
          while (it != drawData->end ())
            {
              dd = (*it);
              SDL_SetRenderDrawColor (renderer, (gint8) dd->r, (gint8) dd->g, (gint8) dd->b, (gint8) dd->a);

              switch (dd->dataType)
                {
                case SDLWindow::DDT_LINE:
                  if ((dd->coord1 < dstRect.x) || (dd->coord2 < dstRect.y)
                      || (dd->coord1 > dstRect.w)
                      || (dd->coord2 > dstRect.h)
                      || (dd->coord3 > dstRect.w)
                      || (dd->coord4 > dstRect.h))
                    {
                      clog << "SDLDisplay::drawWindow Warning!";
                      clog << " Invalid line coords: " << endl;
                      clog << dd->coord1 << ", ";
                      clog << dd->coord2 << ", ";
                      clog << dd->coord3 << ", ";
                      clog << dd->coord4 << "'";
                      clog << endl;
                      clog << "Window rect coords: " << endl;
                      clog << dstRect.x << ", ";
                      clog << dstRect.y << ", ";
                      clog << dstRect.w << ", ";
                      clog << dstRect.h << "'";
                      clog << endl;
                      break;
                    }

                  if (SDL_RenderDrawLine (renderer, dd->coord1 + dstRect.x,
                                          dd->coord2 + dstRect.y,
                                          dd->coord3 + dstRect.x,
                                          dd->coord4 + dstRect.y)
                      < 0)
                    {
                      clog << "SDLDisplay::drawWindow ";
                      clog << "Warning! Can't draw line ";
                      clog << SDL_GetError ();
                      clog << endl;
                    }

                  break;

                case SDLWindow::DDT_RECT:
                  dr.x = dd->coord1 + dstRect.x;
                  dr.y = dd->coord2 + dstRect.y;
                  dr.w = dd->coord3;
                  dr.h = dd->coord4;

                  if ((dr.x > +dstRect.x + dstRect.w)
                      || (dr.y > +dstRect.y + dstRect.h)
                      || (dd->coord1 + dr.w > dstRect.w)
                      || (dd->coord2 + dr.h > dstRect.h))
                    {
                      clog << "SDLDisplay::drawWindow Warning!";
                      clog << " Invalid rect coords: " << endl;
                      clog << dr.x << ", ";
                      clog << dr.y << ", ";
                      clog << dr.w << ", ";
                      clog << dr.h << "'";
                      clog << endl;
                      clog << "Window rect coords: " << endl;
                      clog << dstRect.x << ", ";
                      clog << dstRect.y << ", ";
                      clog << dstRect.w << ", ";
                      clog << dstRect.h << "'";
                      clog << endl;
                      break;
                    }

                  if (dd->dataType == SDLWindow::DDT_RECT)
                    {
                      if (SDL_RenderDrawRect (renderer, &dr) < 0)
                        {
                          clog << "SDLDisplay::drawWindow ";
                          clog << "Warning! Can't draw rect ";
                          clog << SDL_GetError ();
                          clog << endl;
                        }
                    }
                  else
                    {
                      if (SDL_RenderFillRect (renderer, &dr) < 0)
                        {
                          clog << "SDLDisplay::drawWindow ";
                          clog << "Warning! Can't fill rect ";
                          clog << SDL_GetError ();
                          clog << endl;
                        }
                    }
                  break;

                default:
                  g_assert_not_reached ();
                }
              ++it;
            }
          delete drawData;
        }

      /* window rendering */
      if (hasTexture (srcTxtr))
        {
          /*void* pixels;
          int tpitch;
          bool locked;*/

          // trying to lock texture
          /*locked = SDL_LockTexture(
                          texture, NULL, &pixels, &tpitch) == 0;*/

          /*
           * Warning: there is no need to lock the texture
           * lock the texture can imply some delay in
           * the decoder procedure
           */

          drawing = true;
          if (SDL_RenderCopy (renderer, srcTxtr, NULL, &dstRect) < 0)
            {
              clog << "SDLDisplay::drawWindow Warning! ";
              clog << "can't perform render copy " << SDL_GetError ();
              clog << endl;
            }

          /*if (locked) {
                  SDL_UnlockTexture(texture);
          }*/
        }

      /* window border */
      dstWin->getBorder (&r, &g, &b, &a, &bw);
      if (bw != 0)
        {
          SDL_SetRenderDrawColor (renderer, (gint8) r, (gint8) g, (gint8) b, (gint8) a);

          i = 0;
          while (i != bw)
            {
              dstRect.x = dstWin->getX () - i;
              dstRect.y = dstWin->getY () - i;
              dstRect.w = dstWin->getW () + 2 * i;
              dstRect.h = dstWin->getH () + 2 * i;

              if (SDL_RenderDrawRect (renderer, &dstRect) < 0)
                {
                  clog << "SDLDisplay::drawWindow SDL error: '";
                  clog << SDL_GetError () << "'" << endl;
                }

              if (bw < 0)
                {
                  i--;
                }
              else
                {
                  i++;
                }
            }
        }

      /* setting renderer previous state */
      SDL_SetRenderDrawColor (renderer, rr, rg, rb, ra);
    }
  else
    {
      clog << "SDLDisplay::drawWindow Warning! ";
      clog << "NULL interface window";
      clog << endl;
    }

  unlockSDL ();

  return (drawing);
}

SDL_Texture *
SDLDisplay::createTextureFromSurface (SDL_Renderer *renderer,
                                           SDL_Surface *surface)
{
  SDL_Texture *texture = NULL;

  checkMutexInit ();

  lockSDL ();
  Thread::mutexLock (&surMutex);

  if (SDLDisplay::hasUnderlyingSurface (surface))
    {
      texture = SDL_CreateTextureFromSurface (renderer, surface);
      if (texture == NULL)
        {
          clog << "SDLDisplay::createTextureFromSurface Warning! ";
          clog << "Couldn't create texture: " << SDL_GetError ();
          clog << endl;
        }
      else
        {
          uTexPool.insert (texture);

          /* allowing alpha */
          SDL_SetTextureBlendMode (texture, SDL_BLENDMODE_BLEND);
        }
    }

  Thread::mutexUnlock (&surMutex);
  unlockSDL ();

  return texture;
}

SDL_Texture *
SDLDisplay::createTexture (SDL_Renderer *renderer, int w, int h)
{
  SDL_Texture *texture;

  lockSDL ();

  texture = SDL_CreateTexture (renderer, GINGA_PIXEL_FMT,
                               SDL_TEXTUREACCESS_STREAMING, w, h);

  // w > maxW || h > maxH || format is not supported
  assert (texture != NULL);

  /* allowing alpha */
  SDL_SetTextureBlendMode (texture, SDL_BLENDMODE_BLEND);

  uTexPool.insert (texture);

  unlockSDL ();

  return texture;
}

bool
SDLDisplay::hasTexture (SDL_Texture *uTex)
{
  set<SDL_Texture *>::iterator i;
  bool hasIt = false;

  checkMutexInit ();

  lockSDL ();
  i = uTexPool.find (uTex);
  if (i != uTexPool.end ())
    {
      hasIt = true;
    }
  unlockSDL ();

  return hasIt;
}

void
SDLDisplay::releaseTexture (SDL_Texture *texture)
{
  set<SDL_Texture *>::iterator i;

  checkMutexInit ();

  lockSDL ();
  i = uTexPool.find (texture);
  if (i != uTexPool.end ())
    {
      uTexPool.erase (i);
      SDL_DestroyTexture (texture);
    }
  unlockSDL ();
}

void
SDLDisplay::addUnderlyingSurface (SDL_Surface *uSur)
{
  checkMutexInit ();

  Thread::mutexLock (&surMutex);
  uSurPool.insert (uSur);
  Thread::mutexUnlock (&surMutex);
}

SDL_Surface *
SDLDisplay::createUnderlyingSurface (int width, int height)
{
  SDL_Surface *newUSur = NULL;
  Uint32 rmask, gmask, bmask, amask;
  int bpp;

  checkMutexInit ();

  lockSDL ();

  SDL_PixelFormatEnumToMasks (GINGA_PIXEL_FMT, &bpp, &rmask, &gmask, &bmask,
                              &amask);

  newUSur = SDL_CreateRGBSurface (0, width, height, bpp, rmask, gmask,
                                  bmask, amask);

  SDL_SetColorKey (newUSur, 1, *((Uint8 *)newUSur->pixels));
  unlockSDL ();

  Thread::mutexLock (&surMutex);
  if (newUSur != NULL)
    {
      uSurPool.insert (newUSur);
    }
  else
    {
      clog << "SDLDisplay::createUnderlyingSurface SDL error: '";
      clog << SDL_GetError () << "'" << endl;
    }
  Thread::mutexUnlock (&surMutex);

  return newUSur;
}

SDL_Surface *
SDLDisplay::createUnderlyingSurfaceFromTexture (SDL_Texture *texture)
{
  SDL_Surface *uSur = NULL;
  void *pixels;
  int tpitch[3];
  Uint32 rmask, gmask, bmask, amask, format;
  int textureAccess, w, h, bpp;

  lockSDL ();

  SDL_QueryTexture (texture, &format, &textureAccess, &w, &h);
  if (textureAccess & SDL_TEXTUREACCESS_STREAMING)
    {
      bool locked = true;

      // trying to lock texture
      if (SDL_LockTexture (texture, NULL, &pixels, &tpitch[0]) != 0)
        {
          locked = false;
        }

      SDL_PixelFormatEnumToMasks (GINGA_PIXEL_FMT, &bpp, &rmask, &gmask,
                                  &bmask, &amask);

      uSur = SDL_CreateRGBSurfaceFrom (pixels, w, h, bpp, tpitch[0], rmask,
                                       gmask, bmask, amask);

      if (locked)
        {
          SDL_UnlockTexture (texture);
        }
    }

  unlockSDL ();

  Thread::mutexLock (&surMutex);
  if (uSur != NULL)
    {
      uSurPool.insert (uSur);
    }
  Thread::mutexUnlock (&surMutex);

  return uSur;
}

bool
SDLDisplay::hasUnderlyingSurface (SDL_Surface *uSur)
{
  set<SDL_Surface *>::iterator i;
  bool hasIt = false;

  checkMutexInit ();

  Thread::mutexLock (&surMutex);
  i = uSurPool.find (uSur);
  if (i != uSurPool.end ())
    {
      hasIt = true;
    }
  Thread::mutexUnlock (&surMutex);

  return hasIt;
}

void
SDLDisplay::releaseUnderlyingSurface (SDL_Surface *uSur)
{
  set<SDL_Surface *>::iterator i;

  checkMutexInit ();

  lockSDL ();
  Thread::mutexLock (&surMutex);

  i = uSurPool.find (uSur);
  if (i != uSurPool.end ())
    {
      uSurPool.erase (i);

      SDL_FreeSurface (uSur);
    }

  Thread::mutexUnlock (&surMutex);
  unlockSDL ();
}

GINGA_MB_END