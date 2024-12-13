#include "Arduino.h"
#include "PCD8544.h"
#include "Font.h"


PCD8544::PCD8544(uint8_t RST, uint8_t CE, uint8_t DC, uint8_t DIN, uint8_t CLK):
  _RST(RST), _CE(CE), _DC(DC), _DIN(DIN), _CLK(CLK) {

  pinMode(RST, OUTPUT);
  pinMode(CE, OUTPUT);
  pinMode(DC, OUTPUT);
  pinMode(DIN, OUTPUT);
  pinMode(CLK, OUTPUT);

  // screen initializations  
  reset();
  clear();
  setDisplayMode(NORMAL);
  setBiasSystem(FORTY);
  setContrast(60);

  // initialise str_old*, for its first use in loop(),
  // size is enough to store 10 characters, but often less is used.
  for (int i=0; i<10; i++) {
    str_old[i] = ' ';  
    str_old0[i] = ' ';
    str_old1[i] = ' ';
    str_old2[i] = ' ';
    str_old3[i] = ' ';
    str_old4[i] = ' ';
  }        
}

// contrast c should be between 0x00 and 0x7F inclusive (127 decimal) 
void PCD8544::setContrast(uint8_t c){
    digitalWrite(_CE, LOW); // CS (Upper score)
    digitalWrite(_DC, LOW); // select Command Mode - A0 set as LOW
    shiftOut(_DIN, _CLK, MSBFIRST, ADVANCED); // use extended instruction set
    shiftOut(_DIN, _CLK, MSBFIRST, byte(0x80 + c));
    digitalWrite(_CE, HIGH);
}

// Temperature Coefficient value could be one of 0, 1, 2 or 3;
void PCD8544::setTemperatureCoefficient(uint8_t value){
    digitalWrite(_CE, LOW); // CS (Upper score)
    digitalWrite(_DC, LOW); // select Command Mode - A0 set as LOW
    shiftOut(_DIN, _CLK, MSBFIRST, ADVANCED); // use extended instruction set
    shiftOut(_DIN, _CLK, MSBFIRST, byte(0x04 + value));
    digitalWrite(_CE, HIGH);
}

void PCD8544::setCursor(uint8_t x, uint8_t y){
    Xcur = x; Ycur = y;
    digitalWrite(_CE, LOW); // CS (Upper score)
    digitalWrite(_DC, LOW); // select Command Mode - A0 set as LOW
    shiftOut(_DIN, _CLK, MSBFIRST, BASIC); // use basic instruction set
    shiftOut(_DIN, _CLK, MSBFIRST, byte(0x80 + x)); //set x position
    shiftOut(_DIN, _CLK, MSBFIRST, byte(0x40 + y)); //set y position
    digitalWrite(_CE, HIGH);
}

void PCD8544::clear(){
    Xcur = 0; Ycur = 0;
    digitalWrite(_CE, LOW); // CS (Upper score)
    digitalWrite(_DC, LOW); // select Command Mode - A0 set as LOW
    shiftOut(_DIN, _CLK, MSBFIRST, BASIC); // use basic instruction set
    shiftOut(_DIN, _CLK, MSBFIRST, byte(0x80)); //set x position to 0
    shiftOut(_DIN, _CLK, MSBFIRST, byte(0x40)); //set y position to 0
    digitalWrite(_DC, HIGH); // select Data Mode - A0 set to HIGH
        for (int i=0; i<504; i++) shiftOut(_DIN, _CLK, MSBFIRST, 0x00);
    digitalWrite(_CE, HIGH);
    // at this point cursor has cycled back to (0,0)
}

void PCD8544::setDisplayMode(byte value){
    digitalWrite(_CE, LOW); // CS (Upper score)
    digitalWrite(_DC, LOW); // select Command Mode - A0 set as LOW
    shiftOut(_DIN, _CLK, MSBFIRST, BASIC); // use basic instruction set
    shiftOut(_DIN, _CLK, MSBFIRST, value); // set display configuration
    digitalWrite(_CE, HIGH);
}

void PCD8544::setBiasSystem(byte rate){
    digitalWrite(_CE, LOW); // CS (Upper score)
    digitalWrite(_DC, LOW); // select Command Mode - A0 set as LOW
    shiftOut(_DIN, _CLK, MSBFIRST, ADVANCED); // use extended instruction set
    shiftOut(_DIN, _CLK, MSBFIRST, rate); // set bias system
    digitalWrite(_CE, HIGH);
}

