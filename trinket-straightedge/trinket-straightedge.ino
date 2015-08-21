#include <Arduino.h>
#include <avr/interrupt.h>
#include <stdlib.h>

#include "TinySerial.h"
#include "trinket-straightedge.h"

volatile unsigned long lastPulseMs;
volatile unsigned long pulsesSinceFix;

#define BUFLEN 128
char buffer[BUFLEN];
uint8_t bufpos;
uint8_t inSentence;

struct fix_struct recentFix;

enum state_enum currentState;

#if TESTING
# if ARDUINO_UNO
#  define DEBUGSERIAL Serial.write
# else
#  define DEBUGSERIAL TinySerial::write
# endif
/* Should be DEBUG-only */
unsigned long lastDebug;
uint8_t inDebug;
void debugLong(uint32_t x) { 
  for (unsigned long l = 100000000L; l > 0; l = l / 10L) {
    DEBUGSERIAL('0' + ((x / l) % 10));
  }
}
#endif

#if TESTING_ISR
volatile unsigned long isrCount = 0;
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

  TinySerial::begin(RXPIN, TXPIN);

  bufpos = 0;
  inSentence = false;

  recentFix.fixValid = 0;

  currentState = stateStartup;
  startupEnter();

#if TESTING
  lastDebug = 0;
  inDebug = false;
  DEBUGSERIAL('H');
  DEBUGSERIAL('i');
  DEBUGSERIAL('\r');
  DEBUGSERIAL('\n');
#endif

#if ARDUINO_UNO
  Serial.begin(57600);
  Serial.write("Arduino Uno xXx test code");

  char test1[BUFLEN] = "$GPRMC,063012.00,A,3746.81357,N,12224.40698,W,1.656,46.73,050815,,,A*";
  char test2[BUFLEN] = "$GPRMC,062908.00,A,3746.81259,N,12224.40523,W,1.277,,050815,,,A*";
  char test3[BUFLEN] = "$GPRMC,063012.00,A,3746.81357,N,12224.40698,W,1.656,146.73,050815,,,A*";
  char test4[BUFLEN] = "$GPRMC,063012.00,A,3746.81357,N,12224.40698,W,1.656,6.73,050815,,,A*";
  char test5[BUFLEN] = "$GPRMC,081618.00,A,3746.78884,N,12224.42742,W,11.085,225.66,150815,,,A*";

  struct fix_struct test_fix;
  updateFixFromNmea(&test_fix, test1, strlen(test1));
  updateFixFromNmea(&test_fix, test2, strlen(test2));
  updateFixFromNmea(&test_fix, test3, strlen(test3));
  updateFixFromNmea(&test_fix, test4, strlen(test4));
  updateFixFromNmea(&test_fix, test5, strlen(test5));

  for (unsigned long i = 99; i < (99 + SEISMIC_DURATION_FULL + 10); i++) {
    Serial.print(i);
    Serial.write('\t');
    Serial.print(seismicBrightness(100, i));
    Serial.println();
  }
#endif
}

