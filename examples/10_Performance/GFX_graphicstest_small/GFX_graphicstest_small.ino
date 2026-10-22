/*
 Adapted from the Adafruit graphicstest sketch.

 This sketch uses the TomThumb4x6 bitmap font only. Disable other fonts to make
 the sketch fit in an UNO!

 Make sure all the display driver and pin connections are correct by
 editing the Hardware.h file in this sketch folder.

 Note that yield() or delay(0) must be called in long duration for/while
 loops to stop the ESP8266 watchdog triggering.

 #########################################################################
 ###### DON'T FORGET TO UPDATE THE Hardware.h FILE IN THIS SKETCH ######
 #########################################################################
 */


#include <NICE_GFX.h> // Hardware-specific library
#include "Hardware.h"
#include <SPI.h>

ExampleDisplay gfx;       // Invoke custom library

float p = 3.1415926;

void setup(void) {
  Serial.begin(9600);
  Serial.print("Hello! ST7735 TFT Test");

  // Use this initializer if you're using a 1.8" TFT
  gfx.begin();
  gfx.setFont(nice_gfx::fonts::TomThumb4x6);   // initialize a ST7735S chip

  Serial.println("Initialized");

  uint16_t time = millis();
  gfx.fillScreen(GFX_BLACK);
  time = millis() - time;

  Serial.println(time, DEC);
  delay(500);

  // large block of text
  gfx.fillScreen(GFX_BLACK);
  testdrawtext("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur adipiscing ante sed nibh tincidunt feugiat. Maecenas enim massa, fringilla sed malesuada et, malesuada sit amet turpis. Sed porttitor neque ut ante pretium vitae malesuada nunc bibendum. Nullam aliquet ultrices massa eu hendrerit. Ut sed nisi lorem. In vestibulum purus a tortor imperdiet posuere. ", GFX_WHITE);
  delay(1000);

  // gfx print function!
  tftPrintTest();
  delay(4000);

  // a single pixel
  gfx.drawPixel(gfx.width()/2, gfx.height()/2, GFX_GREEN);
  delay(500);

  // line draw test
  testlines(GFX_YELLOW);
  delay(500);

  // optimized lines
  testfastlines(GFX_RED, GFX_BLUE);
  delay(500);

  testdrawrects(GFX_GREEN);
  delay(500);

  testfillrects(GFX_YELLOW, GFX_MAGENTA);
  delay(500);

  gfx.fillScreen(GFX_BLACK);
  testfillcircles(10, GFX_BLUE);
  testdrawcircles(10, GFX_WHITE);
  delay(500);

  testroundrects();
  delay(500);

  testtriangles();
  delay(500);

  mediabuttons();
  delay(500);

  Serial.println("done");
  delay(1000);
}

void loop() {
  gfx.invertDisplay(true);
  delay(500);
  gfx.invertDisplay(false);
  delay(500);
}

void testlines(uint16_t color) {
  gfx.fillScreen(GFX_BLACK);
  for (int16_t x=0; x < gfx.width(); x+=6) {
    gfx.drawLine(0, 0, x, gfx.height()-1, color);
  }
  for (int16_t y=0; y < gfx.height(); y+=6) {
    gfx.drawLine(0, 0, gfx.width()-1, y, color);
  }

  gfx.fillScreen(GFX_BLACK);
  for (int16_t x=0; x < gfx.width(); x+=6) {
    gfx.drawLine(gfx.width()-1, 0, x, gfx.height()-1, color);
  }
  for (int16_t y=0; y < gfx.height(); y+=6) {
    gfx.drawLine(gfx.width()-1, 0, 0, y, color);
  }

  gfx.fillScreen(GFX_BLACK);
  for (int16_t x=0; x < gfx.width(); x+=6) {
    gfx.drawLine(0, gfx.height()-1, x, 0, color);
  }
  for (int16_t y=0; y < gfx.height(); y+=6) {
    gfx.drawLine(0, gfx.height()-1, gfx.width()-1, y, color);
  }

  gfx.fillScreen(GFX_BLACK);
  for (int16_t x=0; x < gfx.width(); x+=6) {
    gfx.drawLine(gfx.width()-1, gfx.height()-1, x, 0, color);
  }
  for (int16_t y=0; y < gfx.height(); y+=6) {
    gfx.drawLine(gfx.width()-1, gfx.height()-1, 0, y, color);
  }
}

void testdrawtext(char *text, uint16_t color) {
  gfx.setCursor(0, 0);
  gfx.setTextColor(color);
  gfx.setTextWrap(true);
  gfx.print(text);
}

void testfastlines(uint16_t color1, uint16_t color2) {
  gfx.fillScreen(GFX_BLACK);
  for (int16_t y=0; y < gfx.height(); y+=5) {
    gfx.drawFastHLine(0, y, gfx.width(), color1);
  }
  for (int16_t x=0; x < gfx.width(); x+=5) {
    gfx.drawFastVLine(x, 0, gfx.height(), color2);
  }
}

