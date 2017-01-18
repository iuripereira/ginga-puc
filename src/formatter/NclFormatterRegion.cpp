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

#include "config.h"
#include "util/Color.h"
using namespace ::ginga::util;

#include "NclCascadingDescriptor.h"
using namespace ::ginga::formatter;

#include "NclFormatterRegion.h"
#include "NclFormatterLayout.h"

#include "mb/LocalScreenManager.h"
#include "mb/ScreenManagerFactory.h"
using namespace ::ginga::mb;

GINGA_FORMATTER_BEGIN

static LocalScreenManager *dm = ScreenManagerFactory::getInstance ();
NclFormatterRegion::NclFormatterRegion (string objectId, void *descriptor,
                                  void *layoutManager)
{

  this->layoutManager = layoutManager;
  this->objectId = objectId;
  this->descriptor = descriptor;

  initializeNCMRegion ();

  this->outputDisplay = 0;
  this->renderedSurface = 0;
  this->imVisible = false;
  this->externHandler = false;
  this->focusState = NclFormatterRegion::UNSELECTED;
  this->focusBorderColor = NULL;
  this->focusBorderWidth = 0;
  this->focusComponentSrc = "";
  this->selBorderColor = NULL;
  this->selBorderWidth = 0;
  this->selComponentSrc = "";
  this->chromaKey = NULL;
  this->bgColor = NULL;
  this->transitionIn = "";
  this->transitionOut = "";
  this->abortTransitionIn = false;
  this->abortTransitionOut = false;
  this->focusIndex = "";
  this->moveUp = "";
  this->moveDown = "";
  this->moveLeft = "";
  this->moveRight = "";
  this->plan = "";
  this->zIndex = -1;

  Thread::mutexInit (&mutex);
  Thread::mutexInit (&mutexT);
  Thread::mutexInit (&mutexFI);

  // TODO: look for descriptor parameters overriding region attributes
  string value;
  value = ((NclCascadingDescriptor *)descriptor)
              ->getParameterValue ("transparency");

  this->setTransparency (value);

  value = ((NclCascadingDescriptor *)descriptor)
              ->getParameterValue ("background");

  if (::ginga::util::trim (value) != "")
    {
      if (value.find (",") == std::string::npos)
        {
          this->setBackgroundColor (value);
        }
      else
        {
          Color *bg = NULL;
          vector<string> *params = NULL;

          params = split (trim (value), ",");
          if (params->size () == 3)
            {
              bg = new Color (::ginga::util::stof ((*params)[0]),
                              ::ginga::util::stof ((*params)[1]),
                              ::ginga::util::stof ((*params)[2]));

              setBackgroundColor (bg);
            }
          else if (params->size () == 4)
            {
              bg = new Color (::ginga::util::stof ((*params)[0]),
                              ::ginga::util::stof ((*params)[1]),
                              ::ginga::util::stof ((*params)[2]),
                              ::ginga::util::stof ((*params)[3]));

              setBackgroundColor (bg);
            }
          delete params;
        }
    }

  value = ((NclCascadingDescriptor *)descriptor)->getParameterValue ("fit");

  this->setFit (value);

  value = ((NclCascadingDescriptor *)descriptor)->getParameterValue ("scroll");

  this->setScroll (value);

  value = ((NclCascadingDescriptor *)descriptor)
              ->getParameterValue ("chromakey");

  this->setChromaKey (value);

  value = ((NclCascadingDescriptor *)descriptor)
              ->getParameterValue ("rgbChromakey");

  if (value == "")
    {
      value = ((NclCascadingDescriptor *)descriptor)
                  ->getParameterValue ("x-rgbChromakey");
    }

  this->setRgbChromaKey (value);

  // TODO: methods setTransIn and setTransOut
  value = ((NclCascadingDescriptor *)descriptor)
              ->getParameterValue ("transitionIn");

  if (value != "")
    {
      transitionIn = value;
    }

  value = ((NclCascadingDescriptor *)descriptor)
              ->getParameterValue ("transitionOut");

  if (value != "")
    {
      transitionOut = value;
    }

  // TODO: fit and scroll
}

NclFormatterRegion::~NclFormatterRegion ()
{
  lock ();
  lockFocusInfo ();
  layoutManager = NULL;
  descriptor = NULL;

  originalRegion = NULL;
  if (ncmRegion != NULL)
    {
      // delete ncmRegion;
      ncmRegion = NULL;
    }

  disposeOutputDisplay ();

  if (focusBorderColor != NULL)
    {
      delete focusBorderColor;
      focusBorderColor = NULL;
    }

  if (selBorderColor != NULL)
    {
      delete selBorderColor;
      selBorderColor = NULL;
    }

  if (bgColor != NULL)
    {
      delete bgColor;
      bgColor = NULL;
    }

  if (chromaKey != NULL)
    {
      delete chromaKey;
      chromaKey = NULL;
    }

  unlock ();
  Thread::mutexDestroy (&mutex);
  lockTransition ();
  unlockTransition ();
  Thread::mutexDestroy (&mutexT);
  unlockFocusInfo ();
  Thread::mutexDestroy (&mutexFI);
}

void
NclFormatterRegion::initializeNCMRegion ()
{
  int left, top, width, height;

  originalRegion = NULL;

  if (descriptor != NULL)
    {
      originalRegion = ((NclCascadingDescriptor *)descriptor)->getRegion ();
    }

  if (originalRegion != NULL)
    {
      ncmRegion = originalRegion->cloneRegion ();

      /*if (originalRegion->isLeftPercentual() ||
                      originalRegion->isTopPercentual() ||
                      originalRegion->isWidthPercentual() ||
                      originalRegion->isHeightPercentual()) {

              left   = originalRegion->getAbsoluteLeft();
              top    = originalRegion->getAbsoluteTop();
              width  = originalRegion->getWidthInPixels();
              height = originalRegion->getHeightInPixels();

              originalRegion->resetLeft();
              originalRegion->resetTop();
              originalRegion->resetWidth();
              originalRegion->resetHeight();

              originalRegion->setLeft(left, false);
              originalRegion->setTop(top, false);
              originalRegion->setWidth(width, false);
              originalRegion->setHeight(height, false);
      }*/
    }
  else
    {
      ncmRegion = NULL;
    }
}

void
NclFormatterRegion::setRenderedSurface (GingaSurfaceID iSur)
{
  this->renderedSurface = iSur;
}

void
NclFormatterRegion::setZIndex (int zIndex)
{
  string layoutId;
  float cvtZIndex;

  this->zIndex = zIndex;

  if (ncmRegion != NULL)
    {
      ncmRegion->setZIndex (zIndex);
    }

  if (originalRegion != NULL && layoutManager != NULL)
    {
      layoutId = originalRegion->getId ();

      cvtZIndex = ((NclFormatterLayout *)layoutManager)
                      ->refreshZIndex (this, layoutId, zIndex, plan,
                                       renderedSurface);

      if (outputDisplay != 0)
        {
          clog << "NclFormatterRegion::setZIndex(" << layoutId << "): ";
          clog << "original zIndex = '" << zIndex << "'";
          clog << "converted zIndex = '" << cvtZIndex << "'";
          clog << endl;
          dm->setWindowZ (
              ((NclFormatterLayout *)layoutManager)->getScreenID (),
              outputDisplay, cvtZIndex);
        }

      toFront ();
    }
}