// displays a block of data to screen, at top-left-hand position (page, col),
// ie pixel row is pagex8 and pixel column is col.
// the height is pages high (ie pagesx8 pixels) and cols pixels wide.
// the data (in vertical-encoding) is stored in a 1d array of bytes called Arr,
// while the offset for individual bytes (to display) is address which has to be uint16_t
// since the array might have more than 256 elements.
void PCD8544::writeBlock(uint8_t page, uint8_t col, uint8_t pages, uint8_t cols, uint16_t address, const uint8_t Arr[]) {
  uint8_t columnByte;
  digitalWrite(_CE, LOW); // CS (Upper score)
  for (uint8_t j = 0; j < pages; j++) {
    digitalWrite(_DC, LOW); // select Command Mode - A0 set as LOW
    shiftOut(_DIN, _CLK, MSBFIRST, BASIC); // use basic instruction set
    shiftOut(_DIN, _CLK, MSBFIRST, byte(0x80 + col));      //set x position
    shiftOut(_DIN, _CLK, MSBFIRST, byte(0x40 + page + j)); //set y position

    digitalWrite(_DC, HIGH); // Datamode - A0 set to HIGH
    for (uint8_t i = 0; i < cols; i++)  {
      columnByte = pgm_read_byte(&Arr[address + cols*j + i]);
      shiftOut(_DIN, _CLK, MSBFIRST, columnByte);
    }
  }
  digitalWrite(_CE, HIGH);
}

void PCD8544::write8x8Char(uint8_t page, uint8_t column, uint16_t charCode, const uint8_t Arr[][8]) {
  uint8_t columnByte;
  digitalWrite(_CE, LOW); // CS (Upper score)

  digitalWrite(_DC, LOW); // select Command Mode - A0 set as LOW
  shiftOut(_DIN, _CLK, MSBFIRST, BASIC); // use basic instruction set
  shiftOut(_DIN, _CLK, MSBFIRST, byte(0x80 + column)); //set x position
  shiftOut(_DIN, _CLK, MSBFIRST, byte(0x40 + page));   //set y position

  digitalWrite(_DC, HIGH); // Datamode - A0 set to HIGH
  for (uint8_t i = 0; i < 8; i++)  {
    columnByte = pgm_read_byte(&Arr[charCode][i]);
    shiftOut(_DIN, _CLK, MSBFIRST, columnByte);
  }

  digitalWrite(_CE, HIGH);
}

// generate and display formatted string for temperature temp_,
// but for speed only redisplay changed characters.
void PCD8544::Temperature(float temp_, uint8_t page, uint8_t col) {
  int w = 5; // width of text in characters
  if (temp_<=-10.0) {
    dtostrf(temp_,3,0,str_new0);  
    str_new0[3] = 0x00; // degree character
    w--;
  } else {
    dtostrf(temp_,4,1,str_new0);
    str_new0[4] = 0x00; // degree character
  }
  for (int i=0; i<w; i++) {
    if (str_new0[i] != str_old0[i]) {
      write8x8Char(page, col+i*8, str_new0[i], Font8x8_);
      str_old0[i] = str_new0[i]; // after loop finish make str_old0 the current str_new0
    }      
  }
}

// generate and display formatted string for temperature temp_,
// but for speed only redisplay changed characters.
// minimal display using only 2 chars with 0dp. if <=-10 then no sign.
void PCD8544::Temperature_tinyfont(float temp_, uint8_t page, uint8_t col, bool Redraw) {
  if (temp_<=-10.0) temp_ = -1.0 * temp_*-1.0;
  dtostrf(temp_,2,0,str_new0);
  str_new2[2] = 0;  
  for (int i=0; i<2; i++) {
    if ((str_new0[i] != str_old0[i]) || Redraw) {
      writeBlock(page, col+4*i, 1, 4,  ASCII2offset(str_new0[i], 0x0004), FontNums3x5);
      str_old0[i] = str_new0[i]; // after loop finish make str_old0 the current str_new0
    }      
  }
}


// display battery voltage to 2dp, using 8x8 font.
void PCD8544::Battery_smallfont(float battery, uint8_t page, uint8_t col, bool Redraw) {
  dtostrf(battery,4,2,str_new2);
  str_new2[4] = 'v';
  for (int i=0; i<5; i++) {
    if ((str_new2[i] != str_old2[i]) || Redraw) { 
      write8x8Char(page, col+i*8, str_new2[i], Font8x8_);
      str_old2[i] = str_new2[i]; // after loop finish make str_old2 the current str_new2
    }      
  }
}

// display battery*10 voltage to 0dp, using 3x5 font.
void PCD8544::Battery_tinyfont(float battery, uint8_t page, uint8_t col, bool Redraw) {
  dtostrf(battery*10.0,2,0,str_new3);
  for (int i=0; i<2; i++) {
    if ((str_new3[i] != str_old3[i]) || Redraw) {
      writeBlock(page, col+4*i, 1, 4,  ASCII2offset(str_new3[i], 0x0004), FontNums3x5);
      str_old3[i] = str_new3[i]; // after loop finish make str_old3 the current str_new3
    }      
  }
}

