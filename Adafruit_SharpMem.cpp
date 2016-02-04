/*********************************************************************
This is an Arduino library for our Monochrome SHARP Memory Displays

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/1393

These displays use SPI to communicate, 3 pins are required to  
interface

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.  
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution
*********************************************************************/

#include "Adafruit_SharpMem.h"

#ifndef _swap_int16_t
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#endif
#ifndef _swap_uint16_t
#define _swap_uint16_t(a, b) { uint16_t t = a; a = b; b = t; }
#endif

/**************************************************************************
    Sharp Memory Display Connector
    -----------------------------------------------------------------------
    Pin   Function        Notes
    ===   ==============  ===============================
      1   VIN             3.3-5.0V (into LDO supply)
      2   3V3             3.3V out
      3   GND
      4   SCLK            Serial Clock
      5   MOSI            Serial Data Input
      6   CS              Serial Chip Select
      9   EXTMODE         COM Inversion Select (Low = SW clock/serial)
      7   EXTCOMIN        External COM Inversion Signal
      8   DISP            Display On(High)/Off(Low)

 **************************************************************************/

#define SHARPMEM_BIT_DUMMY      0x00
#define SHARPMEM_BIT_WRITECMD   0x01
#define SHARPMEM_BIT_VCOM       0x02
#define SHARPMEM_BIT_CLEAR      0x04
#define TOGGLE_VCOM             do { _sharpmem_vcom = _sharpmem_vcom ? 0x00 : SHARPMEM_BIT_VCOM; } while(0);

byte sharpmem_buffer[(SHARPMEM_LCDWIDTH * SHARPMEM_LCDHEIGHT) / 8];

// Sharp memory display takes LSB first, but we'll do the byte-flipping in software
// Despite what it says on the datasheet, the display can accept up to 48MHz clock
SPISettings sharpmemSPI(48000000, LSBFIRST, SPI_MODE0);

/* ************* */
/* CONSTRUCTORS  */
/* ************* */

/******************************************************************************/
/*!
    @brief Instantiates a new instance of the Adafruit_SharpMem class

    @param[in]  ss
                The location of the CS pin for the SPI interface
*/
/******************************************************************************/
Adafruit_SharpMem::Adafruit_SharpMem(int8_t ss) :
Adafruit_GFX(SHARPMEM_LCDWIDTH, SHARPMEM_LCDHEIGHT) {
  _ss = ss;
  _mosi = _clk = -1;

  // Set pin state before direction to make sure they start this way (no glitching)
  SPI_CS_DISABLE();
  pinMode(_ss, OUTPUT);
  
  // Set the vcom bit to a defined state
  _sharpmem_vcom = SHARPMEM_BIT_VCOM;
}

/******************************************************************************/
/*!
    @brief Instantiates a new instance of the Adafruit_SharpMem class
           using software SPI

    @param[in]  clk
                The location of the SCK/clock pin for the SPI interface
    @param[in]  mosi
                The location of the MOSI pin for the SPI interface
    @param[in]  ss
                The location of the CS pin for the SPI interface
*/
/******************************************************************************/
Adafruit_SharpMem::Adafruit_SharpMem(int8_t clk, int8_t mosi, int8_t ss) :
Adafruit_GFX(SHARPMEM_LCDWIDTH, SHARPMEM_LCDHEIGHT) {
  _clk = clk;
  _mosi = mosi;
  _ss = ss;

  // Set pin state before direction to make sure they start this way (no glitching)
  SPI_CS_DISABLE();
  digitalWrite(_clk, LOW);
  digitalWrite(_mosi, HIGH);
  
  pinMode(_ss, OUTPUT);
  pinMode(_clk, OUTPUT);
  pinMode(_mosi, OUTPUT);

  // Set the vcom bit to a defined state
  _sharpmem_vcom = SHARPMEM_BIT_VCOM;
}

