#include <SoftwareSerial.h>

#include <avr/interrupt.h>

#include <stdlib.h>

#define RXPIN 0
#define TXPIN 3
#define RATE 9600

SoftwareSerial serialGPS = SoftwareSerial(RXPIN, TXPIN);

volatile unsigned long lastPulseMs;
unsigned int cycle = 0;

#define BUFLEN 128
char buffer[BUFLEN];
int bufpos;
bool inSentence;

void setup() {
  // put your setup code here, to run once:

  pinMode(RXPIN, INPUT);
  pinMode(TXPIN, OUTPUT);

  pinMode(1, OUTPUT);
  pinMode(2, INPUT);
  pinMode(4, OUTPUT);

  lastPulseMs = 0;

  attachInterrupt(0, ppsIsr, RISING);

  serialGPS.begin(RATE);

  bufpos = 0;
  inSentence = false;
}

void loop() {
  // put your main code here, to run repeatedly:

  unsigned long now = millis();

  digitalWrite(1, (now < lastPulseMs + 250) ? HIGH : LOW);

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
        blink(5, 5);

        int sec = (buffer[12] - '0');
        analogWrite(4, sec*25);
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

