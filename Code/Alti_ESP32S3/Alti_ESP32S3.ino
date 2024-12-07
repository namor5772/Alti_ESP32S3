#include "MS5637.h"
#include "PCD8544.h"
#include "Font.h"

#include <Preferences.h>
#include <esp_sleep.h>
#include <esp32-hal-gpio.h>

// To Do :
// 1. Make nicer big numbers font for displaying Altitude
// 2. Test Deep-Sleep functionality

// Define the analog pin connected to the voltage divider
#define BATTERY_PIN 8 // GPIO8 (A9)

// Define the pin connected to a momentary button, to toggle
// deep-sleep mode for trhis altimeter, as an alternative to
// a physical switch that disconnects LiPo battery.
#define ONOFF_PIN 7 // GPIO7 (D8)

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
// An instance called lcd of the PCD8544 class is created
// The module uses an enhanced bit-bashed 3-Wire SPI communications protcol
// The specified pins are: CLK, MOS, RES, DC, CS.
#define RST 2
#define CE 1
#define DC 3
#define DIN 4
#define CLK 9
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
bool isGround;
uint32_t chipId = 0;
uint16_t adcRaw;
float temp, pressure, altBase, altRel, batVol;

void setup() {
  Serial.begin(115200);

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  // setup deep-sleep on/off pin,
  // actually need an external pull-up (4.7k)
  pinMode(ONOFF_PIN,INPUT_PULLUP);

  // reads the persistent isGround boolean value
  Preferences preferences;
  preferences.begin("alti_prefs", false);
  bool isGround = preferences.getBool("isGround", false);

  // setup MS5637 sensor (An instance of the MS5637 object BARO has been constructed above)
  BARO.begin();
  BARO.dumpDebugOutput();
  BARO.getTempAndPressure(&temp, &pressure);

  // toggles mode as ground or absolute between powering altimeter
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
//  lcd.clear();
  lcd.print("Alti ESP32v2.0 Nov24");
  lcd.print("----------");
  lcd.setCursor(0, 3);
  lcd.print("Roman M   Groblicki ");
  lcd.print("----------");
  delay(3000);
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
  if (batVol>3.19){
    // just display battery voltage
//    lcd.Battery_tinyfont(batVol, 0, 76);
    lcd.Battery_tinyfont2(batVol, 0, 72);
  }
  else { //battery voltage too low so indicate need to charge LiPo
    // flash a block in the battery voltage display position
    // half second on, half second off.
    float mTime = millis();
    mTime = mTime-floor(mTime/1000.0)*1000.0;
    bool isFlash = false;
    if (mTime<500.0) isFlash = true;
    lcd.BatteryFlash_tinyfont(isFlash, 0, 76);
  }

  Serial.print("Altitude: ");
  Serial.println(altRel);
  Serial.print("Adjusted temp: ");
  Serial.println(temp);
  Serial.print("Raw ADC: ");
  Serial.println(adcRaw);
  Serial.print("Battery Voltage: ");
  Serial.println(batVol);

  int pinState = digitalRead(ONOFF_PIN);
  if (pinState == LOW) {
    Serial.println("LOW");

    // setup lcd screen and display 'shutdown' screen for 4 seconds
    // then clear just before 'shutdown'
    lcd.clear();
    // lcd.setCursor(0, 1);
    lcd.print("----------");
    lcd.print("-SHUTTING-");
    lcd.print("---DOWN---");    
    lcd.print("---INTO---");
    lcd.print("DEEP SLEEP");
    lcd.print("----------");
    Serial.println("SHUTTING DOWN");
    delay(5000);

    // Enable EXT0 wake-up source (wake up when pin goes LOW) 
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_7, 0);

    // Enter deep sleep mode 
    esp_deep_sleep_start();

  } 
  else {
      Serial.println("HIGH");
  }

  
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(200);                      // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(200);                      // wait for a second


  delay(50);
  Serial.println("LOOPING");
  Serial.println(" ");
}
