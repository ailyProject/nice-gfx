/*
Tests the updated floating point plot function
The datum is set to the centre of the 320 x 240 screen
so numbers are printed the middle.

The last test shows the datum point as a red dot.

Normally strings are printed relative to the top left corner but this can be
changed with the setTextDatum() function. The library has #defines for:

TL_DATUM  0 //Top left
TC_DATUM  1 //Top centre
TR_DATUM  2 //Top right
ML_DATUM  3 //Middle left
MC_DATUM  4 //Middle centre
MR_DATUM  5 //Middle right
BL_DATUM  6 //Bottom left
BC_DATUM  7 //Bottom centre
BR_DATUM  8 //Bottom right

 
 Needs fonts 2 and 6

 Make sure all the display driver and pin connections are correct by
 editing the Hardware.h file in this sketch folder.

 #########################################################################
 ###### DON'T FORGET TO UPDATE THE Hardware.h FILE IN THIS SKETCH ######
 #########################################################################
 */


#include <NICE_GFX.h>
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include "Hardware.h"
#include <SPI.h>

ExampleDisplay gfx;       // Invoke custom library

unsigned long drawTime = 0;

void setup(void) {
  Serial.begin(115200);
  gfx.begin();
  gfx.setRotation(1);

}

void loop() {

  char tmp[12];
  
  gfx.fillScreen(GFX_BLACK);
  gfx.setTextColor(GFX_WHITE, GFX_BLACK);
  
  // Datum is middle centre
  gfx.setTextDatum(MC_DATUM);

  // Test floating point drawing function:
  // drawFloat(value, precision, x, y, font);
  
  float test = 67.125;
  nice_gfx::text::decimal(gfx, test, 4, 160, 120, nice_gfx::fonts::Spleen12x24, 2);
    nice_gfx::text::draw(gfx, dtostrf(test,4,4,tmp), 100, 200, nice_gfx::fonts::Spleen12x24, 2);

  delay(1000);
  gfx.fillScreen(GFX_BLACK);
  
  test = -0.555555;
  nice_gfx::text::decimal(gfx, test, 3, 160, 120, nice_gfx::fonts::Spleen12x24, 2);
    nice_gfx::text::draw(gfx, dtostrf(test,2,2,tmp), 100, 200, nice_gfx::fonts::Spleen12x24, 2);

  delay(1000);
  gfx.fillScreen(GFX_BLACK);
  
  test = 0.123;
  nice_gfx::text::decimal(gfx, test, 4, 160, 120, nice_gfx::fonts::Spleen12x24, 2);

  nice_gfx::text::draw(gfx, dtostrf(test,4,4,tmp), 100, 200, nice_gfx::fonts::Spleen12x24, 2);

  delay(1000);
  gfx.fillScreen(GFX_BLACK);

  // This does not work at the moment....
  test = 9999999;
  nice_gfx::text::decimal(gfx, test, 0, 160, 120, nice_gfx::fonts::Spleen12x24, 2);
  nice_gfx::text::draw(gfx, dtostrf(test,4,4,tmp), 100, 200, nice_gfx::fonts::Spleen12x24, 2);
  delay(1000);

  //Plot the datum for the last number
  gfx.fillCircle(160,120,5,GFX_RED);
  gfx.setTextDatum(MC_DATUM);
  gfx.setTextColor(GFX_BLACK);
  nice_gfx::text::draw(gfx, "X", 160, 120, nice_gfx::fonts::GfxTerm8x16, 1);
 
  delay(4000);
}








