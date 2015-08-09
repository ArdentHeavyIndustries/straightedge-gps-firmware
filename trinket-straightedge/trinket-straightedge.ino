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

/* N.B. 02:49 August 31st UTC = Sunday nightfall
 * Pre-event
 */
#ifndef TESTING
# define DUSK_START       7740 /* 19:04 PDT = 02:04 UTC */
# define NIGHT_START     10140 /* 19:49 PDT = 02:49 UTC */
# define EVENT_START_SEC 25200 /* 00:00 PDT = 07:00 UTC */
# define NIGHT_END       46800 /* 06:00 PDT = 13:00 UTC */
/* 10 hours and 11 minutes of blinky each day */
# define EVENT_START_DAY 242 /* August 31st in UTC-land */
#else
# define DUSK_START       7740 /* 19:04 PDT = 02:04 UTC */
# define NIGHT_START     10140 /* 19:49 PDT = 02:49 UTC */
# define EVENT_START_SEC 25200 /* 00:00 PDT = 07:00 UTC */
# define NIGHT_END       46800 /* 06:00 PDT = 13:00 UTC */
/* 10 hours and 11 minutes of blinky each day */
# define EVENT_START_DAY 242 /* August 31st in UTC-land */
#endif

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
  stateDusk,
  stateNightPre,
  stateNightStart,
  stateNightEvent
};

enum state_enum currentState;

enum state_enum stateLoop(enum state_enum);
enum state_enum startupLoop(void);
enum state_enum daytimeLoop(void);
enum state_enum duskLoop(void);
enum state_enum nightPreLoop(void);
enum state_enum nightStartLoop(void);
enum state_enum nightEventLoop(void);

int enterState(enum state_enum nextState);
void startupEnter(void); /* Can't fail! */
int daytimeEnter(void);
int duskEnter(void);
int nightPreEnter(void);
int nightStartEnter(void);
int nightEventEnter(void);

void setup() {
  pinMode(RXPIN, INPUT);
  pinMode(ENABLEPIN, OUTPUT);
  pinMode(PPSPIN, INPUT);
  pinMode(TXPIN, OUTPUT);
  pinMode(LEDPIN, OUTPUT);

  digitalWrite(ENABLEPIN, LOW);
  digitalWrite(LEDPIN, LOW);
  digitalWrite(TXPIN, LOW);

  lastPulseMs = 0;

  attachInterrupt(0, ppsIsr, RISING);

  serialGPS.begin(RATE);

  bufpos = 0;
  inSentence = false;

  recentFix.fixValid = 0;

  currentState = stateStartup;
  startupEnter();
}

#define MAX_PULSE_WAIT   1100
#define PULSE_START       100
#define PULSE_DUR          40
#define UNSYNCH_INTERVAL 2500L

void loop(void) {
  enum state_enum nextState = stateLoop(currentState);

  if (nextState != currentState) {
    if (enterState(nextState) < 0) {
      currentState = stateStartup;
      startupEnter();
    } else {
      currentState = nextState;
    }
  }
}

enum state_enum stateLoop(enum state_enum) {
  serialLoop();
  
  switch(currentState) {
    case stateStartup:
      return startupLoop();
    case stateDaytime:
      return daytimeLoop();
    case stateDusk:
      return duskLoop();
    case stateNightPre:
      return nightPreLoop();
    case stateNightStart:
      return nightStartLoop();
    case stateNightEvent:
      return nightEventLoop();
    
    default:
      return stateStartup; // Recover from error by switching back to startup
  }
}

int enterState(enum state_enum nextState)
{
  digitalWrite(LEDPIN, LOW);
  
  switch(nextState) {
    case stateStartup:
      {
        startupEnter();
        return 0;
      }
    case stateDaytime:
      return daytimeEnter();
    case stateDusk:
      return duskEnter();
    case stateNightPre:
      return nightPreEnter();
    case stateNightStart:
      return nightStartEnter();
    case stateNightEvent:
      return nightEventEnter();
    default:
      return -1;
  }  
}

void startupEnter(void)   { recentFix.fixValid = 0;   activateGPS(); }
int daytimeEnter(void)    {                         deactivateGPS(); return 0; }
int duskEnter(void)       { recentFix.fixValid = 0;   activateGPS(); return 0; }
int nightPreEnter(void)   {                         deactivateGPS(); return 0; }
int nightStartEnter(void) {                           activateGPS(); return 0; }
int nightEventEnter(void) {                           activateGPS(); return 0; }

enum state_enum startupLoop(void) {
  unsigned long now = millis();
  unsigned int cycle = (now / 32L) % 64L;
  int bright = (cycle < 8) ? cycle : ((cycle < 16) ? 16 - cycle : 0);
  analogWrite(LEDPIN, bright);
  
