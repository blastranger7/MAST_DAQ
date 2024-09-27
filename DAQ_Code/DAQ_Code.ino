#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"
#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>

// MPU VALUES
#define SEALEVELPRESSURE_HPA (1018.8)
const int MPU = 0x68; // MPU6050 I2C address
float AccX, AccY, AccZ;
float GyroX, GyroY, GyroZ;
float accAngleX, accAngleY, gyroAngleX, gyroAngleY, gyroAngleZ;
float roll, pitch, yaw;
float AccErrorX, AccErrorY, GyroErrorX, GyroErrorY, GyroErrorZ;
float elapsedTime, currentTime, previousTime;
int c = 0;

// BMP VALUES
Adafruit_BMP3XX bmp;
float alt;

// SD
File myFile;
Sd2Card card;
SdVolume volume;
SdFile root;
const int chipSelect = 5;
float secondsRun = 0;

#define baseName "flight"
const uint8_t baseNameSize = sizeof(baseName) - 1;
char fileName[] = baseName "00.csv";

// GPS
TinyGPS gps;
SoftwareSerial ss(2, 3);//software serial pins RX,TX
float timezone = -5;
float flat, flon,speed,altitude;
unsigned  long age, date, time;
int year;
byte month, day, hour, minute, second, hundredths,satellites;

void setup() {
  Serial.begin(9600);
  ss.begin(9600);
  Wire.begin();                      // Initialize comunication
  Wire.beginTransmission(MPU);       // Start communication with MPU6050 // MPU=0x68
  Wire.write(0x6B);                  // Talk to the register 6B
  Wire.write(0x00);                  // Make reset - place a 0 into the 6B register
  Wire.endTransmission(true);        //end the transmission
  Wire.beginTransmission(MPU);
  Wire.write(0x1C);                  //Talk to the ACCEL_CONFIG register (1C hex)
  Wire.write(0x10);                  //Set the register bits as 00010000 (+/- 8g full scale range)
  Wire.endTransmission(true);
  //while (!Serial);
  

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

 card.init(SPI_HALF_SPEED,chipSelect);//SD card initialization
  if (SD.begin(chipSelect)){
    Serial.println("SD card is ready to use.");
  } 
  else {
    Serial.println("SD card initialization failed");
    return;
  }
  fileSetup();
}

void loop() {
  bool newData = false;
  unsigned long chars;
  unsigned short sentences, failed;
  if (! bmp.performReading()) {
    Serial.println("Failed to perform reading :(");
    return;
  }
  secondsRun += 0.5;
  getAlt();
  getAcc();
  //getOrient();
  /*for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (ss.available())
    {
      char c = ss.read();
      // Serial.write(c); // uncomment this line if you want to see the GPS data flowing
      if (gps.encode(c)) // Did a new valid sentence come in?
        getGPS();
     }
  }*/
  updateFile();
 

  Serial.println(alt);
  Serial.print(AccX);
  Serial.print("/");
  Serial.print(AccY);
  Serial.print("/");
  Serial.println(AccZ);
  Serial.print(pitch);
  Serial.print("/");
  Serial.print(roll);
  Serial.print("/");
  Serial.println(yaw);
  Serial.println();
  delay(500);
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
  myFile.println("Time (s),Altitude (m),AccX,AccY,AccZ");//,Time,GPS Altitude,Speed,GPS Sats,Lat,Long");//column labeling
  myFile.close();
}

void updateFile(){
  myFile = SD.open(fileName, FILE_WRITE);
  if (myFile) {
     myFile.print(secondsRun);
     /*myFile.print(", ");
     myFile.print(hour+timezone);
     myFile.print(":");
     myFile.print(minute);
     myFile.print(":");
     myFile.print(second);*/
     myFile.print(", ");
     myFile.print(alt);
     myFile.print(", ");
     myFile.print(AccX);
     myFile.print(",");
     myFile.print(AccY);
     myFile.print(",");
     myFile.println(AccZ);
     /*myFile.print(", ");
     myFile.print(altitude);
     myFile.print(",");
     myFile.print(speed);
     myFile.print(",");
     myFile.print(satellites);
     myFile.print(",");
     myFile.print(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
     myFile.print(",");
     myFile.println(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);*/
  }else{
    Serial.println("Write failure");
  }
  myFile.close();
}

void getAlt(){
  alt = bmp.readAltitude(SEALEVELPRESSURE_HPA);
}

void getAcc(){
  Wire.beginTransmission(MPU);
  Wire.write(0x3B); // Start with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true); // Read 6 registers total, each axis value is stored in 2 registers
  //For a range of +-2g, we need to divide the raw values by 16384, according to the datasheet
  AccX = (Wire.read() << 8 | Wire.read()) / 4096.0; // X-axis value
  AccY = (Wire.read() << 8 | Wire.read()) / 4096.0; // Y-axis value
  AccZ = (Wire.read() << 8 | Wire.read()) / 4096.0; // Z-axis value 
}

void getOrient(){
  previousTime = currentTime;        // Previous time is stored before the actual time read
  currentTime = millis();            // Current time actual time read
  elapsedTime = (currentTime - previousTime) / 1000; // Divide by 1000 to get seconds
  Wire.beginTransmission(MPU);
  Wire.write(0x43); // Gyro data first register address 0x43
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true); // Read 4 registers total, each axis value is stored in 2 registers
  GyroX = (Wire.read() << 8 | Wire.read()) / 131.0; // For a 250deg/s range we have to divide first the raw value by 131.0, according to the datasheet
  GyroY = (Wire.read() << 8 | Wire.read()) / 131.0;
  GyroZ = (Wire.read() << 8 | Wire.read()) / 131.0;
  // Correct the outputs with the calculated error values
  GyroX = GyroX + 0.56; // GyroErrorX ~(-0.56)
  GyroY = GyroY - 2; // GyroErrorY ~(2)
  GyroZ = GyroZ + 0.79; // GyroErrorZ ~ (-0.8)
  // Currently the raw values are in degrees per seconds, deg/s, so we need to multiply by sendonds (s) to get the angle in degrees
  gyroAngleX = gyroAngleX + GyroX;// * elapsedTime; // deg/s * s = deg
  gyroAngleY = gyroAngleY + GyroY;// * elapsedTime;
  yaw =  yaw + GyroZ * elapsedTime;
  // Complementary filter - combine acceleromter and gyro angle values
  roll = 0.96 * gyroAngleX + 0.04 * accAngleX;
  pitch = 0.96 * gyroAngleY + 0.04 * accAngleY;
}

void getGPS(){
  gps.f_get_position(&flat, &flon);
  satellites = gps.satellites() == TinyGPS::GPS_INVALID_SATELLITES ? 0 : gps.satellites();//get number of GPS satellites
  speed = gps.f_speed_mps();  //get speed. I am using mp/s but this can be changed to f_speed_mph or f_speed_kmh for difrent units
  altitude = gps.altitude()/100; //Get altitude from GPD
  gps.get_datetime(&date, &time, &age);
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age); //update time from GPS
}