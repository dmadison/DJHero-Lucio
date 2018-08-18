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

#ifndef DJLucio_LED_h
#define DJLucio_LED_h

#include "DJLucio_Platforms.h"

// SoftwareOscillator: oscillates its state output based on the given period, using the millis() timekeeper
class SoftwareOscillator {
public:
	boolean getState() {
		if (period != 0) {  // Only oscillate if a period is set
			unsigned long timeNow = millis();

			// Toggle output at period
			if (timeNow - lastFlip >= period) {
				state = !state;
				lastFlip = timeNow;
			}
		}

		return state;
	}

	void stopOscillating() {
		setPeriod(0);
	}

	void setPeriod(unsigned long p) {
		period = p;
		reset();
	}

	void setFrequency(float h) {
		if (h == 0) { return; }  // Avoiding div/0
		period = frequencyToPeriod(h);
		reset();
	}

	unsigned long frequencyToPeriod(float h) {
		if (h == 0.0) { return 0; }  // Avoiding div/0
		return (1000.0 / h) / 2.0;  // 1000 because we're in milliseconds, /2 because the output has two phases
	}

	float periodToFrequency(unsigned long p) {
		if (p == 0) { return 0.0; }  // Avoiding div/0
		return 1000.0 / (float)(p * 2);  // 1000 because we're in milliseconds, *2 because the output has two phases
	}

private:
	void reset() {
		lastFlip = millis();  // Set the timer to ready
		state = LOW;  // Start oscillator on low
	}

	boolean state;  // State of the oscillator, high or low
	unsigned long period;  // Period of the oscillation
	unsigned long lastFlip;  // Timestamp of the last state flip
};

// LEDHandler: for dealing with user notifications on the built-in LED
class LEDHandler {
public:
	LEDHandler(uint8_t pin = LED_BUILTIN) : LEDHandler(pin, false) {}
	LEDHandler(uint8_t pin, boolean inverted) : Pin(pin), Inverted(inverted) {}

	void begin() {
		pinMode(Pin, OUTPUT);
	}

	void update() {
		if (!currentlyBlinking) {
			return;  // Nothing to do here
		}

		setLED(oscillator.getState());  // Write LED based on the blink state

		if (duration != 0 && millis() - patternStart >= duration) {
			stopBlinking();  // Blinking is done!
		}
	}

	void write(boolean out) {
		state = out;  // Save current state
		if (currentlyBlinking == true) { return; }  // LED blinking, don't write
		setLED(state);
	}

	void blink(float hertz) {
		blink(hertz, 0);  // Blink forever
	}

	void blink(float hertz, unsigned long length) {
		if (hertz == 0) {  // If frequency is 0, no blinking
			stopBlinking();
			return;
		}

		patternStart = millis();  // Record the time that blinking started
		duration = length;  // Duration to blink, in milliseconds
		oscillator.setFrequency(hertz);
		currentlyBlinking = true;
	}

	void stopBlinking() {
		currentlyBlinking = false;
		setLED(state);  // Re-write the state before blinking
	}

private:
	void setLED(boolean s) {
		digitalWrite(Pin, s ^ Inverted);  // Bool XOR with the inverted flag
	}

	const uint8_t Pin;
	const boolean Inverted = false;

	boolean state = LOW;

	boolean currentlyBlinking = false;
	SoftwareOscillator oscillator;
	unsigned long duration;
	unsigned long patternStart;
};

LEDHandler LED(LED_Pin, LED_Inverted);  // Default LED instance, using the platform definitions

#endif
