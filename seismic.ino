// all the blinking!
// so far written for once per minute
// so far only implements swave
// check to make sure it's longiMinutes - x0, not x0 - longiMinutes
// notes on calculations are in seismic_notes.txt

//recentFix.fixLongiUMin // longitude in micro longitude minutes (longi. minutes x 10^6)

#define DIM_INTENSITY 32 // intensity of dim LEDs while the animation is happening

#define X0 10845360L // longitude minutes of first post, times 1,000,000
#define LONG_INTERVAL 7680 //longitude interval between any two stakes, in longitude minutes * 1,000,000

#define SWAVE_INTERVAL 50 // milliseconds between posts for shear wave
#define PWAVE_INTERVAL 30 // milliseconds between posts for pressure wave;

#define CLEAR_WINDOW 250 // clear out a window of 500 ms around the current animated LED
#define SEISMIC_DURATION 150 // how long LED stays on when lit during the seismic animation

// am i going in the right direction? (should be positive...)
unsigned long swaveOffset = (( recentFix.fixLongiUMin - X0 ) / LONG_INTERVAL ) * SWAVE_INTERVAL;
unsigned long pwaveOffset = (( recentFix.fixLongiUMin - X0 ) / LONG_INTERVAL ) * PWAVE_INTERVAL;

// check if this works as intended
unsigned long msNow = ( dtSecond(nowDateTime) * 1000 ) + millisInSecond;

if (msNow > 15000) {
  // not in animation phase - proceed normally

   digitalWrite(LEDPIN, ((millisInSecond >= PULSE_START) && (millisInSecond < (PULSE_START + PULSE_DUR))) ? HIGH : LOW);
 
 } else {
  // EARTHQUAKE!

  // change around the order: animation-on gets the highest priority
  // we are in the 0-15000 range here.

  if ( ((swaveOffset < msNow) && (msNow < swaveOffset + SEISMIC_DURATION)) ||
       ((pwaveOffset < msNow) && (msNow < pwaveOffset + SEISMIC_DURATION)) ) {

    // animate!: turn on LED. Highest priority.
    digitalWrite(LEDPIN, HIGH); 

  } else if ( ((swaveOffset - CLEAR_WINDOW < msNow) && (msNow < swaveOffset + CLEAR_WINDOW + SEISMIC_DURATION)) ||
	      ((pwaveOffset - CLEAR_WINDOW < msNow) && (msNow < pwaveOffset + CLEAR_WINDOW + SEISMIC_DURATION)) ) {
    
    // in clearout window: turn off LED
    digitalWrite(LEDPIN, LOW);

  } else {
    // it's animation time, but this position isn't currently activated: blink dimly

    analogWrite(LEDPIN, ((millisInSecond >= PULSE_START) && (millisInSecond < (PULSE_START + PULSE_DUR))) ? DIM_INTENSITY : LOW);
    
  }
 }
