  /******************************************************************************

******************************************************************************/

#include <SPI.h>
#include <SD.h>
#include <TinyGPS++.h>
#include<Wire.h>
#define ARDUINO_USD_CS 10 // uSD card CS pin (pin 10 on SparkFun GPS Logger Shield)

const int MPU_addr=0x68; // I2C address of the MPU-6050
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
const int ledPin = 13;
const int buttonPin = 3;
int logState = LOW;
int buttonState = LOW;
int lastButtonState;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
int lightState = LOW;
/////////////////////////
// Log File Defintions //
/////////////////////////
// Keep in mind, the SD library has max file name lengths of 8.3 - 8 char prefix,
// and a 3 char suffix.
// Our log files are called "gpslogXX.csv, so "gpslog99.csv" is our max file.
#define LOG_FILE_PREFIX "gpslog" // Name of the log file.
#define MAX_LOG_FILES 100 // Number of log files that can be made
#define LOG_FILE_SUFFIX "csv" // Suffix of the log file
#define comma F(",")
char logFileName[13]; // Char string to store the log file name
// Data to be logged:
//#define LOG_COLUMN_COUNT 15
#define COLUMN_NAMES F("millis, AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ, date, time, longitude, latitude, altitude, speed, course, satellites")
//char * log_col_names[LOG_COLUMN_COUNT] = {
//  "longitude", "latitude", "altitude", "speed", "course", "date", "time", "satellites",
//  "AcX", "AcY", "AcZ", "Tmp", "GyX", "GyY", "GyZ"
//}; // log_col_names is printed at the top of the file.

//////////////////////
// Log Rate Control //
//////////////////////
#define LOG_RATE 10 // Log every seconds
unsigned long lastLog = 0; // Global var to keep of last time we logged

/////////////////////////
// TinyGPS Definitions //
/////////////////////////
TinyGPSPlus tinyGPS; // tinyGPSPlus object to be used throughout
#define GPS_BAUD 9600 // GPS module's default baud rate

/////////////////////////////////
// GPS Serial Port Definitions //
/////////////////////////////////
// If you're using an Arduino Uno, Mega, RedBoard, or any board that uses the
// 0/1 UART for programming/Serial monitor-ing, use SoftwareSerial:
#include <SoftwareSerial.h>
#define ARDUINO_GPS_RX 9 // GPS TX, Arduino RX pin
#define ARDUINO_GPS_TX 8 // GPS RX, Arduino TX pin
SoftwareSerial ssGPS(ARDUINO_GPS_TX, ARDUINO_GPS_RX); // Create a SoftwareSerial

// Set gpsPort to either ssGPS if using SoftwareSerial or Serial1 if using an
// Arduino with a dedicated hardware serial port
#define gpsPort ssGPS  // Alternatively, use Serial1 on the Leonardo

// Define the serial monitor port. On the Uno, Mega, and Leonardo this is 'Serial'
//  on other boards this may be 'SerialUSB'
#define SerialMonitor Serial

void setup()
{
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
  SerialMonitor.begin(9600);
  gpsPort.begin(GPS_BAUD);
  SerialMonitor.println(F("Setting up SD card."));
  // see if the card is present and can be initialized:
  if (!SD.begin(ARDUINO_USD_CS))
  {
    SerialMonitor.println(F("Error initializing SD card."));
  }
}

