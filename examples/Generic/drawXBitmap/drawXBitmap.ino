// Example sketch to demonstrate the drawing of X BitMap (XBM)
// format image onto the display.

// Information on the X BitMap (XBM) format can be found here:
// https://en.wikipedia.org/wiki/X_BitMap

// This example is part of the NICE_GFX library:
// Created by Bodmer 23/04/18

#include "xbm.h"             // Sketch tab header for xbm images

#include <NICE_GFX.h>        // Hardware-specific library
#include <Display/Presets/ExampleBoard.h>

ExampleDisplay gfx;   // Invoke library


void setup()
{
  gfx.begin();               // Initialise the display
  gfx.fillScreen(GFX_BLACK); // Black screen fill
}

void loop()
{

  // Example 1
  // =========
  // Random x and y coordinates
  int x = random(gfx.width()  - logoWidth);
  int y = random(gfx.height() - logoHeight);

  // Draw bitmap with top left corner at x,y with foreground only color
  // Bits set to 1 plot as the defined color, bits set to 0 are not plotted
  //              x  y  xbm   xbm width  xbm height  color
  gfx.drawXBitmap(x, y, logo, logoWidth, logoHeight, GFX_WHITE);

  delay(500);

  // Erase old one by drawing over with background colour
  gfx.drawXBitmap(x, y, logo, logoWidth, logoHeight, GFX_BLACK);


  // Example 2
  // =========
  // New random x and y coordinates
  x = random(gfx.width()  - logoWidth);
  y = random(gfx.height() - logoHeight);

  // Draw bitmap with top left corner at x,y with foreground and background colors
  // Bits set to 1 plot as the defined fg color, bits set to 0 are plotted as bg color
  //              x  y  xbm   xbm width  xbm height  fg color   bg color
  gfx.drawXBitmap(x, y, logo, logoWidth, logoHeight, GFX_WHITE, GFX_RED);

  delay(500);

  // Erase old one by drawing over with background colour
  gfx.drawXBitmap(x, y, logo, logoWidth, logoHeight, GFX_BLACK, GFX_BLACK);

}
