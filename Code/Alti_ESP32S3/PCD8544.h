#ifndef PCD8544_H
#define PCD8544_H
#include "Arduino.h"

// Display Mode constants
#define BLANK 0x08
#define NORMAL 0x0c
#define ALL_ON 0x09
#define INVERSE 0x0d

// Bias System constants
#define HUNDRED 0x10
#define EIGHTY 0x11
#define SIXTY_FIVE 0x12
#define FORTY_EIGHT 0x13
#define FORTY 0x14
#define TWENTY_FOUR 0x15
#define EIGHTEEN 0x16
#define TEN 0x17

// Command type constants 
#define BASIC 0x20
#define ADVANCED 0x21

class PCD8544 : public Print {

  public:
    PCD8544(uint8_t RST, uint8_t CE, uint8_t DC, uint8_t DIN, uint8_t CLK);

    void setContrast(uint8_t c);
    void setTemperatureCoefficient(uint8_t value);
    void setCursor(uint8_t x, uint8_t y);
    void clear();
    void setDisplayMode(byte mode);
    void setBiasSystem(byte rate);

    void writeBlock(uint8_t page, uint8_t col, uint8_t pages, uint8_t cols, uint16_t address, const uint8_t Arr[]);
    void write8x8Char(uint8_t page, uint8_t column, uint16_t charCode, const uint8_t Arr[][8]);
    void Temperature(float temp_, uint8_t page, uint8_t col);
    void Altitude_smallfont(float altitude, uint8_t page, uint8_t col);
    uint16_t ASCII2offset(char char_, uint16_t offsetScale);

    virtual size_t write(uint8_t ch); // Overriding Print's write method
    
  private:

    // low level SPI comms with PC8544
    void TransferStart();
    void CommandMode(byte cm);
    void DataMode();
    void WriteByte(byte d);
    void TransferEnd();

    // setup functions    
    void reset();

    // data variables
    uint8_t _RST, _CE, _DC, _DIN, _CLK;
    uint8_t Xcur, Ycur;

    // specific char arrays used when displaying text on LED
    char str_old[11]; char str_new[11];
    char str_old0[11]; char str_new0[11];
};

#endif

