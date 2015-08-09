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

enum state_enum {
  stateStartup,
  stateDaytime,
  stateDuskPreUnfixed,
  stateDuskPreFixed,
  stateNightPre,
  stateNightStart,
  stateDuskEvent,
  stateNightEvent
};

enum state_enum currentState;

enum state_enum stateLoop(enum state_enum);
enum state_enum startupLoop(void);
enum state_enum nightEventLoop(void);

void enterState(enum state_enum nextState);

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

  currentState = stateStartup;
}

#define MAX_PULSE_WAIT 1100
#define PULSE_START     100
#define PULSE_DUR        40

void loop(void) {
  enum state_enum nextState = stateLoop(currentState);

  if (nextState != currentState) {
    enterState(nextState);
  }

  currentState = nextState;
}

enum state_enum stateLoop(enum state_enum) {
  switch(currentState) {
    case stateStartup:
      return startupLoop();

    case stateNightEvent:
      return nightEventLoop();
    
    default:
      return stateStartup; // Recover from error by switching back to startup
  }
}

void enterState(enum state_enum nextState)
{
  
}

enum state_enum startupLoop(void) {
  return stateNightEvent;
}

enum state_enum nightEventLoop(void) {
  // Can't drain all serial input on one loop -- blocks too long, miss cycles through updateBlink
  if (serialGPS.available()) {
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
    unsigned long now = millis();
    
    struct datetime_struct nowDateTime = recentFix.fixDateTime;

    if (now < lastPulseMs + MAX_PULSE_WAIT) {    
      addSeconds(&nowDateTime, pulsesSinceFix);
    } else {
      addSeconds(&nowDateTime, ((unsigned int) ((now - lastPulseMs) / 1000L)));
    }

    updateBlink(&nowDateTime);
  }

  return stateNightEvent;
}

#define ALWAYS_BLINK 1

#ifndef ALWAYS_BLINK
# define BLINK_START 10800 /* 19:00 PDT = 03:00 UTC */
# define BLINK_STOP  54000 /* 07:00 PDT = 15:00 UTC */
#else
# define BLINK_START 0
# define BLINK_STOP  86400
#endif

void updateBlink(struct datetime_struct *nowDateTime)
{
  if (nowDateTime->secondInDay >= BLINK_START && nowDateTime->secondInDay <= BLINK_STOP) {
    unsigned long nowMs = millis();
    unsigned long millisInSecond = (nowMs - lastPulseMs) % 1000;
    
    digitalWrite(LEDPIN, ((millisInSecond >= PULSE_START) && (millisInSecond < (PULSE_START + PULSE_DUR))) ? HIGH : LOW);

    // Also seismic pulses
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

#define RMC_VALIDITY 17

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

  fupd->fixValid = (buffer[RMC_VALIDITY] == 'A');
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

