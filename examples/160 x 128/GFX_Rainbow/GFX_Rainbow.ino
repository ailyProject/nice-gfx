/*
  An example showing rainbow colours on a 1.8" TFT LCD screen
  and to show a basic example of font use.

  Make sure all the display driver and pin connections are correct by
  editing the Display/Presets/ExampleBoard.h file in the NICE_GFX library folder.

  Note that yield() or delay(0) must be called in long duration for/while
  loops to stop the ESP8266 watchdog triggering.

  #########################################################################
  ###### DON'T FORGET TO UPDATE THE Display/Presets/ExampleBoard.h FILE IN THE LIBRARY ######
  #########################################################################
*/

#include <NICE_GFX.h>
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include <Display/Presets/ExampleBoard.h>
#include <SPI.h>

ExampleDisplay gfx;  // Invoke library, pins in Display/Presets/ExampleBoard.h

unsigned long targetTime = 0;
byte red = 31;
byte green = 0;
byte blue = 0;
byte state = 0;
unsigned int colour = red << 11;

void setup(void) {
  gfx.begin();
  gfx.setFont(nice_gfx::fonts::TomThumb4x6);
  gfx.setRotation(1);
  gfx.fillScreen(GFX_BLACK);

  targetTime = millis() + 1000;
}

void loop() {

  if (targetTime < millis()) {
    targetTime = millis() + 10000;

    // Colour changing state machine
    for (int i = 0; i < 160; i++) {
      gfx.drawFastVLine(i, 0, gfx.height(), colour);
      switch (state) {
        case 0:
          green += 2;
          if (green == 64) {
            green = 63;
            state = 1;
          }
          break;
        case 1:
          red--;
          if (red == 255) {
            red = 0;
            state = 2;
          }
          break;
        case 2:
          blue ++;
          if (blue == 32) {
            blue = 31;
            state = 3;
          }
          break;
        case 3:
          green -= 2;
          if (green == 255) {
            green = 0;
            state = 4;
          }
          break;
        case 4:
          red ++;
          if (red == 32) {
            red = 31;
            state = 5;
          }
          break;
        case 5:
          blue --;
          if (blue == 255) {
            blue = 0;
            state = 0;
          }
          break;
      }
      colour = red << 11 | green << 5 | blue;
    }

    // The TomThumb4x6 bitmap font still works as before
    gfx.setTextColor(GFX_BLACK);
    gfx.setCursor (12, 5);
    gfx.print("TomThumb4x6 font");

    // The new larger fonts do not use the .setCursor call, coords are embedded
    gfx.setTextColor(GFX_BLACK, GFX_BLACK); // Do not plot the background colour

    // Overlay the black text on top of the rainbow plot (the advantage of not drawing the background colour!)
    nice_gfx::text::center(gfx, "GfxTerm8x16", 80, 14, nice_gfx::fonts::GfxTerm8x16, 1); // Draw text centre at position 80, 12 using GfxTerm8x16


    nice_gfx::text::center(gfx, "Spleen12x24", 80, 30, nice_gfx::fonts::Spleen12x24, 1); // Draw text centre at position 80, 24 using Spleen12x24

    nice_gfx::text::center(gfx, "12.34", 80, 54, nice_gfx::fonts::Spleen12x24, 2); // Draw text centre at position 80, 24 using Spleen12x24 at 2x

    nice_gfx::text::center(gfx, "12.34 is in Spleen12x24 at 2x", 80, 92, nice_gfx::fonts::GfxTerm8x16, 1); // Draw text centre at position 80, 90 using GfxTerm8x16

    // Note the x position is the top left of the font!

    // draw a floating point number
    float pi = 3.14159; // Value to print
    int precision = 3;  // Number of digits after decimal point
    int xpos = 50;      // x position
    int ypos = 110;     // y position
    const nice_gfx::MonoFont *font = &nice_gfx::fonts::GfxTerm8x16;
  uint8_t fontScale = 1;       // Select the font descriptor explicitly with setFont().
    xpos += nice_gfx::text::decimal(gfx, pi, precision, xpos, ypos, *font, fontScale); // Draw rounded number and return new xpos delta for next print position
    nice_gfx::text::draw(gfx, " is pi", xpos, ypos, *font, fontScale); // Continue printing from new x position
  }
}






