# MAST_DAQ

## A data aquisition system for the McMaster Aerospace Team
Intended to gather altitude, accelleration, and GPS data and save it to a SD card as a csv file.

### Hardware
> ESP32

> GT-U7  GPS Module

> MPU6050 Accelerometer

> Micro SD Card adapter

### Use
Turning the system on will immediately create a new csv file on the SD card and start writing accelerometer and GPS data to it.

If the system in turned on and GPS data is all 0s then the likely cause is bad sattelite reception and you need to move outside.

### Notes

.cddx file is used to design the circuit diagram at https://www.circuit-diagram.org/editor/

The SD Card in this system needs to be formatted to FAT32. Most SD cards do not come this way and it is likely third party software is needed if the SD card is over 32GB. Free software can be found to do this, although I've found that **DiskGenius** (https://www.diskgenius.com) works the best (also free).

Use an ESP32 and not an Arduino as an Arduino doesn't have enough SRAM to handle the 3 communication protocols needed.

Hold down the boot button on the ESP32 for a second or two while connecing when uploading code
