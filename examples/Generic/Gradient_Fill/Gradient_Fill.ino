/*
  This sketch demonstrates the use of the horizontal and vertical gradient
  rectangle fill functions.

  Example for library:
  Created by Bodmer 27/1/22
*/

#include <NICE_GFX.h>
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include "Hardware.h"
ExampleDisplay gfx;  // Create object "gfx"

// -------------------------------------------------------------------------
// Setup
// -------------------------------------------------------------------------
void setup(void) {
  gfx.begin();
  gfx.setRotation(1);
  gfx.fillScreen(GFX_DARKGREY);
  nice_gfx::text::select(gfx, nice_gfx::fonts::GfxTerm8x16, 1);
}

// -------------------------------------------------------------------------
// Main loop
// -------------------------------------------------------------------------
void loop()
{
  gfx.fillRectHGradient(0, 0, 160, 50, GFX_MAGENTA, GFX_BLUE);
  gfx.setCursor(10,10);
  gfx.print("Horizontal gradient");

  gfx.fillRectVGradient(0, 60, 160, 50, GFX_ORANGE, GFX_RED);
  gfx.setCursor(10,70);
  gfx.print("Vertical gradient");

  while(1) delay(100); // Wait here
}
