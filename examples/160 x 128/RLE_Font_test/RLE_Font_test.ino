/*
 Display all the fast rendering fonts.

 This sketch uses explicit MonoFont descriptors and integer text scaling.
 
 Make sure all the display driver and pin connections are correct by
 editing the Hardware.h file in this sketch folder.

 #########################################################################
 ###### DON'T FORGET TO UPDATE THE Hardware.h FILE IN THIS SKETCH ######
 #########################################################################
*/

// New background colour
#define GFX_BROWN 0x38E0

// Pause in milliseconds between screens, change to 0 to time font rendering
#define WAIT 500

#include <NICE_GFX.h>
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include "Hardware.h"
#include <SPI.h>

ExampleDisplay gfx;  // Invoke library, pins in Hardware.h

unsigned long targetTime = 0; // Used for testing draw times

void setup(void) {
  gfx.begin();
  gfx.setRotation(1);
}

void loop() {
  targetTime = millis();

  // First we test them with a background colour set
  gfx.setTextSize(1);
  gfx.fillScreen(GFX_BLACK);
  gfx.setTextColor(GFX_GREEN, GFX_BLACK);

  nice_gfx::text::draw(gfx, " !\"#$%&'()*+,-./0123456", 0, 0, nice_gfx::fonts::GfxTerm8x16, 1);
  nice_gfx::text::draw(gfx, "789:;<=>?@ABCDEFGHIJKL", 0, 16, nice_gfx::fonts::GfxTerm8x16, 1);
  nice_gfx::text::draw(gfx, "MNOPQRSTUVWXYZ[\\]^_`", 0, 32, nice_gfx::fonts::GfxTerm8x16, 1);
  nice_gfx::text::draw(gfx, "abcdefghijklmnopqrstuvw", 0, 48, nice_gfx::fonts::GfxTerm8x16, 1);
  int xpos = 0;
  xpos += nice_gfx::text::draw(gfx, "xyz{|}~", 0, 64, nice_gfx::fonts::GfxTerm8x16, 1);
  nice_gfx::text::glyph(gfx, 127, xpos, 64, nice_gfx::fonts::GfxTerm8x16, 1);
  delay(WAIT);

  gfx.fillScreen(GFX_BLACK);
  gfx.setTextColor(GFX_GREEN, GFX_BLACK);

  nice_gfx::text::draw(gfx, " !\"#$%&'()*+,-.", 0, 0, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(gfx, "/0123456789:;", 0, 26, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(gfx, "<=>?@ABCDE", 0, 52, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(gfx, "FGHIJKLMNO", 0, 78, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(gfx, "PQRSTUVWX", 0, 104, nice_gfx::fonts::Spleen12x24, 1);
  delay(WAIT);

  gfx.fillScreen(GFX_BLACK);
  nice_gfx::text::draw(gfx, "YZ[\\]^_`abc", 0, 0, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(gfx, "defghijklmno", 0, 26, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(gfx, "pqrstuvwxyz", 0, 52, nice_gfx::fonts::Spleen12x24, 1);
  xpos = 0;
  xpos += nice_gfx::text::draw(gfx, "{|}~", 0, 78, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::glyph(gfx, 127, xpos, 78, nice_gfx::fonts::Spleen12x24, 1);
  delay(WAIT);

  gfx.fillScreen(GFX_BLACK);
  gfx.setTextColor(GFX_BLUE, GFX_BLACK);

  nice_gfx::text::draw(gfx, "012345", 0, 0, nice_gfx::fonts::Spleen12x24, 2);
  nice_gfx::text::draw(gfx, "6789", 0, 40, nice_gfx::fonts::Spleen12x24, 2);
  nice_gfx::text::draw(gfx, "apm-:.", 0, 80, nice_gfx::fonts::Spleen12x24, 2);
  delay(WAIT);

  gfx.fillScreen(GFX_BLACK);
  gfx.setTextColor(GFX_RED, GFX_BLACK);

  nice_gfx::text::draw(gfx, "0123", 0, 0, nice_gfx::fonts::Spleen12x24, 2);
  nice_gfx::text::draw(gfx, "4567", 0, 60, nice_gfx::fonts::Spleen12x24, 2);
  delay(WAIT);

  gfx.fillScreen(GFX_BLACK);
  nice_gfx::text::draw(gfx, "890:.", 0, 0, nice_gfx::fonts::Spleen12x24, 2);
  nice_gfx::text::draw(gfx, "", 0, 60, nice_gfx::fonts::Spleen12x24, 2);
  delay(WAIT);

  gfx.fillScreen(GFX_BLACK);
  gfx.setTextColor(GFX_YELLOW, GFX_BLACK);

  nice_gfx::text::draw(gfx, "01", 0, 0, nice_gfx::fonts::Spleen12x24, 3);
  delay(WAIT);
  
  nice_gfx::text::draw(gfx, "23", 0, 0, nice_gfx::fonts::Spleen12x24, 3);
  delay(WAIT);

  nice_gfx::text::draw(gfx, "45", 0, 0, nice_gfx::fonts::Spleen12x24, 3);
  delay(WAIT);
  
  nice_gfx::text::draw(gfx, "67", 0, 0, nice_gfx::fonts::Spleen12x24, 3);
  delay(WAIT);

  nice_gfx::text::draw(gfx, "89", 0, 0, nice_gfx::fonts::Spleen12x24, 3);
  delay(WAIT);
 
  nice_gfx::text::draw(gfx, "0:.", 0, 0, nice_gfx::fonts::Spleen12x24, 3);
  delay(WAIT);

  gfx.setTextColor(GFX_MAGENTA);
  nice_gfx::text::number(gfx, millis() - targetTime, 0, 100, nice_gfx::fonts::Spleen12x24, 1);
  delay(4000);

  // Now test them with transparent background
  targetTime = millis();

  gfx.setTextSize(1);
  gfx.fillScreen(GFX_BROWN);
  gfx.setTextColor(GFX_GREEN);

  nice_gfx::text::draw(gfx, " !\"#$%&'()*+,-./0123456", 0, 0, nice_gfx::fonts::GfxTerm8x16, 1);
  nice_gfx::text::draw(gfx, "789:;<=>?@ABCDEFGHIJKL", 0, 16, nice_gfx::fonts::GfxTerm8x16, 1);
  nice_gfx::text::draw(gfx, "MNOPQRSTUVWXYZ[\\]^_`", 0, 32, nice_gfx::fonts::GfxTerm8x16, 1);
  nice_gfx::text::draw(gfx, "abcdefghijklmnopqrstuvw", 0, 48, nice_gfx::fonts::GfxTerm8x16, 1);
  xpos = 0;
  xpos += nice_gfx::text::draw(gfx, "xyz{|}~", 0, 64, nice_gfx::fonts::GfxTerm8x16, 1);
  nice_gfx::text::glyph(gfx, 127, xpos, 64, nice_gfx::fonts::GfxTerm8x16, 1);
  delay(WAIT);

  gfx.fillScreen(GFX_BROWN);
  gfx.setTextColor(GFX_GREEN);

  nice_gfx::text::draw(gfx, " !\"#$%&'()*+,-.", 0, 0, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(gfx, "/0123456789:;", 0, 26, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(gfx, "<=>?@ABCDE", 0, 52, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(gfx, "FGHIJKLMNO", 0, 78, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(gfx, "PQRSTUVWX", 0, 104, nice_gfx::fonts::Spleen12x24, 1);

  delay(WAIT);
  gfx.fillScreen(GFX_BROWN);
  nice_gfx::text::draw(gfx, "YZ[\\]^_`abc", 0, 0, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(gfx, "defghijklmno", 0, 26, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(gfx, "pqrstuvwxyz", 0, 52, nice_gfx::fonts::Spleen12x24, 1);
  xpos = 0;
  xpos += nice_gfx::text::draw(gfx, "{|}~", 0, 78, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::glyph(gfx, 127, xpos, 78, nice_gfx::fonts::Spleen12x24, 1);
  delay(WAIT);

  gfx.fillScreen(GFX_BROWN);
  gfx.setTextColor(GFX_BLUE);

  nice_gfx::text::draw(gfx, "012345", 0, 0, nice_gfx::fonts::Spleen12x24, 2);
  nice_gfx::text::draw(gfx, "6789", 0, 40, nice_gfx::fonts::Spleen12x24, 2);
  nice_gfx::text::draw(gfx, "apm-:.", 0, 80, nice_gfx::fonts::Spleen12x24, 2);
  delay(WAIT);

  gfx.fillScreen(GFX_BROWN);
  gfx.setTextColor(GFX_RED);

  nice_gfx::text::draw(gfx, "0123", 0, 0, nice_gfx::fonts::Spleen12x24, 2);
  nice_gfx::text::draw(gfx, "4567", 0, 60, nice_gfx::fonts::Spleen12x24, 2);
  delay(WAIT);

  gfx.fillScreen(GFX_BROWN);
  nice_gfx::text::draw(gfx, "890:.", 0, 0, nice_gfx::fonts::Spleen12x24, 2);
  nice_gfx::text::draw(gfx, "", 0, 60, nice_gfx::fonts::Spleen12x24, 2);
  delay(WAIT);

  gfx.fillScreen(GFX_BROWN);
  gfx.setTextColor(GFX_YELLOW);

  nice_gfx::text::draw(gfx, "0123", 0, 0, nice_gfx::fonts::Spleen12x24, 3);
  delay(WAIT);

  gfx.fillScreen(GFX_BROWN);
  nice_gfx::text::draw(gfx, "4567", 0, 0, nice_gfx::fonts::Spleen12x24, 3);
  delay(WAIT);

  gfx.fillScreen(GFX_BROWN);
  nice_gfx::text::draw(gfx, "890:.", 0, 0, nice_gfx::fonts::Spleen12x24, 3);
  delay(WAIT);

  gfx.setTextColor(GFX_MAGENTA);

  nice_gfx::text::number(gfx, millis() - targetTime, 0, 100, nice_gfx::fonts::Spleen12x24, 1);
  delay(4000);;
}

