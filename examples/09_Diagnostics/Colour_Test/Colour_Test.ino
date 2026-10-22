
//   Diagnostic test for the displayed colour order
//
// Written by Bodmer 17/2/19 for the NICE_GFX library:
/* 
 Different hardware manufacturers use different colour order
 configurations at the hardware level.  This may result in
 incorrect colours being displayed.

 Incorrectly displayed colours could also be the result of
 using the wrong display driver in the library setup file.

 Typically displays have a control register (MADCTL) that can
 be used to set the Red Green Blue (RGB) colour order to RGB
 or BRG so that red and blue are swapped on the display.

 This control register is also used to manage the display
 rotation and coordinate mirroring. The control register
 typically has 8 bits, for the ILI9341 these are:

 Bit Function
 7   Mirror Y coordinate (row address order)
 6   Mirror X coordinate (column address order)
 5   Row/column exchange (for rotation)
 4   Refresh direction (top to bottom or bottom to top in portrait orientation)
 3   RGB order (swaps red and blue)
 2   Refresh direction (top to bottom or bottom to top in landscape orientation)
 1   Not used
 0   Not used

 The control register bits can be written with this example command sequence:
 
    gfx.writecommand(GFX_MADCTL);
    gfx.writedata(0x48);          // Bits 6 and 3 set
    
 0x48 is the default value for ILI9341 (0xA8 for ESP32 M5STACK)
 in rotation 0 orientation.
 
 Another control register can be used to "invert" colours,
 this swaps black and white as well as other colours (e.g.
 green to magenta, red to cyan, blue to yellow).
 
 To invert colours insert this line after gfx.begin() or gfx.begin():

    gfx.invertDisplay( invert ); // Where invert is true or false

*/

#include <SPI.h>

#include <NICE_GFX.h>
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include "Hardware.h"

ExampleDisplay gfx;  // Invoke custom library

void setup(void) {
  gfx.begin();

  gfx.fillScreen(GFX_BLACK);
  gfx.drawRect(0, 0, gfx.width(), gfx.height(), GFX_GREEN);

  // Set "cursor" at top left corner of display (0,0) and select Spleen12x24
  nice_gfx::text::cursor(gfx, 0, 4, nice_gfx::fonts::Spleen12x24, 1);

  // Set the font colour to be white with a black background
  gfx.setTextColor(GFX_WHITE);

  // We can now plot text on screen using the "print" class
  gfx.println(" Initialised default\n");
  gfx.println(" White text");
  
  gfx.setTextColor(GFX_RED);
  gfx.println(" Red text");
  
  gfx.setTextColor(GFX_GREEN);
  gfx.println(" Green text");
  
  gfx.setTextColor(GFX_BLUE);
  gfx.println(" Blue text");

  delay(5000);

}

void loop() {

  gfx.invertDisplay( false ); // Where i is true or false
 
  gfx.fillScreen(GFX_BLACK);
  gfx.drawRect(0, 0, gfx.width(), gfx.height(), GFX_GREEN);

  nice_gfx::text::cursor(gfx, 0, 4, nice_gfx::fonts::Spleen12x24, 1);

  gfx.setTextColor(GFX_WHITE);
  gfx.println(" Invert OFF\n");

  gfx.println(" White text");
  
  gfx.setTextColor(GFX_RED);
  gfx.println(" Red text");
  
  gfx.setTextColor(GFX_GREEN);
  gfx.println(" Green text");
  
  gfx.setTextColor(GFX_BLUE);
  gfx.println(" Blue text");

  delay(5000);


  // Binary inversion of colours
  gfx.invertDisplay( true ); // Where i is true or false
 
  gfx.fillScreen(GFX_BLACK);
  gfx.drawRect(0, 0, gfx.width(), gfx.height(), GFX_GREEN);

  nice_gfx::text::cursor(gfx, 0, 4, nice_gfx::fonts::Spleen12x24, 1);

  gfx.setTextColor(GFX_WHITE);
  gfx.println(" Invert ON\n");

  gfx.println(" White text");
  
  gfx.setTextColor(GFX_RED);
  gfx.println(" Red text");
  
  gfx.setTextColor(GFX_GREEN);
  gfx.println(" Green text");
  
  gfx.setTextColor(GFX_BLUE);
  gfx.println(" Blue text");

  delay(5000);
}
