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

#ifndef FORMATTERDEVICEREGION_H_
#define FORMATTERDEVICEREGION_H_

#include "ncl/Entity.h"
using namespace ::ginga::ncl;

#include "ncl/LayoutRegion.h"
using namespace ::ginga::ncl;

#include "system/Thread.h"
using namespace ::ginga::system;

GINGA_FORMATTER_BEGIN

class NclFormatterDeviceRegion : public LayoutRegion
{
private:
  string id;
  int top, left, width, height;
  vector<LayoutRegion *> *sortedRegions;
  set<LayoutRegion *> regionSet;
  pthread_mutex_t rMutex;

public:
  NclFormatterDeviceRegion (const string &id);
  virtual ~NclFormatterDeviceRegion (){};
  void addRegion (LayoutRegion *region);
  LayoutRegion *cloneRegion ();
  int compareWidthSize (const string &w);
  int compareHeightSize (const string &h);
  short getBackgroundColor ();
  double getBottom ();
  double getHeight ();
  double getLeft ();
  double getRight ();
  LayoutRegion *getRegion (const string &id);
  LayoutRegion *getRegionRecursively (const string &id);
  vector<LayoutRegion *> *getRegions ();
  string getTitle ();
  double getTop ();
  double getWidth ();
  int getZIndex ();
  int getZIndexValue ();
  bool isBottomPercentual ();
  bool isHeightPercentual ();
  bool isLeftPercentual ();
  bool isRightPercentual ();
  bool isTopPercentual ();
  bool isWidthPercentual ();
  string toString ();
  bool removeRegion (LayoutRegion *region);
  void removeRegions ();
  void setBackgroundColor (SDL_Color *newBackgroundColor);
  bool setBottom (double newBottom, bool isPercentual);
  bool setHeight (double newHeight, bool isPercentual);
  bool setLeft (double newLeft, bool isPercentual);
  bool setRight (double newRight, bool isPercentual);
  void setTitle (const string &newTitle);
  bool setTop (double newTop, bool isPercentual);
  bool setWidth (double newWidth, bool isPercentual);
  void setZIndex (int newZIndex);
  vector<LayoutRegion *> *getRegionsSortedByZIndex ();
  vector<LayoutRegion *> *getRegionsOverRegion (LayoutRegion *region);

  LayoutRegion *getParent ();
  void setParent (LayoutRegion *parent);
  int getTopInPixels ();
  int getBottomInPixels ();
  int getRightInPixels ();
  int getLeftInPixels ();
  int getHeightInPixels ();
  int getWidthInPixels ();
  bool isMovable ();
  bool isResizable ();
  bool isDecorated ();
  void setMovable (bool movable);
  void setResizable (bool resizable);
  void setDecorated (bool decorated);
  void resetTop ();
  void resetBottom ();
  void resetRight ();
  void resetLeft ();
  void resetHeight ();
  void resetWidth ();
  void resetZIndex ();
  void resetDecorated ();
  void resetMovable ();
  void resetResizable ();
  int getAbsoluteLeft ();
  int getAbsoluteTop ();

  void dispose ();
  string getId ();
  void setId (const string &id);
  Entity *getDataEntity ();
  int compareTo (void *arg0);
};

GINGA_FORMATTER_END

#endif /*FORMATTERDEVICEREGION_H_*/
