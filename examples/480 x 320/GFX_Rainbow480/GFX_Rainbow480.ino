/*
 An example showing rainbow colours on a 3.0 or 3.2" TFT LCD screen
 and to show basic examples of font use.

 This sketch uses explicit MonoFont descriptors and integer text scaling.

 Make sure all the required fonts are loaded by editing the
 Display/Presets/ExampleBoard.h file in the NICE_GFX library folder.


  #########################################################################
  ###### DON'T FORGET TO UPDATE THE Display/Presets/ExampleBoard.h FILE IN THE LIBRARY ######
  ######           TO SELECT THE FONTS AND PINS YOU USE              ######
  #########################################################################
 */

#include <SPI.h>

#include <NICE_GFX.h>
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include <Display/Presets/ExampleBoard.h>

ExampleDisplay gfx;       // Invoke custom library

unsigned long targetTime = 0;
byte red = 31;
byte green = 0;
byte blue = 0;
byte state = 0;
unsigned int colour = red << 11; // Colour order is RGB 5+6+5 bits each

void setup(void) {
  gfx.begin();
  gfx.setRotation(2);
  gfx.fillScreen(GFX_BLACK);

  targetTime = millis() + 1000;
}

void loop() {

  if (targetTime < millis()) {
    targetTime = millis() + 10000;

    rainbow_fill(); // Fill the screen with rainbow colours

    // The TomThumb4x6 bitmap font still works as before
    gfx.setTextColor(GFX_BLACK); // Background is not defined so it is transparent
    gfx.setCursor (100, 5);
    nice_gfx::text::select(gfx, nice_gfx::fonts::TomThumb4x6, 1);        // Select the font descriptor explicitly with setFont().
    gfx.print("TomThumb4x6 font");

    // The new larger fonts do not need to use the .setCursor call, coords are embedded
    gfx.setTextColor(GFX_BLACK); // Do not plot the background colour
    // Overlay the black text on top of the rainbow plot (the advantage of not drawing the background colour!)
    nice_gfx::text::center(gfx, "GfxTerm8x16", 160, 14, nice_gfx::fonts::GfxTerm8x16, 1); // Draw text centre at position 120, 14 using GfxTerm8x16
    nice_gfx::text::center(gfx, "Spleen12x24", 160, 30, nice_gfx::fonts::Spleen12x24, 1); // Draw text centre at position 120, 30 using Spleen12x24
    nice_gfx::text::center(gfx, "12.34", 160, 54, nice_gfx::fonts::Spleen12x24, 2);       // Draw text centre at position 120, 54 using Spleen12x24 at 2x
    nice_gfx::text::center(gfx, "12.34 is in Spleen12x24 at 2x", 160, 92, nice_gfx::fonts::GfxTerm8x16, 1); // Draw text centre at position 120, 92 using GfxTerm8x16
    // Note the x, y position is the top left corner of the first character printed!

    // draw a floating point number
    float pi = 3.14159; // Value to print
    int precision = 3;  // Number of digits after decimal point
    int xpos = 130;     // x position
    int ypos = 110;     // y position
    const nice_gfx::MonoFont *font = &nice_gfx::fonts::GfxTerm8x16;
  uint8_t fontScale = 1;       // font number 2
    xpos += nice_gfx::text::decimal(gfx, pi, precision, xpos, ypos, *font, fontScale); // Draw rounded number and return new xpos delta for next print position
    nice_gfx::text::draw(gfx, " is pi", xpos, ypos, *font, fontScale);             // Continue printing from new x position

    gfx.setTextSize(1);           // We are using a text size multiplier of 1

    gfx.setTextColor(GFX_BLACK);  // Set text colour to black, no background (so transparent)
    nice_gfx::text::cursor(gfx, 76, 150, nice_gfx::fonts::Spleen12x24, 1);    // Set cursor to x = 76, y = 150 and use Spleen12x24
    gfx.println("Transparent...");  // As we use println, the cursor moves to the next line

    gfx.setCursor(70, 175);    // Set cursor to x = 70, y = 175
    gfx.setTextColor(GFX_WHITE, GFX_BLACK);  // Set text colour to white and background to black
    gfx.println("White on black");

    nice_gfx::text::select(gfx, nice_gfx::fonts::Spleen12x24, 1);        // Select Spleen12x24 without moving cursor
    gfx.setCursor(00, 210);    // Set cursor to x = 90, y = 210 without changing the font
    gfx.setTextColor(GFX_WHITE);
    
    // By using the print class we can use all the formatting features like printing HEX
    gfx.print(57005, HEX);    // Cursor does no move to next line
    gfx.println(48879, HEX);  // print and move cursor to next line

    gfx.setTextColor(GFX_GREEN, GFX_BLACK); // This time we will use green text on a black background
    nice_gfx::text::select(gfx, nice_gfx::fonts::GfxTerm8x16, 1); // Select GfxTerm8x16
    //Text will wrap to the next line if needed, by luck it breaks the lines at a space..
    gfx.println(" Ode to a Small Lump of Green Putty I Found in My Armpit One Midsummer Morning ");

    nice_gfx::text::center(gfx, "34.56", 160, 300, nice_gfx::fonts::Spleen12x24, 2);       // Draw text centre at position 120, 54 using Spleen12x24 at 2x
    nice_gfx::text::center(gfx, "34.56 is in Spleen12x24 at 2x", 160, 350, nice_gfx::fonts::GfxTerm8x16, 1); // Draw text centre at position 120, 92 using GfxTerm8x16

    nice_gfx::text::center(gfx, "78.90", 160, 370, nice_gfx::fonts::Spleen12x24, 3);       // Draw text centre at position 120, 54 using Spleen12x24 at 2x
    nice_gfx::text::center(gfx, "78.90 is in Spleen12x24 at 3x", 160, 450, nice_gfx::fonts::GfxTerm8x16, 1); // Draw text centre at position 120, 92 using GfxTerm8x16
  }
}

// Fill screen with a rainbow pattern
void rainbow_fill()
{
  // The colours and state are not initialised so the start colour changes each time the function is called
  
  for (int i = 479; i > 0; i--) {
    // Draw a vertical line 1 pixel wide in the selected colour
    gfx.drawFastHLine(0, i, gfx.width(), colour); // in this example gfx.width() returns the pixel width of the display
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
  }
}



