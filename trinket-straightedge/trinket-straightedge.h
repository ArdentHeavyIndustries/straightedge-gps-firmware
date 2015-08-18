#ifndef TRINKET_STRAIGHTEDGE_H
#define TRINKET_STRAIGHTEDGE_H 1

#define TESTING 0
#define ARDUINO_UNO 0

/* Times and dates for Burning Man 2015
/* N.B. 02:49 August 31st UTC = Sunday nightfall
 * State machine requires dusk+night don't cross midnight UTC
 * 
 * 1% clock skew => 15 minutes in 24 hours
 * GPS cold-start fix in <30 minutes
 * Nominal 45 minute "dusk" period allows 15 minute fast drift 
 * (i.e., 30 minutes real dusk period) and GPS fix before night.
 */
#if 0
# define DUSK_START       7740L /* 19:04 PDT = 02:04 UTC */
# define NIGHT_START     10140L /* 19:49 PDT = 02:49 UTC */
# define EVENT_START_SEC 25200L /* 00:00 PDT = 07:00 UTC */
# define NIGHT_END       46800L /* 06:00 PDT = 13:00 UTC */
/* 10 hours and 11 minutes of blinky each day */
# define EVENT_START_DAY 242 /* August 31st in UTC-land */
#else
# define DUSK_START      (0L*3600L + 1L*60L)
# define NIGHT_START     (0L*3600L + 2L*60L)
# define EVENT_START_SEC (0L*3600L + 3L*60L)
# define NIGHT_END       (23L*3600L + 59L*60L)
/* Tuesday Aug 11 = 222 */
# define EVENT_START_DAY 225
#endif

/* Blink configuration */
#define PULSE_FREQUENCY         2  // Pulse when seconds % PULSE_FREQUENCY == 0
#define PULSE_START_A         100L
#define PULSE_START_B         300L
#define PULSE_DUR              40L
#define PULSE_END_A          (PULSE_START_A + PULSE_DUR)
#define PULSE_END_B          (PULSE_START_B + PULSE_DUR)
#define UNSYNCH_PRE_PERIOD   2500L
#define PRE_PULSE_DUR          20L
#define UNSYNCH_START_PERIOD 2000L
#define START_PULSE_DUR        30L

/* Hardware configuration:
 * PPSPIN must be 2 for INT0 external interrupt
 */
#define RXPIN     0 /* GPS->trinket serial */
#define ENABLEPIN 1 /* GPS enable/on = high, disable/off = low */
#define PPSPIN    2 /* Pulse-per-second GPS->trinket */
#define TXPIN     3 /* trinket->GPS serial */
#define LEDPIN    4 /* LED control, high = on */

#if ARDUINO_UNO
/* UNO change to free up pins 0 & 1 (hdwr serial) and put LED on a PWM pin*/
# define RXPIN     5
# define LEDPIN    6
# define ENABLEPIN 7
#endif

/* Max milliseconds to wait for PPS pulse
 * before falling back on internal clock
 */
#define MAX_PULSE_WAIT   1100

/* Data structure for date & time, always in UTC */
struct datetime_struct {
  /* Milliseconds into the second, 0 - 999 */
  unsigned int millisInSecond;
  /* Seconds since midnight UTC, 0 - 86399 */
  long secondInDay;
  /* Days since 1 January, in UTC */
  int dayInYear;
};

/* GPS fix data: UTC time from fix, along with internal clock time it was received */
struct fix_struct {
  struct datetime_struct fixDateTime;
  /* Microminutes of longitude */
  unsigned long fixLongiUMin;
  /* Onboard clock ms when fix was received */
  unsigned long fixReceiveMs;
  /* True iff the fix is valid A not V */
  uint8_t fixValid;
};

/* States of the state machine */
enum state_enum {
  stateStartup,    /* A -- GPS on, await first fix */
  stateDaytime,    /* B -- GPS off, wait for dusk according to internal clock */
  stateDusk,       /* C -- GPS on, obtain fix and wait for nightfall in GPS time */
  stateNightPre,   /* D -- GPS off, blink on internal clock (unsynchronized) */
  stateNightStart, /* E -- GPS on, but blink on internal clock until event start according to GPS time */
  stateNightEvent  /* F -- GPS on, blink synchronzied with GPS */
};

/* Two kinds of functions for the state machine:
 *
 * xxxLoop functions run repeatedly in the state, does "work" for the
 * state (monitor GPS, blink LED, etc.) and returns the new state
 * (which can be, and usually is, the same as the current state).
 *
 * xxxEnter functions run once when switching into new state.
 */

/* Dispatch function for per-state loop functions
 * Always calls the serial buffer handler serialLoop
 */
enum state_enum stateLoop(enum state_enum currState);

/* Serial buffer handler called regardless of state */
void serialLoop(void);

/* Per-state loop functions */
enum state_enum startupLoop(void);
enum state_enum daytimeLoop(void);
enum state_enum duskLoop(void);
enum state_enum nightPreLoop(void);
enum state_enum nightStartLoop(void);
enum state_enum nightEventLoop(void);

/* Dispatch function for per-state entry functions
 * Always switches off LED
 */
void enterState(enum state_enum nextState);

/* Per-state functions for entering a new state
 * Turn GPS on or off as appropriate
 * Invalidate cached GPS fix when we want a new one
 */
void startupEnter(void);
void daytimeEnter(void);
void duskEnter(void);
void nightPreEnter(void);
void nightStartEnter(void);
void nightEventEnter(void);

void updateFixFromNmea(struct fix_struct *fupd, const char *buffer, int buflen);

// seismic animation presets
#define X0  10845360L // longitude minutes of first post, times 1,000,000
#define LONG_INTERVAL 7680 //longitude interval between any two stakes, in longitude minutes * 1,000,000

#define SWAVE_INTERVAL 50 // milliseconds between posts for shear wave
#define PWAVE_INTERVAL 30 // milliseconds between posts for pressure wave;

#define CLEAR_WINDOW 250 // clear out a window of 500 ms around the current animated LED
#define SEISMIC_DURATION 150 // how long LED stays on when lit during the seismic animation

#define DIM_INTENSITY 32 // intensity of dim LEDs while the animation is happening

#define SEISMIC_INTERVAL 5 // Minutes between seismic waves: happen when (minute-in-hour % SEISMIC_INTERVAL == 0)

#define SEISMIC_TOTAL_TIME (SWAVE_INTERVAL * 300) // milliseconds of seismic wave mode

#endif

