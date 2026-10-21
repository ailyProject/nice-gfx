/*
 Font draw speed and flicker test, draws all numbers 0-999 in each font
 (0-99 in Spleen12x24 at 3x)
 Average time in milliseconds to draw a character is shown in red
 A total of 2890 characters are drawn in each font (190 in Spleen12x24 at 3x)
 
 Needs fonts 2, 4, 6, 7 and 8

 Make sure all the display driver and pin connections are correct by
 editing the Hardware.h file in this sketch folder.

 Note that yield() or delay(0) must be called in long duration for/while
 loops to stop the ESP8266 watchdog triggering.

 #########################################################################
 ###### DON'T FORGET TO UPDATE THE Hardware.h FILE IN THIS SKETCH ######
 #########################################################################
 */


#include <NICE_GFX.h>
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include "Hardware.h"
#include <SPI.h>

ExampleDisplay gfx;  // Invoke library, pins in Hardware.h

unsigned long drawTime = 0;

void setup(void) {
  gfx.begin();
  gfx.setRotation(1);
}

void loop() {

  gfx.fillScreen(GFX_BLACK);
  gfx.setTextColor(GFX_WHITE, GFX_BLACK);

  drawTime = millis();

  for (int i = 0; i < 1000; i++) {
    nice_gfx::text::number(gfx, i, 0, 0, nice_gfx::fonts::TomThumb4x6, 1);
  }

  drawTime = millis() - drawTime;

  gfx.setTextColor(GFX_RED, GFX_BLACK);
  nice_gfx::text::decimal(gfx, drawTime / 2890.0, 3, 0, 80, nice_gfx::fonts::Spleen12x24, 1);
 
  delay(4000);
  gfx.fillScreen(GFX_BLACK);
  gfx.setTextColor(GFX_WHITE, GFX_BLACK);
  drawTime = millis();

  for (int i = 0; i < 1000; i++) {
    nice_gfx::text::number(gfx, i, 0, 0, nice_gfx::fonts::GfxTerm8x16, 1);
  }

  drawTime = millis() - drawTime;

  gfx.setTextColor(GFX_RED, GFX_BLACK);
  nice_gfx::text::decimal(gfx, drawTime / 2890.0, 3, 0, 80, nice_gfx::fonts::Spleen12x24, 1);

  delay(4000);
  gfx.fillScreen(GFX_BLACK);
  gfx.setTextColor(GFX_WHITE, GFX_BLACK);
  drawTime = millis();

  for (int i = 0; i < 1000; i++) {
    nice_gfx::text::number(gfx, i, 0, 0, nice_gfx::fonts::Spleen12x24, 1);
  }

  drawTime = millis() - drawTime;

  gfx.setTextColor(GFX_RED, GFX_BLACK);
  nice_gfx::text::decimal(gfx, drawTime / 2890.0, 3, 0, 80, nice_gfx::fonts::Spleen12x24, 1);

  delay(4000);
  gfx.fillScreen(GFX_BLACK);
  gfx.setTextColor(GFX_WHITE, GFX_BLACK);
  drawTime = millis();

  for (int i = 0; i < 1000; i++) {
    yield(); nice_gfx::text::number(gfx, i, 0, 0, nice_gfx::fonts::Spleen12x24, 2);
  }

  drawTime = millis() - drawTime;

  gfx.setTextColor(GFX_RED, GFX_BLACK);
  nice_gfx::text::decimal(gfx, drawTime / 2890.0, 3, 0, 80, nice_gfx::fonts::Spleen12x24, 1);
 
  delay(4000);
  gfx.fillScreen(GFX_BLACK);
  gfx.setTextColor(GFX_WHITE, GFX_BLACK);
  drawTime = millis();

  for (int i = 0; i < 1000; i++) {
    yield(); nice_gfx::text::number(gfx, i, 0, 0, nice_gfx::fonts::Spleen12x24, 2);
  }

  drawTime = millis() - drawTime;

  gfx.setTextColor(GFX_RED, GFX_BLACK);
  nice_gfx::text::decimal(gfx, drawTime / 2890.0, 3, 0, 80, nice_gfx::fonts::Spleen12x24, 1);

  delay(4000);
  gfx.fillScreen(GFX_BLACK);
  gfx.setTextColor(GFX_WHITE, GFX_BLACK);
  drawTime = millis();

  for (int i = 0; i < 100; i++) {
    yield(); nice_gfx::text::number(gfx, i, 0, 0, nice_gfx::fonts::Spleen12x24, 3);
  }

  drawTime = millis() - drawTime;

  gfx.setTextColor(GFX_RED, GFX_BLACK);
  nice_gfx::text::decimal(gfx, drawTime / 190.0, 3, 0, 80, nice_gfx::fonts::Spleen12x24, 1);

  delay(4000);
}








