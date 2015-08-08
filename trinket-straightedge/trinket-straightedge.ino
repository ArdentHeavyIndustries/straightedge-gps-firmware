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
volatile unsigned int pulsesSinceFix;

#define BUFLEN 128
char buffer[BUFLEN];
int bufpos;
bool inSentence;

struct datetime_struct {
  long secondInDay;
  int dayInYear;
};

struct fix_struct {
  struct datetime_struct fixDateTime;
  unsigned long fixReceiveMs;
  uint8_t fixValid;
};

struct fix_struct recentFix;

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

  recentFix.fixValid = 0;
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
      }
    }
  }

  if (recentFix.fixValid) {
    struct datetime_struct nowDateTime = recentFix.fixDateTime;
    addSeconds(&nowDateTime, pulsesSinceFix);
    updateBlink(&nowDateTime);
  }
}

#define BLINK_START 10800 /* 19:00 PDT = 03:00 UTC */
#define BLINK_STOP  54000 /* 07:00 PDT = 15:00 UTC */
#define BLINK_DUR 40

void updateBlink(struct datetime_struct *nowDateTime)
{
  if (nowDateTime->secondInDay >= BLINK_START && nowDateTime->secondInDay <= BLINK_STOP)
  {
    unsigned long nowMs = millis();
    
    if (dtSecond(nowDateTime) < 2) {
      
    } else {
      digitalWrite(LEDPIN, (nowMs < lastPulseMs + BLINK_DUR) ? HIGH : LOW);
    }
  } else {
    digitalWrite(LEDPIN, LOW); 
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

#define SECONDS_IN_DAY 86400
void addSeconds(struct datetime_struct *dt, unsigned int nsecs)
{
  dt->secondInDay += (long) nsecs;
  while (dt->secondInDay >= SECONDS_IN_DAY) {
    dt->secondInDay -= SECONDS_IN_DAY;
    dt->dayInYear++;
  }
}

inline uint8_t dtSecond(const datetime_struct *dt)
{
  return (uint8_t) (dt->secondInDay % 60);
}

inline uint8_t dtMinute(const datetime_struct *dt)
{
  return (uint8_t) ((dt->secondInDay / 60) % 60);
}

inline uint8_t dtHour(const datetime_struct *dt)
{
  return (uint8_t) (dt->secondInDay / 3600);
}

void ppsIsr(void)
{
  lastPulseMs = millis();
  pulsesSinceFix++;
}

