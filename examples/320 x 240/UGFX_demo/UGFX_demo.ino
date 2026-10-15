// Demo based on:
// UGFX_Demo_320x240 by Henning Karlsen
// web: http://www.henningkarlsen.com/electronics
//
/*

 This sketch uses explicit MonoFont descriptors and integer text scaling.
 
 Make sure all the display driver and pin connections are correct by
 editing the Display/Presets/ExampleBoard.h file in the NICE_GFX library folder.

 #########################################################################
 ###### DON'T FORGET TO UPDATE THE Display/Presets/ExampleBoard.h FILE IN THE LIBRARY ######
 #########################################################################
 */

#include "SPI.h"

#include "NICE_GFX.h"
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include <Display/Presets/ExampleBoard.h>

#define GFX_GREY 0x7BEF

ExampleDisplay myGLCD;       // Invoke custom library

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
  randomSeed(millis());
  //randomSeed(1234); // This ensure test is repeatable with exact same draws each loop
  int buf[318];
  int x, x2;
  int y, y2;
  int r;
  runTime = millis();
// Clear the screen and draw the frame
  myGLCD.fillScreen(GFX_BLACK);


  myGLCD.fillRect(0, 0, 319, 14,GFX_RED);

  myGLCD.fillRect(0, 226, 319, 14,GFX_GREY);

  myGLCD.setTextColor(GFX_BLACK,GFX_RED);
  nice_gfx::text::center(myGLCD, "* NICE_GFX *", 160, 4, nice_gfx::fonts::TomThumb4x6, 1);
  myGLCD.setTextColor(GFX_YELLOW,GFX_GREY);
  nice_gfx::text::center(myGLCD, "NICE_GFX", 160, 228, nice_gfx::fonts::TomThumb4x6, 1);

  myGLCD.drawRect(0, 14, 319, 211, GFX_BLUE);

// Draw crosshairs
  myGLCD.drawLine(159, 15, 159, 224,GFX_BLUE);
  myGLCD.drawLine(1, 119, 318, 119,GFX_BLUE);
  for (int i=9; i<310; i+=10)
    myGLCD.drawLine(i, 117, i, 121,GFX_BLUE);
  for (int i=19; i<220; i+=10)
    myGLCD.drawLine(157, i, 161, i,GFX_BLUE);

// Draw sin-, cos- and tan-lines  
  myGLCD.setTextColor(GFX_CYAN);
  nice_gfx::text::draw(myGLCD, "Sin", 5, 15, nice_gfx::fonts::GfxTerm8x16, 1);
  for (int i=1; i<318; i++)
  {
    myGLCD.drawPixel(i,119+(sin(((i*1.13)*3.14)/180)*95),GFX_CYAN);
  }
  myGLCD.setTextColor(GFX_RED);
  nice_gfx::text::draw(myGLCD, "Cos", 5, 30, nice_gfx::fonts::GfxTerm8x16, 1);
  for (int i=1; i<318; i++)
  {
    myGLCD.drawPixel(i,119+(cos(((i*1.13)*3.14)/180)*95),GFX_RED);
  }
  myGLCD.setTextColor(GFX_YELLOW);
  nice_gfx::text::draw(myGLCD, "Tan", 5, 45, nice_gfx::fonts::GfxTerm8x16, 1);
  for (int i=1; i<318; i++)
  {
    myGLCD.drawPixel(i,119+(tan(((i*1.13)*3.14)/180)),GFX_YELLOW);
  }

  delay(0);

  myGLCD.fillRect(1,15,317,209,GFX_BLACK);

  myGLCD.drawLine(159, 15, 159, 224,GFX_BLUE);
  myGLCD.drawLine(1, 119, 318, 119,GFX_BLUE);
int col = 0;
// Draw a moving sinewave
  x=1;
  for (int i=1; i<(317*20); i++) 
  {
    x++;
    if (x==318)
      x=1;
    if (i>318)
    {
      if ((x==159)||(buf[x-1]==119))
        col = GFX_BLUE;
      else
      myGLCD.drawPixel(x,buf[x-1],GFX_BLACK);
    }
    y=119+(sin(((i*1.1)*3.14)/180)*(90-(i / 100)));
    myGLCD.drawPixel(x,y,GFX_BLUE);
    buf[x-1]=y;
  }

  delay(0);

  myGLCD.fillRect(1,15,317,209,GFX_BLACK);

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
    myGLCD.fillRect(70+(i*20), 30+(i*20), 60, 60,col);
  }

  delay(0);

  myGLCD.fillRect(1,15,317,209,GFX_BLACK);

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
    myGLCD.fillRoundRect(190-(i*20), 30+(i*20), 60,60, 3,col);
  }
  
  delay(0);

  myGLCD.fillRect(1,15,317,209,GFX_BLACK);

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
    myGLCD.fillCircle(100+(i*20),60+(i*20), 30,col);
  }
  
  delay(0);

  myGLCD.fillRect(1,15,317,209,GFX_BLACK);

