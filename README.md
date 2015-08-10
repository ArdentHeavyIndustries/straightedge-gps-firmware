# straightedge-gps-firmware
Firmware for the Trinket Microcontrolers that control the GPS and leds

## Setup

To get started working with the firmware, you'll need a few pieces of software for your Mac, Windows, or Linux development machine.

* Download, install, and run the [Arduino IDE](https://www.arduino.cc/en/Main/Software).
* You will some additional components for the Trinket hardware we're using. Open the Arduino IDE's Preferences. In the field marked "Additional Boards Manager URLs", add this URL:

<!-- -->

	https://adafruit.github.io/arduino-board-index/package_adafruit_index.json

* Open Tools > Board: "*Something*" >  Boards Manager.
* Find the entry named "Adafruit AVRBoards by Adafruit". The list of the boards should include a bunch of Adafruit Trinket models. Click Install, then close the Boards Manager.
* In Tools > Board: "*Something*" select the "Adafruit Trinket 8MHz".
* In Tools > Programmer: "*Something*" select USBtinyISP.

You should now be able to upload one of the firmwares to the boards. Some screenshots of the process are available in the [Adafruit Arduino IDE setup guide](https://learn.adafruit.com/adafruit-arduino-ide-setup/arduino-1-dot-6-x-ide) and the [Adafruit Trinket Setting Up with Arduino IDE guide](https://learn.adafruit.com/introducing-trinket/setting-up-with-arduino-ide).
