This is a repository for my digital altimeter based on the ESP32-S3 microcontroller. It uses an I2C interfaced pressure/temperature sensor board based on the MS5637 chip and an 84x48 SPI interfaced lcd screen based on the PCD8544 chip. The electronics is housed in the enclosure used by an old analog skydiving altimeter. Eveything is powered by a single cell LiPo battery that can be charged by a USB-C port. This can also be used to update the softwaqre.

When switched on the altimeter sets to zero, so as to give you the height above ground in thousands of feet. It displays the height in large numerical characters to the nearest 100 feet, with the remaining altitude < 100 ft in a very small font. Negative altitudes are displayed as required.

In addition the voltage x 10 is displayed in a tiny font at the top right of the screen, under it is displayed the approximate air temnperature also in a tiny font. If the battery voltage drops to 3.2V (32 displayed) the voltage displayed is replayed by a flashing blosk.

![ESP32-S3 pinout](Images/ESP32-S3_pinout.png)