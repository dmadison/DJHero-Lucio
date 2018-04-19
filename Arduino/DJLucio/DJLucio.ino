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

// In-game sensitivity of 6.5

/* To-do:
	#1: Support both turntables
	#2: Fix the effect dial so it's more reliable
	#3: Better / smoother aiming, probably with floats
	-----(Also linked to sensitivity)
*/

enum HID_Input_Type { KEYBOARD, MOUSE };

class button {
public:
	const char key;
	boolean pressed = 0;
	const HID_Input_Type lib;

	button(char k, HID_Input_Type t = KEYBOARD) : key(k), lib(t) {}

	void press(boolean state) {
		if (state == pressed) {
			return; // Nothing to see here, folks
		}

		switch (lib) {
		case KEYBOARD:
			state ? Keyboard.press(key) : Keyboard.release(key);
			break;
		case MOUSE:
			state ? Mouse.press(key) : Mouse.release(key);
			break;
		}

		pressed = state;
	}
};

const long UpdateRate = 4;  // Update frequency in ms
long lastUpdate = 0;

DJTurntableController dj;

button fire(MOUSE_LEFT, MOUSE);
button boop(MOUSE_RIGHT, MOUSE);

button ultimate('q');
button amp('e');
button crossfade(KEY_LEFT_SHIFT);

button emotes('c');

button moveForward('w');
button moveLeft('a');
button moveBack('s');
button moveRight('d');
button jump(' ');

void setup() {
	//Serial.begin(115200);
	//while (!Serial);  // Wait for connection

	pinMode(9, INPUT_PULLUP);

	if (digitalRead(9) == LOW) {
		for (;;);  // Safety loop!
	}

	Wire.begin();
	startMultiplexer();

	while (!dj.connect()) {
		Serial.println("Couldn't connect to controller!");
		delay(500);
	}
}

void loop() {
	long t = millis();
	if (t - lastUpdate >= UpdateRate) {
		lastUpdate = t;
		if (dj.update()) {
			djController();
		}
		else {
			dj.reconnect();
			delay(250);
		}
	}
}

void djController() {
	// Aiming
	if (dj.buttonMinus()) {  // Vertical selector
		aiming(0, dj.turntable());
	}
	else {
		aiming(dj.turntable(), 0);
	}

	// Movement
	joyWASD(dj.joyX(), dj.joyY());
	jump.press(dj.buttonRed());

	// Weapons
	fire.press(dj.buttonGreen());
	boop.press(dj.buttonBlue());

	// Abilities
	ultimate.press(dj.buttonEuphoria());
	amp.press(effectChange());
	crossfade.press(dj.crossfadeSlider() > 1);

	// Fun stuff!
	emotes.press(dj.buttonPlus());
}

void aiming(int8_t xIn, int8_t yIn) {
	const int8_t HorizontalSens = 5;
	const int8_t VerticalSens = 2;
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
	button * movementKeys[4] = { &moveRight, &moveLeft, &moveForward, &moveBack }; // X+, X-, Y+, Y-
	const uint8_t joyDeadzone = 7; // +/-, centered at 32

	uint8_t joyXY[2] = { x, y };

	for (int i = 0; i < 2; i++) {
		movementKeys[i * 2]->press(joyXY[i] >= 32 + joyDeadzone);
		movementKeys[i * 2 + 1]->press(joyXY[i] <= 32 - joyDeadzone);
	}
}

boolean effectChange() {
	static long lastTrigger = 0;

	const uint8_t triggerLevel = 17;
	static boolean effectTriggered = false;

	uint8_t effectLevel = dj.effectDial();  // 0 - 31

	if (effectLevel >= triggerLevel && effectTriggered == false) {
		effectTriggered = true;
		if (millis() - lastTrigger >= 1000) {
			return true;
		}
	}
	else if (effectLevel < triggerLevel) {
		effectTriggered = false;
	}

	return false;
}

void startMultiplexer() {
	Wire.beginTransmission(0x70);
	Wire.write(1);
	Wire.endTransmission();
}