void testdrawrects(uint16_t color) {
  gfx.fillScreen(GFX_BLACK);
  for (int16_t x=0; x < gfx.width(); x+=6) {
    gfx.drawRect(gfx.width()/2 -x/2, gfx.height()/2 -x/2 , x, x, color);
  }
}

void testfillrects(uint16_t color1, uint16_t color2) {
  gfx.fillScreen(GFX_BLACK);
  for (int16_t x=gfx.width()-1; x > 6; x-=6) {
    gfx.fillRect(gfx.width()/2 -x/2, gfx.height()/2 -x/2 , x, x, color1);
    gfx.drawRect(gfx.width()/2 -x/2, gfx.height()/2 -x/2 , x, x, color2);
  }
}

void testfillcircles(uint8_t radius, uint16_t color) {
  for (int16_t x=radius; x < gfx.width(); x+=radius*2) {
    for (int16_t y=radius; y < gfx.height(); y+=radius*2) {
      gfx.fillCircle(x, y, radius, color);
    }
  }
}

void testdrawcircles(uint8_t radius, uint16_t color) {
  for (int16_t x=0; x < gfx.width()+radius; x+=radius*2) {
    for (int16_t y=0; y < gfx.height()+radius; y+=radius*2) {
      gfx.drawCircle(x, y, radius, color);
    }
  }
}

void testtriangles() {
  gfx.fillScreen(GFX_BLACK);
  int color = 0xF800;
  int t;
  int w = gfx.width()/2;
  int x = gfx.height()-1;
  int y = 0;
  int z = gfx.width();
  for(t = 0 ; t <= 15; t+=1) {
    gfx.drawTriangle(w, y, y, x, z, x, color);
    x-=4;
    y+=4;
    z-=4;
    color+=100;
  }
}

void testroundrects() {
  gfx.fillScreen(GFX_BLACK);
  int color = 100;
  int i;
  int t;
  for(t = 0 ; t <= 4; t+=1) {
    int x = 0;
    int y = 0;
    int w = gfx.width()-2;
    int h = gfx.height()-2;
    for(i = 0 ; i <= 16; i+=1) {
      gfx.drawRoundRect(x, y, w, h, 5, color);
      x+=2;
      y+=3;
      w-=4;
      h-=6;
      color+=1100;
    }
    color+=100;
  }
}

void tftPrintTest() {
  gfx.setTextWrap(false);
  gfx.fillScreen(GFX_BLACK);
  gfx.setCursor(0, 30);
  gfx.setTextColor(GFX_RED);
  gfx.setTextSize(1);
  gfx.println("Hello World!");
  gfx.setTextColor(GFX_YELLOW);
  gfx.setTextSize(2);
  gfx.println("Hello World!");
  gfx.setTextColor(GFX_GREEN);
  gfx.setTextSize(3);
  gfx.println("Hello World!");
  gfx.setTextColor(GFX_BLUE);
  gfx.setTextSize(4);
  gfx.print(1234.567);
  delay(1500);
  gfx.setCursor(0, 0);
  gfx.fillScreen(GFX_BLACK);
  gfx.setTextColor(GFX_WHITE);
  gfx.setTextSize(0);
  gfx.println("Hello World!");
  gfx.setTextSize(1);
  gfx.setTextColor(GFX_GREEN);
  gfx.print(p, 6);
  gfx.println(" Want pi?");
  gfx.println(" ");
  gfx.print(8675309, HEX); // print 8,675,309 out in HEX!
  gfx.println(" Print HEX!");
  gfx.println(" ");
  gfx.setTextColor(GFX_WHITE);
  gfx.println("Sketch has been");
  gfx.println("running for: ");
  gfx.setTextColor(GFX_MAGENTA);
  gfx.print(millis() / 1000);
  gfx.setTextColor(GFX_WHITE);
  gfx.print(" seconds.");
}

void mediabuttons() {
  // play
  gfx.fillScreen(GFX_BLACK);
  gfx.fillRoundRect(25, 10, 78, 60, 8, GFX_WHITE);
  gfx.fillTriangle(42, 20, 42, 60, 90, 40, GFX_RED);
  delay(500);
  // pause
  gfx.fillRoundRect(25, 90, 78, 60, 8, GFX_WHITE);
  gfx.fillRoundRect(39, 98, 20, 45, 5, GFX_GREEN);
  gfx.fillRoundRect(69, 98, 20, 45, 5, GFX_GREEN);
  delay(500);
  // play color
  gfx.fillTriangle(42, 20, 42, 60, 90, 40, GFX_BLUE);
  delay(50);
  // pause color
  gfx.fillRoundRect(39, 98, 20, 45, 5, GFX_RED);
  gfx.fillRoundRect(69, 98, 20, 45, 5, GFX_RED);
  // play color
  gfx.fillTriangle(42, 20, 42, 60, 90, 40, GFX_GREEN);
}