int
NclFormatterRegion::getZIndex ()
{
  return zIndex;
}

void
NclFormatterRegion::setPlan (string plan)
{
  if (this->plan != plan)
    {
      this->plan = plan;

      if (zIndex < 0)
        {
          zIndex = ncmRegion->getZIndexValue ();
        }

      setZIndex (this->zIndex);
    }
}

string
NclFormatterRegion::getPlan ()
{
  return plan;
}

void
NclFormatterRegion::setFocusIndex (string focusIndex)
{
  this->focusIndex = focusIndex;
}

string
NclFormatterRegion::getFocusIndex ()
{
  if (focusIndex != "")
    {
      return focusIndex;
    }
  else if (descriptor != NULL)
    {
      return ((NclCascadingDescriptor *)descriptor)->getFocusIndex ();
    }

  return "";
}

void
NclFormatterRegion::setMoveUp (string moveUp)
{
  this->moveUp = moveUp;
}

string
NclFormatterRegion::getMoveUp ()
{
  if (moveUp != "")
    {
      return moveUp;
    }
  else if (descriptor != NULL)
    {
      return ((NclCascadingDescriptor *)descriptor)->getMoveUp ();
    }

  return "";
}

void
NclFormatterRegion::setMoveDown (string moveDown)
{
  this->moveDown = moveDown;
}

string
NclFormatterRegion::getMoveDown ()
{
  if (moveDown != "")
    {
      return moveDown;
    }
  else if (descriptor != NULL)
    {
      return ((NclCascadingDescriptor *)descriptor)->getMoveDown ();
    }

  return "";
}

void
NclFormatterRegion::setMoveLeft (string moveLeft)
{
  this->moveLeft = moveLeft;
}

string
NclFormatterRegion::getMoveLeft ()
{
  if (moveLeft != "")
    {
      return moveLeft;
    }
  else if (descriptor != NULL)
    {
      return ((NclCascadingDescriptor *)descriptor)->getMoveLeft ();
    }

  return "";
}

void
NclFormatterRegion::setMoveRight (string moveRight)
{
  this->moveRight = moveRight;
}

string
NclFormatterRegion::getMoveRight ()
{
  if (moveRight != "")
    {
      return moveRight;
    }
  else if (descriptor != NULL)
    {
      return ((NclCascadingDescriptor *)descriptor)->getMoveRight ();
    }

  return "";
}

void
NclFormatterRegion::setFocusBorderColor (Color *focusBorderColor)
{
  lockFocusInfo ();
  if (this->focusBorderColor == focusBorderColor)
    {
      unlockFocusInfo ();
      return;
    }

  if (this->focusBorderColor != NULL)
    {
      delete this->focusBorderColor;
      this->focusBorderColor = NULL;
    }

  if (focusBorderColor != NULL)
    {
      this->focusBorderColor
          = new Color (focusBorderColor->getR (), focusBorderColor->getG (),
                       focusBorderColor->getB ());
    }
  unlockFocusInfo ();
}

Color *
NclFormatterRegion::getFocusBorderColor ()
{
  Color *bColor = NULL;

  lockFocusInfo ();
  if (focusBorderColor != NULL)
    {
      bColor = focusBorderColor;
    }
  else if (descriptor != NULL)
    {
      bColor = ((NclCascadingDescriptor *)descriptor)->getFocusBorderColor ();
    }
  unlockFocusInfo ();

  return bColor;
}

void
NclFormatterRegion::setFocusBorderWidth (int focusBorderWidth)
{
  this->focusBorderWidth = focusBorderWidth;
}

int
NclFormatterRegion::getFocusBorderWidth ()
{
  if (focusBorderWidth != 0)
    {
      return focusBorderWidth;
    }
  else if (descriptor != NULL)
    {
      return ((NclCascadingDescriptor *)descriptor)->getFocusBorderWidth ();
    }

  return 0;
}

void
NclFormatterRegion::setFocusComponentSrc (string focusComponentSrc)
{
  this->focusComponentSrc = focusComponentSrc;
}

string
NclFormatterRegion::getFocusComponentSrc ()
{
  if (focusComponentSrc != "")
    {
      return focusComponentSrc;
    }
  else if (descriptor != NULL)
    {
      return ((NclCascadingDescriptor *)descriptor)->getFocusSrc ();
    }

  return "";
}

void
NclFormatterRegion::setSelBorderColor (Color *selBorderColor)
{
  lockFocusInfo ();
  if (this->selBorderColor == selBorderColor)
    {
      unlockFocusInfo ();
      return;
    }

  if (this->selBorderColor != NULL)
    {
      delete this->selBorderColor;
      this->selBorderColor = NULL;
    }

  if (selBorderColor != NULL)
    {
      this->selBorderColor
          = new Color (selBorderColor->getR (), selBorderColor->getG (),
                       selBorderColor->getB ());
    }

  unlockFocusInfo ();
}

Color *
NclFormatterRegion::getSelBorderColor ()
{
  Color *sColor = NULL;

  lockFocusInfo ();
  if (selBorderColor != NULL)
    {
      sColor = selBorderColor;
    }
  else if (descriptor != NULL)
    {
      sColor = ((NclCascadingDescriptor *)descriptor)->getSelBorderColor ();
    }

  unlockFocusInfo ();

  return sColor;
}

void
NclFormatterRegion::setSelBorderWidth (int selBorderWidth)
{
  this->selBorderWidth = selBorderWidth;
}

int
NclFormatterRegion::getSelBorderWidth ()
{
  if (selBorderWidth != 0)
    {
      return selBorderWidth;
    }
  else if (descriptor != NULL)
    {
      return ((NclCascadingDescriptor *)descriptor)->getSelBorderWidth ();
    }

  return 0;
}

void
NclFormatterRegion::setSelComponentSrc (string selComponentSrc)
{
  this->selComponentSrc = selComponentSrc;
}

string
NclFormatterRegion::getSelComponentSrc ()
{
  if (selComponentSrc != "")
    {
      return selComponentSrc;
    }
  else if (descriptor != NULL)
    {
      return ((NclCascadingDescriptor *)descriptor)->getSelectionSrc ();
    }

  return "";
}

void
NclFormatterRegion::setFocusInfo (Color *focusBorderColor,
                               int focusBorderWidth,
                               string focusComponentSrc,
                               Color *selBorderColor, int selBorderWidth,
                               string selComponentSrc)
{

  setFocusBorderColor (focusBorderColor);
  setFocusBorderWidth (focusBorderWidth);
  setFocusComponentSrc (focusComponentSrc);
  setSelBorderColor (selBorderColor);
  setSelBorderWidth (selBorderWidth);
  setSelComponentSrc (selComponentSrc);
}

