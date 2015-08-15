#include <Arduino.h>
#include <avr/interrupt.h>
#include <stdlib.h>

#include "TinySerial.h"
#include "trinket-straightedge.h"

TinySerial serialGPS = TinySerial(RXPIN, TXPIN);

volatile unsigned long lastPulseMs;
volatile unsigned long pulsesSinceFix;

#define BUFLEN 128
char buffer[BUFLEN];
int bufpos;
bool inSentence;

struct fix_struct recentFix;

enum state_enum currentState;

/* Should be DEBUG-only */
unsigned long lastDebug;
uint8_t inDebug;
void debugDigit(uint8_t x) { serialGPS.write('0' + (x%10)); }
void debugLong(uint32_t x) { 
  debugDigit((x / 1000000L) % 10); 
  debugDigit((x / 100000L) % 10); 
  debugDigit((x / 10000L) % 10); 
  debugDigit((x / 1000L) % 10); 
  debugDigit((x / 100L) % 10); 
  debugDigit((x / 10L) % 10); 
  debugDigit(x % 10); 
}

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
    enterState(nextState);
    currentState = nextState;
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

void enterState(enum state_enum nextState)
{
  digitalWrite(LEDPIN, LOW);
  
  switch(nextState) {
    case stateStartup:
      return startupEnter();
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
  }  
}

void startupEnter(void)    { recentFix.fixValid = 0;   activateGPS(); }
void daytimeEnter(void)    {                         deactivateGPS(); }
void duskEnter(void)       { recentFix.fixValid = 0;   activateGPS(); }
void nightPreEnter(void)   {                         deactivateGPS(); }
void nightStartEnter(void) {                           activateGPS(); }
void nightEventEnter(void) {                           activateGPS(); }

enum state_enum startupLoop(void) {
  /* Dim LED heart-beat every other second during start-up */
  unsigned long now = millis();
  unsigned int cycle = (now / 32L) % 64L;
  int bright = (cycle < 8) ? cycle : ((cycle < 16) ? 16 - cycle : 0);
  analogWrite(LEDPIN, bright);
  
  if (!recentFix.fixValid) {
    return stateStartup;
  }

  if (recentFix.fixDateTime.secondInDay < DUSK_START || recentFix.fixDateTime.secondInDay >= NIGHT_END) {
    return stateDaytime;
  } else { /* Jump from startup into dusk and let dusk state switch into night -- keeps all logic to pick which night state in one function */
    return stateDusk;
  }
}

enum state_enum daytimeLoop(void) {
  if (recentFix.fixValid) {
    struct datetime_struct nowDateTime;
    estimateNow(&nowDateTime);

    if ((nowDateTime.secondInDay < DUSK_START) || (nowDateTime.secondInDay >= NIGHT_END)) {
      return stateDaytime;
    } else { /* Switch from daytime to dusk and never directly to night, in order to get a good GPS fix */
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
      /* Note -- this runs one cycle of nightStart even if we're switch out of dusk after the event start time */
      return stateNightStart;
    } else {
      return stateNightEvent;
    }
  } else { /* Keep waiting for a valid fix &/or nightfall */
    return stateDusk;
  }
}

enum state_enum nightPreLoop(void) 
{
  struct datetime_struct nowDateTime;
  estimateNow(&nowDateTime);

  if (nowDateTime.secondInDay >= NIGHT_START) {
    unsigned long millisInPeriod = millis() % UNSYNCH_PRE_PERIOD;
    digitalWrite(LEDPIN, ((millisInPeriod >= PULSE_START) && (millisInPeriod < (PULSE_START + PRE_PULSE_DUR))) ? HIGH : LOW);
  }
  
  if (nowDateTime.secondInDay >= NIGHT_END) {
    return stateDaytime;
  } else {
    return stateNightPre;
  }
}

