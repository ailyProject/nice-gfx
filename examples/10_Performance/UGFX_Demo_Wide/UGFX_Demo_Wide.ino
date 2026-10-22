// Demo based on:
// UGFX_Demo by Henning Karlsen
// web: http://www.henningkarlsen.com/electronics
/*

 The delay between tests is set to 0. The tests run so fast you will need to
 change the WAIT value below to see what is being plotted!
 
 This sketch uses explicit MonoFont descriptors and integer text scaling.

 Make sure all the required fonts are loaded by editing the
 Hardware.h file in this sketch folder.

  #########################################################################
  ###### DON'T FORGET TO UPDATE THE Hardware.h FILE IN THIS SKETCH ######
  ######            TO SELECT THE FONTS YOU USE, SEE ABOVE           ######
  #########################################################################
 */

// Delay between demo pages
#define WAIT 0 // Delay between tests, set to 0 to demo speed, 2000 to see what it does!

#define CENTRE 240

#include <NICE_GFX.h>
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include "Hardware.h"
#include <SPI.h>

ExampleDisplay gfx;                   // Invoke custom library with default width and height

#define GFX_GREY 0x7BEF

uint32_t runTime = 0;

void setup()
{
  randomSeed(analogRead(0));
  Serial.begin(38400);
// Setup the LCD
  gfx.begin();
  gfx.setRotation(1);
}

void loop()
{
  int buf[478];
  int x, x2;
  int y, y2;
  int r;

  runTime = millis();
// Clear the screen and draw the frame
  gfx.fillScreen(GFX_BLACK);

  gfx.fillRect(0, 0, 480, 13, GFX_RED);

  gfx.fillRect(0, 305, 480, 320, GFX_GREY);
  gfx.setTextColor(GFX_BLACK,GFX_RED);

  nice_gfx::text::center(gfx, "* NICE_GFX *", CENTRE, 3, nice_gfx::fonts::TomThumb4x6, 1);
  gfx.setTextColor(GFX_YELLOW,GFX_GREY);
  nice_gfx::text::center(gfx, "NICE_GFX", CENTRE, 309, nice_gfx::fonts::TomThumb4x6, 1);

  gfx.drawRect(0, 14, 479, 305-14, GFX_BLUE);

// Draw crosshairs
  gfx.drawLine(239, 15, 239, 304, GFX_BLUE);
  gfx.drawLine(1, 159, 478, 159, GFX_BLUE);
  for (int i=9; i<470; i+=10)
    gfx.drawLine(i, 157, i, 161, GFX_BLUE);
  for (int i=19; i<220; i+=10)
    gfx.drawLine(237, i, 241, i, GFX_BLUE);

// Draw sin-, cos- and tan-lines  
  gfx.setTextColor(GFX_CYAN);
  nice_gfx::text::draw(gfx, "Sin", 5, 15, nice_gfx::fonts::GfxTerm8x16, 1);
  for (int i=1; i<478; i++)
  {
    gfx.drawPixel(i,159+(sin(((i*1.13)*3.14)/180)*95),GFX_CYAN);
  }
  
  gfx.setTextColor(GFX_RED);
  nice_gfx::text::draw(gfx, "Cos", 5, 30, nice_gfx::fonts::GfxTerm8x16, 1);
  for (int i=1; i<478; i++)
  {
    gfx.drawPixel(i,159+(cos(((i*1.13)*3.14)/180)*95),GFX_RED);
  }

  gfx.setTextColor(GFX_YELLOW);
  nice_gfx::text::draw(gfx, "Tan", 5, 45, nice_gfx::fonts::GfxTerm8x16, 1);
  for (int i=1; i<478; i++)
  {
    gfx.drawPixel(i,159+(tan(((i*1.13)*3.14)/180)),GFX_YELLOW);
  }

  delay(WAIT);

  gfx.fillRect(1,15,478-1,304-15,GFX_BLACK);
  gfx.drawLine(239, 15, 239, 304,GFX_BLUE);
  gfx.drawLine(1, 159, 478, 159,GFX_BLUE);

// Draw a moving sinewave
int col = 0;
  x=1;
  for (int i=1; i<(477*15); i++) 
  {
    x++;
    if (x==478)
      x=1;
    if (i>478)
    {
      if ((x==239)||(buf[x-1]==159))
        col = GFX_BLUE;
      else
        gfx.drawPixel(x,buf[x-1],GFX_BLACK);
    }
    y=159+(sin(((i*0.7)*3.14)/180)*(90-(i / 100)));
    gfx.drawPixel(x,y, GFX_BLUE);
    buf[x-1]=y;
  }

  delay(WAIT);
  
  gfx.fillRect(1,15,478-1,304-15,GFX_BLACK);

// Draw some filled rectangles
  for (int i=1; i<6; i++)
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
    gfx.fillRect(150+(i*20), 70+(i*20), 60, 60,col);
  }

  delay(WAIT);
  
  gfx.fillRect(1,15,478-1,304-15,GFX_BLACK);