void *
NclFormatterRegion::getLayoutManager ()
{
  return layoutManager;
}

GingaWindowID
NclFormatterRegion::getOutputId ()
{
  //		GingaWindowID outputId = NULL;
  //		lock();
  //		if (outputDisplay != NULL) {
  //			outputId = outputDisplay->getId();
  //		}
  //		unlock();

  return outputDisplay;
}

void
NclFormatterRegion::meetComponent (int width, int height, int prefWidth,
                                int prefHeight, GingaSurfaceID component)
{

  int finalH, finalW;

  if (prefWidth == 0 || prefHeight == 0)
    {
      return;
    }

  finalH = (prefHeight * width) / prefWidth;
  if (finalH <= height)
    {
      finalW = width;
    }
  else
    {
      finalH = height;
      finalW = (prefWidth * height) / prefHeight;
    }
  // component->setSize(finalW, finalH);
}

void
NclFormatterRegion::sliceComponent (int width, int height, int prefWidth,
                                 int prefHeight, GingaSurfaceID component)
{

  int finalH, finalW;

  if (prefWidth == 0 || prefHeight == 0)
    {
      return;
    }

  finalH = (prefHeight * width) / prefWidth;
  if (finalH > height)
    {
      finalW = width;
    }
  else
    {
      finalH = height;
      finalW = (prefWidth * height) / prefHeight;
    }

  // component->setSize(finalW, finalH);
}

void
NclFormatterRegion::updateCurrentComponentSize ()
{
  // int prefWidth, prefHeight, width, height;

  // sizeRegion();

  switch (fit)
    {
    case Descriptor::FIT_HIDDEN:
      /*currentComponent.setSize(
              (int)currentComponent.getPreferredSize().getWidth(),
              (int)currentComponent.getPreferredSize().getHeight());*/
      break;

    case Descriptor::FIT_MEET:
      /*				prefWidth = (int)currentComponent.
                                                  getPreferredSize().getWidth();

                                      prefHeight = (int)currentComponent.
                                                  getPreferredSize().getHeight();

                                      width = outputDisplay.getWidth();
                                      height = outputDisplay.getHeight();
                                      meetComponent(
                                                  width,
                                                  height,
                                                  prefWidth,
                                                  prefHeight,
                                                  currentComponent);*/

      break;

    case Descriptor::FIT_MEETBEST:
      /*prefWidth = (int)currentComponent.
                  getPreferredSize().getWidth();

      prefHeight = (int)currentComponent.
                  getPreferredSize().getHeight();

      width = outputDisplay.getWidth();
      height = outputDisplay.getHeight();

      // the scale factor must not overtake 100% (2 times)
      if ((2 * prefWidth) >= width &&	(2 * prefHeight) >= height) {
              meetComponent(
                          width,
                          height,
                          prefWidth,
                          prefHeight,
                          currentComponent);

      }*/

      break;

    case Descriptor::FIT_SLICE:
      /*prefWidth = (int)currentComponent->
                  getPreferredSize().getWidth();

      prefHeight = (int)currentComponent->
                  getPreferredSize().getHeight();

      width = outputDisplay.getWidth();
      height = outputDisplay.getHeight();
      sliceComponent(
                  width,
                  height,
                  prefWidth,
                  prefHeight,
                  currentComponent);*/

      break;

    case Descriptor::FIT_FILL:
    default:
      /*currentComponent->setSize(
                      outputDisplay.getWidth(),
                      outputDisplay.getHeight());*/
      break;
    }
}

void
NclFormatterRegion::updateRegionBounds ()
{
  sizeRegion ();
  if (focusState == NclFormatterRegion::UNSELECTED)
    {
      unselect ();
    }
  else
    { // is focused (at least)
      // if is focused and selected
      if (focusState == NclFormatterRegion::SELECTED)
        {
          setSelection (true);
        }
      else if (focusState == NclFormatterRegion::FOCUSED)
        {
          setFocus (true);
        }
    }
}

void
NclFormatterRegion::sizeRegion ()
{
  int left = 0;
  int top = 0;
  int width = 0;
  int height = 0;

  if (ncmRegion != NULL)
    {
      left = ncmRegion->getLeftInPixels ();
      top = ncmRegion->getTopInPixels ();
      width = ncmRegion->getWidthInPixels ();
      height = ncmRegion->getHeightInPixels ();
    }

  /*clog << "NclFormatterRegion::sizeRegion windowAdd = '" << outputDisplay;
  clog << "' x = '" << left;
  clog << "' y = '" << top;
  clog << "' w = '" << width;
  clog << "' h = '" << height << "'" << endl;*/

  if (left < 0)
    left = 0;

  if (top < 0)
    top = 0;

  if (width <= 0)
    width = 1;

  if (height <= 0)
    height = 1;

  lock ();
  if (outputDisplay != 0)
    {
      dm->setWindowBounds (
          ((NclFormatterLayout *)layoutManager)->getScreenID (), outputDisplay,
          left, top, width, height);
    }
  unlock ();
}

bool
NclFormatterRegion::intersects (int x, int y)
{
  if (ncmRegion != NULL)
    {
      return ncmRegion->intersects (x, y);
    }

  return false;
}

LayoutRegion *
NclFormatterRegion::getLayoutRegion ()
{
  return ncmRegion;
}

LayoutRegion *
NclFormatterRegion::getOriginalRegion ()
{
  return originalRegion;
}

