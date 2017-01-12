/* Copyright (C) 1989-2017 PUC-Rio/Laboratorio TeleMidia

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

#include "InputManager.h"
#include "LocalScreenManager.h"
#include "ScreenManagerFactory.h"
#include "CodeMap.h"

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace mb {
	InputManager::InputManager(GingaScreenID screenId) : Thread() {
		currentXAxis          = 0;
		currentYAxis          = 0;
		maxX                  = 0;
		maxY                  = 0;
		lastEventTime         = 0;
		myScreen              = screenId;
		eventBuffer           = NULL;
		running               = false;
		notifying             = false;
		notifyingApp          = false;
		cmdListener           = NULL;
		Thread::mutexInit(&actInpMutex);
		Thread::mutexInit(&actAppMutex);
		Thread::mutexInit(&appMutex);
		Thread::mutexInit(&mlMutex);
		initializeInputIntervalTime();
	}

	InputManager::~InputManager() {
		release();
	}

	void InputManager::initializeInputIntervalTime() {
		string strVar;
		ifstream fis;

		imperativeIntervalTime  = 0;
		declarativeIntervalTime = 0;

		strVar = SystemCompat::appendGingaFilesPrefix("mb/config/input.cfg");

		fis.open(strVar.c_str(), ifstream::in);
		if (!fis.is_open()) {
			clog << "InputManager: can't open input file:";
			clog << strVar.c_str() << endl;
			return;
		}

		while (fis.good()) {
			fis >> strVar;
			if (strVar == "imperative.inputIntervalTime" && fis.good()) {
				fis >> strVar;
				if (strVar == "=" && fis.good()) {
					fis >> strVar;
					if (strVar != "") {
						imperativeIntervalTime = util::stof(strVar);
					}
				}

			} else if (strVar == "declarative.inputIntervalTime" &&
					fis.good()) {

				fis >> strVar;
				if (strVar == "=" && fis.good()) {
					fis >> strVar;
					if (strVar != "") {
						declarativeIntervalTime = util::stof(strVar);
					}
				}
			}
		}
		fis.close();

		clog << "InputManager::initializeInputIntervalTime imperative = '";
		clog << imperativeIntervalTime << "' declarative = '";
		clog << declarativeIntervalTime << "'" << endl;
	}

	void InputManager::release() {
		map<IInputEventListener*, set<int>*>::iterator i;
		bool runDone = false;
		IInputEvent* ie;
		int mbKeyCode;
		bool wasRunning = running;

		running = false;
		if (eventBuffer != NULL) {
			eventBuffer->wakeUp();

		} else {
			return;
		}

		while (!runDone) {
			SystemCompat::uSleep(10000);
			runDone = (maxX == 0 && maxY == 0);
		}

		mbKeyCode = ScreenManagerFactory::getInstance()->fromGingaToMB(
				myScreen, CodeMap::KEY_QUIT);

		ie = ScreenManagerFactory::getInstance()->createInputEvent(
				myScreen, NULL, mbKeyCode);

		if (wasRunning) {
			running = true;
			dispatchEvent(ie);
			running = false;
		}

		delete ie;

		lock();

		i = eventListeners.begin();
		while (i != eventListeners.end()) {
			if (i->second != NULL) {
				delete i->second;
			}
			++i;
		}
		eventListeners.clear();

		if (!actionsToInpListeners.empty()) {
			Thread::mutexLock(&actInpMutex);
			actionsToInpListeners.clear();
			Thread::mutexUnlock(&actInpMutex);
		}
		Thread::mutexDestroy(&actInpMutex);


		if (!applicationListeners.empty()) {
			Thread::mutexLock(&appMutex);
			applicationListeners.clear();

			if (!actionsToAppListeners.empty()) {
				Thread::mutexLock(&actAppMutex);
				actionsToAppListeners.clear();
				Thread::mutexUnlock(&actAppMutex);
			}

			Thread::mutexUnlock(&appMutex);
		}
		Thread::mutexDestroy(&actAppMutex);
		Thread::mutexDestroy(&appMutex);

		if (eventBuffer != NULL) {
			delete eventBuffer;
			eventBuffer = NULL;
		}

		if (!motionListeners.empty()) {
			Thread::mutexLock(&mlMutex);
			motionListeners.clear();
			Thread::mutexUnlock(&mlMutex);
		}
		Thread::mutexDestroy(&mlMutex);

		unlock();
	}

	void InputManager::addMotionEventListener(IMotionEventListener* listener) {
		assert(listener != NULL);

		Thread::mutexLock(&mlMutex);
		motionListeners.insert(listener);
		Thread::mutexUnlock(&mlMutex);
	}

	void InputManager::removeMotionEventListener(
			IMotionEventListener* listener) {

		set<IMotionEventListener*>::iterator i;

		assert(listener != NULL);

		Thread::mutexLock(&mlMutex);
		i = motionListeners.find(listener);
		if (i != motionListeners.end()) {
			motionListeners.erase(i);
		}
		Thread::mutexUnlock(&mlMutex);
	}

	void InputManager::notifyMotionListeners(int x, int y, int z) {

		set<IMotionEventListener*>::iterator i;

		Thread::mutexLock(&mlMutex);
		i = motionListeners.begin();
		while (i != motionListeners.end()) {
			(*i)->motionEventReceived(x, y, z);
			++i;
		}
		Thread::mutexUnlock(&mlMutex);
	}

	void InputManager::addInputEventListener(
		    IInputEventListener* listener, set<int>* evs) {

		LockedAction* action;
		set<int>* iEvs;
		map<IInputEventListener*, set<int>*>::iterator i;

		assert(listener != NULL);

		if (!running) {
			eventBuffer = getEventBuffer();
		}

		if (notifying) {
			Thread::mutexLock(&actInpMutex);
			action = new LockedAction;
			action->l      = listener;
			action->isAdd  = true;
			action->events = evs;

			actionsToInpListeners.push_back(action);
			Thread::mutexUnlock(&actInpMutex);

		} else {
			lock();
			performInputLockedActions();
			i = eventListeners.find(listener);
			if (i != eventListeners.end()) {
				iEvs = i->second;
				eventListeners.erase(i);
				if (iEvs != NULL) {
					delete iEvs;
				}
			}
			eventListeners[listener] = evs;
			unlock();
		}
	}

	void InputManager::removeInputEventListener(
		    IInputEventListener* listener) {

		LockedAction* action;
		map<IInputEventListener*, set<int>*>::iterator i;

		assert(listener != NULL);

		if (!running) {
			return;
		}

		if (notifying) {
			Thread::mutexLock(&actInpMutex);
			action = new LockedAction;
			action->l      = listener;
			action->isAdd  = false;
			action->events = NULL;

			actionsToInpListeners.push_back(action);
			Thread::mutexUnlock(&actInpMutex);

		} else {
			lock();
			if (!running) {
				unlock();
				return;
			}
			performInputLockedActions();
			i = eventListeners.find(listener);
			if (i != eventListeners.end()) {
				if (i->second != NULL) {
					delete i->second;
				}
				eventListeners.erase(i);
			}
			unlock();
		}
	}

	void InputManager::performInputLockedActions() {
		LockedAction* action;
		IInputEventListener* listener;

		set<int>* evs;
		map<IInputEventListener*, set<int>*>::iterator i;
		vector<LockedAction*>::iterator j;

		if (!running) {
			return;
		}

		Thread::mutexLock(&actInpMutex);
		j = actionsToInpListeners.begin();
		while (j != actionsToInpListeners.end()) {
			action   = *j;
			listener = action->l;
			evs      = action->events;

			i = eventListeners.find(listener);
			if (i != eventListeners.end()) {
				if (i->second != NULL) {
					delete i->second;
				}
				eventListeners.erase(i);
			}

			if (action->isAdd) {
				eventListeners[listener] = evs;
			}

			delete action;
			++j;
		}

		actionsToInpListeners.clear();
		Thread::mutexUnlock(&actInpMutex);
	}

	void InputManager::performApplicationLockedActions() {
		LockedAction* action;
		IInputEventListener* listener;

		set<IInputEventListener*>::iterator i;
		vector<LockedAction*>::iterator j;

		if (!running) {
			return;
		}

		Thread::mutexLock(&actAppMutex);
		j = actionsToAppListeners.begin();
		while (j != actionsToAppListeners.end()) {
			action = *j;
			listener = action->l;

			if (action->isAdd) {
				applicationListeners.insert(listener);

			} else {
				i = applicationListeners.find(listener);
				if (i != applicationListeners.end()) {
					applicationListeners.erase(i);
				}
			}

			delete action;
			++j;
		}

		actionsToAppListeners.clear();
		Thread::mutexUnlock(&actAppMutex);
	}

	bool InputManager::dispatchEvent(IInputEvent* inputEvent) {
		map<IInputEventListener*, set<int>*>::iterator i;

		IInputEventListener* lis;
		set<int>* evs;
		int keyCode;

		if (!running) {
			clog << "InputManger::dispatchEvent !running" << endl;
			return false;
		}

		notifying = true;
		lock();
		notifying = true;

		performInputLockedActions();

		if (eventListeners.empty() || inputEvent == NULL || !running) {
			unlock();
			notifying = false;

			clog << "InputManger::dispatchEvent no listeners to notify '";
			clog << inputEvent << "'" << endl;
			return true;
		}

		keyCode = inputEvent->getKeyCode(myScreen);

		clog << "InputManger::dispatchEvent ";
		if (keyCode == CodeMap::KEY_TAP) {
			clog << "TAP ";

		} else {
			clog << "keyCode = '" << keyCode << "' ";
		}
		clog << "on screen id = '";
		clog << myScreen << "'";
		clog << endl;

		i = eventListeners.begin();
		while (i != eventListeners.end() && running) {
			lis = i->first;
			evs = i->second;
			if (evs != NULL) {
				if (evs->find(keyCode) != evs->end()) {
					//return false means an event with changed keySymbol
					if (!lis->userEventReceived(inputEvent)) {
						unlock();
						notifying = false;
						return false;
					}

				} else if (keyCode == CodeMap::KEY_QUIT) {
					if (!lis->userEventReceived(inputEvent)) {
						unlock();
						notifying = false;
						return false;
					}
				}

			} else if (!lis->userEventReceived(inputEvent)) {
				unlock();
				notifying = false;
				return false;
			}
			++i;
		}
		unlock();
		notifying = false;
		return true;
	}

	bool InputManager::dispatchApplicationEvent(IInputEvent* inputEvent) {
		set<IInputEventListener*>::iterator i;
		vector<LockedAction*>::iterator j;

		if (!running) {
			return false;
		}

		notifyingApp = true;
		Thread::mutexLock(&appMutex);

		performApplicationLockedActions();

		if (applicationListeners.empty() || inputEvent == NULL) {
			Thread::mutexUnlock(&appMutex);
			notifyingApp = false;
			return true;
		}

		i = applicationListeners.begin();
		while (i != applicationListeners.end()) {
			if (*i != NULL) {
				if (!(*i)->userEventReceived(inputEvent)) {
					Thread::mutexUnlock(&appMutex);
					notifyingApp = false;
					return false;
				}
			}
			++i;
		}
		Thread::mutexUnlock(&appMutex);
		notifyingApp = false;
		return true;
	}

	void InputManager::addApplicationInputEventListener(
		    IInputEventListener* listener) {

		LockedAction* action;

		assert(listener != NULL);

		if (!running) {
			return;
		}

		if (notifyingApp) {
			Thread::mutexLock(&actAppMutex);
			action         = new LockedAction;
			action->l      = listener;
			action->isAdd  = true;
			action->events = NULL;

			actionsToAppListeners.push_back(action);
			Thread::mutexUnlock(&actAppMutex);

		} else {
			Thread::mutexLock(&appMutex);
			performApplicationLockedActions();
			applicationListeners.insert(listener);
			Thread::mutexUnlock(&appMutex);
		}
	}

	void InputManager::removeApplicationInputEventListener(
		    IInputEventListener* listener) {

		LockedAction* action;
		set<IInputEventListener*>::iterator i;

		assert(listener != NULL);

		if (!running) {
			return;
		}

		if (notifyingApp) {
			Thread::mutexLock(&actAppMutex);
			action         = new LockedAction;
			action->l      = listener;
			action->isAdd  = false;
			action->events = NULL;

			actionsToAppListeners.push_back(action);
			Thread::mutexUnlock(&actAppMutex);

		} else {
			Thread::mutexLock(&appMutex);
			performApplicationLockedActions();
			i = applicationListeners.find(listener);
			if (i != applicationListeners.end()) {
				applicationListeners.erase(i);
			}
			Thread::mutexUnlock(&appMutex);
		}
	}

	void InputManager::setCommandEventListener(
			ICmdEventListener* listener) {

		this->cmdListener = listener;
	}

	void InputManager::postInputEvent(IInputEvent* event) {
		if (!running) {
			delete event;
			return;
		}

		eventBuffer->postInputEvent(event);
	}

	void InputManager::postInputEvent(int keyCode) {
		IInputEvent* ie;
		int mbKeyCode;

		mbKeyCode = ScreenManagerFactory::getInstance()->fromGingaToMB(
				myScreen, keyCode);

		ie = ScreenManagerFactory::getInstance()->createInputEvent(
				myScreen, NULL, mbKeyCode);

		postInputEvent(ie);
	}

	void InputManager::postCommand(string cmd, string args) {
		if (cmdListener != NULL) {
			cmdListener->cmdEventReceived(cmd, args);
		}
	}

	void InputManager::setAxisValues(int x, int y, int z) {
		this->currentXAxis = x;
		this->currentYAxis = y;
		this->currentZAxis = z;
	}

	void InputManager::setAxisBoundaries(int x, int y, int z) {
		this->maxX = x;
		this->maxY = y;
	}

	int InputManager::getCurrentXAxisValue() {
		return currentXAxis;
	}

	int InputManager::getCurrentYAxisValue() {
		return currentYAxis;
	}

	IEventBuffer* InputManager::getEventBuffer() {
		if (eventBuffer == NULL) {
			eventBuffer = ScreenManagerFactory::getInstance()->
					createEventBuffer(myScreen);

			if (!running) {
				running = true;
				Thread::startThread();
			}
		}

		return eventBuffer;
	}

	void InputManager::run() {
		IInputEvent* inputEvent;

		int pLastCode     = -1;
		int lastCode      = -1;
		double pTimeStamp = 0;
		double timeStamp  = 0;

		int mouseX, mouseY;

		clog << "InputManager::run main loop" << endl;

		while (running && eventBuffer != NULL) {
			eventBuffer->waitEvent();
			if (!running) {
				break;
			}

			inputEvent = eventBuffer->getNextEvent();
			while (inputEvent != NULL) {
				handleInputEvent(inputEvent, pLastCode, lastCode, pTimeStamp, timeStamp, mouseX, mouseY);

				delete inputEvent;
				inputEvent = eventBuffer->getNextEvent();
			}
		}

		if (running && eventBuffer == NULL) {
			clog << "InputManager::run Warning! Can't receive events: ";
			clog << "event buffer is NULL" << endl;
		}

		currentXAxis = 0;
		currentYAxis = 0;
		currentZAxis = 0;
		maxX         = 0;
		maxY         = 0;

		clog << "InputManager::run all done" << endl;
	}

	void InputManager::handleInputEvent(
		IInputEvent *inputEvent, int& pLastCode, int& lastCode,
		double& pTimeStamp, double& timeStamp, int& mouseX, int& mouseY)
	{
		if (inputEvent->isMotionType()) {
			mouseX = currentXAxis;
			mouseY = currentYAxis;

			inputEvent->getAxisValue(
					&currentXAxis, &currentYAxis, &currentZAxis);

			if (currentXAxis == 0) {
				currentXAxis = mouseX;

			} else if (currentXAxis > maxX && maxX != 0) {
				currentXAxis = maxX;
			}

			if (currentYAxis == 0) {
				currentYAxis = mouseY;

			} else if (currentYAxis > maxY && maxY != 0) {
				currentYAxis = maxY;
			}

			if (eventBuffer != NULL) {
				inputEvent = eventBuffer->getNextEvent();
			}

			notifyMotionListeners(
					currentXAxis, currentYAxis, currentZAxis);

			return;
		}

		if (!inputEvent->isApplicationType() &&
				inputEvent->getKeyCode(myScreen) == CodeMap::KEY_NULL) {

			return;
		}

		if (inputEvent->isButtonPressType()) {
			inputEvent->setAxisValue(currentXAxis, currentYAxis, 0);

			inputEvent->getAxisValue(
					&currentXAxis,
					&currentYAxis,
					&currentZAxis);

			dispatchEvent(inputEvent);
			return;
		}

		if (inputEvent->isPressedType() && ((getCurrentTimeMillis() -
				timeStamp) >= declarativeIntervalTime)) {

			lastCode  = inputEvent->getKeyCode(myScreen);
			timeStamp = getCurrentTimeMillis();

			clog << "InputManager::run event code = '";
			clog << lastCode << "'" << endl;

			if (!dispatchEvent(inputEvent)) {
				return;
			}
		}

		if (inputEvent->isKeyType() ||
				inputEvent->isApplicationType()) {

			clog << "InputManager::run key or application" << endl;

			dispatchApplicationEvent(inputEvent);
		}
	}
}
}
}
}
}
}