// Draw some filled, rounded rectangles
  for (int i=1; i<6; i++)
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
    gfx.fillRoundRect(270-(i*20), 70+(i*20), 60, 60, 3, col);
  }
  
  delay(WAIT);
  
  gfx.fillRect(1,15,478-1,304-15,GFX_BLACK);

// Draw some filled circles
  for (int i=1; i<6; i++)
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
    gfx.fillCircle(180+(i*20),100+(i*20), 30,col);
  }
  
  delay(WAIT);
  
  gfx.fillRect(1,15,478-1,304-15,GFX_BLACK);

// Draw some lines in a pattern

  for (int i=15; i<304; i+=5)
  {
    gfx.drawLine(1, i, (i*1.6)-10, 303, GFX_RED);
  }

  for (int i=304; i>15; i-=5)
  {
    gfx.drawLine(477, i, (i*1.6)-11, 15, GFX_RED);
  }

  for (int i=304; i>15; i-=5)
  {
    gfx.drawLine(1, i, 491-(i*1.6), 15, GFX_CYAN);
  }

  for (int i=15; i<304; i+=5)
  {
    gfx.drawLine(477, i, 490-(i*1.6), 303, GFX_CYAN);
  }
  
  delay(WAIT);
  
  gfx.fillRect(1,15,478-1,304-15,GFX_BLACK);

// Draw some random circles
  for (int i=0; i<100; i++)
  {
    x=32+random(416);
    y=45+random(226);
    r=random(30);
    gfx.drawCircle(x, y, r,random(0xFFFF));
  }

  delay(WAIT);
  
  gfx.fillRect(1,15,478-1,304-15,GFX_BLACK);

// Draw some random rectangles
  for (int i=0; i<100; i++)
  {
    x=2+random(476);
    y=16+random(289);
    x2=2+random(476);
    y2=16+random(289);
    if (x2<x) {
      r=x;x=x2;x2=r;
    }
    if (y2<y) {
      r=y;y=y2;y2=r;
    }
    gfx.drawRect(x, y, x2-x, y2-y,random(0xFFFF));
  }

  delay(WAIT);
  
  gfx.fillRect(1,15,478-1,304-15,GFX_BLACK);

// Draw some random rounded rectangles
  for (int i=0; i<100; i++)
  {
    x=2+random(476);
    y=16+random(289);
    x2=2+random(476);
    y2=16+random(289);
    if (x2<x) {
      r=x;x=x2;x2=r;
    }
    if (y2<y) {
      r=y;y=y2;y2=r;
    }
    gfx.drawRoundRect(x, y, x2-x, y2-y, 3,random(0xFFFF));
  }

  delay(WAIT);
  
  gfx.fillRect(1,15,478-1,304-15,GFX_BLACK);

  for (int i=0; i<100; i++)
  {
    x=2+random(476);
    y=16+random(289);
    x2=2+random(476);
    y2=16+random(289);
    col=random(0xFFFF);
    gfx.drawLine(x, y, x2, y2,col);
  }

  delay(WAIT);
  
  gfx.fillRect(1,15,478-1,304-15,GFX_BLACK);

  for (int i=0; i<10000; i++)
  {
    gfx.drawPixel(2+random(476), 16+random(289),random(0xFFFF));
  }

  delay(WAIT);

  gfx.fillRect(0, 0, 480, 320, GFX_BLUE);

  gfx.fillRoundRect(160, 70, 319-160, 169-70, 3,GFX_RED);
  
  gfx.setTextColor(GFX_WHITE,GFX_RED);
  nice_gfx::text::center(gfx, "That's it!", CENTRE, 93, nice_gfx::fonts::GfxTerm8x16, 1);
  nice_gfx::text::center(gfx, "Restarting in a", CENTRE, 119, nice_gfx::fonts::GfxTerm8x16, 1);
  nice_gfx::text::center(gfx, "few seconds...", CENTRE, 132, nice_gfx::fonts::GfxTerm8x16, 1);

  gfx.setTextColor(GFX_GREEN,GFX_BLUE);
  nice_gfx::text::center(gfx, "Runtime: (msecs)", CENTRE, 280, nice_gfx::fonts::GfxTerm8x16, 1);
  gfx.setTextDatum(TC_DATUM);
  runTime = millis()-runTime;
  nice_gfx::text::number(gfx, runTime, CENTRE, 300, nice_gfx::fonts::GfxTerm8x16, 1);
  gfx.setTextDatum(TL_DATUM);
  delay (10000);
}

