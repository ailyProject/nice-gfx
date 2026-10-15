/*
  This sketch has been written to test the Processing screenshot client.

  It has been created to work with the NICE_GFX library here:
  It sends screenshots to a PC running a Processing client sketch.

  The Processing IDE that will run the client sketch can be downloaded
  here:  https://processing.org/

  The Processing sketch needed is contained within a tab attached to this
  Arduino sketch. Cut and paste that tab into the Processing IDE and run.
  Read the Processing sketch header for instructions.

  This sketch uses explicit MonoFont descriptors and integer text scaling.

  Make sure all the display driver and pin connections are correct by
  editing the Display/Presets/ExampleBoard.h file in the NICE_GFX library folder.

  Maximum recommended SPI clock rate is 27MHz when reading pixels, 40MHz
  seems to be OK with ILI9341 displays but this is above the manufacturers
  specified maximum clock rate.

  In the setup file you can define different write and read SPI clock rates
  
  In the setup file you can define GFX_SDA_READ for a TFT with bi-directional
  SDA pin (otherwise the normal MISO pin will be used to read from the TFT)

  >>>>   NOTE: NOT ALL TFTs SUPPORT READING THE CGRAM (pixel) MEMORY   <<<<

  #########################################################################
  ###### DON'T FORGET TO UPDATE THE Display/Presets/ExampleBoard.h FILE IN THE LIBRARY ######
  #########################################################################
*/

// Created by: Bodmer  5/3/17
// Updated by: Bodmer 10/3/17
// Updated by: Bodmer 23/11/18 to support SDA reads and the ESP32
// Version: 0.07

// MIT licence applies, all text above must be included in derivative works

#include <NICE_GFX.h>
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include <Display/Presets/ExampleBoard.h>
#include <SPI.h>

ExampleDisplay gfx; // Invoke custom library

unsigned long targetTime = 0;
byte red = 0x1F;
byte green = 0;
byte blue = 0;
byte state = 0;
unsigned int colour = red << 11; // Colour order is RGB 5+6+5 bits each

void setup(void) {
  Serial.begin(921600); // Set to a high rate for fast image transfer to a PC

  gfx.begin();
  gfx.setRotation(0);
  gfx.fillScreen(GFX_BLACK);

  randomSeed(analogRead(A0));

  targetTime = millis() + 1000;
}

#define RGB_TEST false // true produces a simple RGB color test screen

void loop() {

  if (targetTime < millis()) {
    if (!RGB_TEST)
    {
      targetTime = millis() + 1500; // Wait a minimum of 1.5s

      gfx.setRotation(random(4));
      rainbow_fill(); // Fill the screen with rainbow colours

      gfx.setTextColor(GFX_BLACK); // Text background is not defined so it is transparent
      gfx.setTextDatum(TC_DATUM);  // Top Centre datum
      int xpos = gfx.width() / 2;  // Centre of screen

      nice_gfx::text::select(gfx, nice_gfx::fonts::TomThumb4x6, 1);        // Select font 0 which is the TomThumb4x6 bitmap font
      gfx.drawString("TomThumb4x6 font", xpos, 5);

      // The new larger fonts do not need to use the .setCursor call, coords are embedded
      gfx.setTextColor(GFX_BLACK); // Do not plot the background colour

      // Overlay the black text on top of the rainbow plot (the advantage of not drawing the background colour!)
      nice_gfx::text::draw(gfx, "GfxTerm8x16", xpos, 14, nice_gfx::fonts::GfxTerm8x16, 1); // Draw text centre at position xpos, 14 using GfxTerm8x16
      nice_gfx::text::draw(gfx, "Spleen12x24", xpos, 30, nice_gfx::fonts::Spleen12x24, 1); // Draw text centre at position xpos, 30 using Spleen12x24
      nice_gfx::text::draw(gfx, "12.34", xpos, 54, nice_gfx::fonts::Spleen12x24, 2);       // Draw text centre at position xpos, 54 using Spleen12x24 at 2x

      nice_gfx::text::draw(gfx, "12.34 is in Spleen12x24 at 2x", xpos, 92, nice_gfx::fonts::GfxTerm8x16, 1); // Draw text centre at position xpos, 92 using GfxTerm8x16
      // Note the x position is the top of the font!

      // draw a floating point number
      float pi = 3.1415926; // Value to print
      int precision = 3;    // Number of digits after decimal point

      int ypos = 110;     // y position

      gfx.setTextDatum(TR_DATUM); // Top Right datum so text butts neatly to xpos (right justified)

      nice_gfx::text::decimal(gfx, pi, precision, xpos, ypos, nice_gfx::fonts::GfxTerm8x16, 1); // Draw rounded number and return new xpos delta for next print position

      gfx.setTextDatum(TL_DATUM);  // Top Left datum so text butts neatly to xpos (left justified)

      nice_gfx::text::draw(gfx, " is pi", xpos, ypos, nice_gfx::fonts::GfxTerm8x16, 1);

      gfx.setTextSize(1);           // We are using a font size multiplier of 1
      gfx.setTextDatum(TC_DATUM);   // Top Centre datum
      gfx.setTextColor(GFX_BLACK);  // Set text colour to black, no background (so transparent)

      nice_gfx::text::draw(gfx, "Transparent...", xpos, 125, nice_gfx::fonts::Spleen12x24, 1);  // Spleen12x24

      gfx.setTextColor(GFX_WHITE, GFX_BLACK);          // Set text colour to white and background to black
      nice_gfx::text::draw(gfx, "White on black", xpos, 150, nice_gfx::fonts::Spleen12x24, 1);  // Spleen12x24

      gfx.setTextColor(GFX_GREEN, GFX_BLACK); // This time we will use green text on a black background

      nice_gfx::text::select(gfx, nice_gfx::fonts::GfxTerm8x16, 1); // Select GfxTerm8x16, now we do not need to specify the font in drawString()

      // An easier way to position text and blank old text is to set the datum and use width padding
      gfx.setTextDatum(BC_DATUM);          // Bottom centre for text datum
      gfx.setTextPadding(gfx.width() + 1); // Pad text to full screen width + 1 spare for +/-1 position rounding

      gfx.drawString("Ode to a Small Lump of Green Putty", xpos, 230 - 32);
      gfx.drawString("I Found in My Armpit One Midsummer", xpos, 230 - 16);
      gfx.drawString("Morning", xpos, 230);

      gfx.setTextDatum(TL_DATUM); // Reset to top left for text datum
      gfx.setTextPadding(0);      // Reset text padding to 0 pixels

      // Now call the screen server to send a copy of the TFT screen to the PC running the Processing client sketch
      screenServer();
    }
    else
    {
      gfx.fillScreen(GFX_BLACK);
      gfx.fillRect( 0, 0, 16, 16, GFX_RED);
      gfx.fillRect(16, 0, 16, 16, GFX_GREEN);
      gfx.fillRect(32, 0, 16, 16, GFX_BLUE);
      screenServer();
    }
  }
}

// Fill screen with a rainbow pattern
void rainbow_fill()
{
  // The colours and state are not initialised so the start colour changes each time the function is called
  int rotation = gfx.getRotation();
  gfx.setRotation(random(4));
  for (int i = gfx.height() - 1; i >= 0; i--) {
    // This is a "state machine" that ramps up/down the colour brightnesses in sequence
    switch (state) {
      case 0:
        green ++;
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
        green --;
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
    // Draw a line 1 pixel wide in the selected colour
    gfx.drawFastHLine(0, i, gfx.width(), colour); // gfx.width() returns the pixel width of the display
  }
  gfx.setRotation(rotation);
}