GingaWindowID
NclFormatterRegion::prepareOutputDisplay (GingaSurfaceID renderedSurface,
                                       float cvtIndex)
{

  GingaWindowID windowId = 0;

  // clog << "NclFormatterRegion::prepareOutputDisplay" << endl;
  lock ();
  GingaScreenID screenId
      = ((NclFormatterLayout *)layoutManager)->getScreenID ();

  if (outputDisplay == 0)
    {
      string title;
      int left = 0;
      int top = 0;
      int width = 0;
      int height = 0;

      if (ncmRegion == NULL)
        {
          title = objectId;
        }
      else
        {
          if (ncmRegion->getTitle () == "")
            {
              title = objectId;
            }
          else
            {
              title = ncmRegion->getTitle ();
            }

          left = ncmRegion->getAbsoluteLeft ();
          top = ncmRegion->getAbsoluteTop ();
          width = ncmRegion->getWidthInPixels ();
          height = ncmRegion->getHeightInPixels ();
        }

      if (left < 0)
        left = 0;

      if (top < 0)
        top = 0;

      if (width <= 0)
        width = 1;

      if (height <= 0)
        height = 1;

      this->renderedSurface = renderedSurface;

      if (renderedSurface != 0
          && dm->hasSurfaceExternalHandler (renderedSurface))
        {

          externHandler = true;
          outputDisplay = dm->getSurfaceParentWindow (renderedSurface);
          ;
        }

      if (!externHandler)
        {
          outputDisplay = dm->createWindow (screenId, left, top, width,
                                            height, cvtIndex);
        }

      clog << "NclFormatterRegion::prepareOutputDisplay '" << outputDisplay;
      clog << "' created with ";
      clog << "left   = '" << left << "' ";
      clog << "top    = '" << top << "' ";
      clog << "width  = '" << width << "' ";
      clog << "height = '" << height << "' ";
      clog << endl;

      lockFocusInfo ();
      if (bgColor != NULL)
        {
          clog << "NclFormatterRegion::prepareOutputDisplay bg color ";

          clog << "r = '" << bgColor->getR () << "', ";
          clog << "g = '" << bgColor->getG () << "' and ";
          clog << "b = '" << bgColor->getB () << "' ";
          clog << endl;

          dm->setWindowBgColor (screenId, outputDisplay, bgColor->getR (),
                                bgColor->getG (), bgColor->getB (),
                                bgColor->getAlpha ());
        }
      unlockFocusInfo ();

      if (!externHandler)
        {
          dm->setWindowCurrentTransparency (screenId, outputDisplay,
                                            (int)(transparency * 255));
        }

      int caps = dm->getWindowCap (screenId, outputDisplay, "ALPHACHANNEL");
      if (!externHandler && renderedSurface != 0
          && (caps & dm->getSurfaceCaps (renderedSurface)))
        {

          dm->addWindowCaps (screenId, outputDisplay, caps);
        }

      if (chromaKey != NULL)
        {
          caps = dm->getWindowCap (screenId, outputDisplay, "NOSTRUCTURE");

          dm->setWindowCaps (screenId, outputDisplay, caps);
          dm->drawWindow (screenId, outputDisplay);

          dm->setWindowColorKey (screenId, outputDisplay,
                                 chromaKey->getR (), chromaKey->getG (),
                                 chromaKey->getB ());
        }
      else if (!externHandler)
        {
          dm->drawWindow (screenId, outputDisplay);
        }

      if (scroll != Descriptor::SCROLL_NONE)
        {
          // int vertPolicy, horzPolicy;
          switch (scroll)
            {
            case Descriptor::SCROLL_HORIZONTAL:
              /*vertPolicy = JScrollPane.
                          VERTICAL_SCROLLBAR_NEVER;

              horzPolicy = JScrollPane.
                          HORIZONTAL_SCROLLBAR_ALWAYS;*/
              break;

            case Descriptor::SCROLL_VERTICAL:
              /*vertPolicy = JScrollPane.
                          VERTICAL_SCROLLBAR_ALWAYS;

              horzPolicy = JScrollPane.
                          HORIZONTAL_SCROLLBAR_NEVER;*/

              break;

            case Descriptor::SCROLL_BOTH:
              /*vertPolicy = JScrollPane.
                          VERTICAL_SCROLLBAR_ALWAYS;

              horzPolicy = JScrollPane.
                          HORIZONTAL_SCROLLBAR_ALWAYS;*/

              break;

            case Descriptor::SCROLL_AUTOMATIC:
              /*vertPolicy = JScrollPane.
                          VERTICAL_SCROLLBAR_AS_NEEDED;

              horzPolicy = JScrollPane.
                          HORIZONTAL_SCROLLBAR_AS_NEEDED;*/
              break;

            default:
              /*vertPolicy = JScrollPane.
                          VERTICAL_SCROLLBAR_NEVER;

              horzPolicy = JScrollPane.
                          HORIZONTAL_SCROLLBAR_NEVER;*/
              break;
            }
        }
    }
  else
    {
      clog << "NclFormatterRegion::prepareOutputDisplay Warning!";
      clog << "window != NULL" << endl;
    }

  if (renderedSurface != 0 && !externHandler)
    {
      if (dm->setSurfaceParentWindow (screenId, renderedSurface,
                                      outputDisplay))
        {
          dm->renderWindowFrom (screenId, outputDisplay, renderedSurface);
        }
    }

  unlock ();

  return outputDisplay;
}

void
NclFormatterRegion::showContent ()
{
  string value;
  NclCascadingDescriptor *desc;
  int transitionType;
  unsigned int i;
  vector<Transition *> *transitions;
  Transition *transition;
  TransInfo *t;
  pthread_t threadId_;

  lockTransition ();
  desc = ((NclCascadingDescriptor *)descriptor);
  value = desc->getParameterValue ("visible");
  abortTransitionIn = false;
  abortTransitionOut = true;
  if (value != "false")
    {
      imVisible = true;
      transitions = desc->getInputTransitions ();

      if (!transitions->empty ())
        {
          for (i = 0; i < transitions->size (); i++)
            {
              transition = (*transitions)[i];
              transitionType = transition->getType ();
              if (transitionType == Transition::TYPE_FADE)
                {
                  toFront ();

                  // fade(transition, true);
                  t = new TransInfo;
                  t->fr = this;
                  t->t = transition;

                  // show with fade transition type
                  pthread_create (&threadId_, 0, NclFormatterRegion::fadeT,
                                  (void *)t);

                  pthread_detach (threadId_);
                  unlockTransition ();
                  return;
                }
              else if (transitionType == Transition::TYPE_BARWIPE)
                {
                  toFront ();

                  // barWipe(transition, true);
                  t = new TransInfo;
                  t->fr = this;
                  t->t = transition;

                  // show with barwipe transition type
                  pthread_create (&threadId_, 0, NclFormatterRegion::barWipeT,
                                  (void *)t);

                  pthread_detach (threadId_);
                  unlockTransition ();
                  return;
                }
            }
        }

      toFront ();
      unlockTransition ();
      setRegionVisibility (true);
      /*clog << "NclFormatterRegion::showContent '" << desc->getId();
      clog << "'" << endl;*/
    }
  else
    {
      unlockTransition ();
    }
}

void
NclFormatterRegion::hideContent ()
{
  setRegionVisibility (false);
  abortTransitionIn = true;
  abortTransitionOut = true;

  lock ();
  disposeOutputDisplay ();
  unlock ();
}

void
NclFormatterRegion::performOutTrans ()
{
  vector<Transition *> *transitions;
  unsigned int i;
  Transition *transition;
  int transitionType;
  bool currentVisibility;

  abortTransitionIn = true;
  abortTransitionOut = false;
  focusState = NclFormatterRegion::UNSELECTED;

  lockTransition ();

  currentVisibility = imVisible;
  if (currentVisibility)
    {
      transitions
          = ((NclCascadingDescriptor *)descriptor)->getOutputTransitions ();

      if (!transitions->empty ())
        {
          for (i = 0; i < transitions->size (); i++)
            {
              transition = (*transitions)[i];
              transitionType = transition->getType ();
              if (transitionType == Transition::TYPE_FADE)
                {
                  // hide with fade transition type
                  fade (transition, false);
                  unlockTransition ();
                  return;
                }
              else if (transitionType == Transition::TYPE_BARWIPE)
                {
                  // hide with bar wipe transition type
                  barWipe (transition, false);
                  unlockTransition ();
                  return;
                }
            }
        }
    }

  unlockTransition ();
}

