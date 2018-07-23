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

extern LEDHandler LED;

// EffectHandler: Keeps track of changes to the turntable's "effect dial"
class EffectHandler {
public:
	EffectHandler(DJTurntableController &dj, unsigned long t) : fx(dj), timeout(t) {}

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
	RateLimiter timeout;  // Timeout for the fx tracker to be zero'd

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
				LED.write(LOW);  // LED low = disconnected
				D_COMMS("Bad update! Must reconnect");
			}
			else {
				LED.write(HIGH);  // LED high = connected
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

extern DJTurntableController::TurntableExpansion * mainTable;
extern DJTurntableController::TurntableExpansion * altTable;

// TurntableConfig: Handles switching between "main" and "alternate" sides of the turntable
class TurntableConfig {
public:
	typedef boolean(DJTurntableController::Data::*DJFunction)(void) const;  // Wrapper for the body function pointer
	typedef boolean(DJTurntableController::Data::TurntableExpansion::*ExpansionFunction)(void) const;  // Wrapper for the expansion function pointers
	typedef DJTurntableController::TurntableConfig Config;

	TurntableConfig(DJTurntableController &obj, DJFunction baseFunc, ExpansionFunction exFunc, unsigned long t)
		: Controller(obj), ConfigInput(baseFunc), SideSelectInput(exFunc), StableTime(t), limiter(t / 2) {}

	void check() {
		if (ConfigInput == nullptr || SideSelectInput == nullptr) {
			return;  // Bad function pointers, would otherwise throw exception
		}

		boolean configPressed = (Controller.*ConfigInput)();
		if (!configPressed || Controller.getNumTurntables() < 2) {
			return;  // Config button not pressed or < 2 turntables, no reason to check anything else
		}

		// Check the held times for each control input
		unsigned long configTime = configButton.check(configPressed);
		unsigned long leftTime = leftExpansion.check((Controller.left.*SideSelectInput)());
		unsigned long rightTime = rightExpansion.check((Controller.right.*SideSelectInput)());

		// Check if inputs have been held long enough
		if (configTime >= StableTime) {  // Main / base input
			Config selection = Config::BaseOnly;

			if (leftTime >= StableTime) {  // Left turntable
				selection = Config::Left;
			}
			else if (rightTime >= StableTime) {  // Right turntable
				selection = Config::Right;
			}

			// We've made a selection! Let's save it
			if (selection != Config::BaseOnly && limiter.ready()) {
				write(selection);
			}
		}
	}

	void read() {
		EEPROM.get(EEPROM_Addr, currentConfig);
		if (!validConfig(currentConfig)) {
			write(Config::Right);  // Fix dirty memory
		}
		reload();
	}

private:
	// Load current configuration to pointers 
	void reload() {
		// Left as main, right as alternate
		if (currentConfig == Config::Left) {
			mainTable = &Controller.left;
			altTable = &Controller.right;
		}
		else if (currentConfig == Config::Right) {
			mainTable = &Controller.right;
			altTable = &Controller.left;
		}
	}

	// Save side configuration to EEPROM
	void write(Config side) {
		if (!validConfig(side)) {
			return;  // Not a left/right selection, don't write to memory
		}

		EEPROM.put(EEPROM_Addr, side);  // Save in 'permanent' memory
		currentConfig = side;  // Save in local memory
		reload();  // Rewrite current pointers with new selection

		LED.blink(5, 1000);  // Flash the LED to alert the user! 5 hz for 1 second (non-blocking)

		D_CFG("Wrote new config! Main table: ");
		D_CFGLN(side == Config::Left ? "Left" : "Right");
	}

	boolean validConfig(Config side) {
		return side == Config::Left || side == Config::Right;
	}

	// Just for fun. Works out to be 508, which is less than
	// the 512 bytes available on most smaller AVR boards
	static const uint16_t EEPROM_Addr = 'L' + 'u' + 'c' + 'i' + 'o';

	DJTurntableController & Controller;
	const DJFunction ConfigInput;
	const ExpansionFunction SideSelectInput;

	const unsigned long StableTime;  // How long inputs must be stable for
	RateLimiter limiter;  // Prevents spamming config writes

	HeldFor configButton = HeldFor(true);  // Looking for "pressed" state on all 3 buttons
	HeldFor leftExpansion = HeldFor(true);
	HeldFor rightExpansion = HeldFor(true);

	Config currentConfig = Config::Right;  // Assume right side is 'main' if none is set
};

#endif