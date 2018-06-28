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

#ifdef DEBUG_COMMS
#define D_COMMS(x) DEBUG_PRINTLN(x)
#else
#define D_COMMS(x)
#endif

#ifdef DEBUG_CONTROLDETECT
#define D_CD(x)   DEBUG_PRINT(x)
#else
#define D_CD(x)
#endif

#if MAIN_TABLE==right
#define ALT_TABLE left
#elif MAIN_TABLE==left
#define ALT_TABLE right
#endif

// HID_Button: Handles HID button state to prevent input spam
class HID_Button {
public:
	HID_Button(char k) : key(k) {}

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

	const char key;
private:
	virtual void sendState(boolean state) = 0;
	boolean pressed = 0;
};

// HID_Button: Sending mouse inputs
class MouseButton : public HID_Button {
public:
	using HID_Button::HID_Button;
private:
	void sendState(boolean state) {
		state ? Mouse.press(key) : Mouse.release(key);
	}
};

// HID_Button: Sending keyboard inputs
class KeyboardButton : public HID_Button {
public:
	using HID_Button::HID_Button;
private:
	void sendState(boolean state) {
		state ? Keyboard.press(key) : Keyboard.release(key);
	}
};

// RateLimiter: Simple timekeeper that returns 'true' if X time has passed
class RateLimiter {
public:
	RateLimiter(long rate) : UpdateRate(rate) {}

	boolean ready() {
		return ready(millis());
	}

	boolean ready(long timeNow) {
		if (timeNow - lastUpdate >= UpdateRate) {
			lastUpdate = timeNow;
			return true;
		}
		return false;
	}

	void reset() {
		lastUpdate = millis();
	}

	const long UpdateRate = 0;  // Rate limit, in ms
private:
	long lastUpdate = 0;
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
	ControllerDetect(uint8_t pin, long stableWait) : Pin(pin), StableTime(stableWait) {}

	void begin() {
		pinMode(Pin, INPUT);
	}

	boolean isDetected() {
		boolean currentState = digitalRead(Pin);  // Read status of CD pin

		D_CD("CD pin is ");
		D_CD(currentState ? "HIGH " : "LOW ");

		// Controller detected! (Rising edge)
		if (currentState == HIGH) {
			if (lastPinState == LOW) {  // This is new!
				stableSince = millis();  // Reset the stable counter
			}
			else if (lastPinState == HIGH) {  // We're still connected
				D_CD("Stable for: ");
				D_CD(millis() - stableSince);
				D_CD(" / ");
				D_CD(StableTime);
				D_CD(' ');
			}
		}

		lastPinState = currentState;  // Save pin state for future reference

		// If we've connected and have been been stable for X time, return true
		return currentState && (millis() - stableSince >= StableTime);
	}

private:
	const uint8_t Pin;  // Connected pin to read from. High == connected, Low == disconnected (needs pull-down)
	unsigned long StableTime;  // Time before the connection is considered "stable", in milliseconds

	unsigned long stableSince = 0;
	boolean lastPinState = LOW;
};

// ConnectionHelper: Keeps track of the controller's 'connected' state, and auto-updating control data
class ConnectionHelper {
public:
	ConnectionHelper(ExtensionController &con, uint8_t cdPin, long pollTime, long cdWaitTime, long reconnectTime) :
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
				releaseAll();  // Something went wrong, clear current presses
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

		// If not connected, attempt reconnection at regular interval
		if (!connected && reconnectRate.ready()) {
			connected = controller.reconnect();
			D_COMMS("Attempting to reconnect");
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
