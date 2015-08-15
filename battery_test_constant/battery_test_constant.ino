#include<SoftwareSerial.h>

// turn on the GPS in powersave mode.
// blink once per second forever.
// doesn't stop for daytime, doesn't read in the GPS.

// LED controller
#define led 4

// GPS Setup
#define enGPS 1
#define inGPS 2
#define rxGPS 0
#define txGPS 3

SoftwareSerial serialGPS = SoftwareSerial(rxGPS, txGPS);
 
uint8_t setupPSM[] = { 0xB5, 0x62, 0x06, 0x3B, 0x2C, 0x00, 0x01, 0x06, 0x00, 0x00, 0x00, 0x80, 0x02, 0x01, 0x10, 0x27, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2C, 0x01, 0x00, 0x00, 0x4F, 0xC1, 0x03, 0x00, 0x87, 0x02, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x64, 0x40, 0x01, 0x00, 0xD2, 0xEA } ; 

uint8_t activatePSM[] = { 0xB5, 0x62, 0x06, 0x11, 0x02, 0x00, 0x08, 0x01, 0x22, 0x92 } ;

void setup() {                
  // initialize the digital pin as an output.
  pinMode(led, OUTPUT);     
  pinMode(rxGPS, INPUT);
  pinMode(txGPS, OUTPUT);
  pinMode(enGPS, OUTPUT);
  pinMode(inGPS, INPUT);

  //digitalWrite(enGPS, HIGH);
  digitalWrite(enGPS, LOW);

  serialGPS.begin(9600);

  delay(1000);

  digitalWrite(enGPS, HIGH);
  
  int i = 0;
  while ( i < 20 ) {
    digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(40);
    digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
    delay(460);
    i++;
  }
	 
  setupPowerSaveMode();
}

// the loop routine runs over and over again forever:
void loop() {
  // blink the LED for 40 us every second
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(40);
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  delay(960);
}

// Send a byte array of UBX protocol to the GPS
void sendUBX(uint8_t *MSG, uint8_t len) {
  for(int i=0; i<len; i++) {
    serialGPS.write(MSG[i]);
  }
}

void setupPowerSaveMode() {
  sendUBX( setupPSM, 52 );
  sendUBX( activatePSM, 10 );
}
 

