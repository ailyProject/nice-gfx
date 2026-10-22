/*
Tests string alignment

Normally strings are printed relative to the top left corner but this can be
changed with the setTextDatum() function. The library has #defines for:

TL_DATUM = Top left
TC_DATUM = Top centre
TR_DATUM = Top right
ML_DATUM = Middle left
MC_DATUM = Middle centre
MR_DATUM = Middle right
BL_DATUM = Bottom left
BC_DATUM = Bottom centre
BR_DATUM = Bottom right

 
 Needs fonts 2, 4, 6, 7 and 8

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

  gfx.fillScreen(GFX_BLACK);
  
  for(byte datum = 0; datum < 9; datum++) {
    gfx.setTextColor(GFX_WHITE, GFX_BLACK);
    
    gfx.setTextDatum(datum);
    
    nice_gfx::text::number(gfx, 88, 160, 120, nice_gfx::fonts::Spleen12x24, 3);
    gfx.fillCircle(160,120,5,GFX_RED);
    
    gfx.setTextDatum(MC_DATUM);
    
    gfx.setTextColor(GFX_BLACK);
    nice_gfx::text::draw(gfx, "X", 160, 120, nice_gfx::fonts::GfxTerm8x16, 1);
    delay(1000);
    gfx.fillScreen(GFX_BLACK);
  }

  gfx.setTextColor(GFX_BLUE, GFX_BLACK);
  nice_gfx::text::center(gfx, "69", 160, 120, nice_gfx::fonts::Spleen12x24, 3);
  gfx.fillCircle(160,120,5,GFX_YELLOW);
  
  gfx.setTextDatum(MC_DATUM);
  
  gfx.setTextColor(GFX_BLACK);
  nice_gfx::text::draw(gfx, "X", 160, 120, nice_gfx::fonts::GfxTerm8x16, 1);
  delay(1000);
  gfx.fillScreen(GFX_BLACK);
  
  gfx.setTextColor(GFX_RED, GFX_BLACK);
  nice_gfx::text::right(gfx, "88", 160, 120, nice_gfx::fonts::Spleen12x24, 3);
  gfx.fillCircle(160,120,5,GFX_YELLOW);
  
  gfx.setTextDatum(MC_DATUM);
  
  gfx.setTextColor(GFX_BLACK);
  nice_gfx::text::draw(gfx, "X", 160, 120, nice_gfx::fonts::GfxTerm8x16, 1);
  delay(1000);
  gfx.fillScreen(GFX_BLACK);

  gfx.setTextColor(GFX_WHITE, GFX_BLUE);

  gfx.setTextDatum(MC_DATUM);

  //Test floating point drawing function
  float test = 67.125;
  nice_gfx::text::decimal(gfx, test, 4, 160, 180, nice_gfx::fonts::Spleen12x24, 1);
  delay(1000);
  gfx.fillScreen(GFX_BLACK);
  test = -0.555555;
  nice_gfx::text::decimal(gfx, test, 3, 160, 180, nice_gfx::fonts::Spleen12x24, 1);
  delay(1000);
  gfx.fillScreen(GFX_BLACK);
  test = 0.1;
  nice_gfx::text::decimal(gfx, test, 4, 160, 180, nice_gfx::fonts::Spleen12x24, 1);
  delay(1000);
  gfx.fillScreen(GFX_BLACK);
  test = 9999999;
  nice_gfx::text::decimal(gfx, test, 1, 160, 180, nice_gfx::fonts::Spleen12x24, 1);
  delay(1000);
  
  gfx.fillCircle(160,180,5,GFX_YELLOW);
  
  gfx.setTextDatum(MC_DATUM);
  
  gfx.setTextColor(GFX_BLACK);
  nice_gfx::text::draw(gfx, "X", 160, 180, nice_gfx::fonts::GfxTerm8x16, 1);
 
  delay(4000);
}