double
NclFormatterRegion::getOutTransDur ()
{
  vector<Transition *> *transitions;
  unsigned int i;
  Transition *transition;
  int transitionType;

  lockTransition ();

  transitions
      = ((NclCascadingDescriptor *)descriptor)->getOutputTransitions ();

  if (!transitions->empty ())
    {
      for (i = 0; i < transitions->size (); i++)
        {
          transition = (*transitions)[i];
          transitionType = transition->getType ();

          switch (transitionType)
            {
            case Transition::TYPE_FADE:
              unlockTransition ();
              return transition->getDur ();

            case Transition::TYPE_BARWIPE:
              unlockTransition ();
              return transition->getDur ();
            }
        }
    }

  unlockTransition ();
  return 0.0;
}

void
NclFormatterRegion::setRegionVisibility (bool visible)
{
  lock ();
  if (externHandler)
    {
      unlock ();
      return;
    }

  GingaScreenID screenId
      = ((NclFormatterLayout *)layoutManager)->getScreenID ();

  if (outputDisplay != 0)
    {
      if (!visible)
        {
          clog << "NclFormatterRegion::setRegionVisibility (" << this;
          clog << ") object '" << objectId << "' display '";
          clog << outputDisplay;
          clog << "' HIDE" << endl;

          dm->hideWindow (screenId, outputDisplay);
        }
      else
        {
          clog << "NclFormatterRegion::setRegionVisibility (" << this;
          clog << ") object '" << objectId << "' display '";
          clog << outputDisplay;
          clog << "' SHOW" << endl;

          dm->showWindow (screenId, outputDisplay);
        }
    }
  imVisible = visible;
  unlock ();
}

void
NclFormatterRegion::disposeOutputDisplay ()
{
  if (outputDisplay != 0)
    {
      GingaScreenID screenId
          = ((NclFormatterLayout *)layoutManager)->getScreenID ();
      if (!externHandler)
        {
          dm->disposeWindow (screenId, outputDisplay);
        }
      outputDisplay = 0;
    }

  // rendered surface is deleted by player
  renderedSurface = 0;
}

void
NclFormatterRegion::toFront ()
{
  lock ();
  GingaScreenID screenId
      = ((NclFormatterLayout *)layoutManager)->getScreenID ();
  if (outputDisplay != 0 && !externHandler)
    {
      dm->raiseWindowToTop (screenId, outputDisplay);
      unlock ();
      if (ncmRegion != NULL)
        {
          bringChildrenToFront (ncmRegion);
        }
      bringSiblingToFront (this);
    }
  else
    {
      unlock ();
    }
}

void
NclFormatterRegion::bringChildrenToFront (LayoutRegion *parentRegion)
{
  vector<LayoutRegion *> *regions = NULL;
  vector<LayoutRegion *>::iterator i;
  set<NclFormatterRegion *> *formRegions = NULL;
  set<NclFormatterRegion *>::iterator j;
  LayoutRegion *layoutRegion;
  NclFormatterRegion *region;

  if (parentRegion != NULL)
    {
      regions = parentRegion->getRegionsSortedByZIndex ();
    }

  // clog << endl << endl << endl;
  // clog << "DEBUG REGIONS SORTED BY ZINDEX parentID: ";
  // clog << parentRegion->getId() << endl;
  // i = regions->begin();
  // while (i != regions->end()) {
  //	clog << "region: " << (*i)->getId();
  //	clog << "zindex: " << (*i)->getZIndex();
  //	++i;
  //}
  // clog << endl << endl << endl;

  if (regions != NULL)
    {
      i = regions->begin ();
      while (i != regions->end ())
        {
          layoutRegion = *i;

          if (layoutRegion != NULL)
            {
              bringChildrenToFront (layoutRegion);
              formRegions = ((NclFormatterLayout *)layoutManager)
                                ->getFormatterRegionsFromNcmRegion (
                                    layoutRegion->getId ());

              if (formRegions != NULL)
                {
                  j = formRegions->begin ();
                  while (j != formRegions->end ())
                    {
                      region = *j;
                      if (region != NULL)
                        {
                          region->toFront ();
                        }
                      else
                        {
                          clog << "NclFormatterRegion::";
                          clog << "bringChildrenToFront";
                          clog << " Warning! region == NULL";
                        }

                      ++j;
                    }

                  delete formRegions;
                  formRegions = NULL;
                }
            }
          ++i;
        }

      delete regions;
    }
}

void
NclFormatterRegion::traverseFormatterRegions (LayoutRegion *region,
                                           LayoutRegion *baseRegion)
{

  LayoutRegion *auxRegion;
  set<NclFormatterRegion *> *formRegions;
  NclFormatterRegion *formRegion;
  set<NclFormatterRegion *>::iterator it;

  formRegions = ((NclFormatterLayout *)layoutManager)
                    ->getFormatterRegionsFromNcmRegion (region->getId ());

  if (formRegions != NULL)
    {
      it = formRegions->begin ();
      while (it != formRegions->end ())
        {
          formRegion = *it;
          if (formRegion != NULL)
            {
              auxRegion = formRegion->getLayoutRegion ();
              if (ncmRegion != NULL)
                {
                  if (ncmRegion->intersects (auxRegion)
                      && ncmRegion != auxRegion)
                    {

                      formRegion->toFront ();
                    }
                }
            }
          else
            {
              clog << "NclFormatterRegion::traverseFormatterRegion";
              clog << " Warning! formRegion == NULL" << endl;
            }

          // clog << "NclFormatterRegion::traverseFormatterRegion toFront = ";
          // clog << "'" << formRegion->getLayoutRegion()->getId();
          // clog << "'" << endl;

          ++it;
        }
      delete formRegions;
      formRegions = NULL;
    }
  else
    {
      bringHideWindowToFront (baseRegion, region);
    }
}

void
NclFormatterRegion::bringHideWindowToFront (LayoutRegion *baseRegion,
                                         LayoutRegion *hideRegion)
{

  vector<LayoutRegion *> *regions;
  LayoutRegion *region;
  vector<LayoutRegion *>::iterator it;

  if (ncmRegion->intersects (hideRegion) && ncmRegion != hideRegion)
    {
      regions = hideRegion->getRegions ();
      if (regions != NULL)
        {
          for (it = regions->begin (); it != regions->end (); ++it)
            {
              region = *it;
              traverseFormatterRegions (region, baseRegion);
            }
        }
      delete regions;
      regions = NULL;
    }
}

void
NclFormatterRegion::bringSiblingToFront (NclFormatterRegion *region)
{
  LayoutRegion *layoutRegion, *parentRegion, *baseRegion, *siblingRegion;
  vector<LayoutRegion *> *regions;
  vector<LayoutRegion *>::iterator it;

  layoutRegion = region->getOriginalRegion ();
  if (layoutRegion == NULL)
    {
      return;
    }
  parentRegion = layoutRegion->getParent ();
  baseRegion = layoutRegion;

  while (parentRegion != NULL)
    {
      regions = parentRegion->getRegionsOverRegion (baseRegion);
      for (it = regions->begin (); it != regions->end (); ++it)
        {
          siblingRegion = *it;
          traverseFormatterRegions (siblingRegion, layoutRegion);
        }
      baseRegion = parentRegion;
      delete regions;
      regions = NULL;
      parentRegion = parentRegion->getParent ();
    }
}