  if (!recentFix.fixValid) {
    return stateStartup;
  }

  if (recentFix.fixDateTime.secondInDay < DUSK_START || recentFix.fixDateTime.secondInDay > NIGHT_END) {
    return stateDaytime;
  } else {
    return stateDusk;
  }
}

enum state_enum daytimeLoop(void) {
  if (recentFix.fixValid) {
    unsigned long now = millis();
    unsigned long extraSeconds = (now - recentFix.fixReceiveMs) / 1000L;
    struct datetime_struct nowDateTime = recentFix.fixDateTime;
    addSeconds(&nowDateTime, (unsigned int) extraSeconds);
    if (nowDateTime.secondInDay < DUSK_START || nowDateTime.secondInDay > NIGHT_END) {
      return stateDaytime;
    } else { /* Always switch to dusk in order to get a good GPS fix */
      return stateDusk;
    }
  } else { /* No idea what time it is! */
    return stateStartup;
  }
}

enum state_enum duskLoop(void) 
{ 
  if (recentFix.fixValid) {
    struct datetime_struct nowDateTime;
    estimateNow(&nowDateTime);
    
    if (nowDateTime.dayInYear < EVENT_START_DAY) {
      return stateNightPre;
    } else if (nowDateTime.secondInDay < NIGHT_START) {
      return stateDusk;
    } else if (nowDateTime.dayInYear == EVENT_START_DAY) {
      return stateNightStart;
    } else {
      return stateNightEvent;
    }
  } else { /* Keep waiting for a valid fix &/or nightfall */
    return stateDusk;
  }
}

inline void unsynchedBlink(void) {
  unsigned long now = millis();
  unsigned long millisInInterval = (now - lastPulseMs) % UNSYNCH_INTERVAL;
  digitalWrite(LEDPIN, ((millisInInterval >= PULSE_START) && (millisInInterval < (PULSE_START + PULSE_DUR))) ? HIGH : LOW);  
}

enum state_enum nightPreLoop(void) 
{
  struct datetime_struct nowDateTime;
  estimateNow(&nowDateTime);

  unsynchedBlink();

  if (nowDateTime.secondInDay >= NIGHT_END) {
    return stateDaytime;
  } else {
    return stateNightPre;
  }
}

enum state_enum nightStartLoop(void) {
  struct datetime_struct nowDateTime;
  estimateNow(&nowDateTime);

  unsynchedBlink();

  if (nowDateTime.secondInDay >= NIGHT_END || nowDateTime.secondInDay < DUSK_START) {
    return stateDaytime;
  } else if (nowDateTime.secondInDay >= EVENT_START_SEC) {
    return stateNightEvent;
  } else {
    return stateNightStart;
  }
}

enum state_enum nightEventLoop(void) 
{
  struct datetime_struct nowDateTime;
  estimateNow(&nowDateTime);

  unsigned long now = millis();
  unsigned long millisInSecond = (now - lastPulseMs) % 1000L;
  digitalWrite(LEDPIN, ((millisInSecond >= PULSE_START) && (millisInSecond < (PULSE_START + PULSE_DUR))) ? HIGH : LOW);

  if (nowDateTime.secondInDay >= NIGHT_END || nowDateTime.secondInDay < DUSK_START) {
    return stateDaytime;
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
                             /* J  F   M   A   M    Jun  Jul  A    S    O    N    D */
// Can't drain all serial input on one loop -- blocks too long, miss cycles through updateBlink
void serialLoop(void)
{
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
}

void updateFixFromNmea(struct fix_struct *fupd, const char *buffer, int buflen)
{
  fupd->fixReceiveMs = millis();

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

void estimateNow(struct datetime_struct *nowDateTime)
{
  unsigned long now = millis();
  *nowDateTime = recentFix.fixDateTime;

  if (now < lastPulseMs + MAX_PULSE_WAIT) {    
    addSeconds(nowDateTime, pulsesSinceFix);
  } else {
    addSeconds(nowDateTime, ((unsigned int) ((now - lastPulseMs) / 1000L)));
  }
}

inline uint8_t dtSecond(const datetime_struct *dt) { return (uint8_t) (dt->secondInDay % 60); }
inline uint8_t dtMinute(const datetime_struct *dt) { return (uint8_t) ((dt->secondInDay / 60) % 60); }
inline uint8_t dtHour(const datetime_struct *dt) { return (uint8_t) (dt->secondInDay / 3600); }

void ppsIsr(void)
{
  lastPulseMs = millis();
  pulsesSinceFix++;
}

void activateGPS(void)
{
  digitalWrite(ENABLEPIN, HIGH);

  /* XXX POWER SAVE MODE */
}

void deactivateGPS(void)
{
  digitalWrite(ENABLEPIN, LOW);
}

