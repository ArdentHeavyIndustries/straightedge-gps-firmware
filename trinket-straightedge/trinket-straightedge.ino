#include <Arduino.h>
#include <avr/interrupt.h>
#include <stdlib.h>

#include "TinySerial.h"

#define RXPIN 0
#define ENABLEPIN 1
#define PPSPIN 2
#define TXPIN 3
#define LEDPIN 4

#define RATE 9600

TinySerial serialGPS = TinySerial(RXPIN, TXPIN);

volatile unsigned long lastPulseMs;
unsigned int cycle = 0;

#define BUFLEN 128
char buffer[BUFLEN];
int bufpos;
bool inSentence;

void setup() {
  // put your setup code here, to run once:

  pinMode(RXPIN, INPUT);
  pinMode(ENABLEPIN, OUTPUT);
  pinMode(PPSPIN, INPUT);
  pinMode(TXPIN, OUTPUT);
  pinMode(LEDPIN, OUTPUT);

  lastPulseMs = 0;

  attachInterrupt(0, ppsIsr, RISING);

  serialGPS.begin(RATE);

  bufpos = 0;
  inSentence = false;

  digitalWrite(ENABLEPIN, HIGH);
  digitalWrite(LEDPIN, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:

  unsigned long now = millis();
  

//  digitalWrite(1, (now < lastPulseMs + 40) ? HIGH : LOW);

  while (serialGPS.available()) {
    char c = serialGPS.read();
    if (c == '$') {
      inSentence = true;
      bufpos = 0;
    }
    
    if (inSentence) {
      buffer[bufpos] = c;
      bufpos++;
      if (bufpos >= BUFLEN) {
        bufpos = 0; // XXX
      }
    }

    if (c == '*') {
      inSentence = false;

      if (strncmp(buffer, "$GPRMC", 6) == 0) {
        digitalWrite(LEDPIN, HIGH);
        delay(5);
        digitalWrite(LEDPIN, LOW); 
      }

    }
  }
}

void blink(int onTime, int offTime)
{
  digitalWrite(4, HIGH);
  delay(onTime);
  digitalWrite(4, LOW);
  delay(offTime);
}

void ppsIsr(void)
{
  lastPulseMs = millis();
}

