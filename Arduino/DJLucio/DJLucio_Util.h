/*
*  Project     DJ Hero - Lucio
*  @author     David Madison
*  @link       github.com/dmadison/DJHero-Lucio
*  @license    GPLv3 - Copyright (c) 2018 David Madison
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DJLucio_Util_h
#define DJLucio_Util_h

#ifdef DEBUG
#define DEBUG_PRINT(x)   do {Serial.print(x);}   while(0)
#define DEBUG_PRINTLN(x) do {Serial.println(x);} while(0)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

#ifdef DEBUG_HID
#define D_HID(x)   DEBUG_PRINT(x)
#define D_HIDLN(x) DEBUG_PRINTLN(x)
#else
#define D_HID(x)
#define D_HIDLN(x)
#endif

#ifdef DEBUG_COMMS
#define D_COMMS(x) DEBUG_PRINTLN(x)
#else
#define D_COMMS(x)
#endif

#ifdef DEBUG_CONTROLDETECT
#define D_CD(x)   DEBUG_PRINT(x)
#define D_CDLN(x) DEBUG_PRINTLN(x)
#else
#define D_CD(x)
#define D_CDLN(x)
#endif

#if MAIN_TABLE==right
#define ALT_TABLE left
#elif MAIN_TABLE==left
#define ALT_TABLE right
#endif

// HID_Button: Handles HID button state to prevent input spam
class HID_Button {
public:
	HID_Button(char k) : key(k) {
		// If linked list is empty, set both head and tail
		if (head == nullptr) {
			head = this;
			tail = this;
		}
		// If linked list is *not* empty, set the 'next' ptr of the tail
		// and update the tail
		else {
			tail->next = this;
			tail = this;
		}
	}

	~HID_Button() {
		if (this == head) {
			// Option #1: Only element in the list
			if (this == tail) {
				head = nullptr;
				tail = nullptr;  // List is now empty
			}
			// Option #2: First element in the list,
			// but not *only* element
			else {
				head = next;  // Set head to next, and we're done
			}
			return;
		}

		// Option #3: Somewhere else in the list.
		// Iterate through to find it

		HID_Button * ptr = head;

		while (ptr != nullptr) {
			if (ptr->next == this) {  // FOUND!
				ptr->next = next;  // Set the previous "next" as this entry's "next" (skip this object)
				break;  // Stop searching
			}
			ptr = ptr->next;  // Not found. Next entry...
		}

		// Option #4: Last entry in the list
		if (this == tail) {
			tail = ptr;  // Set the tail as the previous entry
		}
	}

	void press(boolean state = true) {
		if (state == pressed) {
			return; // Nothing to see here, folks
		}

		sendState(state);
		pressed = state;
	}

	void release() {
		press(false);
	}

	static void releaseAll() {  // Release all buttons, using the linked list
		HID_Button * ptr = head;

		while (ptr != nullptr) {
			ptr->release();
			ptr = ptr->next;
		}
	}

	const unsigned char key;
private:
	static HID_Button * head;
	static HID_Button * tail;

	virtual void sendState(boolean state) = 0;
	boolean pressed = 0;
	HID_Button * next = nullptr;
};

// Allocate space for the static pointers
HID_Button * HID_Button::head = nullptr;
HID_Button * HID_Button::tail = nullptr;

// HID_Button: Sending mouse inputs
class MouseButton : public HID_Button {
public:
	using HID_Button::HID_Button;
private:
	void sendState(boolean state) {
		state ? Mouse.press(key) : Mouse.release(key);

		#ifdef DEBUG_HID
		DEBUG_PRINT("Mouse ");
		switch (key) {
			case(MOUSE_LEFT):
				DEBUG_PRINT("left");
				break;
			case(MOUSE_RIGHT):
				DEBUG_PRINT("right");
				break;
			case(MOUSE_MIDDLE):
				DEBUG_PRINT("middle");
				break;
		}
		DEBUG_PRINT(' ');
		DEBUG_PRINTLN(state ? "pressed" : "released");
		#endif
	}
};

// HID_Button: Sending keyboard inputs
class KeyboardButton : public HID_Button {
public:
	using HID_Button::HID_Button;
private:
	void sendState(boolean state) {
		state ? Keyboard.press(key) : Keyboard.release(key);

		#ifdef DEBUG_HID
		DEBUG_PRINT("Keyboard ");
		switch (key) {
			case(KEY_LEFT_SHIFT):
			case(KEY_RIGHT_SHIFT):
				DEBUG_PRINT("shift");
				break;
			case(' '):
				DEBUG_PRINT("(space)");
				break;
			default:
				DEBUG_PRINT((char)key);
				break;
		}
		DEBUG_PRINT(' ');
		DEBUG_PRINTLN(state ? "pressed" : "released");
		#endif
	}
};

// RateLimiter: Simple timekeeper that returns 'true' if X time has passed
class RateLimiter {
public:
	RateLimiter(unsigned long rate) : UpdateRate(rate) {
		lastUpdate = millis() - rate;  // Guarantee 'ready' on first call 
	}

	boolean ready() {
		return ready(millis());
	}

	boolean ready(unsigned long timeNow) {
		if (timeNow - lastUpdate >= UpdateRate) {
			lastUpdate = timeNow;
			return true;
		}
		return false;
	}

	void reset() {
		lastUpdate = millis();
	}

	const unsigned long UpdateRate = 0;  // Rate limit, in ms
private:
	unsigned long lastUpdate;
};

// HeldFor: Checks how long a two-state variable has been in a given state
class HeldFor {
public:
	HeldFor(boolean goalState) : HeldFor(goalState, !goalState) {}
	HeldFor(boolean goalState, boolean setInitial)
		: MatchState(goalState), lastState(setInitial) {}

	unsigned long check(boolean state) {
		if (state == MatchState) {
			if (lastState == !MatchState) {  // This is new!
				stableSince = millis();  // Reset the stable counter
			}
			lastState = state;  // Save for future reference
			return millis() - stableSince;  // How long we've been stable
		}

		lastState = state;
		return 0;  // Nothing yet!
	}

private:
	const boolean MatchState;  // The state we're looking for
	boolean lastState;  // Last recorded state
	unsigned long stableSince;  // Timestamp for edge change
};

// EffectHandler: Keeps track of changes to the turntable's "effect dial"
class EffectHandler {
public:
	EffectHandler(DJTurntableController &dj) : fx(dj) {}

	boolean changed(uint8_t threshold) {
		return abs(total) >= threshold;
	}

	void update() {
		const uint8_t MaxChange = 5;

		int8_t fxChange = fx.getChange();  // Change since last update

		 // Check inactivity timer
		if (fxChange != 0) {
			timeout.reset();  // Keep alive
		}
		else if (timeout.ready()) {
			total = 0;
		}

		if (abs(fxChange) > MaxChange) {  // Assumed spurious
			fxChange = 0;
		}
		total += fxChange;
	}

	int16_t getTotal() {
		return total;
	}

	void reset() {
		total = 0;
	}

private:
	DJTurntableController::EffectRollover fx;
	RateLimiter timeout = RateLimiter(EffectsTimeout);  // Timeout for the fx tracker to be zero'd

	int16_t total = 0;
};


// ControllerDetect: Measures and debounces the controller's "connected" pin
class ControllerDetect {
public:
	ControllerDetect(uint8_t pin, unsigned long stableWait) : Pin(pin), StableTime(stableWait) {}

	void begin() {
		pinMode(Pin, INPUT);
	}

	boolean isDetected() {
		boolean currentState = digitalRead(Pin);  // Read status of CD pin

		D_CD("CD pin is ");
		D_CD(currentState ? "HIGH " : "LOW ");

		if (currentState == HIGH && detected == true) {
			D_CDLN("Controller connected!");
			return true;  // We're still good!
		}

		// Check how long the pin has been high. 0 if it's low.
		unsigned long currentTime = stateDuration.check(currentState);

		D_CD("Stable for: ");
		D_CD(currentTime);
		D_CD(" / ");
		D_CD(StableTime);
		D_CDLN();

		return detected = currentTime >= StableTime;  // Set flag and return to user.
	}

private:
	const uint8_t Pin;  // Connected pin to read from. High == connected, Low == disconnected (needs pull-down)
	unsigned long StableTime;  // Time before the connection is considered "stable", in milliseconds

	HeldFor stateDuration = HeldFor(HIGH, HIGH);  // Looking for a high connection, assume first read was high
	boolean detected = true;  // Assume controller is detected for first call
};

// ConnectionHelper: Keeps track of the controller's 'connected' state, and auto-updating control data
class ConnectionHelper {
public:
	ConnectionHelper(ExtensionController &con, uint8_t cdPin, unsigned long pollTime, unsigned long cdWaitTime, unsigned long reconnectTime) :
		controller(con), detect(cdPin, cdWaitTime), pollRate(pollTime), reconnectRate(reconnectTime) {}

	void begin() {
		detect.begin();  // Initialize CD pin as input
		controller.begin();  // Start I2C bus
	}

	// Automatically connects the controller, checks if it's ready for a new update, and 
	// returns 'true' if there is new data to process.
	boolean isReady() {
		if (pollRate.ready() && isConnected()) {
			connected = controller.update();  // New data
			if (!connected) {
				HID_Button::releaseAll();  // Something went wrong, clear current pressed buttons
				D_COMMS("Bad update! Must reconnect");
			}
			else {
				D_COMMS("Successul update!");
				#ifdef DEBUG_RAW
				dj.printDebug();
				#endif
			}
			return connected;
		}

		return false;
	}

	boolean isConnected() {
		// Check if the controller detect pin is inactive.
		// If so, invalidate any present connection
		if (!controllerDetected()) {
			D_COMMS("Controller not detected (check your connections)");
			return connected = false;
		}

		// If not connected, attempt connection at regular interval
		if (!connected && reconnectRate.ready()) {
			connected = controller.connect();
			D_COMMS("Connecting to controller...");
		}

		return connected;
	}

private:
	boolean controllerDetected() {
		return detect.isDetected();
	}

	ExtensionController & controller;
	ControllerDetect detect;

	RateLimiter pollRate;
	RateLimiter reconnectRate;

	boolean connected = false;
};

#endif
