Another fork of Adafruit's Sharp Memory LCD library, v1.0.2:
https://github.com/adafruit/Adafruit_SHARP_Memory_Display

Inspired by holachek's fork:
https://github.com/holachek/Adafruit_SHARP_Memory_Display



SPI code was updated to use the latest SPI library (supporting transactions). 

You now have the choice of hardware or software SPI.
Software SPI (bit-banging) code from original Adafruit library.


To use hardware SPI, connect CS, SCK, MOSI pins on microcontroller to display, then define the display object as follows:

/* ...hardware SPI, using SCK/MOSI hardware SPI pins and then user selected CS */
/* Define the CS pin, any GPIO pin can be used */
#define SS 11
Adafruit_SharpMem display(SS);



To use software SPI, connect your choice of microcontroller pins to display, then define the display object as follows:

/* ...software SPI, using SCK/MOSI/MISO user-defined SPI pins and then user selected CS */
/* Define the SCK, MOSI and SS pins */
#define SCK 24
#define MOSI 23
#define SS 11
Adafruit_SharpMem display(SCK, MOSI, SS);



All original credit goes to Adafruit.