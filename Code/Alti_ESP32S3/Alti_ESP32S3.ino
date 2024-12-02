#include "MS5637.h"
#include "PCD8544.h"
#include "Font.h"

#include <Preferences.h>



// To Do :
// DONE 1. Flash battery voltage (Top righ) when drops to 3.2V
// 2. Make nicer big numbers font for displaying Altitude
// DONE 3. buy DSDT smallest switch possible





// Define the analog pin connected to the voltage divider
#define BATTERY_PIN 8 // GPIO8 (A9)

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
uint16_t adcRaw;
float batVol; 
bool isGround;

void setup() {
  Serial.begin(115200);

  Preferences preferences;
  preferences.begin("alti_prefs", false);

  // reads the persistent isGround boolean value
  bool isGround = preferences.getBool("isGround", false);

  // setup MS5637 sensor (An instance of the MS5637 object BARO has been constructed above)
  BARO.begin();
  BARO.dumpDebugOutput();
  BARO.getTempAndPressure(&temp, &pressure);

  // toggles mode as ground or absolute between powering on altimeter
  if (isGround) {
    altBase = BARO.pressure2altitude(pressure);
    preferences.putBool("isGround", false);
  }
  else { // if not isGround
    altBase = 0.0;
    preferences.putBool("isGround", true);
  }
  Serial.println(altBase);  

  // just some chip info on startup
  for (int i = 0; i < 17; i = i + 8) {chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;}
  Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("This chip has %d cores\n", ESP.getChipCores());
  Serial.print("Chip ID: ");
  Serial.print(chipId);
  Serial.println("\n");

  // setup lcd screen and display splash screen for 4 seconds
  // then clear in readiness for normal data display
  lcd.setDisplayMode(NORMAL);
  lcd.setContrast(60);
  lcd.print("Alti ESP32v2.0 Nov24");
  lcd.print("----------");
  lcd.setCursor(0, 3);
  lcd.print("Roman M   Groblicki ");
  lcd.print("----------");
  delay(4000);
  lcd.clear();
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

  // Calculate and display the altitude relative to where the altimeter was turned on
  altRel = BARO.pressure2altitude(pressure) - altBase; 
  //altRel = 13456.0;
  if (altRel < 0.0) altRel = -1.0*altRel; // take the absolute value 
  lcd.Altitude_largefont(altRel);

  // display current temperature measured by the MS5637 (and used to
  // improve calculation of air pressure above)
  temp = temp - 0.5; // fudjustment for circuit temp being above ambient
  lcd.Temperature_tinyfont(temp, 1, 76);

  // Read raw ADC value, convert to battery voltage and display 
  adcRaw = analogRead(BATTERY_PIN);
  batVol = adcRaw*0.000940767+0.31; // 0.287561; // raw adc to battery voltage conversion
  if (batVol>4.1){ //>3.19
    // just display battery voltage
    lcd.Battery_tinyfont(batVol, 0, 76);
  }
  else { //battery voltage too low need to charge LiPo
    // flash a block in the battery voltage display position
    float mTime = millis();
    mTime = mTime-floor(mTime/1000.0)*1000.0;
    bool isFlash = false;
    if (mTime<500.0) isFlash = true;
    lcd.BatteryFlash_tinyfont(isFlash, 0, 76);
  }

/*
  float mTime = millis();
  mTime = mTime-floor(mTime/1000.0)*1000.0;
  Serial.println(mTime);
*/


/*
  delay(2000);
  lcd.Altitude_largefont(-1.0);  delay(2000);
  lcd.Altitude_largefont(-13456.0);  delay(2000);
  lcd.Altitude_largefont(-23456.0);  delay(2000);
  lcd.Altitude_largefont(-13456.0);  delay(2000);
  lcd.Altitude_largefont(-1345.0);  delay(2000);
  lcd.Altitude_largefont(-134.0);  delay(2000);
  lcd.Altitude_largefont(-13.0);  delay(2000);
  lcd.Altitude_largefont(-1.0);  delay(2000);
  lcd.Altitude_largefont(-0.9);  delay(2000);
  lcd.Altitude_largefont(13456.0);  delay(2000);
  lcd.Altitude_largefont(1345.0);  delay(2000);
  lcd.Altitude_largefont(134.0);  delay(2000);
  lcd.Altitude_largefont(13.0);  delay(2000);
  lcd.Altitude_largefont(1.0);  delay(2000);
  lcd.Altitude_largefont(0.9);  delay(2000);
*/

  Serial.print("Altitude: ");
  Serial.println(altRel);
  Serial.print("Adjusted temp: ");
  Serial.println(temp);
  Serial.print("Raw ADC: ");
  Serial.println(adcRaw);
  Serial.print("Battery Volatge: ");
  Serial.println(batVol);

  delay(100);
}
