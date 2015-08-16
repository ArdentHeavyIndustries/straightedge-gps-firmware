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

## Assembly test

To test the trinket-GPS assembly, download the trinket-gps-assembly-test directory, with trinket-gps-assembly-test.ino inside it. Compile it with the "verify" button on the Arduino IDE. 

Connect a trinket-GPS assembly to your computer with a microUSB cable. Push and release the reset button on the trinket, and when its red light starts blinking, hit the "upload" arrow button on the Arduino IDE. After the firmware is uploaded, unplug the microUSB.

Next, connect the trinket to the GPS Tester board. The connector goes to the 3 pins on top of the trinket. Plug it in with the black wire closest to the USB port. (Yes, this is confusing - the black wire here is power. Sorry.)

You can test 6 units at once. Connect the power and ground from the GPS Tester to a ~4.8 V power supply or battery pack.

The proper result depends on all the connections working.

On power-up, the LEDs on the tester should blink as follows:

* 1 heartbeat with no GPS power (enable off)
* 1 heartbeat with yes GPS power (enable on)
* After GPS gets a fix (green GPS light blinks) -- two blinks per second from the GPS echoed on the LED pin

If you leave it on for up to 15 minutes (usually less) you will see all 6 LEDs eventually flashing in sync as the GPS gets an adequate position lock.

The test firmware tests all four GPS-to-trinket connections:

No LED pin blinks until a valid GPRMC NMEA message (i.e., GPS position fix) is received. It only receives a fix if the GPS->trinket serial works.

No LED pin blinks if the pulse-to-pulse interval is <400 ms or >600 ms. Default pulse-to-pulse interval is 1 s (= 1000 ms) and it only changes to 500 ms if the trinket->GPS serial works.

No LED pin blinks if no pulses are received at all. Pulses are received only if the GPS->trinket PPS signal works.

Switching the GPS off and then back on during start-up depends on a working enable connection.
