/*
 Font draw speed and flicker test, draws all numbers 0-999 in each font
 Average time in milliseconds to draw is shown in red
 A total of 2890 characters are drawn in each font
 
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
  gfx.begin();
  gfx.setRotation(1);
}

void loop() {

  gfx.fillScreen(GFX_BLACK);
  gfx.setTextColor(GFX_WHITE, GFX_BLACK);

  drawTime = millis();

  for (int i = 0; i < 1000; i++) {
    yield();nice_gfx::text::number(gfx, i, 100, 80, nice_gfx::fonts::TomThumb4x6, 1);
  }

  drawTime = millis() - drawTime;

  gfx.setTextColor(GFX_RED, GFX_BLACK);
  int xpos = 20;
  xpos += nice_gfx::text::decimal(gfx, drawTime / 2890.0, 3, xpos, 180, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(gfx, " ms per character", xpos, 180, nice_gfx::fonts::Spleen12x24, 1);
  if (drawTime < 100) nice_gfx::text::draw(gfx, "Font 1 not loaded!", 50, 210, nice_gfx::fonts::Spleen12x24, 1);
  
  delay(4000);
  gfx.fillScreen(GFX_BLACK);
  gfx.setTextColor(GFX_WHITE, GFX_BLACK);
    drawTime = millis();

  for (int i = 0; i < 1000; i++) {
    yield();nice_gfx::text::number(gfx, i, 100, 80, nice_gfx::fonts::GfxTerm8x16, 1);
  }

  drawTime = millis() - drawTime;

  gfx.setTextColor(GFX_RED, GFX_BLACK);
  xpos = 20;
  xpos += nice_gfx::text::decimal(gfx, drawTime / 2890.0, 3, xpos, 180, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(gfx, " ms per character", xpos, 180, nice_gfx::fonts::Spleen12x24, 1);
  if (drawTime < 200) nice_gfx::text::draw(gfx, "GfxTerm8x16 not loaded!", 50, 210, nice_gfx::fonts::Spleen12x24, 1);
  
  delay(4000);
  gfx.fillScreen(GFX_BLACK);
  gfx.setTextColor(GFX_WHITE, GFX_BLACK);
    drawTime = millis();

  for (int i = 0; i < 1000; i++) {
    yield();nice_gfx::text::number(gfx, i, 100, 80, nice_gfx::fonts::Spleen12x24, 1);
  }

  drawTime = millis() - drawTime;

  gfx.setTextColor(GFX_RED, GFX_BLACK);
  xpos = 20;
  xpos += nice_gfx::text::decimal(gfx, drawTime / 2890.0, 3, xpos, 180, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(gfx, " ms per character", xpos, 180, nice_gfx::fonts::Spleen12x24, 1);
  if (drawTime < 200) nice_gfx::text::draw(gfx, "Spleen12x24 not loaded!", 50, 210, nice_gfx::fonts::Spleen12x24, 1);
  
  delay(4000);
  gfx.fillScreen(GFX_BLACK);
  gfx.setTextColor(GFX_WHITE, GFX_BLACK);
    drawTime = millis();

  for (int i = 0; i < 1000; i++) {
    yield();nice_gfx::text::number(gfx, i, 100, 80, nice_gfx::fonts::Spleen12x24, 2);
  }

  drawTime = millis() - drawTime;

  gfx.setTextColor(GFX_RED, GFX_BLACK);
  xpos = 20;
  xpos += nice_gfx::text::decimal(gfx, drawTime / 2890.0, 3, xpos, 180, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(gfx, " ms per character", xpos, 180, nice_gfx::fonts::Spleen12x24, 1);
  if (drawTime < 200) nice_gfx::text::draw(gfx, "Spleen12x24 at 2x not loaded!", 50, 210, nice_gfx::fonts::Spleen12x24, 1);
  
  delay(4000);
  gfx.fillScreen(GFX_BLACK);
  gfx.setTextColor(GFX_WHITE, GFX_BLACK);
    drawTime = millis();

  for (int i = 0; i < 1000; i++) {
    yield();nice_gfx::text::number(gfx, i, 100, 80, nice_gfx::fonts::Spleen12x24, 2);
  }

  drawTime = millis() - drawTime;

  gfx.setTextColor(GFX_RED, GFX_BLACK);
  xpos = 20;
  xpos += nice_gfx::text::decimal(gfx, drawTime / 2890.0, 3, xpos, 180, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(gfx, " ms per character", xpos, 180, nice_gfx::fonts::Spleen12x24, 1);
  if (drawTime < 200) nice_gfx::text::draw(gfx, "Spleen12x24 at 2x not loaded!", 50, 210, nice_gfx::fonts::Spleen12x24, 1);
  
  delay(4000);
  gfx.fillScreen(GFX_YELLOW);
  gfx.setTextColor(GFX_WHITE, GFX_BLACK);
    drawTime = millis();

  for (int i = 0; i < 1000; i++) {
    yield();nice_gfx::text::number(gfx, i, 100, 80, nice_gfx::fonts::Spleen12x24, 3);
  }

  drawTime = millis() - drawTime;

  gfx.setTextColor(GFX_RED, GFX_BLACK);
  xpos = 20;
  xpos += nice_gfx::text::decimal(gfx, drawTime / 2890.0, 3, xpos, 180, nice_gfx::fonts::Spleen12x24, 1);
  nice_gfx::text::draw(gfx, " ms per character", xpos, 180, nice_gfx::fonts::Spleen12x24, 1);
  if (drawTime < 200) nice_gfx::text::draw(gfx, "Spleen12x24 at 3x not loaded!", 50, 210, nice_gfx::fonts::Spleen12x24, 1);
  
  delay(4000);
}








