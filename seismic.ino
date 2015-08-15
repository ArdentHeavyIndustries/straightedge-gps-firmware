// all the blinking!
// so far only implements swave
// check to make sure it's longiMinutes - x0, not x0 - longiMinutes

// unsigned long longiMinutes // from Nick
// is longitude minutes times 10^6

unsigned int swaveInterval = 50; //milliseconds between posts for shear wave
unsigned int pwaveInterval = 30; //milliseconds between posts for pressure wave;

unsigned long x0 = 10845360; // longitude minutes of first post, times 1,000,000
unsigned int longiInterval = 7680; //longitude interval between any two stakes, in longitude minutes * 1,000,000

/* https://docs.google.com/spreadsheets/d/1CT_LNAaMf-9v3a6IHFaSOiXDCC1DUdTH3UsWY7A_910/edit#gid=0
   first longitude = -119.180756 degrees = 119 degrees 10.84536 longitude minutes
   x_0 =  10.84536 * 1000000 = 10,845,360
   second longitude = -119.180884 degrees = 119 degrees 10.85304 longitude minutes
   longitude difference = -0.000128 degrees = -0.00768 longitude minutes */

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
    // 1. in animation period, but not currently activated: blink dimly
    // a. msNow < swaveOffset - clearWindow OR
    // b. msNow > swaveOffset + clearWindow

    analogWrite(LEDPIN, ((millisInSecond >= PULSE_START) && (millisInSecond < (PULSE_START + PULSE_DUR))) ? 32 : LOW);

  } else if ( (msNow < swaveOffset) || (msNow > swaveOffset + 150) ) {
    // 2. in clearout window: turn off LED
    // a. swaveOffset - clearWindow < msNow < swaveOffset OR
    // b. swaveOffset + 150 < msNow < swaveOffset + clearWindow
    digitalWrite(LEDPIN, LOW);

  } else {
    // 3. animate!: turn on LED
    // swaveOffset < msNow < swaveOffset + 150
    digitalWrite(LEDPIN, HIGH);
  }
 }
