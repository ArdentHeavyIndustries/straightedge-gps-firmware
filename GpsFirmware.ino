#include<SoftwareSerial.h>

// INCUDE LINK
#include<TinyGPS.h>

// GPS Setup
#define rxGPS 7
#define txGPS 2


int count = 0;
SoftwareSerial serialGps = SoftwareSerial(rxGPS, txGPS);
TinyGPS gps;
String line = "";

void setup() {
  pinMode(rxGPS, INPUT);
  pinMode(txGPS, OUTPUT);

  int baudRate = 9600;
  
  Serial.begin(baudRate);
  Serial.println("Started");

  // GPS Setup
  serialGps.begin(baudRate);
}



void loop()
{ 
  while (serialGps.available())
  {
    if (gps.encode(serialGps.read())){
      long lat, lon;
      unsigned long fix_age, time, date, speed, course;
      unsigned long chars;
      unsigned short sentences, failed_checksum;
 
      // retrieves +/- lat/long in 100000ths of a degree
      gps.get_position(&lat, &lon, &fix_age);

      Serial.println("lat: " + String(lat));
 
      // time in hhmmsscc, date in ddmmyy
      gps.get_datetime(&date, &time, &fix_age);
 
      // returns speed in 100ths of a knot
      speed = gps.speed();
 
      // course in 100ths of a degree
      course = gps.course(); 
    }
  }
}