void checkLogState(){
  int buttonReading = digitalRead(buttonPin);
  if(buttonReading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if(debounceDelay + lastDebounceTime < millis()){
    if (buttonReading != buttonState) {
      buttonState = buttonReading;
      if(buttonState == HIGH){
        logState = !logState;
      }
    }
  }
  digitalWrite(ledPin, logState);
  Serial.print(logState);
  lastButtonState = buttonReading;
}

byte logGPSData()
{
  File logFile = SD.open(logFileName, FILE_WRITE); // Open the log file
  if (logFile)
  { // Print longitude, latitude, altitude (in feet), speed (in mph), course
    // in (degrees), date, time, and number of satellites.
    logFile.print(tinyGPS.date.value());
    logFile.print(comma);
    logFile.print(tinyGPS.time.value());
    logFile.print(comma);
    logFile.print(tinyGPS.location.lng(), 6);
    logFile.print(comma);
    logFile.print(tinyGPS.location.lat(), 6);
    logFile.print(comma);
    logFile.print(tinyGPS.altitude.feet(), 1);
    logFile.print(comma);
    logFile.print(tinyGPS.speed.mph(), 1);
    logFile.print(comma);
    logFile.print(tinyGPS.course.deg(), 1);
    logFile.print(comma);
    logFile.print(tinyGPS.satellites.value());
    logFile.print(comma);
    logFile.println();
    logFile.close();
    return 1; // Return success
  }
  return 0; // If we failed to open the file, return fail
}

void logIMUData(){
    Wire.beginTransmission(MPU_addr);
    Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H)
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_addr,14,true);  // request a total of 14 registers
    File logFile = SD.open(logFileName, FILE_WRITE);
    logFile.print(millis());
    logFile.print(comma);
    logFile.print(Wire.read()<<8|Wire.read());
    logFile.print(comma);
    logFile.print(Wire.read()<<8|Wire.read());
    logFile.print(comma);
    logFile.print(Wire.read()<<8|Wire.read());
    logFile.print(comma);
    logFile.print((Wire.read()<<8|Wire.read())/340.00+36.5);
    logFile.print(comma);
    logFile.print(Wire.read()<<8|Wire.read());
    logFile.print(comma);
    logFile.print(Wire.read()<<8|Wire.read());
    logFile.print(comma);
    logFile.print(Wire.read()<<8|Wire.read());
    logFile.print(comma);
    logFile.close();
}

// updateFileName() - Looks through the log files already present on a card,
// and creates a new file with an incremented file index.
void makeNewFile()
{
  int i = 0;
  for (; i < MAX_LOG_FILES; i++)
  {
    memset(logFileName, 0, strlen(logFileName)); // Clear logFileName string
    // Set logFileName to "gpslogXX.csv":
    sprintf(logFileName, "%s%d.%s", LOG_FILE_PREFIX, i, LOG_FILE_SUFFIX);
    if (!SD.exists(logFileName)) // If a file doesn't exist
    {
      break; // Break out of this loop. We found our index
    }
    else // Otherwise:
    {
      SerialMonitor.print(logFileName);
      SerialMonitor.println(F(" exists")); // Print a debug statement
    }
  }
  SerialMonitor.print(F("File name: "));
  SerialMonitor.println(logFileName); // Debug print the file name
  File logFile = SD.open(logFileName, FILE_WRITE);
  logFile.println(COLUMN_NAMES);
  logFile.close();
}

void feedGPS(){
  while(gpsPort.available()){
    tinyGPS.encode(gpsPort.read());
  }
}

void loop()
{
  checkLogState();
  if(logState == HIGH){
    makeNewFile();
    while(logState == HIGH){
      if ((lastLog + LOG_RATE) <= millis())
      { // If it's been LOG_RATE milliseconds since the last log:
        lastLog = millis(); // Update the lastLog variable
        logIMUData(); 
        if (tinyGPS.location.isUpdated()) // If the GPS data is vaild
        {
          if (logGPSData()) // Log the GPS data
          {
            SerialMonitor.println(F("GPS logged.")); // Print a debug message
          }
          else // If we failed to log GPS
          { // Print an error, don't update lastLog   
            SerialMonitor.println(F("Failed to log new GPS data."));
          }
        }
        else // If GPS data isn't valid
        {
          File logFile = SD.open(logFileName, FILE_WRITE); // Open the log file
          for(int i = 0; i < 8; i++){
            logFile.print(comma);
          }
          logFile.println();
          logFile.close();
          // Print a debug message. Maybe we don't have enough satellites yet.
          SerialMonitor.print(F("No GPS data. Sats: "));
          SerialMonitor.println(tinyGPS.satellites.value());
        }
      }
      checkLogState();
      feedGPS();
    }
  }
  feedGPS();
}
//  // If we're not logging, continue to "feed" the tinyGPS object:
//  while (gpsPort.available())
//    tinyGPS.encode(gpsPort.read());
