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

#define D_COMMS(x) DEBUG_PRINTLN(x)

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

// ConnectionHelper: Keeps track of the controller's 'connected' state, and auto-updating control data
class ConnectionHelper {
public:
	ConnectionHelper(ExtensionController &con, long pollTime, long reconnectTime) :
		controller(con), pollRate(pollTime), reconnectRate(reconnectTime) {}

	// Automatically connects the controller, checks if it's ready for a new update, and 
	// returns 'true' if there is new data to process.
	boolean isReady() {
		long timeNow = millis();

		// Attempt to reconnect at a regular interval
		if (!connected && reconnectRate.ready(timeNow)) {
			connected = controller.reconnect();
			D_COMMS("Attempting to reconnect");
		}

		// If connected, update at a regular interval
		if (connected && pollRate.ready(timeNow)) {
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

	boolean isConnected() const {
		return connected;
	}

private:
	ExtensionController & controller;

	RateLimiter pollRate;
	RateLimiter reconnectRate;

	boolean connected = false;
};

#endif