void loop(void) {
#if TESTING
/*
  unsigned long now = millis();
  if (now - lastDebug > 1075L) {
    inDebug = true;
    lastDebug = now;
  } else {
    inDebug = false;
  }
*/
  if (lastPulseMs > lastDebug) {
    inDebug = true;
    lastDebug = lastPulseMs;
  } else {
    inDebug = false;
  }
#endif /* TESTING */

#if TESTING_STATE
  if (inDebug) {
    DEBUGSERIAL('A' + currentState);
  }
#endif /* TESTING_STATE */
  
  enum state_enum nextState = stateLoop(currentState);

#if TESTING_STATE
  if (inDebug) {
    DEBUGSERIAL('\r');
    DEBUGSERIAL('\n');
  }
#endif /* TESTING_STATE */

  if (nextState != currentState) {
#if TESTING_STATE
    DEBUGSERIAL('A' + currentState);
    DEBUGSERIAL('a' + nextState);
    DEBUGSERIAL('\r');
    DEBUGSERIAL('\n');
#endif /* TESTING_STATE */
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
#if !TESTING_TIME
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
#endif /* !TESTING_TIME */
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
#if !TESTING_TIME
  struct datetime_struct nowDateTime;
  estimateNow(&nowDateTime);

  if (nowDateTime.secondInDay >= NIGHT_START) {
    unsigned long millisInPeriod = millis() % UNSYNCH_PRE_PERIOD;
    digitalWrite(LEDPIN, ((millisInPeriod >= PULSE_START_A) && (millisInPeriod < (PULSE_START_A + PRE_PULSE_DUR))) ? HIGH : LOW);
  }
  
  if (nowDateTime.secondInDay >= NIGHT_END) {
    return stateDaytime;
  } else {
    return stateNightPre;
  }
#endif /* !TESTING_TIME */
}

enum state_enum nightStartLoop(void) {
#if !TESTING_TIME
  struct datetime_struct nowDateTime;
  estimateNow(&nowDateTime);

  if (nowDateTime.secondInDay >= NIGHT_START) {
    unsigned long millisInPeriod = millis() % UNSYNCH_START_PERIOD;
    digitalWrite(LEDPIN, ((millisInPeriod >= PULSE_START_A) && (millisInPeriod < (PULSE_START_A + START_PULSE_DUR))) ? HIGH : LOW);
  }

  if (nowDateTime.secondInDay >= NIGHT_END || nowDateTime.secondInDay < DUSK_START) {
    return stateDaytime;
  } else if (nowDateTime.secondInDay >= EVENT_START_SEC) {
    return stateNightEvent;
  } else {
    return stateNightStart;
  }
#endif /* !TESTING_TIME */  
}

uint8_t seismicBrightness(unsigned long waveOffset, unsigned long msNow)
{
  if ( (waveOffset < msNow) && (msNow < (waveOffset + SEISMIC_DURATION_FULL)) ) {
    if (msNow <= (waveOffset + SEISMIC_DURATION_BRIGHT)) {
      return 255;
    } else {
      unsigned int fadeTime = msNow - (waveOffset + SEISMIC_DURATION_BRIGHT);
      if (fadeTime <= SEISMIC_DURATION_FADE) {
        return (uint8_t) (255 - (255 * fadeTime / SEISMIC_DURATION_FADE));
      }
    }
  }

  return 0;
}

enum state_enum nightEventLoop(void) 
{
  struct datetime_struct nowDateTime;
  estimateNow(&nowDateTime);

  // am i going in the right direction? (should be positive...)
  // Time offset in milliseconds from start of seismic event to pulse starting at this location
  unsigned long swaveOffset = PULSE_START_A + (( recentFix.fixLongiUMin - X0 ) / LONG_INTERVAL ) * SWAVE_INTERVAL;
  unsigned long pwaveOffset = PULSE_START_A + (( recentFix.fixLongiUMin - X0 ) / LONG_INTERVAL ) * PWAVE_INTERVAL;
  
  // check if this works as intended
  // seismic event always starts at the top of a minute
  unsigned long msNow = ( dtSecond(&nowDateTime) * 1000L ) + ((unsigned long) nowDateTime.millisInSecond);

  // True iff we're in a "normal" pulse with 
  uint8_t inNormalPulse = (dtSecond(&nowDateTime) % PULSE_FREQUENCY == 0) &&
                          ( (nowDateTime.millisInSecond >= PULSE_START_A && nowDateTime.millisInSecond < PULSE_END_A) ||
                            (nowDateTime.millisInSecond >= PULSE_START_B && nowDateTime.millisInSecond < PULSE_END_B) );

#if TESTING_ISR
  digitalWrite(LEDPIN, ((isrCount % 2) == 0) ? HIGH : LOW);
#endif /* TESTING_ISR */

  if ( (dtMinute(&nowDateTime) % SEISMIC_INTERVAL) ||
       (msNow > SEISMIC_TOTAL_TIME) ){
    // not in animation phase - proceed normally

#if TESTING_SEISMIC
    if (inDebug) {
      DEBUGSERIAL('N');
      debugLong(dtMinute(&nowDateTime));
      DEBUGSERIAL(' ');
      debugLong(msNow);
    }
#endif /* TESTING_SEISMIC */

    digitalWrite(LEDPIN, inNormalPulse ? HIGH : LOW);
  } else {
    // EARTHQUAKE!

#if TESTING_SEISMIC
    if (inDebug) {
      DEBUGSERIAL('Y');
      debugLong(dtMinute(&nowDateTime));
      DEBUGSERIAL(' ');
      debugLong(msNow);
      DEBUGSERIAL(' ');
      debugLong(swaveOffset);
      DEBUGSERIAL(' ');
      debugLong(pwaveOffset);
      DEBUGSERIAL(' ');
    }
#endif /* TESTING_SEISMIC */
    
    if ( ((swaveOffset < msNow) && (msNow < swaveOffset + SEISMIC_DURATION_FULL)) ||
	       ((pwaveOffset < msNow) && (msNow < pwaveOffset + SEISMIC_DURATION_FULL)) ) {
      // animate!: turn on LED. Highest priority.

      // Calculate the brightness for each wave individually, then take the maximum
      // Default brightness is 0
      uint8_t swaveBrightness = seismicBrightness(swaveOffset, msNow);
      uint8_t pwaveBrightness = seismicBrightness(pwaveOffset, msNow);

      analogWrite(LEDPIN, (swaveBrightness > pwaveBrightness) ? swaveBrightness : pwaveBrightness);       
    } else if ( ((swaveOffset - CLEAR_WINDOW < msNow) && (msNow < swaveOffset + CLEAR_WINDOW + SEISMIC_DURATION_FULL)) ||
		            ((pwaveOffset - CLEAR_WINDOW < msNow) && (msNow < pwaveOffset + CLEAR_WINDOW + SEISMIC_DURATION_FULL)) ) {
      
      // in clearout window: turn off LED
      digitalWrite(LEDPIN, LOW);

    } else {

      // it's animation time, but this position isn't currently activated: blink dimly
      // note that analogWrite 0 is special-cased to digitalWrite LOW already
      analogWrite(LEDPIN, inNormalPulse ? DIM_INTENSITY : LOW);
    }
  }
  
  if (nowDateTime.secondInDay >= NIGHT_END || nowDateTime.secondInDay < DUSK_START) {
    return stateDaytime;
  } else {
    return stateNightEvent;
  }
}

// Can't drain all serial input on one loop -- blocks too long, miss cycles through updateBlink
void serialLoop(void)
{
  if (TinySerial::available()) {
    char c = TinySerial::read();
    if (c == '$') {
      inSentence = true;
      bufpos = 0;
    }
    
    if (inSentence) {
      buffer[bufpos] = c;
      bufpos++;
      if (bufpos >= BUFLEN) {
        inSentence = false;
      }
    }

    if (c == '*') {
      buffer[bufpos] = 0;
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
#define RMC_SPEED_START 46
#define RMC_DAY_TENS     0
#define RMC_DAY_ONES     1
#define RMC_MONTH_TENS   2
#define RMC_MONTH_ONES   3
#define RMC_DATE_LAST    5

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
#if TESTING_NMEA_PARSE
  for (int i = 0; i < buflen; i++) {
    DEBUGSERIAL(buffer[i]);
  }
  DEBUGSERIAL('\r');
  DEBUGSERIAL('\n');
#endif /* TESTING_NMEA_PARSE */

  if (buflen < RMC_MIN_LEN) {
    return;
  }

  // Atomically clear count of pulses since fix
  uint8_t oldSREG = SREG;
  cli();
  pulsesSinceFix = 0;
  SREG = oldSREG;
  
  fupd->fixReceiveMs = millis();

  unsigned long secondInMinute = (buffer[RMC_SECOND_TENS] - '0') * 10 + (buffer[RMC_SECOND_ONES] - '0');
  unsigned long minuteInHour = (buffer[RMC_MINUTE_TENS] - '0') * 10 + (buffer[RMC_MINUTE_ONES] - '0');
  unsigned long hourInDay = (buffer[RMC_HOUR_TENS] - '0') * 10 + (buffer[RMC_HOUR_ONES] - '0');
  fupd->fixDateTime.secondInDay = secondInMinute + 60 * (minuteInHour + (60 * hourInDay));

  fupd->fixValid = (buffer[RMC_VALIDITY] == 'A');

  uint8_t dateStart = RMC_SPEED_START;
  for (uint8_t field = 1; field <= 2; field++) {
    while (buffer[dateStart] != ',') {
      dateStart++;
      if ((dateStart + RMC_DATE_LAST) >= buflen) {
        fupd->fixValid = 0;
        return;
      }
    }
    dateStart++;
  }
  
  unsigned int dayInMonth = (buffer[dateStart + RMC_DAY_TENS] - '0') * 10 + (buffer[dateStart + RMC_DAY_ONES] - '0') - 1;
  unsigned int monthInYear = (buffer[dateStart + RMC_MONTH_TENS] - '0') * 10 + (buffer[dateStart + RMC_MONTH_ONES] - '0') - 1;
  fupd->fixDateTime.dayInYear = monthFirstDate[monthInYear] + dayInMonth;

  fupd->fixLongiUMin = ((unsigned long) (buffer[RMC_LONGIMIN_TENS] - '0')) * 10000000L
    + ((unsigned long) (buffer[RMC_LONGIMIN_ONES] - '0')) * 1000000L
    + ((unsigned long) (buffer[RMC_LONGIMIN_FIRST] - '0')) * 100000L
    + ((unsigned long) (buffer[RMC_LONGIMIN_SECOND] - '0')) * 10000L
    + ((unsigned long) (buffer[RMC_LONGIMIN_THIRD] - '0')) * 1000L
    + ((unsigned long) (buffer[RMC_LONGIMIN_FOURTH] - '0')) * 100L
    + ((unsigned long) (buffer[RMC_LONGIMIN_FIFTH] - '0')) * 10L;

#if TESTING_NMEA_PARSE
  debugLong(fupd->fixReceiveMs);
  DEBUGSERIAL(' ');

  debugLong(fupd->fixDateTime.secondInDay);
  DEBUGSERIAL(' ');

  debugLong(dateStart);
  DEBUGSERIAL(' ');

  debugLong(fupd->fixDateTime.dayInYear);
  DEBUGSERIAL(' ');

  debugLong(fupd->fixLongiUMin);

  DEBUGSERIAL('\r');
  DEBUGSERIAL('\n');
#endif /* TESTING_NMEA_PARSE */
}

#define SECONDS_IN_DAY 86400L
void estimateNow(struct datetime_struct *nowDateTime)
{
  unsigned long now = millis();

  uint8_t oldSREG = SREG;
  cli();
  unsigned long myLastPulseMs = lastPulseMs, myPulsesSinceFix = pulsesSinceFix;
  SREG = oldSREG;
  
  unsigned long extraSeconds = 
    (now < myLastPulseMs + MAX_PULSE_WAIT) ? 
    myPulsesSinceFix : 
    ((now - recentFix.fixReceiveMs) / 1000L);

  *nowDateTime = recentFix.fixDateTime;

  nowDateTime->millisInSecond = (unsigned int) ((now - myLastPulseMs) % 1000L);

  nowDateTime->secondInDay += extraSeconds;
  while (nowDateTime->secondInDay >= SECONDS_IN_DAY) {
    nowDateTime->secondInDay -= SECONDS_IN_DAY;
    nowDateTime->dayInYear++;
  }

#if TESTING_TIME
    if (inDebug) {
      DEBUGSERIAL((now < myLastPulseMs + MAX_PULSE_WAIT) ? 'S' : 'U');
      DEBUGSERIAL(' ');
      debugLong(now);
      DEBUGSERIAL(' ');
      debugLong(myLastPulseMs);
      DEBUGSERIAL(' ');
      debugLong(nowDateTime->millisInSecond);
/*
      DEBUGSERIAL(' ');
      debugLong(extraSeconds);
      DEBUGSERIAL(' ');
      debugLong(recentFix.fixDateTime.secondInDay);
      DEBUGSERIAL(' ');
      debugLong(nowDateTime->secondInDay);
      DEBUGSERIAL(' ');
      debugLong(DUSK_START);
      DEBUGSERIAL(' ');
      debugLong(NIGHT_END);
      DEBUGSERIAL(' ');
      debugLong(nowDateTime->dayInYear);
      DEBUGSERIAL(' ');
      debugLong(EVENT_START_DAY);
*/
      DEBUGSERIAL('\r');
      DEBUGSERIAL('\n');
    }
#endif    
}

inline uint8_t dtSecond(const datetime_struct *dt) { return (uint8_t) (dt->secondInDay % 60L); }
inline uint8_t dtMinute(const datetime_struct *dt) { return (uint8_t) ((dt->secondInDay / 60L) % 60L); }
inline uint8_t dtHour(const datetime_struct *dt) { return (uint8_t) (dt->secondInDay / 3600L); }

// Safe to access directly in an ISR
extern volatile unsigned long timer0_millis;

void ppsIsr(void)
{
  lastPulseMs = timer0_millis;
  pulsesSinceFix++;

#if TESTING_ISR
  isrCount++;
#endif
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
    TinySerial::write(MSG[i]);
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

