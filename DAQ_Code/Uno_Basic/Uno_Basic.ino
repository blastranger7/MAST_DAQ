#include <Wire.h>
#include <SPI.h>
#include <SD.h>

File myFile;
Sd2Card card;
SdVolume volume;
SdFile root;
int chipSelect = 10;

float secondsRun = 0;

#define SEALEVELPRESSURE_HPA (1018.8)
const int MPU = 0x68; // MPU6050 I2C address
float AccX, AccY, AccZ;
float GyroX, GyroY, GyroZ;
float accAngleX, accAngleY, gyroAngleX, gyroAngleY, gyroAngleZ;
float roll, pitch, yaw;
float AccErrorX, AccErrorY, GyroErrorX, GyroErrorY, GyroErrorZ;
float elapsedTime, currentTime, previousTime;

#define baseName "flight"
const uint8_t baseNameSize = sizeof(baseName) - 1;
char fileName[] = baseName "00.csv";

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  //ss.begin(9600);
  Wire.begin();                      // Initialize comunication
  Wire.beginTransmission(MPU);       // Start communication with MPU6050 // MPU=0x68
  Wire.write(0x6B);                  // Talk to the register 6B
  Wire.write(0x00);                  // Make reset - place a 0 into the 6B register
  Wire.endTransmission(true);        //end the transmission
  Wire.beginTransmission(MPU);
  Wire.write(0x1C);                  //Talk to the ACCEL_CONFIG register (1C hex)
  Wire.write(0x10);                  //Set the register bits as 00010000 (+/- 8g full scale range)
  Wire.endTransmission(true);

  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed!");
    return;
  }
  fileSetup();
}



void loop() {
  // put your main code here, to run repeatedly:
  secondsRun += 0.25;
  updateFile();

  Serial.print(AccX);
  Serial.print("/");
  Serial.print(AccY);
  Serial.print("/");
  Serial.println(AccZ);
  delay(250);
}

void updateFile(){
  //Serial.println("entered");
  getAcc();
  myFile = SD.open(fileName, FILE_WRITE);
  if (myFile) {
    //Serial.println("updating");
     //print all the data to the spreadsheet seperated by commas
     myFile.print(secondsRun);
     myFile.print(", ");
     myFile.print(AccX);
     myFile.print(", ");
     myFile.print(AccY);
     myFile.print(", ");
     myFile.println(AccZ);
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
  myFile.println("");
  myFile.println("Time,AccX,AccY,AccZ");//column labeling
  myFile.close();
  //Serial.println("File Created");
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

