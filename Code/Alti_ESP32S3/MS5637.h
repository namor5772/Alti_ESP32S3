/* BaroSensor
 *
 * An Arduino library for the Freetronics BARO sensor module, using
 * the MS5637 Pressure/Temperature sensor module.
 * Stolen and modified from Angus Gratton (angus at freetronics dot com)
 */
#include <Arduino.h>
#include <Wire.h>

/* i2c address of module & pins on micro */
#define BARO_ADDR 0x76
#define SDA 5
#define SCL 6

enum OSR {OSR_256, OSR_512, OSR_1024, OSR_2048, OSR_4096, OSR_8192 };
enum TempUnit {CELSIUS, FAHRENHEIT};

/* delay to wait for sampling to complete, on each OSR level */
const uint8_t SamplingDelayMs[6] PROGMEM = {2,4,6,10,18,34};

/* the 5 basic commands of the MS5637 module */
#define CMD_RESET 0x1E
#define CMD_START_D1(osl) (0x40 + 2*osl) // 6 possibilities, for each OSR enum  
#define CMD_START_D2(osl) (0x50 + 2*osl) // 6 possibilities, for each OSR enum  
#define CMD_PROM_READ(offs) (0xA0+(offs<<1)) // offs ranges from 0 to 6 (7 adressess, totalling 112 bits)
#define CMD_READ_ADC 0x00

/* error codes */
#define ERR_NOREPLY -1
#define ERR_BAD_READLEN -2
#define ERR_NEEDS_BEGIN -3


class MS5637 {
 public:
  MS5637() : initialised(false), err(ERR_NEEDS_BEGIN) { }
  
  void begin();

  /* Update both temperature and pressure together. This takes less
     time than calling each function separately (as pressure result
     depends on temperature.) Returns true for success, false on an
     error, TempUnit & OSR have defaults of CELSIUS & OSR_8192 */
  bool getTempAndPressure(float *temperature,
                          float *pressure,
                          TempUnit tempScale = CELSIUS,
                          OSR level = OSR_8192);

  inline bool isOK() { return initialised && err == 0; }
  inline byte getError() { return initialised ? err : ERR_NEEDS_BEGIN; }

  /* Debugging function that outputs a list of debugging data to Serial */
  void dumpDebugOutput();

  // Returns altitude (in feet) for a given pressure (in mbar)
  float pressure2altitude(float pressure);
  
private:
  bool initialised;
  int8_t err;
  uint16_t c1,c2,c3,c4,c5,c6; // Calibration constants used in producing results

  uint32_t takeReading(uint8_t trigger_cmd, OSR oversample_level);
};