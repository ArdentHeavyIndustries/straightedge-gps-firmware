#include<SoftwareSerial.h>

// Thanks: http://arduiniana.org/libraries/tinygps/
#include "TinyGPS.h"

// Thanks: http://www.pjrc.com/teensy/td_libs_Time.html
#include "Time.h"

// GPS Setup
#define rxGPS 7
#define txGPS 2
SoftwareSerial serialGPS = SoftwareSerial(rxGPS, txGPS);

// PST (atm)
int timezone = -7;

TinyGPS gps;

void setup() {
  pinMode(rxGPS, INPUT);
  pinMode(txGPS, OUTPUT);
  
  int rate = 9600;
  
  // GPS Setup
  Serial.begin(rate);
  serialGPS.begin(rate);
  Serial.println("Setting up...");

  // An initial sync
  waitForUpdate();
  syncToGps();

  Serial.println("Starting...");
}


void loop()
{
  waitForUpdate();

  tmElements_t arduinoTime;
  breakTime(now(), arduinoTime);

  Serial.print("Arduino Time: ");
  print_tmElements_t(arduinoTime);
  
  Serial.println("Drift: " + String(timeDrift()) + '\n');
}



void waitForUpdate(){
  bool updated = false;

  while(!updated){
    while (serialGPS.available())
    {
      char c = serialGPS.read();
      if (gps.encode(c))
      {
          updated = true;
          break;
      }
    }
  }
}

void syncToGps(){
  tmElements_t t = gpsTime();

  // It's annoying they implement the 1970 standard everywhere except for setTime!
  setTime(t.Hour, t.Minute, t.Second, t.Day, t.Month, t.Year - 30);

  Serial.println("Year: " + String(t.Year));
  Serial.println("Arduino Year: " + String(year()));
  Serial.println("Syncing to...");
  print_tmElements_t(t);
}

// The error between the internal clock time and the gps in seconds
long timeDrift(){
  tmElements_t t = gpsTime();
  return now() - makeTime(t);
}


void print_tmElements_t(tmElements_t t){
  Serial.print(String(t.Hour) + ":");
  Serial.print(String(t.Minute) + ":");
  Serial.print(String(t.Second));
  Serial.print(" - ");

  Serial.print(String(t.Day) + "/");
  Serial.print(String(t.Month) + "/");
  Serial.println(String(t.Year + 1970));
}

tmElements_t gpsTime(){
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned long fix_ages;
 
  gps.crack_datetime(&year, &month, &day,
    &hour, &minute, &second, &hundredths, &fix_ages);

  tmElements_t timeElement;
  timeElement.Second = second;
  timeElement.Minute = minute;
  timeElement.Hour = hour;
  timeElement.Day = day;
  timeElement.Month = month;

  // Time.h uses the unix offset from 1970 standard
  timeElement.Year = year - 1970;
  long timeInSeconds = makeTime(timeElement);
  
  // TODO: Make 60^2 a const
  int delta = timezone * 60*60;
  timeInSeconds += delta;

  tmElements_t adjustedTime;
  breakTime(timeInSeconds, adjustedTime);

  return adjustedTime;
}

// TODO: Delete this dev code
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