// Draw some lines in a pattern

  for (int i=15; i<224; i+=5)
  {
    myGLCD.drawLine(1, i, (i*1.44)-10, 223,GFX_RED);
  }

  for (int i=223; i>15; i-=5)
  {
    myGLCD.drawLine(317, i, (i*1.44)-11, 15,GFX_RED);
  }

  for (int i=223; i>15; i-=5)
  {
    myGLCD.drawLine(1, i, 331-(i*1.44), 15,GFX_CYAN);
  }

  for (int i=15; i<224; i+=5)
  {
    myGLCD.drawLine(317, i, 330-(i*1.44), 223,GFX_CYAN);
  }
  
  delay(0);


  myGLCD.fillRect(1,15,317,209,GFX_BLACK);

// Draw some random circles
  for (int i=0; i<100; i++)
  {
    x=32+random(256);
    y=45+random(146);
    r=random(30);
    myGLCD.drawCircle(x, y, r,random(0xFFFF));
  }

  delay(0);

  myGLCD.fillRect(1,15,317,209,GFX_BLACK);

// Draw some random rectangles
  for (int i=0; i<100; i++)
  {
    x=2+random(316);
    y=16+random(207);
    x2=2+random(316);
    y2=16+random(207);
    if (x2<x) {
      r=x;x=x2;x2=r;
    }
    if (y2<y) {
      r=y;y=y2;y2=r;
    }
    myGLCD.drawRect(x, y, x2-x, y2-y,random(0xFFFF));
  }

  delay(0);


  myGLCD.fillRect(1,15,317,209,GFX_BLACK);

// Draw some random rounded rectangles
  for (int i=0; i<100; i++)
  {
    x=2+random(316);
    y=16+random(207);
    x2=2+random(316);
    y2=16+random(207);
    // We need to get the width and height and do some window checking
    if (x2<x) {
      r=x;x=x2;x2=r;
    }
    if (y2<y) {
      r=y;y=y2;y2=r;
    }
    // We need a minimum size of 6
    if((x2-x)<6) x2=x+6;
    if((y2-y)<6) y2=y+6;
    myGLCD.drawRoundRect(x, y, x2-x,y2-y, 3,random(0xFFFF));
  }

  delay(0);

  myGLCD.fillRect(1,15,317,209,GFX_BLACK);

 //randomSeed(1234);
 int colour = 0;
 for (int i=0; i<100; i++)
  {
    x=2+random(316);
    y=16+random(209);
    x2=2+random(316);
    y2=16+random(209);
    colour=random(0xFFFF);
    myGLCD.drawLine(x, y, x2, y2,colour);
  }

  delay(0);

  myGLCD.fillRect(1,15,317,209,GFX_BLACK);

  // This test has been modified as it takes more time to calculate the random numbers
  // than to draw the pixels (3 seconds to produce 30,000 randoms)!
  for (int i=0; i<10000; i++)
  {
    myGLCD.drawPixel(2+random(316), 16+random(209),random(0xFFFF));
  }

  // Draw 10,000 pixels to fill a 100x100 pixel box
  // use the coords as the colour to produce the banding
  //byte i = 100;
  //while (i--) {
  //  byte j = 100;
  //  while (j--) myGLCD.drawPixel(i+110,j+70,i+j);
  //  //while (j--) myGLCD.drawPixel(i+110,j+70,0xFFFF);
  //}
  delay(0);

  myGLCD.fillScreen(GFX_BLUE);
  myGLCD.fillRoundRect(80, 70, 239-80,169-70, 3,GFX_RED);
  
  myGLCD.setTextColor(GFX_WHITE,GFX_RED);
  nice_gfx::text::center(myGLCD, "That's it!", 160, 93, nice_gfx::fonts::GfxTerm8x16, 1);
  nice_gfx::text::center(myGLCD, "Restarting in a", 160, 119, nice_gfx::fonts::GfxTerm8x16, 1);
  nice_gfx::text::center(myGLCD, "few seconds...", 160, 132, nice_gfx::fonts::GfxTerm8x16, 1);

  runTime = millis()-runTime;
  myGLCD.setTextColor(GFX_GREEN,GFX_BLUE);
  nice_gfx::text::center(myGLCD, "Runtime: (msecs)", 160, 210, nice_gfx::fonts::GfxTerm8x16, 1);
  myGLCD.setTextDatum(TC_DATUM);
  nice_gfx::text::number(myGLCD, runTime, 160, 225, nice_gfx::fonts::GfxTerm8x16, 1);
  delay (5000);
}