enum state_enum nightStartLoop(void) {
  struct datetime_struct nowDateTime;
  estimateNow(&nowDateTime);
/*
  if (nowDateTime.secondInDay >= NIGHT_START) {
    unsigned long millisInPeriod = millis() % UNSYNCH_START_PERIOD;
    digitalWrite(LEDPIN, ((millisInPeriod >= PULSE_START) && (millisInPeriod < (PULSE_START + START_PULSE_DUR))) ? HIGH : LOW);
  }
  */
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

/* Byte offsets into $GPRMC message for information we want */

/* $GPRMC,063012.00,A,3746.81357,N,12224.40698,W,1.656,46.73,050815,,,A* */
/* 01234567890123456789012345678901234567890123456789012345678901234567  */
/* 0         1         2         3         4         5         6         */
/* $GPRMC,062908.00,A,3746.81259,N,12224.40523,W,1.277,,050815,,,A* */
/* 0123456789012345678901234567890123456789012345678901234567890123 */
/* 0         1         2         3         4         5         6    */

/* $GPRMC,045628.00,V,,,,,,,150815,,,N*78 */
/* 01234567890123456789012345678901234567 */
/* 0         1         2         3        */

/* UTC validity, A = valid, V = questionable */
#define RMC_VALIDITY 17

/* UTC time */
#define RMC_HOUR_TENS 7
#define RMC_HOUR_ONES 8
#define RMC_MINUTE_TENS 9
#define RMC_MINUTE_ONES 10
#define RMC_SECOND_TENS 11
#define RMC_SECOND_ONES 12

/* UTC date */
#define RMC_SPEED_START   46
#define RMC_HEADING_START 52
#define RMC_DAY_TENS_NOHEADING    53
#define RMC_DAY_ONES_NOHEADING    54
#define RMC_MONTH_TENS_NOHEADING  55
#define RMC_MONTH_ONES_NOHEADING  56
#define RMC_HEADING_EXTRA          5

/* Longitude */
#define RMC_DEGREE_HUNDREDS   32
#define RMC_DEGREE_TENS       33
#define RMC_DEGREE_ONES       34
#define RMC_LONGIMIN_TENS     35
#define RMC_LONGIMIN_ONES     36
#define RMC_LONGIMIN_FIRST    38
#define RMC_LONGIMIN_SECOND   39
#define RMC_LONGIMIN_THIRD    40
#define RMC_LONGIMIN_FOURTH   41
#define RMC_LONGIMIN_FIFTH    42

#define RMC_MIN_LEN 57

#define NMONTHS 12
int monthFirstDate[NMONTHS] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
                             /* J  F   M   A   M    Jun  Jul  A    S    O    N    D */

void updateFixFromNmea(struct fix_struct *fupd, const char *buffer, int buflen)
{
  if (buflen < RMC_MIN_LEN) {
    return;
  }
  
  pulsesSinceFix = 0;
  fupd->fixReceiveMs = millis();

  unsigned long secondInMinute = (buffer[RMC_SECOND_TENS] - '0') * 10 + (buffer[RMC_SECOND_ONES] - '0');
  unsigned long minuteInHour = (buffer[RMC_MINUTE_TENS] - '0') * 10 + (buffer[RMC_MINUTE_ONES] - '0');
  unsigned long hourInDay = (buffer[RMC_HOUR_TENS] - '0') * 10 + (buffer[RMC_HOUR_ONES] - '0');
  fupd->fixDateTime.secondInDay = secondInMinute + 60 * (minuteInHour + (60 * hourInDay));

  fupd->fixValid = (buffer[RMC_VALIDITY] == 'A');

  int extra = 0;
  if (buffer[RMC_HEADING_START] != ',') {
    extra = RMC_HEADING_EXTRA;
  }
  
  unsigned int dayInMonth = (buffer[extra + RMC_DAY_TENS_NOHEADING] - '0') * 10 + (buffer[extra + RMC_DAY_ONES_NOHEADING] - '0') - 1;
  unsigned int monthInYear = (buffer[extra + RMC_MONTH_TENS_NOHEADING] - '0') * 10 + (buffer[extra + RMC_MONTH_ONES_NOHEADING] - '0') - 1;
  fupd->fixDateTime.dayInYear = monthFirstDate[monthInYear] + dayInMonth;

  fupd->fixLongiUMin = ((unsigned long) (buffer[RMC_LONGIMIN_TENS] - '0')) * 10000000L
    + ((unsigned long) (buffer[RMC_LONGIMIN_ONES] - '0')) * 1000000L
    + ((unsigned long) (buffer[RMC_LONGIMIN_FIRST] - '0')) * 100000L
    + ((unsigned long) (buffer[RMC_LONGIMIN_SECOND] - '0')) * 10000L
    + ((unsigned long) (buffer[RMC_LONGIMIN_THIRD] - '0')) * 1000L
    + ((unsigned long) (buffer[RMC_LONGIMIN_FOURTH] - '0')) * 100L
    + ((unsigned long) (buffer[RMC_LONGIMIN_FIFTH] - '0')) * 10L;

#if TESTING
  debugLong(fupd->fixLongiUMin);

  serialGPS.write('\r');
  serialGPS.write('\n');
#endif
}

#define SECONDS_IN_DAY 86400L
void addSeconds(struct datetime_struct *dt, unsigned long extraSeconds)
{
  dt->secondInDay += extraSeconds;
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
      serialGPS.write(' ');
      debugLong(nowDateTime->dayInYear);
      serialGPS.write(' ');
      debugLong(EVENT_START_DAY);
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

