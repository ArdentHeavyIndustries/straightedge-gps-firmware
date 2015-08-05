#include <avr/interrupt.h>

volatile unsigned long lastPulseMs;
unsigned int cycle = 0;

void setup() {
  // put your setup code here, to run once:

  pinMode(1, OUTPUT);
  pinMode(2, INPUT);
  pinMode(4, OUTPUT);

  lastPulseMs = 0;

  attachInterrupt(0, ppsIsr, RISING);
}

void loop() {
  // put your main code here, to run repeatedly:

  unsigned long now = millis();

  digitalWrite(4, (now < lastPulseMs + 250) ? HIGH : LOW);
  digitalWrite(1, ((cycle % 256) > 127) ? HIGH : LOW);
  cycle++;

  delay(1);
}

void ppsIsr(void)
{
  lastPulseMs = millis();
}

