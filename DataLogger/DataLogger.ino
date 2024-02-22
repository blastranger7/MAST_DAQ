#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <Servo.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"

//SD card module setup. Change chipSelect to the digital pin you are using
File myFile;
Sd2Card card;
SdVolume volume;
SdFile root;
const int chipSelect = 10;
 
//these are for file name creation change baseName to whatever name you want.
//baseName cannot be longer than 6 charactars
#define baseName "flight"
const uint8_t baseNameSize = sizeof(baseName) - 1;
char fileName[] = baseName "00.csv";

//how many second the program has been run for
int secondsRun = 0;

//gps setup
TinyGPS gps;
SoftwareSerial ss(3, 4);//software serial pins RX,TX

//gps variables
//how many hours ahead or behind utc you are. Needs to be configured to display the correct time
float timezone = -5;
float flat, flon, speed, altitude;
unsigned  long age, date, time;
int year;
byte month, day, hour, minute, second, hundredths,satellites;

const int MPU = 1101000; // MPU6050 I2C address
float AccX, AccY, AccZ;


//#define BMP_SCK 13
//#define BMP_MISO 12
//#define BMP_MOSI 11
//#define BMP_CS 10

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BMP3XX bmp; // I2C
//Adafruit_BMP3XX bmp(BMP_CS); // hardware SPI
//Adafruit_BMP3XX bmp(BMP_CS, BMP_MOSI, BMP_MISO, BMP_SCK);  // Software SPI

void setup() {
  Serial.begin(9600);
  ss.begin(9600);
  
  Serial.print("Simple TinyGPS library v. "); Serial.println(TinyGPS::library_version());
  Serial.println();
  card.init(SPI_HALF_SPEED,chipSelect);//SD card initialization
  if (SD.begin(chipSelect))
  {
    Serial.println("SD card is ready to use.");
  } else
  {
    Serial.println("SD card initialization failed");
    return;
  }
  fileSetup();

  if (!bmp.begin_I2C()) {   // hardware I2C mode, can pass in address & alt Wire
  //if (! bmp.begin_SPI(BMP_CS)) {  // hardware SPI mode  
  //if (! bmp.begin_SPI(BMP_CS, BMP_SCK, BMP_MISO, BMP_MOSI)) {  // software SPI mode
    Serial.println("Could not find a valid BMP3 sensor, check wiring!");
    while (1);
  }

  // Set up oversampling and filter initialization
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  bmp.setOutputDataRate(BMP3_ODR_50_HZ);

  Wire.begin();                      // Initialize comunication
  Wire.beginTransmission(MPU);       // Start communication with MPU6050 // MPU=0x68
  Wire.write(0x6B);                  // Talk to the register 6B
  Wire.write(0x00);                  // Make reset - place a 0 into the 6B register
  Wire.endTransmission(true);        //end the transmission
}


void loop() {
  {
  bool newData = false;
  unsigned long chars;
  unsigned short sentences, failed;
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (ss.available())
    {
      char c = ss.read();
      // Serial.write(c); // uncomment this line if you want to see the GPS data flowing
      if (gps.encode(c)) // Did a new valid sentence come in?
        gpsValueUpdate();
     }
  }
  if (! bmp.performReading()) {
    Serial.println("Failed to perform reading :(");
    return;
  }
  if(satellites > 3){
    secondsRun++;
    getMPU();
    updateFile();  //update the file
  }
  gps.stats(&chars, &sentences, &failed);//check valid data is being recieved from gps
  if (chars == 0)
    Serial.println("** No characters received from GPS: check wiring **");
  }
}


void updateFile(){
  myFile = SD.open(fileName, FILE_WRITE);
  if (myFile) {
    
     //print all the data to the spreadsheet seperated by commas
     myFile.print(secondsRun);
     myFile.print(",");
     myFile.print(hour+timezone);
     myFile.print(":");
     myFile.print(minute);
     myFile.print(":");
     myFile.print(second);
     myFile.print(",");
     myFile.print(altitude);
     myFile.print(",");
     myFile.print(bmp.readAltitude(SEALEVELPRESSURE_HPA));
     myFile.print(",");
     myFile.print(AccX);
     myFile.print(",");
     myFile.print(AccY);
     myFile.print(",");
     myFile.print(AccZ);
     myFile.print(",");
     myFile.print(speed);
     myFile.print(",");
     myFile.print(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
     myFile.print(",");
     myFile.println(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);
     //Serial.println("updated");
  }else{
    Serial.println("Write failure");
  }
  myFile.close();
}


void fileSetup(){
  //name creation
  while (SD.exists(fileName)) {
    if (fileName[baseNameSize + 1] != '9') {
      fileName[baseNameSize + 1]++;
    } else if (fileName[baseNameSize] != '9') {
      fileName[baseNameSize + 1] = '0';
      fileName[baseNameSize]++;
    } else {
      Serial.println(F("Can't create file name"));
      return;
    }
  }
  myFile = SD.open(fileName, FILE_WRITE);
  myFile.println("Seconds since start,Time,GPS Altitude,Barometer Altitude,Acc X,Acc Y,Acc Z,Speed,Lat,Long");//column labeling
  myFile.println("");
  myFile.close();
}

void gpsValueUpdate(){
  gps.f_get_position(&flat, &flon);
  satellites = gps.satellites() == TinyGPS::GPS_INVALID_SATELLITES ? 0 : gps.satellites();//get number of GPS satellites
  speed = gps.f_speed_mps();  //get speed. I am using mp/s but this can be changed to f_speed_mph or f_speed_kmh for difrent units
  altitude = gps.altitude()/100; //Get altitude from GPD
  gps.get_datetime(&date, &time, &age);
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age); //update time from GPS
}

void getMPU(){
  //BMP388 adress: 1110110
  //MPU6050 adress: 1101000
  Wire.beginTransmission(MPU);
  Wire.write(0x3B); // Start with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true); // Read 6 registers total, each axis value is stored in 2 registers
  //For a range of +-2g, we need to divide the raw values by 16384, according to the datasheet
  AccX = (Wire.read() << 8 | Wire.read()) / 16384.0; // X-axis value
  AccY = (Wire.read() << 8 | Wire.read()) / 16384.0; // Y-axis value
  AccZ = (Wire.read() << 8 | Wire.read()) / 16384.0; // Z-axis value
}
