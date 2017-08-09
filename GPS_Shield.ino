  /******************************************************************************
  CSV_Logger_TinyGPSPlus.ino
  Log GPS data to a CSV file on a uSD card
  By Jim Lindblom @ SparkFun Electronics
  February 9, 2016
  https://github.com/sparkfun/GPS_Shield

  This example uses SoftwareSerial to communicate with the GPS module on
  pins 8 and 9, then communicates over SPI to log that data to a uSD card.

  It uses the TinyGPS++ library to parse the NMEA strings sent by the GPS module,
  and prints interesting GPS information - comma separated - to a newly created
  file on the SD card.

  Resources:
  TinyGPS++ Library  - https://github.com/mikalhart/TinyGPSPlus/releases
  SD Library (Built-in)
  SoftwareSerial Library (Built-in)

  Development/hardware environment specifics:
  Arduino IDE 1.6.7
  GPS Logger Shield v2.0 - Make sure the UART switch is set to SW-UART
  Arduino Uno, RedBoard, Pro, Mega, etc.
******************************************************************************/

#include <SPI.h>
#include <SD.h>
#include <TinyGPS++.h>
#include<Wire.h>
#define ARDUINO_USD_CS 10 // uSD card CS pin (pin 10 on SparkFun GPS Logger Shield)

const int MPU_addr=0x68; // I2C address of the MPU-6050
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
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
#define COLUMN_NAMES F("longitude, latitude, altitude, speed, course, date, time, satellites, AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ")
//char * log_col_names[LOG_COLUMN_COUNT] = {
//  "longitude", "latitude", "altitude", "speed", "course", "date", "time", "satellites",
//  "AcX", "AcY", "AcZ", "Tmp", "GyX", "GyY", "GyZ"
//}; // log_col_names is printed at the top of the file.

//////////////////////
// Log Rate Control //
//////////////////////
#define LOG_RATE 1000 // Log every seconds
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
  SerialMonitor.begin(9600);
  gpsPort.begin(GPS_BAUD);
  SerialMonitor.println(F("Setting up SD card."));
  // see if the card is present and can be initialized:
  if (!SD.begin(ARDUINO_USD_CS))
  {
    SerialMonitor.println(F("Error initializing SD card."));
  }
  updateFileName(); // Each time we start, create a new file, increment the number
//  printHeader(); // Print a header at the top of the new file
  File logFile = SD.open(logFileName, FILE_WRITE);
  logFile.println(COLUMN_NAMES);
  logFile.close();
  
}

void loop()
{
  if ((lastLog + LOG_RATE) <= millis())
  { // If it's been LOG_RATE milliseconds since the last log:
    if (tinyGPS.location.isUpdated()) // If the GPS data is vaild
    {
      if (logGPSData()) // Log the GPS data
      {
        SerialMonitor.println(F("GPS logged.")); // Print a debug message
        lastLog = millis(); // Update the lastLog variable
      }
      else // If we failed to log GPS
      { // Print an error, don't update lastLog   
        SerialMonitor.println(F("Failed to log new GPS data."));
      }
    }
    else // If GPS data isn't valid
    {
      // Print a debug message. Maybe we don't have enough satellites yet.
      SerialMonitor.print(F("No GPS data. Sats: "));
      SerialMonitor.println(tinyGPS.satellites.value());
    }
  }

  // If we're not logging, continue to "feed" the tinyGPS object:
  while (gpsPort.available())
    tinyGPS.encode(gpsPort.read());
}

byte logGPSData()
{
  File logFile = SD.open(logFileName, FILE_WRITE); // Open the log file

  if (logFile)
  { // Print longitude, latitude, altitude (in feet), speed (in mph), course
    // in (degrees), date, time, and number of satellites.
    Wire.beginTransmission(MPU_addr);
    Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H)
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_addr,14,true);  // request a total of 14 registers
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
    logFile.print(tinyGPS.date.value());
    logFile.print(comma);
    logFile.print(tinyGPS.time.value());
    logFile.print(comma);
    logFile.print(tinyGPS.satellites.value());
    logFile.print(comma);
//    AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)     
//    AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
//    AcZ=Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
//    Tmp=Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
//    GyX=Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
//    GyY=Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
//    GyZ=Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
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
    logFile.println();
    logFile.close();

    return 1; // Return success
  }

  return 0; // If we failed to open the file, return fail
}

// printHeader() - prints our eight column names to the top of our log file
//void printHeader()
//{
//  File logFile = SD.open(logFileName, FILE_WRITE); // Open the log file
//  SerialMonitor.println(logFile);
//  if (logFile) // If the log file opened, print our column names to the file
//  {
//    int i = 0;
//    for (; i < LOG_COLUMN_COUNT; i++)
//    {
//      logFile.print(log_col_names[i]);
//      if (i < LOG_COLUMN_COUNT - 1) // If it's anything but the last column
//        logFile.print(','); // print a comma
//      else // If it's the last column
//        logFile.println(); // print a new line
//    }
//    logFile.close(); // close the file
//  }
//}

// updateFileName() - Looks through the log files already present on a card,
// and creates a new file with an incremented file index.
void updateFileName()
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
}
