/*
  This tests the alpha blending function that is used with the anti-aliased
  fonts:

  Alpha = 0 = 100% background, alpha = 255 = 100% foreground colour

  blendedColor = gfx.alphaBlend(alpha, fg_color, bg_color);

  The alphaBlend() function operates on 16-bit colours only
  A test is included where the colours are mapped to 8 bits after blending

  Information on alpha blending is here
  https://en.wikipedia.org/wiki/Alpha_compositing
  
  Example for library:
  The sketch has been tested on a 320x240 ILI9341 based TFT, it
  could be adapted for other screen sizes.

  Created by Bodmer 10/2/18

  #########################################################################
  ###### DON'T FORGET TO UPDATE THE Hardware.h FILE IN THIS SKETCH ######
  #########################################################################
*/

#include <NICE_GFX.h>       // Include the graphics library
#include "Hardware.h"

ExampleDisplay gfx;  // Create object "gfx"

// -------------------------------------------------------------------------
// Setup
// -------------------------------------------------------------------------
void setup(void) {
  gfx.begin();
  gfx.setRotation(0);
  gfx.fillScreen(GFX_DARKGREY);
}

// -------------------------------------------------------------------------
// Main loop
// -------------------------------------------------------------------------
void loop()
{
  // 16-bit colours  (5 bits red, 6 bits green, 5 bits blue)
  // Blend from white to full spectrum
  for (int a = 0; a < 256; a+=2) // Alpha 0 = 100% background, alpha 255 = 100% foreground
  {
    for (int c = 0; c < 192; c++) gfx.drawPixel(c, a/2, gfx.alphaBlend(a, rainbow(c), GFX_WHITE));
  }

  // Blend from full spectrum to black
  for (int a = 255; a > 2; a-=2)
  {
    for (int c = 0; c < 192; c++) gfx.drawPixel(c, 128 + (255-a)/2, gfx.alphaBlend(a, rainbow(c), GFX_BLACK));
  }

  // Blend from white to black (32 grey levels)
  for (uint16_t a = 0; a < 255; a++) // Alpha 0 = 100% background, alpha 255 = 100% foreground
  {
    gfx.drawFastHLine(192, a, 12, gfx.alphaBlend(a, GFX_BLACK, GFX_WHITE));
    gfx.drawFastHLine(204, a, 12, gfx.alphaBlend(a, GFX_BLACK, GFX_RED));
    gfx.drawFastHLine(216, a, 12, gfx.alphaBlend(a, GFX_BLACK, GFX_GREEN));
    gfx.drawFastHLine(228, a, 12, gfx.alphaBlend(a, GFX_BLACK, GFX_BLUE));
  }

  delay(4000);

  // Blend from white to colour (32 grey levels)
  for (uint16_t a = 0; a < 255; a++) // Alpha 0 = 100% background, alpha 255 = 100% foreground
  {
    //gfx.drawFastHLine(192, a, 12, gfx.alphaBlend(a, GFX_BLACK, GFX_WHITE));
    gfx.drawFastHLine(204, a, 12, gfx.alphaBlend(a, GFX_RED,   GFX_WHITE));
    gfx.drawFastHLine(216, a, 12, gfx.alphaBlend(a, GFX_GREEN, GFX_WHITE));
    gfx.drawFastHLine(228, a, 12, gfx.alphaBlend(a, GFX_BLUE,  GFX_WHITE));
  }

  delay(4000);

  //*
  // Decrease to 8-bit colour (3 bits red, 3 bits green, 2 bits blue)
  // Blend from white to full spectrum
  for (int a = 0; a < 256; a+=2) // Alpha 0 = 100% background, alpha 255 = 100% foreground
  {
    // Convert blended 16-bit colour to 8 bits to reduce colour resolution, then map back to 16 bits for displaying
    for (int c = 0; c < 192; c++) gfx.drawPixel(c, a/2, gfx.color8to16(gfx.color16to8(gfx.alphaBlend(a, rainbow(c), 0xFFFF))));
  }

  // Blend from full spectrum to black
  for (int a = 255; a > 2; a-=2)
  {
    // Convert blended 16-bit colour to 8 bits to reduce colour resolution, then map back to 16 bits for displaying
    for (int c = 0; c < 192; c++) gfx.drawPixel(c, 128 + (255-a)/2, gfx.color8to16(gfx.color16to8(gfx.alphaBlend(a, rainbow(c), 0))));
  }

  // Blend from white to black (4 grey levels - it will draw 4 more with a blue tinge due to lower blue bit count)
  // Blend from black to a primary colour
  for (uint16_t a = 0; a < 255; a++) // Alpha 0 = 100% background, alpha 255 = 100% foreground
  {
    gfx.drawFastHLine(192, a, 12, gfx.color8to16(gfx.color16to8(gfx.alphaBlend(a, GFX_BLACK, GFX_WHITE))));
    gfx.drawFastHLine(204, a, 12, gfx.color8to16(gfx.color16to8(gfx.alphaBlend(a, GFX_BLACK, GFX_RED))));
    gfx.drawFastHLine(216, a, 12, gfx.color8to16(gfx.color16to8(gfx.alphaBlend(a, GFX_BLACK, GFX_GREEN))));
    gfx.drawFastHLine(228, a, 12, gfx.color8to16(gfx.color16to8(gfx.alphaBlend(a, GFX_BLACK, GFX_BLUE))));
  }

  delay(4000);
  //*/

  /*
  // 16-bit colours  (5 bits red, 6 bits green, 5 bits blue)
  for (int a = 0; a < 256; a+=2) // Alpha 0 = 100% background, alpha 255 = 100% foreground
  {
    for (int c = 0; c < 192; c++) gfx.drawPixel(c, a/2, gfx.alphaBlend(a, rainbow(c), GFX_CYAN));
  }

  // Blend from full spectrum to cyan
  for (int a = 255; a > 2; a-=2)
  {
    for (int c = 0; c < 192; c++) gfx.drawPixel(c, 128 + (255-a)/2, gfx.alphaBlend(a, rainbow(c), GFX_YELLOW));
  }
  //*/

  /*
  // Blend other colour transitions for test purposes
  for (uint16_t a = 0; a < 255; a++) // Alpha 0 = 100% background, alpha 255 = 100% foreground
  {
    gfx.drawFastHLine(192, a, 12, gfx.alphaBlend(a, GFX_WHITE,  GFX_WHITE));  // Should show as solid white
    gfx.drawFastHLine(204, a, 12, gfx.alphaBlend(a, GFX_BLACK,  GFX_BLACK));  // Should show as solid black
    gfx.drawFastHLine(216, a, 12, gfx.alphaBlend(a, GFX_YELLOW, GFX_CYAN));   // Brightness should be fairly even
    gfx.drawFastHLine(228, a, 12, gfx.alphaBlend(a, GFX_CYAN,   GFX_MAGENTA));// Brightness should be fairly even
  }

  delay(4000);
  //*/
}


