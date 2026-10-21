/*
 Display all the fast rendering fonts in a sprite

 Make sure all the display driver and pin connections are correct by
 editing the Hardware.h file in this sketch folder.

 #########################################################################
 ###### DON'T FORGET TO UPDATE THE Hardware.h FILE IN THIS SKETCH ######
 #########################################################################
*/

// Specify sprite 160 x 128 pixels (needs 40Kbytes of RAM for 16-bit colour)
#define IWIDTH  160
#define IHEIGHT 128

// Pause in milliseconds between screens, change to 0 to time font rendering
#define WAIT 500

#include <NICE_GFX.h>
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include "Hardware.h"
#include <SPI.h>

ExampleDisplay gfx;  // Invoke library, pins in Hardware.h

GFX_Sprite img = GFX_Sprite(&gfx);

unsigned long targetTime = 0; // Used for testing draw times

void setup(void) {
  gfx.begin();
  gfx.setRotation(0);

  gfx.fillScreen(GFX_BLUE);

  //img.setColorDepth(8); // Optionally set depth to 8 to halve RAM use
  img.createSprite(IWIDTH, IHEIGHT);
  img.fillSprite(GFX_BLACK);
}

void loop() {
  targetTime = millis();

  // First we test them with a background colour set
  img.setTextSize(1);
  img.fillSprite(GFX_BLACK);
  img.setTextColor(GFX_GREEN, GFX_BLACK);

  nice_gfx::text::draw(img, " !\"#$%&'()*+,-./0123456", 0, 0, nice_gfx::fonts::GfxTerm8x16, 1);
  nice_gfx::text::draw(img, "789:;<=>?@ABCDEFGHIJKL", 0, 16, nice_gfx::fonts::GfxTerm8x16, 1);
  nice_gfx::text::draw(img, "MNOPQRSTUVWXYZ[\\]^_`", 0, 32, nice_gfx::fonts::GfxTerm8x16, 1);
  nice_gfx::text::draw(img, "abcdefghijklmnopqrstuvw", 0, 48, nice_gfx::fonts::GfxTerm8x16, 1);

  int xpos = 0;
  xpos += nice_gfx::text::draw(img, "xyz{|}~", 0, 64, nice_gfx::fonts::GfxTerm8x16, 1);
  nice_gfx::text::glyph(img, 127, xpos, 64, nice_gfx::fonts::GfxTerm8x16, 1);
  img.pushSprite(0, 0); delay(WAIT);

  img.fillSprite(GFX_BLACK);
  img.setTextColor(GFX_GREEN, GFX_BLACK);

  nice_gfx::text::draw(img, " !\"#$%&'()*+,-.", 0, 0, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(img, "/0123456789:;", 0, 26, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(img, "<=>?@ABCDE", 0, 52, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(img, "FGHIJKLMNO", 0, 78, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(img, "PQRSTUVWX", 0, 104, nice_gfx::fonts::Spleen12x24, 1);
  img.pushSprite(0, 0); delay(WAIT);

  img.fillSprite(GFX_BLACK);
  nice_gfx::text::draw(img, "YZ[\\]^_`abc", 0, 0, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(img, "defghijklmno", 0, 26, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(img, "pqrstuvwxyz", 0, 52, nice_gfx::fonts::Spleen12x24, 1);
  xpos = 0;
  xpos += nice_gfx::text::draw(img, "{|}~", 0, 78, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::glyph(img, 127, xpos, 78, nice_gfx::fonts::Spleen12x24, 1);
  img.pushSprite(0, 0); delay(WAIT);

  img.fillSprite(GFX_BLACK);
  img.setTextColor(GFX_BLUE, GFX_BLACK);

  nice_gfx::text::draw(img, "012345", 0, 0, nice_gfx::fonts::Spleen12x24, 2);
  nice_gfx::text::draw(img, "6789", 0, 40, nice_gfx::fonts::Spleen12x24, 2);
  nice_gfx::text::draw(img, "apm-:.", 0, 80, nice_gfx::fonts::Spleen12x24, 2);
  img.pushSprite(0, 0); delay(WAIT);

  img.fillSprite(GFX_BLACK);
  img.setTextColor(GFX_RED, GFX_BLACK);

  nice_gfx::text::draw(img, "0123", 0, 0, nice_gfx::fonts::Spleen12x24, 2);
  nice_gfx::text::draw(img, "4567", 0, 60, nice_gfx::fonts::Spleen12x24, 2);
  img.pushSprite(0, 0); delay(WAIT);

  img.fillSprite(GFX_BLACK);
  nice_gfx::text::draw(img, "890:.", 0, 0, nice_gfx::fonts::Spleen12x24, 2);
  nice_gfx::text::draw(img, "", 0, 60, nice_gfx::fonts::Spleen12x24, 2);
  img.pushSprite(0, 0); delay(WAIT);

  img.fillSprite(GFX_BLACK);
  img.setTextColor(GFX_YELLOW, GFX_BLACK);

  nice_gfx::text::draw(img, "01", 0, 0, nice_gfx::fonts::Spleen12x24, 3);
  img.pushSprite(0, 0); delay(WAIT);
  
  nice_gfx::text::draw(img, "23", 0, 0, nice_gfx::fonts::Spleen12x24, 3);
  img.pushSprite(0, 0); delay(WAIT);

  nice_gfx::text::draw(img, "45", 0, 0, nice_gfx::fonts::Spleen12x24, 3);
  img.pushSprite(0, 0); delay(WAIT);
  
  nice_gfx::text::draw(img, "67", 0, 0, nice_gfx::fonts::Spleen12x24, 3);
  img.pushSprite(0, 0); delay(WAIT);

  nice_gfx::text::draw(img, "89", 0, 0, nice_gfx::fonts::Spleen12x24, 3);
  img.pushSprite(0, 0); delay(WAIT);
 
  nice_gfx::text::draw(img, "0:.", 0, 0, nice_gfx::fonts::Spleen12x24, 3);
  img.pushSprite(0, 0); delay(WAIT);

  img.setTextColor(GFX_WHITE);
  nice_gfx::text::number(img, millis() - targetTime, 0, 100, nice_gfx::fonts::Spleen12x24, 1);
  img.pushSprite(0, 0); delay(WAIT);
  delay(4000);

  // Now test them with transparent background
  targetTime = millis();

  img.setTextSize(1);
  img.fillSprite(GFX_BLACK);
  img.setTextColor(GFX_GREEN);

  nice_gfx::text::draw(img, " !\"#$%&'()*+,-./0123456", 0, 0, nice_gfx::fonts::GfxTerm8x16, 1);
  nice_gfx::text::draw(img, "789:;<=>?@ABCDEFGHIJKL", 0, 16, nice_gfx::fonts::GfxTerm8x16, 1);
  nice_gfx::text::draw(img, "MNOPQRSTUVWXYZ[\\]^_`", 0, 32, nice_gfx::fonts::GfxTerm8x16, 1);
  nice_gfx::text::draw(img, "abcdefghijklmnopqrstuvw", 0, 48, nice_gfx::fonts::GfxTerm8x16, 1);
  xpos = 0;
  xpos += nice_gfx::text::draw(img, "xyz{|}~", 0, 64, nice_gfx::fonts::GfxTerm8x16, 1);
  nice_gfx::text::glyph(img, 127, xpos, 64, nice_gfx::fonts::GfxTerm8x16, 1);
  img.pushSprite(0, 0); delay(WAIT);

  img.fillSprite(GFX_BLACK);
  img.setTextColor(GFX_GREEN);

  nice_gfx::text::draw(img, " !\"#$%&'()*+,-.", 0, 0, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(img, "/0123456789:;", 0, 26, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(img, "<=>?@ABCDE", 0, 52, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(img, "FGHIJKLMNO", 0, 78, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(img, "PQRSTUVWX", 0, 104, nice_gfx::fonts::Spleen12x24, 1);

  img.pushSprite(0, 0); delay(WAIT);
  img.fillSprite(GFX_BLACK);
  nice_gfx::text::draw(img, "YZ[\\]^_`abc", 0, 0, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(img, "defghijklmno", 0, 26, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(img, "pqrstuvwxyz", 0, 52, nice_gfx::fonts::Spleen12x24, 1);
  xpos = 0;
  xpos += nice_gfx::text::draw(img, "{|}~", 0, 78, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::glyph(img, 127, xpos, 78, nice_gfx::fonts::Spleen12x24, 1);
  img.pushSprite(0, 0); delay(WAIT);

  img.fillSprite(GFX_BLACK);
  img.setTextColor(GFX_BLUE);

  nice_gfx::text::draw(img, "012345", 0, 0, nice_gfx::fonts::Spleen12x24, 2);
  nice_gfx::text::draw(img, "6789", 0, 40, nice_gfx::fonts::Spleen12x24, 2);
  nice_gfx::text::draw(img, "apm-:.", 0, 80, nice_gfx::fonts::Spleen12x24, 2);
  img.pushSprite(0, 0); delay(WAIT);

  img.fillSprite(GFX_BLACK);
  img.setTextColor(GFX_RED);

  nice_gfx::text::draw(img, "0123", 0, 0, nice_gfx::fonts::Spleen12x24, 2);
  nice_gfx::text::draw(img, "4567", 0, 60, nice_gfx::fonts::Spleen12x24, 2);
  img.pushSprite(0, 0); delay(WAIT);

  img.fillSprite(GFX_BLACK);
  nice_gfx::text::draw(img, "890:.", 0, 0, nice_gfx::fonts::Spleen12x24, 2);
  nice_gfx::text::draw(img, "", 0, 60, nice_gfx::fonts::Spleen12x24, 2);
  img.pushSprite(0, 0); delay(WAIT);

  img.fillSprite(GFX_BLACK);
  img.setTextColor(GFX_YELLOW);

  nice_gfx::text::draw(img, "0123", 0, 0, nice_gfx::fonts::Spleen12x24, 3);
  img.pushSprite(0, 0); delay(WAIT);

  img.fillSprite(GFX_BLACK);
  nice_gfx::text::draw(img, "4567", 0, 0, nice_gfx::fonts::Spleen12x24, 3);
  img.pushSprite(0, 0); delay(WAIT);

  img.fillSprite(GFX_BLACK);
  nice_gfx::text::draw(img, "890:.", 0, 0, nice_gfx::fonts::Spleen12x24, 3);
  img.pushSprite(0, 0); delay(WAIT);

  img.setTextColor(GFX_WHITE);

  nice_gfx::text::number(img, millis() - targetTime, 0, 100, nice_gfx::fonts::Spleen12x24, 1);
  img.pushSprite(0, 0); delay(WAIT);
  delay(4000);;
}