// display battery*100 voltage to 0dp, using 3x5 font.
void PCD8544::Battery_tinyfont2(float battery, uint8_t page, uint8_t col, bool Redraw) {
  dtostrf(battery*100.0,3,0,str_new3);
  for (int i=0; i<3; i++) {
    if ((str_new3[i] != str_old3[i]) || Redraw) {
      writeBlock(page, col+4*i, 1, 4,  ASCII2offset(str_new3[i], 0x0004), FontNums3x5);
      str_old3[i] = str_new3[i]; // after loop finish make str_old3 the current str_new3
    }      
  }
}

// display battery raw ADC, using 3x5 font.
void PCD8544::Battery_RawADC(uint16_t adc, uint8_t page, uint8_t col, bool Redraw) {
  itoa(adc,str_new4,10);
  for (int i=0; i<4; i++) {
    if ((str_new4[i] != str_old4[i]) || Redraw) {
      writeBlock(page, col+4*i, 1, 4,  ASCII2offset(str_new4[i], 0x0004), FontNums3x5);
      str_old4[i] = str_new4[i]; // after loop finish make str_old4 the current str_new4
    }      
  }
}

// flash block at battery voltage position if it is too low
void PCD8544::BatteryFlash_tinyfont(bool flashOn, uint8_t page, uint8_t col, bool Redraw) {
  uint16_t charOfs;
  if (flashOn) {
    charOfs = 0x0038; // block
    str_new3[0]=1;
    str_new3[1]=1;
  }
  else {
    charOfs = 0x0028; // blank
    str_new3[0]=2;
    str_new3[1]=2;
  }

  for (int i=0; i<2; i++) {
    if ((str_new3[i] != str_old3[i]) || Redraw) {
      writeBlock(page, col+4*i, 1, 4, charOfs, FontNums3x5);
      str_old3[i] = str_new3[i]; // after loop finish make str_old3 the current str_new3
    }      
  }
}

// generate and display formatted string for altitude, using 16x24 font.
// 3 pages (24 bits) high, In feet, 2 dp, can change position.
// but for speed only redisplay changed characters.
void PCD8544::Altitude_smallfont(float altitude, uint8_t page, uint8_t col, bool Redraw) {
  dtostrf(altitude/1000.0,5,2,str_new);
  for (int i=0; i<5; i++) {
    if ((str_new[i] != str_old[i]) || Redraw) {
      writeBlock(page, col+16*i, 3, 16,  ASCII2offset(str_new[i], 0x0030), FontNums16x24_);
      str_old[i] = str_new[i]; // after loop finish make str_old the current str_new
    }
  }
}     

// generate and display formatted string for altitude, mainly using the 24x48 large font.
// the last two digits for feet and tens of feet use the tiny font
void PCD8544::Altitude_largefont(float altitude, bool Redraw) {
  Serial.println(altitude);
  uint16_t point = 0x0008;
  uint16_t thickMinus = 0x0010;
  str_new1[6] = 'p'; // a bit of a fudge to avoid redrawing custom point
  str_new1[6] = 'p'; // a bit of a fudge to avoid redrawing custom point

  // extracting absolute altitude string and dealing with negative case
  // quite messy since dealing with three different size minus signs.
  boolean isneg = false;
  boolean issml = false;  
  boolean isbig = false;  
  if (altitude<0.0) {
    altitude = -1.0 * altitude;
    isneg = true;
  }    
  dtostrf(altitude,5,0,str_new1);
  if (isneg) {
    if (altitude >= 10000.0) {
      isbig = true;
    } else if (altitude >= 1000.0) {
      isbig = false;
      str_new1[0] = '-';
    } else if (altitude >= 100.0) {
      str_new1[0] = '-';
    } else if (altitude >= 10.0) {
      issml = true;
    } else if (altitude >= 1.0) {
      issml = false;
      str_new1[3] = '-';
    } else {
      str_new1[4] = '0';
    }
  }

  if (altitude < 100.0) {
    point = 0x000C; // make point blank
    str_new1[6] = 'b';
  } else if (altitude < 1000.0) {
    str_new1[1] = '0';    
  }

  if ((str_new1[0] != str_old1[0]) || Redraw) {
    writeBlock(0, 24*0, 6, 24,  ASCII2offset(str_new1[0], 0x0090), FontNums24x48);
    str_old1[0] = str_new1[0]; // after display make str_old1 the current str_new1

    // dealing with negative sign if any in front of 10 thousands digit
    if (isbig) writeBlock(3, 0, 1, 8, thickMinus, Symbols4x8);
  }

  if ((str_new1[1] != str_old1[1]) || Redraw) {
    writeBlock(0, 24*1, 6, 24,  ASCII2offset(str_new1[1], 0x0090), FontNums24x48);
    str_old1[1] = str_new1[1];
  }

  if ((str_new1[6] != str_old1[6]) || Redraw) {
    writeBlock(3, 24*2, 1, 4,  point, Symbols4x8);
    str_old1[6] = str_new1[6];
  }

  if ((str_new1[2] != str_old1[2]) || Redraw) {
    writeBlock(0, 24*2+4, 6, 24,  ASCII2offset(str_new1[2], 0x0090), FontNums24x48);
    str_old1[2] = str_new1[2];
  }

  // dealing with tiny negative sign in fron to two tiny numbers, including fudgy edge cases
  if (issml) writeBlock(5, 72, 1, 4,  ASCII2offset('-', 0x0004), FontNums3x5);
  else writeBlock(5, 72, 1, 4,  ASCII2offset(' ', 0x0004), FontNums3x5);
  
  if ((str_new1[3] != str_old1[3]) || Redraw) {
    writeBlock(5, 76, 1, 4,  ASCII2offset(str_new1[3], 0x0004), FontNums3x5);
    str_old1[3] = str_new1[3];
  }

  if ((str_new1[4] != str_old1[4]) || Redraw) {
    writeBlock(5, 80, 1, 4,  ASCII2offset(str_new1[4], 0x0004), FontNums3x5);
    str_old1[4] = str_new1[4];
  }
  Serial.println(str_new1);
}     

