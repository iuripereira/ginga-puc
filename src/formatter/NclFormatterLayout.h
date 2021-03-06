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

#ifndef FORMATTERLAYOUT_H_
#define FORMATTERLAYOUT_H_

#include "mb/SDLSurface.h"
using namespace ::ginga::mb;

#include "ncl/LayoutRegion.h"
using namespace ::ginga::ncl;

#include "NclCascadingDescriptor.h"
#include "NclExecutionObject.h"

#include "NclFormatterRegion.h"
#include "NclFormatterDeviceRegion.h"

GINGA_FORMATTER_BEGIN

class NclFormatterLayout
{
protected:
  set<string> typeSet;

private:
  map<string, NclFormatterLayout *> flChilds;
  LayoutRegion *deviceRegion;
  vector<string> sortedRegions;
  map<string, double> regionZIndex;
  map<string, set<NclFormatterRegion *> *> regionMap;
  map<NclFormatterRegion *, NclExecutionObject *> objectMap;
  pthread_mutex_t mutex;

public:
  NclFormatterLayout (int x, int y, int w, int h);
  virtual ~NclFormatterLayout ();
  LayoutRegion *getDeviceRegion ();
  void addChild (const string &objectId, NclFormatterLayout *child);
  NclFormatterLayout *getChild (const string &objectId);
  string getBitMapRegionId ();
  LayoutRegion *getNcmRegion (const string &regionId);

private:
  void printRegionMap ();
  void printObjectMap ();

public:
  void getSortedIds (vector<SDLWindow*> *sortedIds);

private:
  void createDeviceRegion (int x, int y, int w, int h);

public:
  NclExecutionObject *getObject (int x, int y);
  SDLWindow* prepareFormatterRegion (NclExecutionObject *object,
                                        SDLSurface* renderedSurface,
                                        string plan);

  double refreshZIndex (NclFormatterRegion *region, const string &layoutRegionId,
                        int zIndex, const string &plan,
                        SDLSurface* renderedSurface);

private:
  void sortRegion (const string &regionId, double cvtIndex,
                   const string &plan);
  double convertZIndex (int zIndex, const string &plan);

public:
  void showObject (NclExecutionObject *object);
  void hideObject (NclExecutionObject *object);

private:
  SDLWindow* addRegionOnMaps (NclExecutionObject *object,
                                 NclFormatterRegion *region,
                                 SDLSurface* renderedSurface,
                                 string layoutRegionId, int zIndex,
                                 string plan, double *cvtZIndex);

  NclExecutionObject *
  removeFormatterRegionFromMaps (const string &layoutRegionId,
                                 NclFormatterRegion *formatterRegion);

public:
  set<NclFormatterRegion *> *
  getFormatterRegionsFromNcmRegion (const string &regionId);

  void clear ();

private:
  void lock ();
  void unlock ();
};

GINGA_FORMATTER_END

#endif /*FORMATTERLAYOUT_H_*/