// #########################################################################
// Return a 16-bit rainbow colour
// #########################################################################
unsigned int rainbow(byte value)
{
  // If 'value' is in the range 0-159 it is converted to a spectrum colour
  // from 0 = red through to 127 = blue to 159 = violet
  // Extending the range to 0-191 adds a further violet to red band
 
  value = value%192;
  
  byte red   = 0; // Red is the top 5 bits of a 16-bit colour value
  byte green = 0; // Green is the middle 6 bits, but only top 5 bits used here
  byte blue  = 0; // Blue is the bottom 5 bits

  byte sector = value >> 5;
  byte amplit = value & 0x1F;

  switch (sector)
  {
    case 0:
      red   = 0x1F;
      green = amplit; // Green ramps up
      blue  = 0;
      break;
    case 1:
      red   = 0x1F - amplit; // Red ramps down
      green = 0x1F;
      blue  = 0;
      break;
    case 2:
      red   = 0;
      green = 0x1F;
      blue  = amplit; // Blue ramps up
      break;
    case 3:
      red   = 0;
      green = 0x1F - amplit; // Green ramps down
      blue  = 0x1F;
      break;
    case 4:
      red   = amplit; // Red ramps up
      green = 0;
      blue  = 0x1F;
      break;
    case 5:
      red   = 0x1F;
      green = 0;
      blue  = 0x1F - amplit; // Blue ramps down
      break;
  }

  return red << 11 | green << 6 | blue;
}


