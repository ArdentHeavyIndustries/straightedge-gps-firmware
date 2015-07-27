#include<SoftwareSerial.h>
#include "TinyGPS.h"

// GPS Setup
#define rxGPS 7
#define txGPS 2
SoftwareSerial serialGPS = SoftwareSerial(rxGPS, txGPS);

TinyGPS gps;

void setup() {
  pinMode(rxGPS, INPUT);
  pinMode(txGPS, OUTPUT);
  
  int rate = 9600;
  
  // GPS Setup
  Serial.begin(rate);
  serialGPS.begin(rate);
  Serial.println("Started");
}



void loop()
{
  while (serialGPS.available())
  {
    int c = serialGPS.read();
    if (gps.encode(c))
    {
      printInfo();
    }
  }
}

void printDebugInfo(){
  long lat, lon;
  unsigned long fix_age, time, date, speed, course;
  unsigned long chars;
  unsigned short sentences, failed_checksum;
 
  // retrieves +/- lat/long in 100000ths of a degree
  gps.get_position(&lat, &lon, &fix_age);
 
  // time in hhmmsscc, date in ddmmyy
  gps.get_datetime(&date, &time, &fix_age);
 
  // returns speed in 100ths of a knot
  speed = gps.speed();
 
  // course in 100ths of a degree
  course = gps.course();

  Serial.println("Latitude: " + String(lat));
  Serial.println("Longitude: " + String(lon));

  Serial.println("Date: " + String(date));

  unsigned long charss;
  unsigned short good_sentences;
  unsigned short failed_cs;
  gps.stats(&chars, &good_sentences, &failed_cs);

  Serial.println("chars: " + String(charss));
  Serial.println("good_sentences: " + String(good_sentences));
  Serial.println("failed_cs: " + String(failed_cs));

  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned long fix_ages;
 
  gps.crack_datetime(&year, &month, &day,
    &hour, &minute, &second, &hundredths, &fix_ages);
  Serial.println("Year:" + String(year));
  Serial.println("month:" + String(month));
  Serial.println("day:" + String(day));
  Serial.println("hour:" + String(hour));
  Serial.println("minute:" + String(minute));
  Serial.println("second:" + String(second));


  Serial.println("");
}


