# MAST_DAQ

## A data aquisition system for the McMaster Aerospace Team
Intended to gather altitude, accelleration, and GPS data and save it to a SD card as a csv file.

### Hardware
> Arduino Uno

> GT-U7  GPS Module

> MPU6050 Accelerometer

> Micro SD Card adapter

### Notes
Turning the system on will immediately create a new csv file on the SD card and start writing accelerometer and GPS data to it.

If the system in turned on and GPS data is all 0s then the likely cause is bad sattelite reception and you need to move outside.

.cddx file is used to design the circuit diagram at https://www.circuit-diagram.org/editor/

The SD Card in this system needs to be formatted to FAT32. Most SD cards do not come this way and it is likely third party software is needed if the SD card is over 32GB. Free software can be found to do this, although I've found that **DiskGenius** (https://www.diskgenius.com) works the best (also free).

There is a section in the code with 
```
SoftwareSerial ss(4, 3); //software serial pins RX,TX
```
This is correct despite the GT-U7 module having RX connected to pin 3 and TX connected to pin 4. It does not seem to work if you swap these around and I don't know why.

