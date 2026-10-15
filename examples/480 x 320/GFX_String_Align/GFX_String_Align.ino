/*
Tests string alignment

Normally strings are printed relative to the top left corner but this can be
changed with the setTextDatum() function. The library has #defines for:

TL_DATUM = 0 = Top left
TC_DATUM = 1 = Top centre
TR_DATUM = 2 = Top right
ML_DATUM = 3 = Middle left
MC_DATUM = 4 = Middle centre
MR_DATUM = 5 = Middle right
BL_DATUM = 6 = Bottom left
BC_DATUM = 7 = Bottom centre
BR_DATUM = 8 = Bottom right

L_BASELINE =  9 = Left character baseline (Line the 'A' character would sit on)
C_BASELINE = 10 = Centre character baseline
R_BASELINE = 11 = Right character baseline

So you can use lines to position text like:

  gfx.setTextDatum(BC_DATUM); // Set datum to bottom centre
 
 Needs fonts 2, 4, 6, 7 and 8

  #########################################################################
  ###### DON'T FORGET TO UPDATE THE Display/Presets/ExampleBoard.h FILE IN THE LIBRARY ######
  ######        TO SELECT THE FONTS AND PINS YOU USE, SEE ABOVE      ######
  #########################################################################
 */


#include <SPI.h>

#include <NICE_GFX.h>
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include <Display/Presets/ExampleBoard.h>

ExampleDisplay gfx;       // Invoke custom library

unsigned long drawTime = 0;

int x, y;  // Coordinates for drawing

void setup(void) {
  Serial.begin(115200);
  gfx.begin();
  gfx.setRotation(1);
}

void loop() {

  gfx.fillScreen(GFX_BLACK);

  x = 160, y = 120;
  
  //Test all 9 datums 0-8 when plotting numbers
  for(byte datum = 0; datum < 9; datum++) {
    gfx.setTextColor(GFX_WHITE, GFX_BLACK);
    
    gfx.setTextDatum(datum);
    
    nice_gfx::text::number(gfx, 42, x, y, nice_gfx::fonts::Spleen12x24, 3);
    
    drawCross(x, y, GFX_RED);
    
    delay(1000);
    gfx.fillScreen(GFX_BLACK);
  }

  //Test all 9 datums 0-8 when plotting a string
  // Datum works when text size is > 1
  for(byte datum = 0; datum < 9; datum++) {
    gfx.setTextColor(GFX_WHITE, GFX_BLACK);
    
    gfx.setTextDatum(datum);

    gfx.setTextSize(2);
    
    nice_gfx::text::draw(gfx, "[xox]", x, y, nice_gfx::fonts::Spleen12x24, 1);
    
    drawCross(x, y, GFX_GREEN);
    
    delay(1000);
    gfx.fillScreen(GFX_BLACK);
  }

  gfx.setTextSize(1);

  gfx.setTextColor(GFX_BLUE, GFX_BLACK);
  // Here a special function is used that automatically centres the text
  // on the x, y coordinate
  nice_gfx::text::center(gfx, "69", x, y, nice_gfx::fonts::Spleen12x24, 3);
  
  drawCross(x, y, GFX_CYAN);

  delay(1000);
  
  gfx.fillScreen(GFX_BLACK);
  
  gfx.setTextColor(GFX_RED, GFX_BLACK);
  nice_gfx::text::right(gfx, "88", 160, 120, nice_gfx::fonts::Spleen12x24, 3);
  
  drawCross(x, y, GFX_RED);
  
  delay(1000);
  
  gfx.fillScreen(GFX_BLACK);

  gfx.setTextColor(GFX_WHITE, GFX_BLUE);

  //Test floating point drawing function

  gfx.setTextDatum(BC_DATUM); // Set datum to bottom centre

  float test = 67.125;
  y = 180;
  nice_gfx::text::decimal(gfx, test, 4, x, y, nice_gfx::fonts::Spleen12x24, 1);
  delay(1000);
  gfx.fillScreen(GFX_BLACK);
  test = -0.555555;
  nice_gfx::text::decimal(gfx, test, 3, x, y, nice_gfx::fonts::Spleen12x24, 1);
  delay(1000);
  gfx.fillScreen(GFX_BLACK);
  test = 0.1;
  nice_gfx::text::decimal(gfx, test, 4, x, y, nice_gfx::fonts::Spleen12x24, 1);
  delay(1000);
  gfx.fillScreen(GFX_BLACK);
  test = 9999999;
  nice_gfx::text::decimal(gfx, test, 1, x, y, nice_gfx::fonts::Spleen12x24, 1);

  drawCross(x, y, GFX_RED);
  
  delay(4000);
}

void drawCross(int x, int y, unsigned int color)
{
  gfx.drawLine(x - 5, y, x + 5, y, color);
  gfx.drawLine(x, y - 5, x, y + 5, color);
}





