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

#ifndef DJLucio_Platforms_h
#define DJLucio_Platforms_h

#if defined(__AVR_ATmega32U4__)

// Arduino Pro Micro 5V / 3.3V
#if defined(ARDUINO_AVR_PROMICRO)
const uint8_t LED_Pin = 17;  // RX LED on the Pro Micro
const boolean LED_Inverted = true;  // Inverted on the Pro Micro (LOW is lit)

// Arduino Leonardo
#elif defined(ARDUINO_AVR_LEONARDO)
const uint8_t LED_Pin = 13;
const boolean LED_Inverted = false;

// Teensy 2.0
#elif defined(CORE_TEENSY)
const uint8_t LED_Pin = 11;
const boolean LED_Inverted = false;

#endif  // 32U4 boards

// Teensy 2.0++
#elif defined(__AVR_AT90USB1286__) && defined(CORE_TEENSY)
const uint8_t LED_Pin = 6;
const boolean LED_Inverted = false;

// Teensy 3.0/3.1-3.2/LC/3.5/3.6
#elif (defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MKL26Z64__) || \
       defined(__MK64FX512__) || defined(__MK66FX1M0__)) && defined(CORE_TEENSY)
const uint8_t LED_Pin = 13;
const boolean LED_Inverted = false;

#else
#error Unsupported board! Use a Leonardo, Pro Micro, or Teensy
#endif

#endif
