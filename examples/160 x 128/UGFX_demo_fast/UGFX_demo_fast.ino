// Demo based on:
// UGFX_Demo_320x240 by Henning Karlsen
// web: http://www.henningkarlsen.com/electronics
//
/*

  This sketch uses explicit MonoFont descriptors and integer text scaling.

  Make sure all the display driver and pin connections are correct by
  editing the Display/Presets/ExampleBoard.h file in the NICE_GFX library folder.

  Note that yield() or delay(0) must be called in long duration for/while
  loops to stop the ESP8266 watchdog triggering.

  #########################################################################
  ###### DON'T FORGET TO UPDATE THE Display/Presets/ExampleBoard.h FILE IN THE LIBRARY ######
  #########################################################################
*/

#include <NICE_GFX.h>
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include <Display/Presets/ExampleBoard.h>
#include <SPI.h>

ExampleDisplay myGLCD;  // Invoke library, pins in Display/Presets/ExampleBoard.h

#define DELAY 500

#define GFX_GREY 0x7BEF
#define GFX_W 160
#define GFX_H 128

unsigned long runTime = 0;
void setup()
{
  randomSeed(analogRead(A0));
  // Setup the LCD
  myGLCD.begin();
  myGLCD.setRotation(1);
}

void loop()
{
  //randomSeed(millis());
  randomSeed(1234); // This ensure test is repeatable with exact same draws each loop
  int buf[GFX_W - 2];
  int x, x2;
  int y, y2;
  int r;
  runTime = millis();
  // Clear the screen and draw the frame
  myGLCD.fillScreen(GFX_BLACK);


  myGLCD.fillRect(0, 0, GFX_W - 1, 14, GFX_RED);

  myGLCD.fillRect(0, GFX_H - 14, GFX_W - 1, 14, GFX_GREY);

  myGLCD.setTextColor(GFX_BLACK, GFX_RED);
  nice_gfx::text::center(myGLCD, "* GFX_S6D02A1 *", GFX_W / 2, 4, nice_gfx::fonts::TomThumb4x6, 1);
  myGLCD.setTextColor(GFX_YELLOW, GFX_GREY);
  nice_gfx::text::center(myGLCD, "NICE_GFX", GFX_W / 2, GFX_H - 12, nice_gfx::fonts::TomThumb4x6, 1);

  myGLCD.drawRect(0, 14, GFX_W - 1, GFX_H - 28, GFX_BLUE);

  // Draw crosshairs
  myGLCD.drawLine(GFX_W / 2 - 1, 15, GFX_W / 2 - 1, GFX_H - 16, GFX_BLUE);
  myGLCD.drawLine(1, GFX_H / 2 - 1, GFX_W - 2, GFX_H / 2 - 1, GFX_BLUE);
  for (int i = 9; i < GFX_W - 1; i += 10)
    myGLCD.drawLine(i, GFX_H / 2 - 3, i, GFX_H / 2 + 1, GFX_BLUE);
  for (int i = 19; i < GFX_H - 20; i += 10)
    myGLCD.drawLine(GFX_W / 2 - 3, i, GFX_W / 2 + 1, i, GFX_BLUE);

  // Draw sin-, cos- and tan-lines
  myGLCD.setTextColor(GFX_CYAN);
  nice_gfx::text::draw(myGLCD, "Sin", 5, 15, nice_gfx::fonts::GfxTerm8x16, 1);
  for (int i = 1; i < GFX_W - 2; i++)
  {
    myGLCD.drawPixel(i, GFX_H / 2 - 1 + (sin(((i * 2.26) * 3.14) / 180) * 48), GFX_CYAN);
  }
  myGLCD.setTextColor(GFX_RED);
  nice_gfx::text::draw(myGLCD, "Cos", 5, 30, nice_gfx::fonts::GfxTerm8x16, 1);
  for (int i = 1; i < GFX_W - 2; i++)
  {
    myGLCD.drawPixel(i, GFX_H / 2 - 1 + (cos(((i * 2.26) * 3.14) / 180) * 48), GFX_RED);
  }
  myGLCD.setTextColor(GFX_YELLOW);
  nice_gfx::text::draw(myGLCD, "Tan", 5, 45, nice_gfx::fonts::GfxTerm8x16, 1);
  for (int i = 1; i < GFX_W - 2; i++)
  {
    myGLCD.drawPixel(i, GFX_H / 2 - 1 + (tan(((i * 2.26) * 3.14) / 180)), GFX_YELLOW);
  }

  delay(DELAY);

  myGLCD.fillRect(1, 15, GFX_W - 3, GFX_H - 31, GFX_BLACK);

  myGLCD.drawLine(GFX_W / 2 - 1, 15, GFX_W / 2 - 1, GFX_H - 16, GFX_BLUE);
  myGLCD.drawLine(1, GFX_H / 2 - 1, GFX_W - 2, GFX_H / 2 - 1, GFX_BLUE);
  int col = 0;
  // Draw a moving sinewave
  x = 1;
  for (int i = 1; i < ((GFX_W - 3) * 20); i++)
  {
    x++;
    if (x == GFX_W - 2)
      x = 1;
    if (i > GFX_W - 2)
    {
      if ((x == GFX_W / 2 - 1) || (buf[x - 1] == GFX_H / 2 - 1))
        col = GFX_BLUE;
      else
        myGLCD.drawPixel(x, buf[x - 1], GFX_BLACK);
    }
    y = GFX_H / 2 + (sin(((i * 2.2) * 3.14) / 180) * (49 - (i / 100)));
    myGLCD.drawPixel(x, y, GFX_BLUE);
    buf[x - 1] = y;
  }

  delay(DELAY);

  myGLCD.fillRect(1, 15, GFX_W - 3, GFX_H - 31, GFX_BLACK);

  // Draw some filled rectangles
  for (int i = 1; i < 6; i++)
  {
    switch (i)
    {
      case 1:
        col = GFX_MAGENTA;
        break;
      case 2:
        col = GFX_RED;
        break;
      case 3:
        col = GFX_GREEN;
        break;
      case 4:
        col = GFX_BLUE;
        break;
      case 5:
        col = GFX_YELLOW;
        break;
    }
    myGLCD.fillRect(30 + (i * 10), 20 + (i * 10), 30, 30, col);
  }

  delay(DELAY);

  myGLCD.fillRect(1, 15, GFX_W - 3, GFX_H - 31, GFX_BLACK);

  // Draw some filled, rounded rectangles
  for (int i = 1; i < 6; i++)
  {
    switch (i)
    {
      case 1:
        col = GFX_MAGENTA;
        break;
      case 2:
        col = GFX_RED;
        break;
      case 3:
        col = GFX_GREEN;
        break;
      case 4:
        col = GFX_BLUE;
        break;
      case 5:
        col = GFX_YELLOW;
        break;
    }
    myGLCD.fillRoundRect(GFX_W / 2 + 20 - (i * 10), 20 + (i * 10), 30, 30, 3, col);
  }

  delay(DELAY);

  myGLCD.fillRect(1, 15, GFX_W - 3, GFX_H - 31, GFX_BLACK);

  // Draw some filled circles
  for (int i = 1; i < 6; i++)
  {
    switch (i)
    {
      case 1:
        col = GFX_MAGENTA;
        break;
      case 2:
        col = GFX_RED;
        break;
      case 3:
        col = GFX_GREEN;
        break;
      case 4:
        col = GFX_BLUE;
        break;
      case 5:
        col = GFX_YELLOW;
        break;
    }
    myGLCD.fillCircle(45 + (i * 10), 30 + (i * 10), 15, col);
  }

  delay(DELAY);

  myGLCD.fillRect(1, 15, GFX_W - 3, GFX_H - 31, GFX_BLACK);

  // Draw some lines in a pattern

  for (int i = 15; i < GFX_H - 16; i += 5)
  {
    myGLCD.drawLine(1, i, (i * 1.44) - 10, GFX_H - 17, GFX_RED);
  }

  for (int i = GFX_H - 17; i > 15; i -= 5)
  {
    myGLCD.drawLine(GFX_W - 3, i, (i * 1.44) - 11, 15, GFX_RED);
  }

  for (int i = GFX_H - 17; i > 15; i -= 5)
  {
    myGLCD.drawLine(1, i, GFX_W + 11 - (i * 1.44), 15, GFX_CYAN);
  }

  for (int i = 15; i < GFX_H - 16; i += 5)
  {
    myGLCD.drawLine(GFX_W - 3, i, GFX_W + 10 - (i * 1.44), GFX_H - 17, GFX_CYAN);
  }

  delay(DELAY);


  myGLCD.fillRect(1, 15, GFX_W - 3, GFX_H - 31, GFX_BLACK);

  // Draw some random circles
  for (int i = 0; i < 100; i++)
  {
    x = 32 + random(GFX_W - 2 - 32 - 30);
    y = 45 + random(GFX_H - 19 - 45 - 30);
    r = random(30);
    myGLCD.drawCircle(x, y, r, random(0xFFFF));
  }

  delay(DELAY);

  myGLCD.fillRect(1, 15, GFX_W - 3, GFX_H - 31, GFX_BLACK);

  // Draw some random rectangles
  for (int i = 0; i < 100; i++)
  {
    x = 2 + random(GFX_W - 4);
    y = 16 + random(GFX_H - 33);
    x2 = 2 + random(GFX_H - 4);
    y2 = 16 + random(GFX_H - 33);
    if (x2 < x) {
      r = x; x = x2; x2 = r;
    }
    if (y2 < y) {
      r = y; y = y2; y2 = r;
    }
    myGLCD.drawRect(x, y, x2 - x, y2 - y, random(0xFFFF));
  }

  delay(DELAY);


  myGLCD.fillRect(1, 15, GFX_W - 3, GFX_H - 31, GFX_BLACK);

  // Draw some random rounded rectangles
  for (int i = 0; i < 100; i++)
  {
    x = 2 + random(GFX_W - 4);
    y = 16 + random(GFX_H - 33);
    x2 = 2 + random(GFX_W - 4);
    y2 = 16 + random(GFX_H - 33);
    // We need to get the width and height and do some window checking
    if (x2 < x) {
      r = x; x = x2; x2 = r;
    }
    if (y2 < y) {
      r = y; y = y2; y2 = r;
    }
    // We need a minimum size of 6
    if ((x2 - x) < 6) x2 = x + 6;
    if ((y2 - y) < 6) y2 = y + 6;
    myGLCD.drawRoundRect(x, y, x2 - x, y2 - y, 3, random(0xFFFF));
  }

  delay(DELAY);

  myGLCD.fillRect(1, 15, GFX_W - 3, GFX_H - 31, GFX_BLACK);

  //randomSeed(1234);
  int colour = 0;
  for (int i = 0; i < 100; i++)
  {
    x = 2 + random(GFX_W - 4);
    y = 16 + random(GFX_H - 31);
    x2 = 2 + random(GFX_W - 4);
    y2 = 16 + random(GFX_H - 31);
    colour = random(0xFFFF);
    myGLCD.drawLine(x, y, x2, y2, colour);
  }

  delay(DELAY);

  myGLCD.fillRect(1, 15, GFX_W - 3, GFX_H - 31, GFX_BLACK);

  #define RANDOM

#ifdef RANDOM
  // Draw 10,000 pixels
  // It takes 30ms to calculate the 30,000 random numbers so this is not a true drawPixel speed test
  for (int i=0; i<10000; i++)
  {
    myGLCD.drawPixel(2+random(GFX_W - 3), 16+random(GFX_H - 31),random(0xFFFF));
  }
#else
  // Draw 10,000 pixels to fill a 100x100 pixel box, better drawPixel speed test
  // use the coords as the colour to produce the banding
  byte i = 100;
  while (i--) {
    byte j = 100;
    while (j--) myGLCD.drawPixel(i + GFX_W / 2 - 50, j + GFX_H / 2 - 50, i + j);
  }
#endif

  delay(DELAY);

  myGLCD.fillScreen(GFX_BLUE);
  myGLCD.fillRoundRect(20, 20, GFX_W - 40, GFX_H - 60, 6, GFX_RED);

  myGLCD.setTextColor(GFX_WHITE, GFX_RED);
  nice_gfx::text::center(myGLCD, "That's it!", GFX_W / 2 - 1, 23, nice_gfx::fonts::GfxTerm8x16, 1);
  nice_gfx::text::center(myGLCD, "Restarting in a", GFX_W / 2 - 1, 40, nice_gfx::fonts::GfxTerm8x16, 1);
  nice_gfx::text::center(myGLCD, "few seconds...", GFX_W / 2 - 1, 57, nice_gfx::fonts::GfxTerm8x16, 1);

  runTime = millis() - runTime;
  myGLCD.setTextColor(GFX_GREEN, GFX_BLUE);
  nice_gfx::text::center(myGLCD, "Draw time: (msecs)", GFX_W / 2, GFX_H - 34, nice_gfx::fonts::GfxTerm8x16, 1);
  nice_gfx::text::number(myGLCD, runTime - 11 * DELAY, GFX_W / 2 - 20, GFX_H - 17, nice_gfx::fonts::GfxTerm8x16, 1);
  delay (5000);
}


