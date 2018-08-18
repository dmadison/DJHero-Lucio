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

#ifndef DJLucio_Controller_h
#define DJLucio_Controller_h

#include <NintendoExtensionCtrl.h>
#include "DJLucio_Util.h"

#ifdef DEBUG_CONTROLDETECT
#define D_CD(x)   DEBUG_PRINT(x)
#define D_CDLN(x) DEBUG_PRINTLN(x)
#else
#define D_CD(x)
#define D_CDLN(x)
#endif

#ifdef DEBUG_COMMS
#define D_COMMS(x) DEBUG_PRINTLN(x)
#else
#define D_COMMS(x)
#endif

extern DJTurntableController dj;
extern LEDHandler LED;

// EffectHandler: Keeps track of changes to the turntable's "effect dial"
class EffectHandler {
public:
	EffectHandler(DJTurntableController &dj, unsigned long t) : fx(dj), timeout(t) {}

	boolean changed(uint8_t threshold) {
		return abs(total) >= threshold;
	}

	void update() {
		const uint8_t MaxChange = 5;  // Arbitrary, for spurious value check

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
	RateLimiter timeout;  // Timeout for the fx tracker to be zero'd

	int16_t total = 0;
};


// ControllerDetect: Measures and debounces the controller's "connected" pin
class ControllerDetect {
public:
	ControllerDetect(uint8_t pin, unsigned long stableWait) : Pin(pin), StableTime(stableWait) {}

	void begin() {
		pinMode(Pin, INPUT);  // Requires external pull-down!
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

// ConnectionHelper: Keeps track of the controller's 'connected' state, and auto-updates control data
class ConnectionHelper {
public:
	ConnectionHelper(ExtensionController &con, uint8_t cdPin, unsigned long pollTime, unsigned long cdWaitTime, unsigned long reconnectTime) :
		controller(con), detect(cdPin, cdWaitTime), pollRate(pollTime), reconnectRate(reconnectTime) {}

	void begin() {
		detect.begin();  // Initialize CD pin as input
		controller.begin();  // Start I2C bus
		LED.blink(LED_BlinkSpeed);  // Start the LED blinking (disconnected)
	}

	// Automatically connects the controller, checks if it's ready for a new update, and 
	// returns 'true' if there is new data to process.
	boolean isReady() {
		if (pollRate.ready() && isConnected()) {
			if (!controller.update()) {  // Fetch new data
				disconnect();
				D_COMMS("Controller update failed :(");
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
			if (connected) { disconnect(); }  // Disconnect if connected
			return false;  // No controller detected? Nothing else to do here
		}

		// Controller detect pin is high! So let's check our initialization.
		// If not connected, attempt connection at regular interval
		if (!connected && reconnectRate.ready()) {
			D_COMMS("Connecting to controller...");
			if (controller.connect()) {
				onConnect();  // Successsful connection!
			}
		}

		return connected;
	}

private:
	void onConnect() {
		LED.write(HIGH);  // LED high = connected
		LED.stopBlinking();
		connected = true;
		D_COMMS("Controller successfully connected!");	
	}

	void disconnect() {
		HID_Button::releaseAll();  // Something went wrong, clear current pressed buttons
		LED.write(LOW);  // LED low = disconnected
		LED.blink(LED_BlinkSpeed);  // ... also blinking
		connected = false;
		D_COMMS("Uh oh! Controller disconnected");
	}

	boolean controllerDetected() {
		#ifdef IGNORE_DETECT_PIN 
			return true;  // No detect pin, just assume the controller is there
		#else
			return detect.isDetected();
		#endif
	}

	static const float LED_BlinkSpeed;

	ExtensionController & controller;
	ControllerDetect detect;

	RateLimiter pollRate;
	RateLimiter reconnectRate;

	boolean connected = false;
};

const float ConnectionHelper::LED_BlinkSpeed = 0.5;  // Hertz

#endif
