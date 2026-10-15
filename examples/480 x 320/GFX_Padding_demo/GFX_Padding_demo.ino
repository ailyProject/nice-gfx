/*
 Example to show how text padding and setting the datum works.

 Drawing a numbers at a location can leave the remains of previous numbers.
 for example drawing 999 then 17 may display as:
    179 for left datum
    or
    917 for right datum.

 By setting the correct text padding width and setting a background colour then
 the plotted numbers will overprint old values.  The padding width must be set
 to be large enough to cover the old text.

 This sketch shows drawing numbers and text both with and without padding.
 Unpadded text and numbers plot in red.
 Padded text and numbers plot in green.

 Padding works with all plotting datums set by setTextDatum()

 The height of the padded area is set automatically by the font used.
 
  #########################################################################
  ###### DON'T FORGET TO UPDATE THE Display/Presets/ExampleBoard.h FILE IN THE LIBRARY ######
  #########################################################################
 */

#include <SPI.h>

#include <NICE_GFX.h>
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include <Display/Presets/ExampleBoard.h>

ExampleDisplay gfx;       // Invoke custom library

unsigned long drawTime = 0;

void setup(void) {
  gfx.begin();

  gfx.setFont(nice_gfx::fonts::TomThumb4x6);
  gfx.setRotation(1);
}

void loop() {
  int x = 240;
  int y = 60;
  byte decimal_places = 1;
  const nice_gfx::MonoFont *font = &nice_gfx::fonts::Spleen12x24;
  uint8_t fontScale = 3;
  
  gfx.fillScreen(GFX_BLACK);

  header("Right datum padding demo");

  gfx.setTextColor(GFX_RED, GFX_BLUE);

  gfx.setTextDatum(TR_DATUM); // Top Right is datum, so decimal point stays in same place
                              // any datum could be used

  //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  // The value on screen will be wrong as not all digits are over-printed
  
  gfx.setTextPadding(0); // Setting to zero switches off padding

  // Print all numbers from 49.9 to 0.0 in Spleen12x24 at 3x to show the problem
  
  for (int i = 199; i >= 0; i--) {
    yield(); nice_gfx::text::decimal(gfx, i/10.0, decimal_places, x, y, *font, fontScale);
  }

  //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  // Now set padding width to be 3 digits plus decimal point wide in Spleen12x24 at 3x
  // Padding height is set automatically, all numeric digits are the same width
  // in fonts 1 to 8. The value on screen will now be correct as all digits are over-printed

  int padding = nice_gfx::text::width(gfx, "99.9", *font, fontScale); // get the width of the text in pixels
  gfx.setTextColor(GFX_GREEN, GFX_BLUE);
  gfx.setTextPadding(padding);

  for (int i = 199; i >= 0; i--) {
    yield(); nice_gfx::text::decimal(gfx, i/10.0, decimal_places, x, y + 140, *font, fontScale); // Use 1 decimal place
  }

  delay(5000);

  //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  // Now use integers
  // The value on screen will be wrong as not all digits are over-printed
  
  gfx.fillScreen(GFX_BLACK);

  header("Left datum padding demo");

  gfx.setTextColor(GFX_RED, GFX_BLUE);

  gfx.setTextDatum(TL_DATUM); // Top Left is datum
                              // any datum could be used

  gfx.setTextPadding(0); // Setting to zero switches off padding

  // Print all numbers from 49.9 to 0.0 in Spleen12x24 at 3x to show the problem
  
  for (int i = 199; i >= 0; i--) {
    yield(); nice_gfx::text::number(gfx, i, x, y, *font, fontScale);
  }

  //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  // Now set padding width to be 3 digits wide in Spleen12x24 at 3x
  // Padding height is set automatically, all numeric digits are the same width
  // in fonts 1 to 8
  // The value on screen will now be correct as all digits are over-printed

  padding = nice_gfx::text::width(gfx, "999", *font, fontScale); // get the width of the text in pixels
  gfx.setTextColor(GFX_GREEN, GFX_BLUE);
  gfx.setTextPadding(padding);

  for (int i = 199; i >= 0; i--) {
    yield(); nice_gfx::text::number(gfx, i, x, y + 140, *font, fontScale); // Use 1 decimal place
  }

  delay(5000);

  //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  // Now use integers with a centred datum
  // The value on screen will be wrong as not all digits are over-printed
  
  gfx.fillScreen(GFX_BLACK);

  header("Centre datum padding demo");
  
  gfx.setTextColor(GFX_RED, GFX_BLUE);

  gfx.setTextDatum(TC_DATUM); // Top Centre is datum
                              // any datum could be used

  gfx.setTextPadding(0); // Setting to zero switches off padding

  // Print all numbers from 49.9 to 0.0 in Spleen12x24 at 3x to show the problem
  
  for (int i = 199; i >= 0; i--) {
    yield(); nice_gfx::text::number(gfx, i, x, y, *font, fontScale);
  }

  //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  // Now set padding width to be 3 digits wide in Spleen12x24 at 3x
  // Padding height is set automatically, all numeric digits are the same width
  // in fonts 1 to 8
  // The value on screen will now be correct as all digits are over-printed

  padding = nice_gfx::text::width(gfx, "999", *font, fontScale); // get the width of the text in pixels
  gfx.setTextColor(GFX_GREEN, GFX_BLUE);
  gfx.setTextPadding(padding);

  for (int i = 199; i >= 0; i--) {
    yield(); nice_gfx::text::number(gfx, i, x, y + 140, *font, fontScale); // Use 1 decimal place
  }

  delay(5000);

  //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  // Now use text over-printing by setting the padding value
  // Previous text is not wiped by a shorter string

  gfx.fillScreen(GFX_LIGHTGREY);

  header("Centred datum text padding demo");
  
  gfx.setTextSize(2); // Any text size muliplier will work
  
  gfx.setTextColor(GFX_RED, GFX_BLUE);

  gfx.setTextDatum(TC_DATUM); // Top Centre is datum
                              // any datum could be used

  gfx.setTextPadding(0); // Setting to zero switches off padding

  nice_gfx::text::draw(gfx, "Quick brown", x, y, nice_gfx::fonts::Spleen12x24, 1);
  delay(2000);
  nice_gfx::text::draw(gfx, "fox", x, y, nice_gfx::fonts::Spleen12x24, 1);

  delay(2000);
  
  // Now set padding width to longest string
  // Previous text will automatically be wiped by a shorter string!
  font = &nice_gfx::fonts::Spleen12x24; fontScale = 1;

  padding = nice_gfx::text::width(gfx, "Quick brown", *font, fontScale); // get the width of the widest text in pixels
                                                // could set this to any number up to screen width
  gfx.setTextPadding(padding);

  gfx.setTextColor(GFX_GREEN, GFX_BLUE);
  
  nice_gfx::text::draw(gfx, "Quick brown", x, y+80, *font, fontScale);
  delay(2000);
  nice_gfx::text::draw(gfx, "fox", x, y+80, *font, fontScale);

  delay(5000);
}

// Print the header for a display screen
void header(const char *string)
{
  gfx.setTextSize(1);
  gfx.setTextColor(GFX_MAGENTA, GFX_BLACK);
  gfx.setTextDatum(TC_DATUM);
  nice_gfx::text::draw(gfx, string, 240, 10, nice_gfx::fonts::Spleen12x24, 1);
 
}





