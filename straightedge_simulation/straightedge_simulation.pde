public static final int height = 768;
public static final int width = 1024;

public static final int NSTAKES = 278;
public static final float FEET_PER_UNIT = 50.0;
public static final float STAKE_DIST = 50.0 / FEET_PER_UNIT;
public static final float VIEWER_HEIGHT = 5.0 / FEET_PER_UNIT;

public static final int SWAVE_INTERVAL = 50; // milliseconds between posts for shear wave
public static final int PWAVE_INTERVAL = 30; // pressure wave
public static final int CLEAR_WINDOW = 250;
public static final int SEISMIC_INTERVAL = 1; // Interval in minutes between seismic events
public static final int SEISMIC_DURATION_BRIGHT = 64; // Time in ms of full brightness
public static final int SEISMIC_DURATION_FADE = 192; // Time in ms of fade-out
public static final int SEISMIC_DURATION_FULL = SEISMIC_DURATION_BRIGHT + SEISMIC_DURATION_FADE;
public static final int DIM_INTENSITY = 128;
public static final int SEISMIC_TOTAL_TIME = (SWAVE_INTERVAL * 300);

/* From 2.7 miles away, looking head-on */
public static final float CAMERA_X = ((float) NSTAKES) / 2.0;
public static final float CAMERA_Y = ((float) NSTAKES) / 2.0;
public static final float CAMERA_Z = VIEWER_HEIGHT;
public static final float CENTER_X = ((float) NSTAKES) * 0.40;
public static final float CENTER_Y = 0.0;
public static final float CENTER_Z = 0.0;
public static final float BOX_SIZE = dist(CAMERA_X, CAMERA_Y, CAMERA_Z, CENTER_X, CENTER_Y, CENTER_Z) / ((float) NSTAKES);
public static final float FOVY = PI / 3.0;
public static final float ASPECT = ((float) width) / ((float) height);
public static final float CENTER_PLANE = ((float) height) / tan(FOVY / 2.0);
public static final float CLIP_NEAR = CENTER_PLANE / 20.0;
public static final float CLIP_FAR = CENTER_PLANE * 100.0;

void setup() {
  size(width, height, P3D);
}


void draw() {
  background(0);
  lights();

  camera(CAMERA_X, CAMERA_Y, CAMERA_Z, CENTER_X, CENTER_Y, CENTER_Z, 0.0, 0.0, 1.0);
  perspective(FOVY, ASPECT, CLIP_NEAR, CLIP_FAR);
  
  boolean normalPulse = inNormalPulse();
  int msNow = millis() % 60000; // milliseconds in minute
  
  if ((minute() % SEISMIC_INTERVAL != 0) || (msNow >= SEISMIC_TOTAL_TIME * 2)) {
    // Normal mode
    if (inNormalPulse()) {
      drawAll(255);
    }
  } else {
    // Earthquake
    
    for (int i = 0; i < 278; i++) {
//    unsigned long swaveOffset = PULSE_START_A + (( recentFix.fixLongiUMin - X0 ) / LONG_INTERVAL ) * SWAVE_INTERVAL;
//    unsigned long pwaveOffset = PULSE_START_A + (( recentFix.fixLongiUMin - X0 ) / LONG_INTERVAL ) * PWAVE_INTERVAL;
      int swaveOffset = PULSE_START_A + i * SWAVE_INTERVAL;
      int pwaveOffset = PULSE_START_A + i * PWAVE_INTERVAL;

      if (msNow > SEISMIC_TOTAL_TIME) {
        swaveOffset += SEISMIC_TOTAL_TIME;
        pwaveOffset += SEISMIC_TOTAL_TIME;
      }
      
      if ( ((swaveOffset < msNow) && (msNow < swaveOffset + SEISMIC_DURATION_FULL)) ||
                 ((pwaveOffset < msNow) && (msNow < pwaveOffset + SEISMIC_DURATION_FULL)) ) 
      {
        // animate!: turn on LED. Highest priority.

        // Calculate the brightness for each wave individually, then take the maximum
        // Default brightness is 0
        int swaveBrightness = seismicBrightness(swaveOffset, msNow);
        int pwaveBrightness = seismicBrightness(pwaveOffset, msNow);
        
        // analogWrite(LEDPIN, (swaveBrightness > pwaveBrightness) ? swaveBrightness : pwaveBrightness);       
        drawOne(i, (swaveBrightness > pwaveBrightness) ? swaveBrightness : pwaveBrightness);
      } else if ( ((swaveOffset - CLEAR_WINDOW < msNow) && (msNow < swaveOffset + CLEAR_WINDOW + SEISMIC_DURATION_FULL)) ||
                  ((pwaveOffset - CLEAR_WINDOW < msNow) && (msNow < pwaveOffset + CLEAR_WINDOW + SEISMIC_DURATION_FULL)) ) 
      {
      
      // in clearout window: turn off LED
      // digitalWrite(LEDPIN, LOW);
      } else if (inNormalPulse()) {
      drawAll(DIM_INTENSITY);
      }
    }
  }
}

void drawOne(int i, int f) {
  pushMatrix();
  translate(STAKE_DIST * i, 0.0, 0.0);
  fill(f);
  noStroke();
  box(BOX_SIZE);
  popMatrix();
}

void drawAll(int f) {
  for (int i = 0; i < 278; i++) {
    drawOne(i, f);
  }
}

int seismicBrightness(int waveOffset, int msNow)
{
  if ( (waveOffset < msNow) && (msNow < (waveOffset + SEISMIC_DURATION_FULL)) ) {
    if (msNow <= (waveOffset + SEISMIC_DURATION_BRIGHT)) {
      return 255;
    } else {
      int fadeTime = msNow - (waveOffset + SEISMIC_DURATION_BRIGHT);
      if (fadeTime <= SEISMIC_DURATION_FADE) {
        return (255 - (255 * fadeTime / SEISMIC_DURATION_FADE));
      }
    }
  }

  return 0;
}


boolean inNormalPulse() {
  int millisInSecond = millis() % 1000;

  return (((second() % PULSE_PERIOD) == 0) &&
          (((millisInSecond >= PULSE_START_A) && (millisInSecond < PULSE_END_A)) ||
           ((millisInSecond >= PULSE_START_B) && (millisInSecond < PULSE_END_B))));  
}

public static final int PULSE_PERIOD    = 2;
public static final int PULSE_DUR       = 40;
public static final int PULSE_START_A   = 200;
public static final int PULSE_START_B   = 400;
public static final int PULSE_END_A     = PULSE_START_A + PULSE_DUR;
public static final int PULSE_END_B     = PULSE_START_B + PULSE_DUR;
