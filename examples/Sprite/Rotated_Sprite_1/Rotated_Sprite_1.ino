// This example plots a rotated Sprite to the screen using the pushRotated()
// function. It is written for a 240 x 320 TFT screen.

// Two rotation pivot points must be set, one for the Sprite and one for the TFT
// using setPivot(). These pivot points do not need to be within the visible screen
// or Sprite boundary.

// When the Sprite is rotated and pushed to the TFT with pushRotated(angle) it will be
// drawn so that the two pivot points coincide. This makes rotation about a point on the
// screen very simple. The rotation is clockwise with increasing angle. The angle is in
// degrees, an angle of 0 means no Sprite rotation.

// The pushRotated() function works with 1, 4, 8 and 16-bit per pixel (bpp) Sprites.

// The original Sprite is unchanged so can be plotted again at a different angle.

// Optionally a transparent colour can be defined, pixels of this colour will
// not be plotted to the TFT.

// For 1 bpp Sprites the foreground and background colours are defined with the
// function spr.setBitmapColor(foregroundColor, backgroundColor).

// For 4 bpp Sprites the colour map index is used instead of the 16-bit colour
// e.g. spr.setTextColor(5); // Green text in default colour map
// See "Transparent_Sprite_Demo_4bit" example for default colour map details

// Created by Bodmer 6/1/19; adapted for NICE_GFX:
#include <NICE_GFX.h>
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include "Hardware.h"

ExampleDisplay gfx;           // TFT object

GFX_Sprite spr = GFX_Sprite(&gfx); // Sprite object

// =======================================================================================
// Setup
// =======================================================================================

void setup()   {
  Serial.begin(250000); // Debug only

  gfx.begin();

  gfx.setFont(nice_gfx::fonts::TomThumb4x6);

  spr.setFont(nice_gfx::fonts::TomThumb4x6);  // initialize
  gfx.setRotation(0);
}

// =======================================================================================
// Loop
// =======================================================================================

void loop() {
  
  int xw = gfx.width()/2;   // xw, yh is middle of screen
  int yh = gfx.height()/2;


  showMessage("90 degree angles");
  gfx.setPivot(xw, yh);     // Set pivot to middle of TFT screen
  drawX(xw, yh);            // Show where screen pivot is

  // Create the Sprite
  spr.setColorDepth(8);      // Create an 8bpp Sprite of 60x30 pixels
  spr.createSprite(64, 30);  // 8bpp requires 64 * 30 = 1920 bytes
  spr.setPivot(32, 55);      // Set pivot relative to top left corner of Sprite
  spr.fillSprite(GFX_BLACK); // Fill the Sprite with black

  spr.setTextColor(GFX_GREEN);        // Green text
  spr.setTextDatum(MC_DATUM);         // Middle centre datum
  nice_gfx::text::draw(spr, "Hello", 32, 15, nice_gfx::fonts::Spleen12x24, 1); // Plot text, Spleen12x24, in Sprite at 30, 15

  spr.pushRotated(0);
  spr.pushRotated(90);
  spr.pushRotated(180);
  spr.pushRotated(270);

  delay(2000);


  showMessage("45 degree angles");
  drawX(xw, yh);            // Show where screen pivot is

  spr.pushRotated(45);
  spr.pushRotated(135);
  spr.pushRotated(225);
  spr.pushRotated(315);

  delay(2000);  // Pause so we see it


  showMessage("Moved Sprite pivot point");
  drawX(xw, yh);          // Show where screen pivot is

  spr.setPivot(-20, 15);  // Change just the Sprite pivot point

  spr.pushRotated(45);
  spr.pushRotated(135);
  spr.pushRotated(225);
  spr.pushRotated(315);

  delay(2000);  // Pause so we see it


  showMessage("Moved TFT pivot point");
  gfx.setPivot(100, 100); // Change just the TFT pivot point
  drawX(100, 100);        // Show where pivot is

  spr.pushRotated(45);
  spr.pushRotated(135);
  spr.pushRotated(225);
  spr.pushRotated(315);

  delay(2000);  // Pause so we see it


  showMessage("Transparent rotations");
  gfx.fillCircle(xw, yh, 70, GFX_DARKGREY); // Draw a filled circle

  gfx.setPivot(xw, yh);         // Set pivot to middle of screen
  drawX(xw, yh);                // Show where pivot is
  
  spr.deleteSprite();

  spr.setColorDepth(8);         // Create a 8bpp Sprite
  spr.createSprite(40, 30);     // Create a new Sprite 40x30
  spr.setPivot(20, 70);         // Set Sprite pivot at 20,80

  spr.setTextColor(GFX_RED);    // Red text in Sprite
  spr.setTextDatum(MC_DATUM);   // Middle centre datum

  int num = 1;

  for (int16_t angle = 30; angle <= 360; angle += 30)
  {
    spr.fillSprite(GFX_BLACK);         // Clear the Sprite
    nice_gfx::text::number(spr, num, 20, 15, nice_gfx::fonts::Spleen12x24, 1);    // Plot number, in Sprite at 20,15 and with Spleen12x24
    spr.pushRotated(angle, GFX_BLACK); // Plot rotated Sprite, black being transparent
    num++;
  }

  spr.setTextColor(GFX_WHITE);    // White text in Sprite
  spr.setPivot(-75, 15);          // Set Sprite pivot at -75,15

  for (int16_t angle = -90; angle < 270; angle += 30)
  {
    spr.fillSprite(GFX_BLACK);           // Clear the Sprite
    nice_gfx::text::number(spr, angle+90, 20, 15, nice_gfx::fonts::Spleen12x24, 1); // Plot number, in Sprite at 20,15 and with Spleen12x24
    spr.pushRotated(angle, GFX_BLACK);   // Plot rotated Sprite, black being transparent
    num++;
  }
  
  delay(8000);  // Pause so we see it

  spr.deleteSprite();

}

// =======================================================================================
// Draw an X centered on x,y
// =======================================================================================

void drawX(int x, int y)
{
  gfx.drawLine(x-5, y-5, x+5, y+5, GFX_WHITE);
  gfx.drawLine(x-5, y+5, x+5, y-5, GFX_WHITE);
}

// =======================================================================================
// Show a message at the top of the screen
// =======================================================================================

void showMessage(String msg)
{
  // Clear the screen areas
  gfx.fillRect(0, 0, gfx.width(), 20, GFX_BLACK);
  gfx.fillRect(0, 20, gfx.width(), gfx.height()-20, GFX_BLUE);

  uint8_t td = gfx.getTextDatum(); // Get current datum

  gfx.setTextDatum(TC_DATUM);      // Set new datum

  nice_gfx::text::draw(gfx, msg, gfx.width()/2, 2, nice_gfx::fonts::GfxTerm8x16, 1); // Message in GfxTerm8x16

  gfx.setTextDatum(td); // Restore old datum
}

// =======================================================================================
