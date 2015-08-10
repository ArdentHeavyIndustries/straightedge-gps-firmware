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

#define TESTING 1

/* N.B. 02:49 August 31st UTC = Sunday nightfall
 * Pre-event
 */
#ifndef TESTING
# define DUSK_START       7740L /* 19:04 PDT = 02:04 UTC */
# define NIGHT_START     10140L /* 19:49 PDT = 02:49 UTC */
# define EVENT_START_SEC 25200L /* 00:00 PDT = 07:00 UTC */
# define NIGHT_END       46800L /* 06:00 PDT = 13:00 UTC */
/* 10 hours and 11 minutes of blinky each day */
# define EVENT_START_DAY 242 /* August 31st in UTC-land */
#else
# define DUSK_START      (00L*3600L + 26L*60L)
# define NIGHT_START     (00L*3600L + 27L*60L)
# define EVENT_START_SEC 25200 /* 00:00 PDT = 07:00 UTC */
# define NIGHT_END       (00L*3600L + 28L*60L)
/* 10 hours and 11 minutes of blinky each day */
# define EVENT_START_DAY 212 /* August 31st in UTC-land */
#endif


TinySerial serialGPS = TinySerial(RXPIN, TXPIN);

volatile unsigned long lastPulseMs;
volatile unsigned long pulsesSinceFix;

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
  stateStartup,    /* A */
  stateDaytime,    /* B */
  stateDusk,       /* C */
  stateNightPre,   /* D */
  stateNightStart, /* E */
  stateNightEvent  /* F */
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

#if TESTING
unsigned long lastDebug;
uint8_t inDebug;
inline void debugDigit(uint8_t x) { serialGPS.write('0' + (x%10)); }
inline void debugLong(uint32_t x) { 
  debugDigit((x / 1000000L) % 10); 
  debugDigit((x / 100000L) % 10); 
  debugDigit((x / 10000L) % 10); 
  debugDigit((x / 1000L) % 10); 
  debugDigit((x / 100L) % 10); 
  debugDigit((x / 10L) % 10); 
  debugDigit(x % 10); 
}
#endif

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

#if TESTING
  lastDebug = 0;
  inDebug = false;
  serialGPS.write('H');
  serialGPS.write('i');
  serialGPS.write('\r');
  serialGPS.write('\n');
#endif
}

#define MAX_PULSE_WAIT   1100
#define PULSE_START       100
#define PULSE_DUR          40
#define UNSYNCH_INTERVAL 2500L

