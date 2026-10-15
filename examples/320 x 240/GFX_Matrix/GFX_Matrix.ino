// A fun MATRIX-like screen demo of scrolling
// Screen will flicker initially until fully drawn
// then scroll smoothly

// Needs TomThumb4x6 bitmap font

/*
 Make sure all the display driver and pin connections are correct by
 editing the Display/Presets/ExampleBoard.h file in the NICE_GFX library folder.

 #########################################################################
 ###### DON'T FORGET TO UPDATE THE Display/Presets/ExampleBoard.h FILE IN THE LIBRARY ######
 #########################################################################
*/

#include <NICE_GFX.h>
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include <Display/Presets/ExampleBoard.h>
#include <SPI.h>

ExampleDisplay gfx;       // Invoke custom library

#define TEXT_HEIGHT 8 // Height of text to be printed and scrolled
#define BOT_FIXED_AREA 0  // Number of lines in bottom fixed area (lines counted from bottom of screen)
#define TOP_FIXED_AREA 0  // Number of lines in top fixed area (lines counted from top of screen)

uint16_t yStart = TOP_FIXED_AREA;
uint16_t yArea = 320 - TOP_FIXED_AREA - BOT_FIXED_AREA;
uint16_t yDraw = 320 - BOT_FIXED_AREA - TEXT_HEIGHT;
byte     pos[42];
uint16_t xPos = 0;

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(A0));
  gfx.begin();
  gfx.setRotation(0);
  gfx.fillScreen(GFX_BLACK);
  setupScrollArea(TOP_FIXED_AREA, BOT_FIXED_AREA);
}

void loop(void) {
  // First fill the screen with random streaks of characters
  for (int j = 0; j < 600; j += TEXT_HEIGHT) {
    for (int i = 0; i < 40; i++) {
      if (pos[i] > 20) pos[i] -= 3; // Rapid fade initially brightness values
      if (pos[i] > 0) pos[i] -= 1; // Slow fade later
      if ((random(20) == 1) && (j<400)) pos[i] = 63; // ~1 in 20 probability of a new character
      gfx.setTextColor(pos[i] << 5, GFX_BLACK); // Set the green character brightness
      if (pos[i] == 63) gfx.setTextColor(GFX_WHITE, GFX_BLACK); // Draw white character
      xPos += nice_gfx::text::glyph(gfx, random(32, 128), xPos, yDraw, nice_gfx::fonts::TomThumb4x6, 1); // Draw the character
    }
    yDraw = scroll_slow(TEXT_HEIGHT, 14); // Scroll, 14ms per pixel line
    xPos = 0;
  }

  //gfx.setRotation(2);
  //gfx.setTextColor(63 << 5, GFX_BLACK);
  //gfx.setRotation(0);

  // Now scroll smoothly forever
  while (1) {yield(); yDraw = scroll_slow(320,5); }// Scroll 320 lines, 5ms per line

}

void setupScrollArea(uint16_t TFA, uint16_t BFA) {
  gfx.writecommand(0x33); // Vertical scroll definition
  gfx.writedata(TFA >> 8);
  gfx.writedata(TFA);
  gfx.writedata((320 - TFA - BFA) >> 8);
  gfx.writedata(320 - TFA - BFA);
  gfx.writedata(BFA >> 8);
  gfx.writedata(BFA);
}

int scroll_slow(int lines, int wait) {
  int yTemp = yStart;
  for (int i = 0; i < lines; i++) {
    yStart++;
    if (yStart == 320 - BOT_FIXED_AREA) yStart = TOP_FIXED_AREA;
    scrollAddress(yStart);
    delay(wait);
  }
  return  yTemp;
}

void scrollAddress(uint16_t VSP) {
  gfx.writecommand(0x37); // Vertical scrolling start address
  gfx.writedata(VSP >> 8);
  gfx.writedata(VSP);
}



