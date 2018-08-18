# DJ Hero Lucio
This code allows a user to play the character of [Lucio](https://playoverwatch.com/en-us/heroes/lucio/) in *Overwatch* using an Arduino and a DJ Hero turntable for the Nintendo Wii.

## Dependencies
The DJ Hero Lucio program uses my [NintendoExtensionCtrl library](https://github.com/dmadison/NintendoExtensionCtrl/) ([0.5.4](https://github.com/dmadison/NintendoExtensionCtrl/releases/tag/v0.5.4)) to handle communication to the DJ Hero controller itself.

This program was compiled using version [1.8.5](https://www.arduino.cc/en/Main/OldSoftwareReleases) of the [Arduino IDE](https://www.arduino.cc/en/Main/Software). If using an Arduino board such as the Leonardo or Pro Micro, the program is dependent on the Arduino Keyboard, Mouse, and Wire libraries which are installed with the IDE. If using a Teensy microcontroller, the program was built using [Teensyduino 1.42](https://www.pjrc.com/teensyduino-1-42-whats-new/) which you can download [here](https://www.pjrc.com/teensy/td_142), and also requires the [i2c_t3 library](https://github.com/nox771/i2c_t3) version [10.1](https://github.com/nox771/i2c_t3/releases/tag/v10.1).

I've linked to the specific library releases that work with this code. Note that other versions may not be compatible.

## License
This project is licensed under the terms of the [GNU General Public License](https://www.gnu.org/licenses/gpl-3.0.en.html), either version 3 of the License, or (at your option) any later version.