void loop(void) {
#if TESTING
  unsigned long now = millis();
  if (now - lastDebug > 1000L) {
    inDebug = true;
    lastDebug = now;
  } else {
    inDebug = false;
  }

  if (inDebug) {
    serialGPS.write('A' + currentState);
  }
#endif
  
  enum state_enum nextState = stateLoop(currentState);

#if TESTING
  if (inDebug) {
    serialGPS.write('\r');
    serialGPS.write('\n');
  }
#endif

  if (nextState != currentState) {
#if TESTING
    serialGPS.write('A' + currentState);
    serialGPS.write('a' + nextState);
    serialGPS.write('\r');
    serialGPS.write('\n');
#endif

    if (enterState(nextState) < 0) {
#if TESTING
      serialGPS.write('A' + currentState);
      serialGPS.write('x');
      serialGPS.write('\r');
      serialGPS.write('\n');
#endif
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
    struct datetime_struct nowDateTime;
    estimateNow(&nowDateTime);

    if ((nowDateTime.secondInDay < DUSK_START) || (nowDateTime.secondInDay >= NIGHT_END)) {
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
  } else {
    return stateNightEvent;
  }
}

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

void updateFixFromNmea(struct fix_struct *fupd, const char *buffer, int buflen)
{
  pulsesSinceFix = 0;
  fupd->fixReceiveMs = millis();

  unsigned long secondInMinute = (buffer[RMC_SECOND_TENS] - '0') * 10 + (buffer[RMC_SECOND_ONES] - '0');
  unsigned long minuteInHour = (buffer[RMC_MINUTE_TENS] - '0') * 10 + (buffer[RMC_MINUTE_ONES] - '0');
  unsigned long hourInDay = (buffer[RMC_HOUR_TENS] - '0') * 10 + (buffer[RMC_HOUR_ONES] - '0');
  fupd->fixDateTime.secondInDay = secondInMinute + 60 * (minuteInHour + (60 * hourInDay));
  unsigned int dayInMonth = (buffer[RMC_DAY_TENS] - '0') * 10 + (buffer[RMC_DAY_ONES] - '0') - 1;
  unsigned int monthInYear = (buffer[RMC_MONTH_TENS] - '0') * 10 + (buffer[RMC_MONTH_ONES] - '0') - 1;
  fupd->fixDateTime.dayInYear = monthFirstDate[monthInYear] + dayInMonth;

  fupd->fixValid = (buffer[RMC_VALIDITY] == 'A');
}

#define SECONDS_IN_DAY 86400L
void addSeconds(struct datetime_struct *dt, unsigned long nsecs)
{
  dt->secondInDay += nsecs;
  while (dt->secondInDay >= SECONDS_IN_DAY) {
    dt->secondInDay -= SECONDS_IN_DAY;
    dt->dayInYear++;
  }
}

void estimateNow(struct datetime_struct *nowDateTime)
{
  unsigned long now = millis();
  unsigned long extraSeconds = 
    (now < lastPulseMs + MAX_PULSE_WAIT) ? 
    pulsesSinceFix : 
    ((now - recentFix.fixReceiveMs) / 1000L);

  *nowDateTime = recentFix.fixDateTime;

  addSeconds(nowDateTime, extraSeconds);

#if TESTING
    if (inDebug) {
      serialGPS.write((now < lastPulseMs + MAX_PULSE_WAIT) ? 'S' : 'U');
      debugLong(extraSeconds);
      serialGPS.write(' ');
      debugLong(recentFix.fixDateTime.secondInDay);
      serialGPS.write(' ');
      debugLong(nowDateTime->secondInDay);
      serialGPS.write(' ');
      debugLong(DUSK_START);
      serialGPS.write(' ');
      debugLong(NIGHT_END);
    }
#endif    
}

inline uint8_t dtSecond(const datetime_struct *dt) { return (uint8_t) (dt->secondInDay % 60); }
inline uint8_t dtMinute(const datetime_struct *dt) { return (uint8_t) ((dt->secondInDay / 60) % 60); }
inline uint8_t dtHour(const datetime_struct *dt) { return (uint8_t) (dt->secondInDay / 3600); }

void ppsIsr(void)
{
  lastPulseMs = millis();
  pulsesSinceFix++;
}

// configure power save mode to be cyclic mode - UBX-CFG-PM2
#define SETUP_LEN 52
uint8_t setupPSM[SETUP_LEN] = { 0xB5, 0x62, 0x06, 0x3B, 0x2C, 0x00, 0x01, 0x06, 0x00, 0x00, 0x00, 0x80, 0x02, 0x01, 0x10, 0x27, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2C, 0x01, 0x00, 0x00, 0x4F, 0xC1, 0x03, 0x00, 0x87, 0x02, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x64, 0x40, 0x01, 0x00, 0xD2, 0xEA } ; 

// turn on power save mode - UBX-CFG-RXM
#define ACTIVATE_LEN 10
uint8_t activatePSM[ACTIVATE_LEN] = { 0xB5, 0x62, 0x06, 0x11, 0x02, 0x00, 0x08, 0x01, 0x22, 0x92 } ;

// Send a byte array of UBX protocol to the GPS
inline void sendUBX(uint8_t *MSG, uint8_t len) {
  for(int i=0; i<len; i++) {
    serialGPS.write(MSG[i]);
  }
}

void activateGPS(void)
{
  digitalWrite(ENABLEPIN, HIGH);

  sendUBX(setupPSM, SETUP_LEN);
  sendUBX(activatePSM, ACTIVATE_LEN);
}

void deactivateGPS(void)
{
  digitalWrite(ENABLEPIN, LOW);
}