// a private utility function that maps numbers 'only' font chars to memmory offsets
// in font bitmap arrays, needs offsetScale argument to make if useful for different size fonts
// eg. FontNums32x48_ needs offsetScale=192, while FontNums16x24_ needs offsetScale=48
uint16_t PCD8544::ASCII2offset(char char_, uint16_t offsetScale) {
  uint16_t charOfs;
  switch (char_) {
    case '0': charOfs = 0; break;
    case '1': charOfs = 1; break;
    case '2': charOfs = 2; break;
    case '3': charOfs = 3; break;
    case '4': charOfs = 4; break;
    case '5': charOfs = 5; break;
    case '6': charOfs = 6; break;
    case '7': charOfs = 7; break;
    case '8': charOfs = 8; break;
    case '9': charOfs = 9; break;
    case ' ': charOfs = 10; break;
    case '-': charOfs = 11; break;
    case ',': charOfs = 12; break;
    case '.': charOfs = 13; break;
    default: charOfs = 11;
  };
  return charOfs*offsetScale;
} 

size_t PCD8544::write(uint8_t ch) {
  if (ch == 0x0a){ // \n for jumping to the beginning of a new line.
    Xcur = 0; Ycur++; if(Ycur > 5) Ycur = 0;
  }
  else {
    if((8 + Xcur) >= 85) {Xcur = 0; Ycur++; if(Ycur > 5) Ycur = 0;}    
    digitalWrite(_CE, LOW); // CS (Upper score)
    digitalWrite(_DC, LOW); // select Command Mode - A0 set as LOW
    shiftOut(_DIN, _CLK, MSBFIRST, BASIC); // use basic instruction set
    shiftOut(_DIN, _CLK, MSBFIRST, byte(0x80 + Xcur)); //set x position
    shiftOut(_DIN, _CLK, MSBFIRST, byte(0x40 + Ycur)); //set y position
    digitalWrite(_DC, HIGH); // Datamode - A0 set to HIGH
        for (uint8_t i = 0; i < 8; i++) shiftOut(_DIN, _CLK, MSBFIRST, pgm_read_byte(&(Font8x8_[ch][i])));
    digitalWrite(_CE, HIGH);
    Xcur = Xcur + 8; if (Xcur > 83) {Xcur = 0; Ycur++; if(Ycur > 5) Ycur = 0;}
  }
  return 1;
}

void PCD8544::TransferStart(){
    digitalWrite(_CE, LOW); // CS (Upper score)
}

void PCD8544::CommandMode(byte cm){
    digitalWrite(_CE, LOW); // CS (Upper score)
    digitalWrite(_DC, LOW); // select Command Mode - A0 set as LOW
    shiftOut(_DIN, _CLK, MSBFIRST, cm); // select command mode cm
    digitalWrite(_CE, HIGH);
}

void PCD8544::DataMode(){
    digitalWrite(_DC, HIGH); // A0 set to HIGH
}

void PCD8544::WriteByte(byte d){
    digitalWrite(_CE, LOW); // CS (Upper score)
    shiftOut(_DIN, _CLK, MSBFIRST, d);
    digitalWrite(_CE, HIGH);
}

void PCD8544::TransferEnd(){
    digitalWrite(_CE, HIGH);
}

void PCD8544::reset(){
    digitalWrite(_RST, LOW);
    digitalWrite(_RST, HIGH);
}
