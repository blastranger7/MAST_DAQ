//libraries
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <FS.h>
//#include <Servo.h>

//SD card module setup. Change chipSelect to the digital pin you are using
File myFile;
/*Sd2Card card;
SdVolume volume;
SdFile root;*/
int chipSelect = 5;
 
//these are for file name creation change baseName to whatever name you want.
//baseName cannot be longer than 6 charactars
#define baseName "/flight"
const uint8_t baseNameSize = sizeof(baseName) - 1;
char fileName[] = baseName "00.csv";

//how many second the program has been run for
int secondsRun = 0;

//values for voltage divider
//adjust the inputAdjustment value if your voltage is not correct
//float inputVoltage = 0.0;
//const float inputAdjustment = 0.75;
//const float voltageDivider = 2.0;

//gps setup
TinyGPS gps;
SoftwareSerial ss(16, 17);//software serial pins RX,TX

//gps variables
//how many hours ahead or behind utc you are. Needs to be configured to display the correct time
float timezone = 0;
float flat, flon,speed,altitude;
unsigned  long age, date, timee;
int year;
byte month, day, hour, minute, second, hundredths,satellites;

// MPU VALUES
#define SEALEVELPRESSURE_HPA (1018.8)
const int MPU = 0x68; // MPU6050 I2C address
float AccX, AccY, AccZ;
float GyroX, GyroY, GyroZ;
float accAngleX, accAngleY, gyroAngleX, gyroAngleY, gyroAngleZ;
float roll, pitch, yaw;
float AccErrorX, AccErrorY, GyroErrorX, GyroErrorY, GyroErrorZ;
float elapsedTime, currentTime, previousTime;

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
  
  Serial.print("Simple TinyGPS library v. "); Serial.println(TinyGPS::library_version());
  Serial.println();
  //SD card initialization
  /*Serial.println(SS);
  Serial.println(MOSI);
  Serial.println(MISO);
  Serial.println(SCK);*/
  //pinMode(5, OUTPUT);
  //digitalWrite(5, HIGH);
  SPI.begin(18,19,23,chipSelect);
  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed!");
    return;
  }
  
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }

  /*Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);*/
  fileSetup();
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
      //Serial.write(c); // uncomment this line if you want to see the GPS data flowing
      if (gps.encode(c)) // Did a new valid sentence come in?
        gpsValueUpdate();
     }
  }
  //Serial.println("Recieved GPS Data");
  if(satellites > 3){
    //secondsRun++;
    //batteryVoltage(); //call for battery voltage reading
      //update the file
  }
  
  gps.stats(&chars, &sentences, &failed);//check valid data is being recieved from gps
  if (chars == 0)
    Serial.println("** No characters received from GPS: check wiring **");
  }
  updateFile();

  Serial.print(AccX);
  Serial.print("/");
  Serial.print(AccY);
  Serial.print("/");
  Serial.println(AccZ);
  //delay(500);
}


void updateFile(){
  //Serial.println("entered");
  getAcc();
  File myFile = SD.open(fileName, FILE_APPEND);
  if (myFile) {
    //Serial.println("updating");
     //print all the data to the spreadsheet seperated by commas
     //myFile.print(secondsRun);
     //myFile.print(", ");
     myFile.print(hour+timezone);
     myFile.print(":");
     myFile.print(minute);
     myFile.print(":");
     myFile.print(second);
     myFile.print(", ");
     myFile.print(AccX);
     myFile.print(", ");
     myFile.print(AccY);
     myFile.print(", ");
     myFile.print(AccZ);
     myFile.print(", ");
     //myFile.print(inputVoltage);
     //myFile.print(",");
     myFile.print(altitude);
     myFile.print(",");
     myFile.print(speed);
     myFile.print(",");
     myFile.print(satellites);
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
  File myFile = SD.open(fileName, FILE_APPEND);
  myFile.println("");
  myFile.println("Time,AccX,AccY,AccZ,Altitude,Speed,Gps Sats,Lat,Long");//column labeling
  myFile.close();
  Serial.println("File Created");
}

/*void batteryVoltage(){
  int analog_value = analogRead(A3);//get analog voltage from voltage divider
  inputVoltage = (analog_value * 5.0) / 1024.0; //calculate voltage from analog value
  if (analog_value < 460){
    inputVoltage = 0.0;
  }

  else{
    inputVoltage *= voltageDivider; //apply voltage divider
    inputVoltage += inputAdjustment;  //apply voltage inputAdjustment
  }
}
*/
void gpsValueUpdate(){
  gps.f_get_position(&flat, &flon);
  satellites = gps.satellites() == TinyGPS::GPS_INVALID_SATELLITES ? 0 : gps.satellites();//get number of GPS satellites
  speed = gps.f_speed_mps();  //get speed. I am using mp/s but this can be changed to f_speed_mph or f_speed_kmh for difrent units
  altitude = gps.altitude()/100; //Get altitude from GPD
  gps.get_datetime(&date, &timee, &age);
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age); //update time from GPS
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