/******************************************************************************/
/*!
    @brief Initialize the HW to enable communication with the display
*/
/******************************************************************************/
void Adafruit_SharpMem::begin() {
  setRotation(0);

  if (_clk == -1) {
    // setup hardware SPI
    SPI.begin();
  } else {
    // setup software SPI
    clkport     = portOutputRegister(digitalPinToPort(_clk));
    clkpinmask  = digitalPinToBitMask(_clk);
    dataport    = portOutputRegister(digitalPinToPort(_mosi));
    datapinmask = digitalPinToBitMask(_mosi);
  }
}

/******************************************************************************/
/*!
    @brief  Uninitializes the SPI interface
*/
/******************************************************************************/
void Adafruit_SharpMem::end(void)
{
    if (_clk == -1) {
      SPI.end();
    }
}

/* *************** */
/* PRIVATE METHODS */
/* *************** */

/**************************************************************************/
/*!
    @brief  Sends a single byte in pseudo-SPI.
*/
/**************************************************************************/
void Adafruit_SharpMem::sendbyte(uint8_t data) 
{
  // Hardware SPI
  if (_clk == -1) {
    SPI.transfer(data);
    return;
  }

  // Software SPI using original Adafruit code
  uint8_t i = 0;

  // LCD expects LSB first
  for (i=0; i<8; i++) 
  { 
    // Make sure clock starts low
    //digitalWrite(_clk, LOW);
    *clkport &= ~clkpinmask;
    if (data & 0x01) 
      //digitalWrite(_mosi, HIGH);
      *dataport |=  datapinmask;
    else 
      //digitalWrite(_mosi, LOW);
      *dataport &= ~datapinmask;
    // Clock is active high
    //digitalWrite(_clk, HIGH);
    *clkport |=  clkpinmask;
    data >>= 1; 
  }
  // Make sure clock ends low
  //digitalWrite(_clk, LOW);
  *clkport &= ~clkpinmask;
}

/* ************** */
/* PUBLIC METHODS */
/* ************** */

#if defined __AVR__
  // 1<<n is a costly operation on AVR -- table usu. smaller & faster
  static const uint8_t PROGMEM
    set[] = {  1,  2,  4,  8,  16,  32,  64,  128 },
    clr[] = { ~1, ~2, ~4, ~8, ~16, ~32, ~64, ~128 };
#endif

/**************************************************************************/
/*! 
    @brief Draws a single pixel in image buffer

    @param[in]  x
                The x position (0 based)
    @param[in]  y
                The y position (0 based)
*/
/**************************************************************************/
void Adafruit_SharpMem::drawPixel(int16_t x, int16_t y, uint16_t color) 
{
  if((x < 0) || (x >= _width) || (y < 0) || (y >= _height)) return;

  switch(rotation) {
   case 1:
    _swap_int16_t(x, y);
    x = WIDTH  - 1 - x;
    break;
   case 2:
    x = WIDTH  - 1 - x;
    y = HEIGHT - 1 - y;
    break;
   case 3:
    _swap_int16_t(x, y);
    y = HEIGHT - 1 - y;
    break;
   default:
    break;
  }

  #if defined __AVR__
    if(color) {
      sharpmem_buffer[(y*SHARPMEM_LCDWIDTH + x) / 8] |=
        pgm_read_byte(&set[x & 7]);
    } else {
      sharpmem_buffer[(y*SHARPMEM_LCDWIDTH + x) / 8] &=
        pgm_read_byte(&clr[x & 7]);
    }
  #else
    if (color)
      sharpmem_buffer[(y*SHARPMEM_LCDWIDTH + x) /8] |= (1 << x % 8);
    else
      sharpmem_buffer[(y*SHARPMEM_LCDWIDTH + x) /8] &= ~(1 << x % 8);
  #endif
}

/**************************************************************************/
/*! 
    @brief Gets the value (1 or 0) of the specified pixel from the buffer

    @param[in]  x
                The x position (0 based)
    @param[in]  y
                The y position (0 based)

    @return     1 if the pixel is enabled, 0 if disabled
*/
/**************************************************************************/
uint8_t Adafruit_SharpMem::getPixel(uint16_t x, uint16_t y)
{
  if((x >= _width) || (y >= _height)) return 0; // <0 test not needed, unsigned

  switch(rotation) {
   case 1:
    _swap_uint16_t(x, y);
    x = WIDTH  - 1 - x;
    break;
   case 2:
    x = WIDTH  - 1 - x;
    y = HEIGHT - 1 - y;
    break;
   case 3:
    _swap_uint16_t(x, y);
    y = HEIGHT - 1 - y;
    break;
   default:
    break;
  }

  #if defined __AVR__
    return sharpmem_buffer[(y*SHARPMEM_LCDWIDTH + x) / 8] &
      pgm_read_byte(&set[x & 7]) ? 1 : 0;
  #else
    return sharpmem_buffer[(y*SHARPMEM_LCDWIDTH + x) /8] & (1 << x % 8) ? 1 : 0;
  #endif
}