void
NclFormatterRegion::setGhostRegion (bool ghost)
{
  lock ();
  if (outputDisplay != 0 && !externHandler)
    {
      GingaScreenID screenId
          = ((NclFormatterLayout *)layoutManager)->getScreenID ();
      dm->setGhostWindow (screenId, outputDisplay, ghost);
    }
  unlock ();
}

bool
NclFormatterRegion::isVisible ()
{
  return imVisible;
}

short
NclFormatterRegion::getFocusState ()
{
  return focusState;
}

bool
NclFormatterRegion::setSelection (bool selOn)
{
  GingaScreenID screenId;
  GingaSurfaceID selSurface;

  screenId = ((NclFormatterLayout *)layoutManager)->getScreenID ();
  if (selOn && focusState == NclFormatterRegion::SELECTED)
    {
      return false;
    }

  if (selOn)
    {
      focusState = NclFormatterRegion::SELECTED;
      if (selComponentSrc != "")
        {
          lock ();

          selSurface = NclFocusSourceManager::getFocusSourceComponent (
              screenId, selComponentSrc);

          if (selSurface != 0)
            {
              if (outputDisplay != 0 && !externHandler)
                {
                  dm->renderWindowFrom (screenId, outputDisplay,
                                        selSurface);
                }

              dm->deleteSurface (selSurface);
            }
          unlock ();
        }

      lock ();
      if (outputDisplay != 0 && !externHandler)
        {
          lockFocusInfo ();
          if (selComponentSrc == "")
            {
              dm->validateWindow (screenId, outputDisplay);
            }

          if (selBorderColor != NULL)
            {
              dm->setWindowBorder (
                  screenId, outputDisplay, selBorderColor->getR (),
                  selBorderColor->getG (), selBorderColor->getB (),
                  selBorderColor->getAlpha (), selBorderWidth);
            }

          unlockFocusInfo ();
        }
      unlock ();
    }
  else
    {
      unselect ();
    }

  return selOn;
}

void
NclFormatterRegion::setFocus (bool focusOn)
{
  GingaScreenID screenId;
  GingaSurfaceID focusSurface;

  if (focusOn)
    {
      focusState = NclFormatterRegion::FOCUSED;
      screenId = ((NclFormatterLayout *)layoutManager)->getScreenID ();

      /*if (outputDisplay != NULL) {
              outputDisplay->clear();
      }*/

      /*			if (borderWidth > 0) {
                                      outputDisplay->setBounds(
                                                      outputDisplay->getX()
         -
         borderWidth,
                                                      outputDisplay->getY()
         -
         borderWidth,
                                                      outputDisplay->getW()
         +
         (2 * borderWidth),
                                                      outputDisplay->getH()
         +
         (2 * borderWidth));

                                      currentComponent.setLocation(
                                              currentComponent.getX() +
         borderWidth,
                                              currentComponent.getY() +
         borderWidth);

                              } else if (borderWidth < 0) {
                                      currentComponent.setSize(
                                              currentComponent.getWidth() +
         (2
         * borderWidth),
                                      currentComponent.getHeight() + (2*
         borderWidth));

                                      currentComponent.setLocation(
                                              currentComponent.getX() -
         borderWidth,
                                              currentComponent.getY() -
         borderWidth);
                              }*/

      if (focusComponentSrc != "")
        {
          lock ();

          focusSurface = NclFocusSourceManager::getFocusSourceComponent (
              screenId, focusComponentSrc);

          if (focusSurface != 0)
            {
              if (outputDisplay != 0 && !externHandler)
                {
                  dm->renderWindowFrom (screenId, outputDisplay,
                                        focusSurface);
                }
              dm->deleteSurface (focusSurface);
            }
          unlock ();
        }

      lock ();
      if (outputDisplay != 0 && !externHandler)
        {
          lockFocusInfo ();
          if (focusComponentSrc == "")
            {
              dm->validateWindow (screenId, outputDisplay);
            }

          if (focusBorderColor != NULL)
            {
              dm->setWindowBorder (
                  screenId, outputDisplay, focusBorderColor->getR (),
                  focusBorderColor->getG (), focusBorderColor->getB (),
                  focusBorderColor->getAlpha (), focusBorderWidth);
            }

          unlockFocusInfo ();
        }
      unlock ();
    }
  else
    {
      unselect ();
    }
}

void
NclFormatterRegion::unselect ()
{
  focusState = NclFormatterRegion::UNSELECTED;

  lock ();
  if (outputDisplay != 0 && !externHandler)
    {
      GingaScreenID screenId
          = ((NclFormatterLayout *)layoutManager)->getScreenID ();

      dm->setWindowBorder (screenId, outputDisplay, -1, -1, -1, -1, 0);
      if (renderedSurface != 0)
        {
          dm->setSurfaceParentWindow (screenId, renderedSurface,
                                      outputDisplay);
          dm->renderWindowFrom (screenId, outputDisplay, renderedSurface);
        }
      dm->validateWindow (screenId, outputDisplay);
    }
  unlock ();

  /*if (outputDisplay != NULL) {
          outputDisplay->clear();
  }*/

  /*		clog << "NclFormatterRegion::unselect(" << this << ")" << endl;
                  lock();
                  if (outputDisplay == NULL) {
                          unlock();
                          return;
                  }

                  if (bgColor != NULL) {
                          outputDisplay->setBgColor(
                                      bgColor->getR(),
                                      bgColor->getG(),
                                      bgColor->getB());

                  } else {
                          outputDisplay->setBgColor(0, 0, 0);
                  }
  */
  /*currentComponent.setLocation(
                          currentComponent.getX() - borderWidth,
                          currentComponent.getY() - borderWidth);
  }
  else if (borderWidth < 0){
          currentComponent.setLocation(
                          currentComponent.getX() + borderWidth,
                          currentComponent.getY() + borderWidth);

          currentComponent.setSize(
                          currentComponent.getWidth() - (2 * borderWidth),
                  currentComponent.getHeight() - (2* borderWidth));
  }*/

  /*
                  outputDisplay->showContentSurface();
                  outputDisplay->validate();
                  unlock();
  */
}

Color *
NclFormatterRegion::getBackgroundColor ()
{
  return bgColor;
}

