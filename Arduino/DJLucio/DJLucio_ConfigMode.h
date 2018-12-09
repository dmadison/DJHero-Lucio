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

#ifndef DJLucio_ConfigMode_h
#define DJLucio_ConfigMode_h

#include <EEPROM.h>
#include "DJLucio_Util.h"

#ifdef DEBUG_CONFIG
#define D_CFG(x)   DEBUG_PRINT(x)
#define D_CFGLN(x) DEBUG_PRINTLN(x)
#else
#define D_CFG(x)
#define D_CFGLN(x)
#endif

// Table side pointers, included in the main sketch
extern DJTurntableController::TurntableExpansion * mainTable;
extern DJTurntableController::TurntableExpansion * altTable;

// TurntableConfig: Handles switching between "main" and "alternate" sides of the turntable
class TurntableConfig {
public:
	typedef boolean(DJTurntableController::*DJFunction)(void) const;  // Wrapper for the body function pointer
	typedef boolean(DJTurntableController::TurntableExpansion::*ExpansionFunction)(void) const;  // Wrapper for the expansion function pointers
	typedef DJTurntableController::TurntableConfig Config;  // Wrapper for the config side enum

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
			D_CFGLN("CFG: EEPROM is bad! Rewriting...");
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
			D_CFGLN("CFG: Set main table LEFT");
		}
		// Right as main, left as alternate
		else if (currentConfig == Config::Right) {
			mainTable = &Controller.right;
			altTable = &Controller.left;
			D_CFGLN("CFG: Set main table RIGHT");
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

		LED.blink(10, 1200);  // Flash the LED to alert the user! 10 hz for 1.2 seconds (non-blocking)

		D_CFG("CFG: Wrote new config! Main table: ");
		D_CFGLN(side == Config::Left ? "Left" : "Right");
	}

	boolean validConfig(Config side) {
		return side == Config::Left || side == Config::Right;
	}

	// Just for fun. Works out to be 540. No capital 'L' to
	// avoid writing to the 0 address on the Teensy LC (508 % 127)
	static const uint16_t EEPROM_Addr = ('l' + 'u' + 'c' + 'i' + 'o') % E2END;
	static_assert(EEPROM_Addr + 1 <= E2END, "EEPROM address larger than EEPROM space!");  // Var is two bytes

	DJTurntableController & Controller;
	const DJFunction ConfigInput;
	const ExpansionFunction SideSelectInput;

	const unsigned long StableTime;  // How long inputs must be stable for
	RateLimiter limiter;  // Prevents spamming config writes

	HeldFor configButton = HeldFor(true);  // Looking for "pressed" (true) state on all 3 buttons
	HeldFor leftExpansion = HeldFor(true);
	HeldFor rightExpansion = HeldFor(true);

	Config currentConfig = Config::Right;  // Assume right side is 'main' if none is set
};

#endif
