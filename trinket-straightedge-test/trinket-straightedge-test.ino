#include <Arduino.h>
#include <avr/interrupt.h>
#include <stdlib.h>

#include <SoftwareSerial.h>

struct datetime_struct {
  long secondInDay;
  int dayInYear;
};

struct fix_struct {
  struct datetime_struct fixDateTime;
  unsigned long fixReceiveMs;
  uint8_t fixValid;
};

#define RXPIN 0
#define ENABLEPIN 1
#define PPSPIN 2
#define TXPIN 3
#define LEDPIN 4

#define RATE 9600

#define FAST_PULSE_LENGTH 40
uint8_t fastPulse[FAST_PULSE_LENGTH] = { 0xB5, 0x62, 0x06, 0x31, 0x20, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0xA1, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x86, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF3, 0x00, 0x00, 0x00, 0x3A, 0xAF };

SoftwareSerial serialGPS = SoftwareSerial(RXPIN, TXPIN);

volatile unsigned long ultimatePulseMs;
volatile unsigned long penultimatePulseMs;

#define BUFLEN 128
char buffer[BUFLEN];
int bufpos;
bool inSentence;

struct fix_struct recentFix;

void setup() {
  // put your setup code here, to run once:

  pinMode(RXPIN, INPUT);
  pinMode(ENABLEPIN, OUTPUT);
  pinMode(PPSPIN, INPUT);
  pinMode(TXPIN, OUTPUT);
  pinMode(LEDPIN, OUTPUT);

  ultimatePulseMs = 0;
  penultimatePulseMs = 0;

  attachInterrupt(0, ppsIsr, RISING);

  serialGPS.begin(RATE);

  bufpos = 0;
  inSentence = false;

  digitalWrite(ENABLEPIN, LOW);
  digitalWrite(LEDPIN, LOW);

  recentFix.fixValid = 0;

  heartbeat();

  digitalWrite(ENABLEPIN, HIGH);

  delay(1000);

  heartbeat();

  serialGPS.write(fastPulse, FAST_PULSE_LENGTH);
}

void loop() {
  while (serialGPS.available()) {
    char c = serialGPS.read();
    if (c == '$') {
      inSentence = true;
      bufpos = 0;
    }
    
    if (inSentence) {
      buffer[bufpos] = c;
      bufpos++;
      if (bufpos > BUFLEN) {
        inSentence = false;
      }
    }

    if (c == '*') {
      inSentence = false;
      if (strncmp(buffer, "$GPRMC", 6) == 0) {        
        updateFixFromNmea(&recentFix, buffer, bufpos);
        recentFix.fixValid = true;
      }
    }
  }

  if (recentFix.fixValid) {
    unsigned long now = millis();
    if ((now < ultimatePulseMs + 100) 
        && (ultimatePulseMs > penultimatePulseMs + 400)
        && (ultimatePulseMs < penultimatePulseMs + 600)) {
      digitalWrite(LEDPIN, HIGH);
    } else {
      digitalWrite(LEDPIN, LOW);
    }
  }
}

#define RMC_HOUR_TENS 7
#define RMC_HOUR_ONES 8
#define RMC_MINUTE_TENS 9
#define RMC_MINUTE_ONES 10
#define RMC_SECOND_TENS 11
#define RMC_SECOND_ONES 12

#define RMC_DAY_TENS 53
#define RMC_DAY_ONES 54
#define RMC_MONTH_TENS 55
#define RMC_MONTH_ONES 56

#define RMC_MIN_LEN 57

#define NMONTHS 12
int monthFirstDate[NMONTHS] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

void updateFixFromNmea(struct fix_struct *fupd, const char *buffer, int buflen)
{
  long secondInMinute = (buffer[RMC_SECOND_TENS] - '0') * 10 + (buffer[RMC_SECOND_ONES] - '0');
  long minuteInHour = (buffer[RMC_MINUTE_TENS] - '0') * 10 + (buffer[RMC_MINUTE_ONES] - '0');
  long hourInDay = (buffer[RMC_HOUR_TENS] - '0') * 10 + (buffer[RMC_HOUR_ONES] - '0');
  fupd->fixDateTime.secondInDay = secondInMinute = 60 * (minuteInHour + (60 * hourInDay));
  int dayInMonth = (buffer[RMC_DAY_TENS] - '0') * 10 + (buffer[RMC_DAY_ONES] - '0') - 1;
  int monthInYear = (buffer[RMC_MONTH_TENS] - '0') * 10 + (buffer[RMC_MONTH_ONES] - '0') - 1;
  fupd->fixDateTime.dayInYear = monthFirstDate[monthInYear] + dayInMonth;
}

void heartbeat(void)
{
  for (int i = 0; i < 255; i++) {
    analogWrite(LEDPIN, i);
    delay(1);
  }

  for (int i = 254; i >= 0; i--) {
    analogWrite(LEDPIN, i);
    delay(1);
  }
}

void ppsIsr(void)
{
  penultimatePulseMs = ultimatePulseMs;
  ultimatePulseMs = millis();
}

