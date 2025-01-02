This is a repository for my digital altimeter based on the ESP32-S3 microcontroller. It uses an I2C interfaced pressure/temperature sensor board based on the MS5637 chip and an 84x48 SPI interfaced lcd screen based on the PCD8544 chip. The electronics is housed in the enclosure used by an old analog skydiving altimeter. Eveything is powered by a single cell LiPo battery that can be charged by a USB-C port. This can also be used to update the software.

When switched on the altimeter sets to zero, giving the height above ground in thousands of feet or alternately relative to the standard sea level pressure of 1013.25 mb. It displays the height in large numerical characters to the nearest 100 feet, with the remaining altitude < 100 ft in a very small font. Negative altitudes are displayed as required.

In addition the voltage x 10 is displayed in a tiny font at the top right of the screen, under it is displayed the approximate air temperature also in a tiny font. If the battery voltage drop below 3.2V (32 displayed) the voltage displayed is replaced by a flashing block. "Latching" is implemented in code to prevent the display alternating with voltage and flashing block.

The switch for the altimiter is actually a momentary push button and does not physically disconnect/connect the battery from the circuit, it just toggles between the microcontrollers normal and deep sleep states. In the deep sleep state the circuit uses minimal current of around 18µA and should last about a year before recharging. In order that one does not accidentally switch the altimeter off, doing so requires the switch to be pressed twice within a very specific and narrow time gap range. This is indicated by temporary and self explanatory messages displayed on the screen.

It has been successfully tested in freefall!


![ESP32-S3 pinout](Images/ESP32-S3_pinout.png)