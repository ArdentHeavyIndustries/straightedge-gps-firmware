#ifndef TRINKET_STRAIGHTEDGE_H
#define TRINKET_STRAIGHTEDGE_H 1

#define ARDUINO_UNO 0

/* Times and dates for Burning Man 2015
/* N.B. 02:49 August 31st UTC = Sunday nightfall
 * State machine requires dusk+night don't cross midnight UTC
 * 
 * 1% clock skew => 15 minutes in 24 hours
 * GPS cold-start fix in <30 minutes
 * Nominal 1 hour "dusk" period allows for drift and GPS fix before nightfall.
 * That is, if on-board clock is slow, we enter dusk at actual time later than
 * dusk start, and still need time to get a GPS fix and avoid weird blinking.
 */

# define DUSK_START       6540L /* 18:49 PDT = 01:49 UTC */
# define NIGHT_START     10140L /* 19:49 PDT = 02:49 UTC */
# define EVENT_START_SEC 25200L /* 00:00 PDT = 07:00 UTC */
# define NIGHT_END       46800L /* 06:00 PDT = 13:00 UTC */
/* 10 hours and 11 minutes of blinky each day */
# define EVENT_START_DAY 242 /* August 31st in UTC-land */

// # define DUSK_START      (0L*3600L + 1L*60L)
// # define NIGHT_START     (0L*3600L + 2L*60L)
// # define EVENT_START_SEC (0L*3600L + 3L*60L)
// # define NIGHT_END       (23L*3600L + 59L*60L)
/* Tuesday Aug 11 = 222 */
// # define EVENT_START_DAY 229

/* Blink configuration */
#define PULSE_FREQUENCY         2  // Pulse when seconds % PULSE_FREQUENCY == 0
#define PULSE_START_A         200L
#define PULSE_START_B         400L
#define PULSE_DUR              40L
#define PULSE_END_A          (PULSE_START_A + PULSE_DUR)
#define PULSE_END_B          (PULSE_START_B + PULSE_DUR)
#define UNSYNCH_PRE_PERIOD   3000L
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
#define MAX_PULSE_WAIT   1150

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
// stake   0 = -119.180756º = 119º 10.84536' W = 119º 10' 50.72"
// stake   1 = -119.180884º = 119º 10.85304' W
// stake 279 = -119.216469º = 119º 12.98814' W = 119º 12' 59.29"

#define X0  10845360L // longitude minutes of first post, times 1,000,000
#define LONG_INTERVAL 7680 //longitude interval between any two stakes, in longitude minutes * 1,000,000

#define SWAVE_INTERVAL 50 // milliseconds between posts for shear wave
#define PWAVE_INTERVAL 30 // milliseconds between posts for pressure wave;

#define CLEAR_WINDOW 250 // clear out a window of 500 ms around the current animated LED
#define SEISMIC_DURATION_BRIGHT  64 // how long LED stays full brightness during the seismic animation
#define SEISMIC_DURATION_FADE   192 // how long LED fades from full brightness during the seismic animation
#define SEISMIC_DURATION_FULL   (SEISMIC_DURATION_BRIGHT + SEISMIC_DURATION_FADE)

#define DIM_INTENSITY 32 // intensity of dim LEDs while the animation is happening

#define SEISMIC_INTERVAL 10 // Minutes between seismic waves: happen when (minute-in-hour % SEISMIC_INTERVAL == 0)

#define SEISMIC_TOTAL_TIME (SWAVE_INTERVAL * 300) // milliseconds of seismic wave mode

// FOR TESTING
// Longitude of Nick & Liana's house: 122º 25.320000'
// Longitude of Cesar Chavez x US-101 122º 24.320000'
// #define X0 25320000L
// #define X0 24320000L

// Longitude of Octagon 122º 24.393720' (= -122.406217º) 
// Nearby, 122º 23.893720 (= -122.398229º) is 4th & Bryant
// Nearby, 122º 23.393720 (= -122.389895º) is King and Embarcadero -- other end would be Gough & McAllister (or with N/S to beach at maritime park)
// #define X0 24393720L
// #define X0 23893720L
// #define X0 23393720L

#endif

