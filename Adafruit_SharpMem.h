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

#ifndef _ADAFRUIT_SHARPMEM_H_
#define _ADAFRUIT_SHARPMEM_H_

#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif
 
#ifdef __AVR
  #include <avr/pgmspace.h>
#elif defined(ESP8266)
  #include <pgmspace.h>
#endif

#include <Adafruit_GFX.h>
#include <SPI.h>

#define SPI_CS_ENABLE()           digitalWrite(_ss, HIGH)
#define SPI_CS_DISABLE()          digitalWrite(_ss, LOW)		// Sharp memory display uses opposite CS polarity: LOW = de-assert, HIGH = assert
#define SPI_DEFAULT_DELAY_US      6

#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL) 
#define SerialDebug SERIAL_PORT_USBVIRTUAL
#else
#define SerialDebug Serial
#endif

// LCD Dimensions
#define SHARPMEM_LCDWIDTH       (96)
#define SHARPMEM_LCDHEIGHT      (96) 

class Adafruit_SharpMem : public Adafruit_GFX {
 public:
  // Constructor
  Adafruit_SharpMem(int8_t ss);
  Adafruit_SharpMem(int8_t clk, int8_t mosi, int8_t ss);
  
  // HW initialisation
  void begin(void);
  void end(void);
  
  // Methods
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  uint8_t getPixel(uint16_t x, uint16_t y);
  void clearDisplay(void);
  void clearBuffer(void);
  void refresh(void);
  void toggleVcom(void);

 private:
  int8_t _ss, _clk, _mosi, _sharpmem_vcom;

#if defined __AVR__
  // 8-bit processors
  volatile uint8_t *dataport, *clkport;
  uint8_t datapinmask, clkpinmask;
#else
  // 32-bit processors
  volatile uint32_t *dataport, *clkport;
  uint32_t datapinmask, clkpinmask;
#endif

  void sendbyte(uint8_t data);
};

#endif /* _ADAFRUIT_SHARPMEM_H_ */
