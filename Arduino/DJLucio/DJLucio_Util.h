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

// RateLimiter: Simple timekeeper that returns 'true' if X time has passed.
//              Uses millis() as its clock.
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

#endif
