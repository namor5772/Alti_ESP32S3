#include "MS5637.h"
#include "PCD8544.h"
#include "Font.h"

// Nokia 5772 - PCD8544 driver chip
// and pinout to SEED ESP32s3 MICRO
// 1 - VCC (3v3)
// 2 - GND
// 3 - SCE (CE)       - GPIO1 D0
// 4 - RST (RST)      - GPIO2 D1
// 5 - D/C (DC)       - GPIO3 D2
// 6 - DN<MOSI> (DIN) - GPIO4 D3 
// 7 - SCLK (CLK)     - GPIO9 D10
// 8 - LED 
#define RST 2
#define CE 1
#define DC 3
#define DIN 4
#define CLK 9

// An instance called lcd of the PCD8544 class is created
// The module uses an enhanced bit-bashed 3-Wire SPI communications protcol
// The specified pins are: CLK, MOS, RES, DC, CS.
PCD8544 lcd{RST, CE, DC, DIN, CLK};

// An instance of the MS5637 class called BARO is created
// The address and micro pins are defined in the MS5637.h file
// The module is I2C and has just 4 pins:
// 1 GND - connect to ground pin on micro (Seeed ESP32S3 here)
// 2 SDA - connect to SDA pin on micro (also called GPIO5 & D4)
// 3 SCL - connect to SCL pin on micro (also called GPIO6 & D5)
// 4 VCC - connect to 3V3 pin on micro

MS5637 BARO;

// global variables
uint32_t chipId = 0;
float temp, pressure, altBase, altRel;
char str[11];

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 17; i = i + 8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("This chip has %d cores\n", ESP.getChipCores());
  Serial.print("Chip ID: ");
  Serial.print(chipId);
  Serial.println("\n");

  // setup lcd screen and display splash screen for 2 seconds
  lcd.setDisplayMode(NORMAL);
  lcd.setContrast(60);
  lcd.print("Alti ESP32v2.0 Nov24");
  lcd.print("----------");
  lcd.setCursor(0, 3);
  lcd.print("Roman M   Groblicki ");
  lcd.print("----------");
  delay(4000);
  lcd.clear();

  // setup MS5637 sensor (An instance of the MS5637 object BARO has been constructed above)
  BARO.begin();
  BARO.dumpDebugOutput();
  BARO.getTempAndPressure(&temp, &pressure);
//  altBase = BARO.pressure2altitude(pressure);
  altBase = 0.0;
  Serial.println(altBase);  

}


void loop() {
  if (!BARO.isOK()) {
    // Try to reinitialise the sensor if we can and measure temperature and pressure
    BARO.begin();
    BARO.getTempAndPressure(&temp, &pressure);
  }
  else { // just normal measurements on each loop
    BARO.getTempAndPressure(&temp, &pressure);
  }

  // display current temperature measured by the MS5637 (and used to
  // improve calculation of air pressure)
//  lcd.Temperature(temp, 0, 2);

  // Calculate and display the altitude relative to where the altimeter was turned on
  altRel = BARO.pressure2altitude(pressure) - altBase; 
  if (altRel < 0.0) altRel = -1.0*altRel; // take the absolute value 
  lcd.Altitude_largefont(altRel);
//  lcd.Altitude_smallfont(altRel, 2, 0);

  Serial.print(temp);
  Serial.print(" ");
  Serial.println(altRel);

/*
  lcd.Altitude_largefont(34); delay(2000);
  lcd.Altitude_largefont(90); delay(2000);
  lcd.Altitude_largefont(853); delay(2000);
  lcd.Altitude_largefont(1234); delay(2000);
  lcd.Altitude_largefont(23400); delay(2000);
  lcd.Altitude_largefont(56700); delay(2000);
  lcd.Altitude_largefont(8900); delay(2000);
*/

  delay(1000);
}
