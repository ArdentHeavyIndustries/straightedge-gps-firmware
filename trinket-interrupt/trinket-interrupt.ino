#include <SoftwareSerial.h>

#include <avr/interrupt.h>

#define RXPIN 0
#define TXPIN 3
#define RATE 9600

SoftwareSerial serialGPS = SoftwareSerial(RXPIN, TXPIN);

volatile unsigned long lastPulseMs;
unsigned int cycle = 0;

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
}

void loop() {
  // put your main code here, to run repeatedly:

  unsigned long now = millis();

  digitalWrite(1, (now < lastPulseMs + 250) ? HIGH : LOW);

  while (serialGPS.available()) {
//    digitalWrite(4, HIGH);
    char c = serialGPS.read();
    analogWrite(4, c);
  }
  digitalWrite(4, LOW);
  
  cycle++;
}

void ppsIsr(void)
{
  lastPulseMs = millis();
}

