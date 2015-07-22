#include<SoftwareSerial.h>

// GPS Setup
#define rxGPS 8
#define txGPS 9
SoftwareSerial serialGPS = SoftwareSerial(rxGPS, txGPS);
String stringGPS = "";

void setup() {
  pinMode(rxGPS, INPUT);
  pinMode(txGPS, OUTPUT);


  Serial.begin(9600);
  Serial.println("Started");

  // GPS Setup
  serialGPS.begin(9600);
  digitalWrite(txGPS,HIGH);

  // Cut first gibberish
  //while(serialGPS.available())
  //  if (serialGPS.read() == '\r')
  //    break;
}



void loop()
{
  //String s = checkGPS();

  stringGPS = "";
  while (serialGPS.available())
  {
      char c = serialGPS.read();
      stringGPS += c;
  }

  Serial.print(c);
  
  //Serial.println(s);
  //if(s && s.substring(0, 6) == "$GPGGA")
  //{
  //  Serial.println(s);
  //}
}

// Check GPS and returns string if full line recorded, else false
String checkGPS()
{
  String line = "";
  
  while (serialGPS.available())
  {
    char c = serialGPS.read();
    line += c;

    if(c == '\n'){
      break;
    }
  }
}
