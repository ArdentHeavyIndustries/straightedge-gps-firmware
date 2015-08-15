// all the blinking!
// so far only implements swave
// check to make sure it's longiMinutes - x0, not x0 - longiMinutes
// notes on calculations are in seismic_notes.txt

// unsigned long longiMinutes // from Nick
// is longitude minutes times 10^6

unsigned int swaveInterval = 50; //milliseconds between posts for shear wave
unsigned int pwaveInterval = 30; //milliseconds between posts for pressure wave;

unsigned long x0 = 10845360; // longitude minutes of first post, times 1,000,000
unsigned int longiInterval = 7680; //longitude interval between any two stakes, in longitude minutes * 1,000,000

// am i going in the right direction? (should be positive...)
unsigned int swaveOffset = (( longiMinutes - x0 ) / longiInterval ) * swaveInterval;
unsigned int pwaveOffset = (( longiMinutes - x0 ) / longiInterval ) * pwaveInterval;
  
unsigned int clearWindow = 250; // clear out a window of 500 ms 'around' the animated LED

// check if this works as intended
unsigned long msNow = ( dtSecond(nowDateTime) * 1000 ) + millisInSecond;

if (msNow > 15000) {
  // not in animation phase - proceed normally

   digitalWrite(LEDPIN, ((millisInSecond >= PULSE_START) && (millisInSecond < (PULSE_START + PULSE_DUR))) ? HIGH : LOW);
 
 } else {
  // EARTHQUAKE!

  if ((msNow < swaveOffset - clearWindow) || (msNow > swaveOffset + clearWindow)) {
    // in animation period, but not currently activated: blink dimly

    analogWrite(LEDPIN, ((millisInSecond >= PULSE_START) && (millisInSecond < (PULSE_START + PULSE_DUR))) ? 32 : LOW);

  } else if ( (msNow < swaveOffset) || (msNow > swaveOffset + 150) ) {
    // in clearout window: turn off LED
    digitalWrite(LEDPIN, LOW);

  } else {
    // swaveOffset < msNow < swaveOffset + 150
    // animate!: turn on LED
    digitalWrite(LEDPIN, HIGH);
  }
 }