/**************************************************************************/
/*! 
    @brief Clears the screen
*/
/**************************************************************************/
void Adafruit_SharpMem::clearDisplay(void) 
{
  memset(sharpmem_buffer, 0xff, (SHARPMEM_LCDWIDTH * SHARPMEM_LCDHEIGHT) / 8);

  if (_clk == -1)
    SPI.beginTransaction(sharpmemSPI);

  // Send the clear screen command rather than doing a HW refresh (quicker)
  SPI_CS_ENABLE();
  delayMicroseconds(SPI_DEFAULT_DELAY_US);
  sendbyte(_sharpmem_vcom | SHARPMEM_BIT_CLEAR);
  sendbyte(SHARPMEM_BIT_DUMMY);
  TOGGLE_VCOM;
  delayMicroseconds(SPI_DEFAULT_DELAY_US);
  SPI_CS_DISABLE();

  if (_clk == -1)
    SPI.endTransaction();
}

/**************************************************************************/
/*! 
    @brief Clears the buffer, but not the screen
*/
/**************************************************************************/
void Adafruit_SharpMem::clearBuffer(void)
{
  memset(sharpmem_buffer, 0xff, (SHARPMEM_LCDWIDTH * SHARPMEM_LCDHEIGHT) / 8);
}

/**************************************************************************/
/*! 
    @brief Renders the contents of the pixel buffer on the LCD
*/
/**************************************************************************/
void Adafruit_SharpMem::refresh(void) 
{
  uint16_t i, totalbytes, currentline, oldline;  
  totalbytes = (SHARPMEM_LCDWIDTH * SHARPMEM_LCDHEIGHT) / 8;

  if (_clk == -1)
    SPI.beginTransaction(sharpmemSPI);

  // Send the write command
  SPI_CS_ENABLE();
  delayMicroseconds(SPI_DEFAULT_DELAY_US);    
  sendbyte(SHARPMEM_BIT_WRITECMD | _sharpmem_vcom);
  TOGGLE_VCOM;

  // Send the address for line 1
  oldline = currentline = 1;
  sendbyte(currentline);

  // Send image buffer
  for (i=0; i<totalbytes; i++)
  {
    sendbyte(sharpmem_buffer[i]);
    currentline = ((i+1)/(SHARPMEM_LCDWIDTH/8)) + 1;
    if(currentline != oldline)
    {
      // Send end of line and address bytes
      sendbyte(SHARPMEM_BIT_DUMMY);
      if (currentline <= SHARPMEM_LCDHEIGHT)
      {
        sendbyte(currentline);
      }
      oldline = currentline;
    }
  }

  // Send another trailing 8 bits for the last line
  sendbyte(SHARPMEM_BIT_DUMMY);
  delayMicroseconds(SPI_DEFAULT_DELAY_US);    
  SPI_CS_DISABLE();

  if (_clk == -1)
    SPI.endTransaction();
}

void Adafruit_SharpMem::toggleVcom(void)
{
  if (_clk == -1)
  SPI.beginTransaction(sharpmemSPI);

  SPI_CS_ENABLE();
  delayMicroseconds(SPI_DEFAULT_DELAY_US);    
  sendbyte(SHARPMEM_BIT_WRITECMD | _sharpmem_vcom);
  TOGGLE_VCOM;
  sendbyte(SHARPMEM_BIT_DUMMY);
  delayMicroseconds(SPI_DEFAULT_DELAY_US);    
  SPI_CS_DISABLE();

  if (_clk == -1)
  SPI.endTransaction();
}
