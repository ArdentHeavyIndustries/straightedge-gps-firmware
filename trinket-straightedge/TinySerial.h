/* TinySerial.h
 *  Modified from SoftwareSerial.h
 *  No longer derives from Stream!!!

SoftwareSerial.h (formerly NewSoftSerial.h) - 
Multi-instance software serial library for Arduino/Wiring
-- Interrupt-driven receive and other improvements by ladyada
   (http://ladyada.net)
-- Tuning, circular buffer, derivation from class Print/Stream,
   multi-instance support, porting to 8MHz processors,
   various optimizations, PROGMEM delay tables, inverse logic and 
   direct port writing by Mikal Hart (http://www.arduiniana.org)
-- Pin change interrupt macros by Paul Stoffregen (http://www.pjrc.com)
-- 20MHz processor support by Garrett Mace (http://www.macetech.com)
-- ATmega1280/2560 support by Brett Hagman (http://www.roguerobotics.com/)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

The latest version of this library can always be found at
http://arduiniana.org.
*/

#ifndef TinySerial_h
#define TinySerial_h

#include <inttypes.h>

/******************************************************************************
* Definitions
******************************************************************************/

#define _SS_MAX_RX_BUFF 64 // RX buffer size
#ifndef GCC_VERSION
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

class TinySerial
{
private:
  static uint8_t _receivePin;
  static uint8_t _receiveBitMask;
  static volatile uint8_t *_receivePortRegister;
  static uint8_t _transmitBitMask;
  static volatile uint8_t *_transmitPortRegister;

  static const uint16_t _rx_delay_centering;
  static const uint16_t _rx_delay_intrabit;
  static const uint16_t _rx_delay_stopbit;
  static const uint16_t _tx_delay;

  // static data
  static char _receive_buffer[_SS_MAX_RX_BUFF]; 
  static volatile uint8_t _receive_buffer_tail;
  static volatile uint8_t _receive_buffer_head;
  static TinySerial *active_object;

  // private methods
  static void recv();
  static uint8_t rx_pin_read();
  static void tx_pin_write(uint8_t pin_state);
  static void setTX(uint8_t transmitPin);
  static void setRX(uint8_t receivePin);

  // private static method for timing
  static inline void tunedDelay(uint16_t delay);

public:
  // public methods
  static void begin(uint8_t receivePin, uint8_t transmitPin);

  static size_t write(uint8_t byte);
  static int read();
  static int available();
  static void flush();
  
  // public only for easy access by interrupt handlers
  static inline void handle_interrupt();
};

// Arduino 0012 workaround
#undef int
#undef char
#undef long
#undef byte
#undef float
#undef abs
#undef round

#endif
