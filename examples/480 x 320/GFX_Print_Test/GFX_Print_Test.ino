/*  
 Test the gfx.print() viz. the libraries embedded write() function

 This sketch used GfxTerm8x16, 4, 7
 
 Make sure all the required fonts are loaded by editing the
 Hardware.h file in this sketch folder.

  #########################################################################
  ###### DON'T FORGET TO UPDATE THE Hardware.h FILE IN THIS SKETCH ######
  #########################################################################
 */

#include <SPI.h>

#include <NICE_GFX.h>
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include "Hardware.h"

ExampleDisplay gfx;       // Invoke custom library

#define GFX_GREY 0x5AEB // New colour


void setup(void) {
  gfx.begin();
  gfx.setRotation(2);
}

void loop() {
  
  // Fill screen with random colour so we can see the effect of printing with and without 
  // a background colour defined
  gfx.fillScreen(random(0xFFFF));
  
  // Set "cursor" at top left corner of display (0,0) and select GfxTerm8x16
  // (cursor will move to next line automatically during printing with 'gfx.println'
  //  or stay on the line is there is room for the text with gfx.print)
  nice_gfx::text::cursor(gfx, 0, 0, nice_gfx::fonts::GfxTerm8x16, 1);
  // Set the font colour to be white with a black background, set text size multiplier to 1
  gfx.setTextColor(GFX_WHITE,GFX_BLACK);  gfx.setTextSize(1);
  // We can now plot text on screen using the "print" class
  gfx.println("Hello World!");
  
  // Set the font colour to be yellow with no background, set to Spleen12x24 at 2x
  gfx.setTextColor(GFX_YELLOW); nice_gfx::text::select(gfx, nice_gfx::fonts::Spleen12x24, 2);
  gfx.println(1234.56);
  
  // Set the font colour to be red with black background, set to Spleen12x24
  gfx.setTextColor(GFX_RED,GFX_BLACK);    nice_gfx::text::select(gfx, nice_gfx::fonts::Spleen12x24, 1);
  gfx.println((long)3735928559, HEX); // Should print DEADBEEF

  // Set the font colour to be green with black background, set to Spleen12x24
  gfx.setTextColor(GFX_GREEN,GFX_BLACK);
  nice_gfx::text::select(gfx, nice_gfx::fonts::Spleen12x24, 1);
  gfx.println("Groop");
  gfx.println("I implore thee,");

  // Change to GfxTerm8x16
  nice_gfx::text::select(gfx, nice_gfx::fonts::GfxTerm8x16, 1);
  gfx.println(F("my foonting turlingdromes.")); // Can store strings in FLASH to save RAM
  gfx.println("And hooptiously drangle me");
  gfx.println("with crinkly bindlewurdles,");
  // This next line is deliberately made too long for the display width to test
  // automatic text wrapping onto the next line
  gfx.println("Or I will rend thee in the gobberwarts with my blurglecruncheon, see if I don't!");
  
  // Test some print formatting functions
  float fnumber = 123.45;
   // Set the font colour to be blue with no background, set to Spleen12x24
  gfx.setTextColor(GFX_BLUE);    nice_gfx::text::select(gfx, nice_gfx::fonts::Spleen12x24, 1);
  gfx.print("Float = "); gfx.println(fnumber);           // Print floating point number
  gfx.print("Binary = "); gfx.println((int)fnumber, BIN); // Print as integer value in binary
  gfx.print("Hexadecimal = "); gfx.println((int)fnumber, HEX); // Print as integer number in Hexadecimal
  delay(10000);
}



