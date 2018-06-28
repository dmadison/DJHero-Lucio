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

#include <NintendoExtensionCtrl.h>
#include <Mouse.h>
#include <Keyboard.h>

// User Settings
#define MAIN_TABLE right  // Main turntable in a two-turntable setup
const int8_t HorizontalSens = 5;  // Mouse sensitivity multipler - 6 max
const int8_t VerticalSens = 2;    // Mouse sensitivity multipler - 6 max

// Tuning Options
const uint16_t UpdateRate = 4;  // Controller polling rate, in milliseconds (ms)
const uint16_t DetectTime = 1000;  // Time before a connected controller is considered stable (ms)
const uint16_t ConnectRate = 500;  // Rate to attempt reconnections, in ms
const uint8_t  EffectThreshold = 10;  // Threshold to trigger abilities from the fx dial, 10 = 1/3rd of a revolution
const uint16_t EffectsTimeout = 1200;  // Timeout for the effects tracker, in ms

// Debug Flags (uncomment to add)
// #define DEBUG                // Enable to use any prints
// #define DEBUG_RAW            // See the raw data from the turntable
// #define DEBUG_HID            // See HID inputs as they're pressed/released
// #define DEBUG_COMMS          // Follow the controller connect and update calls
// #define DEBUG_CONTROLDETECT  // Trace the controller detect pin functions

// ---------------------------------------------------------------------------

#include "DJLucio_Util.h"  // Utility classes

DJTurntableController dj;

MouseButton fire(MOUSE_LEFT);
MouseButton boop(MOUSE_RIGHT);
KeyboardButton reload('r');

KeyboardButton ultimate('q');
KeyboardButton amp('e');
KeyboardButton crossfade(KEY_LEFT_SHIFT);

KeyboardButton emotes('c');

KeyboardButton moveForward('w');
KeyboardButton moveLeft('a');
KeyboardButton moveBack('s');
KeyboardButton moveRight('d');
KeyboardButton jump(' ');

DJTurntableController::TurntableExpansion * mainTable = &dj.MAIN_TABLE;
DJTurntableController::TurntableExpansion * altTable = &dj.ALT_TABLE;

EffectHandler fx(dj);

const uint8_t DetectPin = 4;
const uint8_t SafetyPin = 9;

ConnectionHelper controller(dj, DetectPin, UpdateRate, DetectTime, ConnectRate);


void setup() {
	#ifdef DEBUG
	Serial.begin(115200);
	while (!Serial);  // Wait for connection
	#endif

	pinMode(SafetyPin, INPUT_PULLUP);

	if (digitalRead(SafetyPin) == LOW) {
		for (;;);  // Safety loop!
	}

	startMultiplexer();  // Enable the multiplexer, currently being used as a level shifter (to be removed)
	controller.begin();  // Initialize controller bus and auto-detect

	while (!controller.isReady()) {
		DEBUG_PRINTLN(F("Couldn't connect to turntable!"));
		delay(500);
	}
	DEBUG_PRINTLN(F("Connected! Starting..."));
}

void loop() {
	if (controller.isReady()) {
		djController();
	}
}

void djController() {
	// Dual turntables
	if (dj.getNumTurntables() == 2) {
		if (!dj.buttonMinus()) {  // Button to disable aiming (for position correction)
			aiming(mainTable->turntable(), altTable->turntable());
		}

		// Movement
		jump.press(mainTable->buttonRed());

		// Weapons
		fire.press(mainTable->buttonGreen() || mainTable->buttonBlue());  // Outside buttons
		boop.press(altTable->buttonGreen() || altTable->buttonRed() || altTable->buttonBlue());
	}

	// Single turntable (either side)
	else if (dj.getNumTurntables() == 1) {
		// Aiming
		if (dj.buttonMinus()) {  // Vertical selector
			aiming(0, dj.turntable());
		}
		else {
			aiming(dj.turntable(), 0);
		}

		// Movement
		jump.press(dj.buttonRed());

		// Weapons
		fire.press(dj.buttonGreen());
		boop.press(dj.buttonBlue());
	}

	// --Base Station Abilities--
	fx.update();

	// Movement
	joyWASD(dj.joyX(), dj.joyY());

	// Weapons
	reload.press(fx.changed(EffectThreshold) && fx.getTotal() < 0);

	// Abilities
	ultimate.press(dj.buttonEuphoria());
	amp.press(fx.changed(EffectThreshold) && fx.getTotal() > 0);
	crossfade.press(dj.crossfadeSlider() > 1);

	// Fun stuff!
	emotes.press(dj.buttonPlus());

	// --Cleanup--
	if (fx.changed(EffectThreshold)) {
		fx.reset();  // Already used abilities, reset to 0
	}
}

void aiming(int8_t xIn, int8_t yIn) {
	const int8_t MaxInput = 20;  // Get rid of extranous values

	static int8_t lastAim[2] = { 0, 0 };
	int8_t * aim[2] = { &xIn, &yIn };  // Store in array for iterative access

	// Iterate through X/Y
	for (int i = 0; i < 2; i++) {
		// Check if above max threshold
		if (abs(*aim[i]) >= MaxInput) {
			*aim[i] = lastAim[i];
		}

		// Set 'last' value to current
		lastAim[i] = *aim[i];
	}

	Mouse.move(xIn * HorizontalSens, yIn * VerticalSens);
}

void joyWASD(uint8_t x, uint8_t y) {
	const uint8_t JoyCenter = 32;
	const uint8_t JoyDeadzone = 6;  // +/-, centered at 32 in (0-63)

	moveLeft.press(x < JoyCenter - JoyDeadzone);
	moveRight.press(x > JoyCenter + JoyDeadzone);

	moveForward.press(y > JoyCenter + JoyDeadzone);
	moveBack.press(y < JoyCenter - JoyDeadzone);
}

void startMultiplexer() {
	Wire.begin();
	Wire.beginTransmission(0x70);
	Wire.write(1);
	Wire.endTransmission();
}