void
NclFormatterRegion::barWipe (Transition *transition, bool isShowEffect)
{
  int i, factor = 1, x, y, width, height;
  double time, initTime;
  int transitionSubType, transparencyValue, initValue, endValue;
  double transitionDur, startProgress, endProgress;
  short transitionDir;

  lock ();
  if (outputDisplay == 0 || externHandler)
    {
      clog << "NclFormatterRegion::barWipe(" << this << ")";
      clog << "Warning! return cause ";
      clog << "abortIn = '" << abortTransitionIn << "' and ";
      clog << "abortOut = '" << abortTransitionOut << "' and ";
      clog << "display = '" << outputDisplay << "' and ";
      clog << "isShow = '" << isShowEffect << "'" << endl;
      unlock ();
      return;
    }

  GingaScreenID screenId
      = ((NclFormatterLayout *)layoutManager)->getScreenID ();
  x = dm->getWindowX (screenId, outputDisplay);
  y = dm->getWindowY (screenId, outputDisplay);
  width = dm->getWindowW (screenId, outputDisplay);
  height = dm->getWindowH (screenId, outputDisplay);
  unlock ();

  transitionDur = transition->getDur ();
  transitionSubType = transition->getSubtype ();
  transitionDir = transition->getDirection ();
  startProgress = transition->getStartProgress ();
  endProgress = transition->getEndProgress ();

  transparencyValue
      = dm->getWindowTransparencyValue (screenId, outputDisplay);

  // outputDisplay->setStretch(false);
  initTime = getCurrentTimeMillis ();

  // clog << transition->getStartProgress() << endl << endl;

  if (transitionSubType == Transition::SUBTYPE_BARWIPE_LEFTTORIGHT)
    {
      if (isShowEffect)
        {
          lock ();
          if (outputDisplay != 0)
            {
              dm->setWindowCurrentTransparency (screenId, outputDisplay,
                                                transparencyValue);
              dm->resizeWindow (screenId, outputDisplay, 1, height);
            }
          unlock ();

          initValue = width * startProgress;
          i = initValue + 1;
          endValue = width * endProgress;
        }
      else
        {
          initValue = width * endProgress;
          endValue = width * startProgress;
          i = initValue - 1;
        }

      while (true)
        {
          time = getCurrentTimeMillis ();
          if (time >= initTime + transitionDur)
            {
              break;
            }

          lock ();
          if (outputDisplay != 0)
            {
              if (transitionDir == Transition::DIRECTION_REVERSE)
                {
                  /*clog << "outDisplay =" << outputDisplay << " x=" << x;
                  clog << " w=" << width << " y=" << y << " h=" << height;
                  clog << " i=" << i << endl; */

                  if (x >= 0 && x + (width - i) >= 0 && i > 0 && height > 0)
                    {
                      dm->setWindowBounds (screenId, outputDisplay,
                                           x + (width - i), y, i, height);
                    }
                }
              else if (i > 0 && height > 0)
                {
                  dm->resizeWindow (screenId, outputDisplay, i, height);
                }
            }
          else
            {
              unlock ();
              return;
            }

          unlock ();

          i = getNextStepValue (initValue, endValue, factor, time, initTime,
                                transitionDur, 0);

          lock ();
          if (outputDisplay != 0)
            {
              dm->validateWindow (screenId, outputDisplay);
            }
          unlock ();

          if ((abortTransitionIn && isShowEffect)
              || (abortTransitionOut && !isShowEffect))
            {

              lock ();
              if (outputDisplay != 0)
                {
                  if (x < 0 || y < 0 || width <= 0 || height <= 0)
                    {
                      clog << "NclFormatterRegion::barWipe Warning! ";
                      clog << "invalid dimensions: ";
                      clog << "x = '" << x << "' ";
                      clog << "y = '" << y << "' ";
                      clog << "w = '" << width << "' ";
                      clog << "h = '" << height << "' ";
                      clog << endl;
                    }
                  else
                    {
                      dm->setWindowBounds (screenId, outputDisplay, x, y,
                                           width, height);
                    }

                  unlock ();
                  setRegionVisibility (isShowEffect);
                  // outputDisplay->setStretch(true);
                  if (!isShowEffect)
                    {
                      lock ();
                      disposeOutputDisplay ();
                      unlock ();
                    }
                }
              else
                {
                  unlock ();
                }
              return;
            }
        }
    }
  else if (transitionSubType == Transition::SUBTYPE_BARWIPE_TOPTOBOTTOM)
    {

      if (isShowEffect)
        {
          lock ();
          if (outputDisplay != 0)
            {
              dm->setWindowCurrentTransparency (screenId, outputDisplay,
                                                transparencyValue);
              if (width > 0)
                {
                  dm->resizeWindow (screenId, outputDisplay, width, 1);
                }
            }
          unlock ();
          initValue = height * startProgress;
          i = initValue + 1;
          endValue = height * endProgress;
        }
      else
        {
          initValue = height * endProgress;
          endValue = height * startProgress;
          i = initValue - 1;
        }

      /*clog << "i = " << i << " startProgress = ";
      clog << startProgress << " endProgress = " << endProgress << endl;
      clog << " initValue = " << initValue << " endValue = " << endValue;
      clog << endl;*/

      while (true)
        {
          time = getCurrentTimeMillis ();
          if (time >= initTime + transitionDur)
            {
              break;
            }

          lock ();
          if (outputDisplay != 0)
            {
              if (transitionDir == Transition::DIRECTION_REVERSE)
                {
                  if (x >= 0 && y + (height - i) >= 0 && width > 0 && i > 0)
                    {
                      dm->setWindowBounds (screenId, outputDisplay, x,
                                           y + (height - i), width, i);
                    }
                }
              else if (width > 0 && i > 0)
                {
                  dm->resizeWindow (screenId, outputDisplay, width, i);
                }
            }
          else
            {
              unlock ();
              return;
            }
          unlock ();

          i = getNextStepValue (initValue, endValue, factor, time, initTime,
                                transitionDur, 0);

          lock ();
          if (outputDisplay != 0)
            {
              dm->validateWindow (screenId, outputDisplay);
            }
          unlock ();

          if ((abortTransitionIn && isShowEffect)
              || (abortTransitionOut && !isShowEffect))
            {

              lock ();
              if (outputDisplay != 0)
                {
                  if (x < 0 || y < 0 || width <= 0 || height <= 0)
                    {
                      clog << "NclFormatterRegion::barWipe Warning! ";
                      clog << "invalid dimensions: ";
                      clog << "x = '" << x << "' ";
                      clog << "y = '" << y << "' ";
                      clog << "w = '" << width << "' ";
                      clog << "h = '" << height << "' ";
                      clog << endl;
                    }
                  else
                    {
                      dm->setWindowBounds (screenId, outputDisplay, x, y,
                                           width, height);
                    }

                  // outputDisplay->setStretch(true);
                  if (!isShowEffect)
                    {
                      disposeOutputDisplay ();
                    }
                }
              unlock ();
              setRegionVisibility (isShowEffect);
              return;
            }
        }
    }

  lock ();
  if (!isShowEffect)
    {
      disposeOutputDisplay ();
    }
  else
    {
      if (outputDisplay != 0)
        {
          // outputDisplay->setStretch(true);
          dm->setWindowBounds (screenId, outputDisplay, x, y, width,
                               height);
          dm->validateWindow (screenId, outputDisplay);
        }
    }
  unlock ();
}

