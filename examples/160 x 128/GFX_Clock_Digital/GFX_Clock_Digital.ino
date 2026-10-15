/*
 An example digital clock using a TFT LCD screen to show the time.
 Demonstrates use of the font printing routines. (Time updates but date does not.)
 
 For a more accurate clock, it would be better to use the RTClib library.
 But this is just a demo. 
 
 This examples uses the hardware SPI only. Non-hardware SPI
 is just too slow (~8 times slower!)
 
 Based on clock sketch by Gilchrist 6/2/2014 1.0
 Updated by Bodmer
A few colour codes:
 
code	color
0x0000	Black
0xFFFF	White
0xBDF7	Light Gray
0x7BEF	Dark Gray
0xF800	Red
0xFFE0	Yellow
0xFBE0	Orange
0x79E0	Brown
0x7E0	Green
0x7FF	Cyan
0x1F	Blue
0xF81F	Pink

 */

#include <NICE_GFX.h>
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include <Display/Presets/ExampleBoard.h>
#include <SPI.h>

ExampleDisplay gfx;  // Invoke library, pins in Display/Presets/ExampleBoard.h

uint32_t targetTime = 0;       // for next 1 second timeout

byte omm = 99;
bool initial = 1;
byte xcolon = 0;
unsigned int colour = 0;

static uint8_t conv2d(const char* p) {
  uint8_t v = 0;
  if ('0' <= *p && *p <= '9')
    v = *p - '0';
  return 10 * v + *++p - '0';
}

uint8_t hh=conv2d(__TIME__), mm=conv2d(__TIME__+3), ss=conv2d(__TIME__+6);  // Get H, M, S from compile time

void setup(void) {
  gfx.begin();
  gfx.setFont(nice_gfx::fonts::TomThumb4x6);
  gfx.setRotation(1);
  gfx.fillScreen(GFX_BLACK);

  gfx.setTextColor(GFX_YELLOW, GFX_BLACK); // Note: the new fonts do not draw the background colour

  targetTime = millis() + 1000; 
}

void loop() {
  if (targetTime < millis()) {
    targetTime = millis()+1000;
    ss++;              // Advance second
    if (ss==60) {
      ss=0;
      omm = mm;
      mm++;            // Advance minute
      if(mm>59) {
        mm=0;
        hh++;          // Advance hour
        if (hh>23) {
          hh=0;
        }
      }
    }

    if (ss==0 || initial) {
      initial = 0;
      gfx.setTextColor(GFX_GREEN, GFX_BLACK);
      gfx.setCursor (8, 52);
      gfx.print(__DATE__); // This uses the standard ADAFruit small font

      gfx.setTextColor(GFX_BLUE, GFX_BLACK);
      nice_gfx::text::center(gfx, "It is windy", 120, 48, nice_gfx::fonts::GfxTerm8x16, 1); // Next size up GfxTerm8x16

      //gfx.setTextColor(0xF81F, GFX_BLACK); // Pink
    }

    // Update digital time
    byte xpos = 6;
    byte ypos = 0;
    if (omm != mm) { // Only redraw every minute to minimise flicker
      // Uncomment ONE of the next 2 lines, using the ghost image demonstrates text overlay as time is drawn over it
      gfx.setTextColor(0x39C4, GFX_BLACK);  // Leave a 7 segment ghost image, comment out next line!
      //gfx.setTextColor(GFX_BLACK, GFX_BLACK); // Set font colour to black to wipe image
      // Spleen12x24 at 2x is to show a pseudo 7 segment display.
      nice_gfx::text::draw(gfx, "88:88", xpos, ypos, nice_gfx::fonts::Spleen12x24, 2); // Overwrite the text to clear it
      gfx.setTextColor(0xFBE0); // Orange
      omm = mm;

      if (hh<10) xpos+= nice_gfx::text::glyph(gfx, '0', xpos, ypos, nice_gfx::fonts::Spleen12x24, 2);
      xpos+= nice_gfx::text::number(gfx, hh, xpos, ypos, nice_gfx::fonts::Spleen12x24, 2);
      xcolon=xpos;
      xpos+= nice_gfx::text::glyph(gfx, ':', xpos, ypos, nice_gfx::fonts::Spleen12x24, 2);
      if (mm<10) xpos+= nice_gfx::text::glyph(gfx, '0', xpos, ypos, nice_gfx::fonts::Spleen12x24, 2);
      nice_gfx::text::number(gfx, mm, xpos, ypos, nice_gfx::fonts::Spleen12x24, 2);
    }

    if (ss%2) { // Flash the colon
      gfx.setTextColor(0x39C4, GFX_BLACK);
      xpos+= nice_gfx::text::glyph(gfx, ':', xcolon, ypos, nice_gfx::fonts::Spleen12x24, 2);
      gfx.setTextColor(0xFBE0, GFX_BLACK);
    }
    else {
      nice_gfx::text::glyph(gfx, ':', xcolon, ypos, nice_gfx::fonts::Spleen12x24, 2);
      colour = random(0xFFFF);
      // Erase the old text with a rectangle, the disadvantage of this method is increased display flicker
      gfx.fillRect (0, 64, 160, 20, GFX_BLACK);
      gfx.setTextColor(colour);
      nice_gfx::text::right(gfx, "Colour", 75, 64, nice_gfx::fonts::Spleen12x24, 1); // Right justified string drawing to x position 75
      String scolour = String(colour,HEX);
      scolour.toUpperCase();
      char buffer[20];
      scolour.toCharArray(buffer,20);
      nice_gfx::text::draw(gfx, buffer, 82, 64, nice_gfx::fonts::Spleen12x24, 1);
    }
  }
}