void *
NclFormatterRegion::barWipeT (void *ptr)
{
  TransInfo *trans;
  Transition *t;
  NclFormatterRegion *fr;

  trans = (TransInfo *)ptr;
  fr = trans->fr;
  t = trans->t;

  delete trans;

  fr->barWipe (t, true);
  return NULL;
}

void
NclFormatterRegion::fade (Transition *transition, bool isShowEffect)
{
  int i, factor = 1;
  double time, initTime, initValue, endValue, startProgress, endProgress;
  int opacityValue;
  double transitionDur;
  GingaScreenID screenId
      = ((NclFormatterLayout *)layoutManager)->getScreenID ();

  lock ();
  if (outputDisplay == 0 || externHandler)
    {
      clog << "NclFormatterRegion::fade(" << this << ")";
      clog << "Warning! return cause ";
      clog << "abortIn = '" << abortTransitionIn << "' and ";
      clog << "abortOut = '" << abortTransitionOut << "' and ";
      clog << "display = '" << outputDisplay << "' and ";
      clog << "isShow = '" << isShowEffect << "'" << endl;
      unlock ();
      return;
    }

  opacityValue
      = (255 - dm->getWindowTransparencyValue (screenId, outputDisplay));
  unlock ();

  transitionDur = transition->getDur ();
  startProgress = transition->getStartProgress ();
  endProgress = transition->getEndProgress ();

  if (isShowEffect)
    {
      initValue = opacityValue * startProgress;
      endValue = opacityValue * endProgress;
      i = initValue + 1;
    }
  else
    {
      initValue = opacityValue * endProgress;
      endValue = opacityValue * startProgress;
      i = initValue + 1; // TODO: confirm if + or -
    }

  initTime = getCurrentTimeMillis ();
  while (true)
    {
      time = getCurrentTimeMillis ();
      if (time >= initTime + transitionDur)
        {
          break;
        }

      lock ();
      if (outputDisplay != 0)
        {
          dm->setWindowCurrentTransparency (screenId, outputDisplay,
                                            255 - i);
        }
      else
        {
          unlock ();
          return;
        }
      unlock ();

      i = getNextStepValue (initValue, endValue, factor, time, initTime,
                            transitionDur, 0);

      if ((abortTransitionIn && isShowEffect)
          || (abortTransitionOut && !isShowEffect))
        {

          break;
        }
    }

  if (!isShowEffect)
    {
      disposeOutputDisplay ();
    }
  else
    {
      setRegionVisibility (isShowEffect);
    }
}

void *
NclFormatterRegion::fadeT (void *ptr)
{
  TransInfo *trans;
  Transition *t;
  NclFormatterRegion *fr;

  trans = (TransInfo *)ptr;
  fr = trans->fr;
  t = trans->t;

  delete trans;

  fr->fade (t, true);
  return NULL;
}

/*void changeCurrentComponent(Component newComponent) {
        if (newComponent != null && currentComponent != null &&
                        outputDisplay != null) {
                newComponent.setBounds(currentComponent.getBounds());
                outputDisplay.remove(currentComponent);
                outputDisplay.add(newComponent);
                currentComponent = newComponent;
                outputDisplay.validate();
        }
}*/

void
NclFormatterRegion::lock ()
{
  Thread::mutexLock (&mutex);
}

void
NclFormatterRegion::unlock ()
{
  Thread::mutexUnlock (&mutex);
}

void
NclFormatterRegion::lockTransition ()
{
  Thread::mutexLock (&mutexT);
}

void
NclFormatterRegion::unlockTransition ()
{
  Thread::mutexUnlock (&mutexT);
}

void
NclFormatterRegion::lockFocusInfo ()
{
  Thread::mutexLock (&mutexFI);
}

void
NclFormatterRegion::unlockFocusInfo ()
{
  Thread::mutexUnlock (&mutexFI);
}

void
NclFormatterRegion::setTransparency (string strTrans)
{
  float trans;
  if (strTrans == "")
    {
      trans = 0.0;
    }
  else
    {
      trans = ::ginga::util::stof (strTrans);
    }

  setTransparency (trans);
}

float
NclFormatterRegion::getTransparency ()
{
  return this->transparency;
}

void
NclFormatterRegion::setTransparency (float transparency)
{
  lock ();
  if (transparency < 0.0)
    {
      this->transparency = 0.0;
    }
  else if (transparency > 1.0)
    {
      this->transparency = 1.0;
    }
  else
    {
      this->transparency = transparency;
    }

  /*clog << "NclFormatterRegion::setTransparency : calling with value ";
  clog << transparency << endl;*/

  if (outputDisplay != 0 && !externHandler)
    {
      GingaScreenID screenId
          = ((NclFormatterLayout *)layoutManager)->getScreenID ();
      dm->setWindowCurrentTransparency (screenId, outputDisplay,
                                        (int)(this->transparency * 255));
    }

  unlock ();
}

void
NclFormatterRegion::setBackgroundColor (string color)
{
  Color *bg = NULL;
  string trimColor = ::ginga::util::trim (color);

  if (trimColor != "" && trimColor != "transparent")
    {
      bg = new Color (color, (int)(transparency * 255));
    }

  setBackgroundColor (bg);
}

void
NclFormatterRegion::setBackgroundColor (Color *color)
{
  lockFocusInfo ();
  if (color != bgColor && bgColor != NULL)
    {
      delete bgColor;
      bgColor = NULL;
    }

  this->bgColor = color;
  unlockFocusInfo ();
}

void
NclFormatterRegion::setChromaKey (string value)
{
  if (value != "")
    {
      if (value == "black")
        {
          this->chromaKey = new Color (0, 0, 0, 0);
        }
      else
        {
          this->chromaKey = new Color (value);
        }
    }
}

void
NclFormatterRegion::setRgbChromaKey (string value)
{
  if (value != "")
    {
      vector<string> *params;

      params = split (value, ",");
      if (params->size () == 3)
        {
          this->chromaKey
              = new Color ((int)::ginga::util::stof ((*params)[0]),
                           (int)::ginga::util::stof ((*params)[1]),
                           (int)::ginga::util::stof ((*params)[2]));
        }
      delete params;
    }
}

void
NclFormatterRegion::setFit (string value)
{
  if (value != "")
    {
      setFit (DescriptorUtil::getFitCode (value));
    }
  else
    {
      setFit (Descriptor::FIT_FILL);
    }
}

void
NclFormatterRegion::setFit (short value)
{
  if (value < 0)
    {
      fit = Descriptor::FIT_FILL;
    }
  else
    {
      fit = value;
    }
}

void
NclFormatterRegion::setScroll (string value)
{
  if (value != "")
    {
      setScroll (DescriptorUtil::getScrollCode (value));
    }
  else
    {
      setScroll (Descriptor::SCROLL_NONE);
    }
}

void
NclFormatterRegion::setScroll (short value)
{
  if (value < 0)
    {
      scroll = Descriptor::SCROLL_NONE;
    }
  else
    {
      scroll = value;
    }
}

GINGA_FORMATTER_END