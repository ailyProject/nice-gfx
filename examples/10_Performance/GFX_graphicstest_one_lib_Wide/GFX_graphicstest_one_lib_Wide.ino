/*
 Adapted from the Adafruit graphicstest sketch, see original header at end
 of sketch.

 This sketch uses the TomThumb4x6 bitmap font only.

 Make sure all the display driver and pin connections are correct by
 editing the Hardware.h file in this sketch folder.

 #########################################################################
 ###### DON'T FORGET TO UPDATE THE Hardware.h FILE IN THIS SKETCH ######
 #########################################################################
*/


#include "SPI.h"
#include "NICE_GFX.h"
#include "Hardware.h"

ExampleDisplay gfx;

unsigned long total = 0;
unsigned long tn = 0;
void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println(""); Serial.println("");
  Serial.println("NICE_GFX library test!");

  gfx.begin();
  gfx.setFont(nice_gfx::fonts::TomThumb4x6);

  tn = micros();
  gfx.fillScreen(GFX_BLACK);

  yield(); Serial.println(F("Benchmark                Time (microseconds)"));

  yield(); Serial.print(F("Screen fill              "));
  yield(); Serial.println(testFillScreen());
  //total+=testFillScreen();
  //delay(500);

  yield(); Serial.print(F("Text                     "));
  yield(); Serial.println(testText());
  //total+=testText();
  //delay(3000);

  yield(); Serial.print(F("Lines                    "));
  yield(); Serial.println(testLines(GFX_CYAN));
  //total+=testLines(GFX_CYAN);
  //delay(500);

  yield(); Serial.print(F("Horiz/Vert Lines         "));
  yield(); Serial.println(testFastLines(GFX_RED, GFX_BLUE));
  //total+=testFastLines(GFX_RED, GFX_BLUE);
  //delay(500);

  yield(); Serial.print(F("Rectangles (outline)     "));
  yield(); Serial.println(testRects(GFX_GREEN));
  //total+=testRects(GFX_GREEN);
  //delay(500);

  yield(); Serial.print(F("Rectangles (filled)      "));
  yield(); Serial.println(testFilledRects(GFX_YELLOW, GFX_MAGENTA));
  //total+=testFilledRects(GFX_YELLOW, GFX_MAGENTA);
  //delay(500);

  yield(); Serial.print(F("Circles (filled)         "));
  yield(); Serial.println(testFilledCircles(10, GFX_MAGENTA));
  //total+= testFilledCircles(10, GFX_MAGENTA);

  yield(); Serial.print(F("Circles (outline)        "));
  yield(); Serial.println(testCircles(10, GFX_WHITE));
  //total+=testCircles(10, GFX_WHITE);
  //delay(500);

  yield(); Serial.print(F("Triangles (outline)      "));
  yield(); Serial.println(testTriangles());
  //total+=testTriangles();
  //delay(500);

  yield(); Serial.print(F("Triangles (filled)       "));
  yield(); Serial.println(testFilledTriangles());
  //total += testFilledTriangles();
  //delay(500);

  yield(); Serial.print(F("Rounded rects (outline)  "));
  yield(); Serial.println(testRoundRects());
  //total+=testRoundRects();
  //delay(500);

  yield(); Serial.print(F("Rounded rects (filled)   "));
  yield(); Serial.println(testFilledRoundRects());
  //total+=testFilledRoundRects();
  //delay(500);

  yield(); Serial.println(F("Done!")); yield();
  //Serial.print(F("Total = ")); Serial.println(total);
  
  //yield();Serial.println(millis()-tn);
}

void loop(void) {
  for (uint8_t rotation = 0; rotation < 4; rotation++) {
    gfx.setRotation(rotation);
    testText();
    delay(2000);
  }
}


unsigned long testFillScreen() {
  unsigned long start = micros();
  gfx.fillScreen(GFX_BLACK);
  gfx.fillScreen(GFX_RED);
  gfx.fillScreen(GFX_GREEN);
  gfx.fillScreen(GFX_BLUE);
  gfx.fillScreen(GFX_BLACK);
  return micros() - start;
}

unsigned long testText() {
  gfx.fillScreen(GFX_BLACK);
  unsigned long start = micros();
  gfx.setCursor(0, 0);
  gfx.setTextColor(GFX_WHITE);  gfx.setTextSize(1);
  gfx.println("Hello World!");
  gfx.setTextColor(GFX_YELLOW); gfx.setTextSize(2);
  gfx.println(1234.56);
  gfx.setTextColor(GFX_RED);    gfx.setTextSize(3);
  gfx.println(0xDEADBEEF, HEX);
  gfx.println();
  gfx.setTextColor(GFX_GREEN);
  gfx.setTextSize(5);
  gfx.println("Groop");
  gfx.setTextSize(2);
  gfx.println("I implore thee,");
  //gfx.setTextColor(GFX_GREEN,GFX_BLACK);
  gfx.setTextSize(1);
  gfx.println("my foonting turlingdromes.");
  gfx.println("And hooptiously drangle me");
  gfx.println("with crinkly bindlewurdles,");
  gfx.println("Or I will rend thee");
  gfx.println("in the gobberwarts");
  gfx.println("with my blurglecruncheon,");
  gfx.println("see if I don't!");
  return micros() - start;
}

unsigned long testLines(uint16_t color) {
  unsigned long start, t;
  int           x1, y1, x2, y2,
                w = gfx.width(),
                h = gfx.height();

  gfx.fillScreen(GFX_BLACK);

  x1 = y1 = 0;
  y2    = h - 1;
  start = micros();
  for (x2 = 0; x2 < w; x2 += 6) gfx.drawLine(x1, y1, x2, y2, color);
  x2    = w - 1;
  for (y2 = 0; y2 < h; y2 += 6) gfx.drawLine(x1, y1, x2, y2, color);
  t     = micros() - start; // fillScreen doesn't count against timing
  yield();
  gfx.fillScreen(GFX_BLACK);

  x1    = w - 1;
  y1    = 0;
  y2    = h - 1;
  start = micros();
  for (x2 = 0; x2 < w; x2 += 6) gfx.drawLine(x1, y1, x2, y2, color);
  x2    = 0;
  for (y2 = 0; y2 < h; y2 += 6) gfx.drawLine(x1, y1, x2, y2, color);
  t    += micros() - start;
  yield();
  gfx.fillScreen(GFX_BLACK);

  x1    = 0;
  y1    = h - 1;
  y2    = 0;
  start = micros();
  for (x2 = 0; x2 < w; x2 += 6) gfx.drawLine(x1, y1, x2, y2, color);
  x2    = w - 1;
  for (y2 = 0; y2 < h; y2 += 6) gfx.drawLine(x1, y1, x2, y2, color);
  t    += micros() - start;
  yield();
  gfx.fillScreen(GFX_BLACK);

  x1    = w - 1;
  y1    = h - 1;
  y2    = 0;
  start = micros();
  for (x2 = 0; x2 < w; x2 += 6) gfx.drawLine(x1, y1, x2, y2, color);
  x2    = 0;
  for (y2 = 0; y2 < h; y2 += 6) gfx.drawLine(x1, y1, x2, y2, color);
  yield();
  return micros() - start;
}

unsigned long testFastLines(uint16_t color1, uint16_t color2) {
  unsigned long start;
  int           x, y, w = gfx.width(), h = gfx.height();

  gfx.fillScreen(GFX_BLACK);
  start = micros();
  for (y = 0; y < h; y += 5) gfx.drawFastHLine(0, y, w, color1);
  for (x = 0; x < w; x += 5) gfx.drawFastVLine(x, 0, h, color2);

  return micros() - start;
}

unsigned long testRects(uint16_t color) {
  unsigned long start;
  int           n, i, i2,
                cx = gfx.width()  / 2,
                cy = gfx.height() / 2;

  gfx.fillScreen(GFX_BLACK);
  n     = min(gfx.width(), gfx.height());
  start = micros();
  for (i = 2; i < n; i += 6) {
    i2 = i / 2;
    gfx.drawRect(cx - i2, cy - i2, i, i, color);
  }

  return micros() - start;
}

unsigned long testFilledRects(uint16_t color1, uint16_t color2) {
  unsigned long start, t = 0;
  int           n, i, i2,
                cx = gfx.width()  / 2 - 1,
                cy = gfx.height() / 2 - 1;

  gfx.fillScreen(GFX_BLACK);
  n = min(gfx.width(), gfx.height());
  for (i = n - 1; i > 0; i -= 6) {
    i2    = i / 2;
    start = micros();
    gfx.fillRect(cx - i2, cy - i2, i, i, color1);
    t    += micros() - start;
    // Outlines are not included in timing results
    gfx.drawRect(cx - i2, cy - i2, i, i, color2);
  }

  return t;
}

unsigned long testFilledCircles(uint8_t radius, uint16_t color) {
  unsigned long start;
  int x, y, w = gfx.width(), h = gfx.height(), r2 = radius * 2;

  gfx.fillScreen(GFX_BLACK);
  start = micros();
  for (x = radius; x < w; x += r2) {
    for (y = radius; y < h; y += r2) {
      gfx.fillCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

unsigned long testCircles(uint8_t radius, uint16_t color) {
  unsigned long start;
  int           x, y, r2 = radius * 2,
                      w = gfx.width()  + radius,
                      h = gfx.height() + radius;

  // Screen is not cleared for this one -- this is
  // intentional and does not affect the reported time.
  start = micros();
  for (x = 0; x < w; x += r2) {
    for (y = 0; y < h; y += r2) {
      gfx.drawCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

unsigned long testTriangles() {
  unsigned long start;
  int           n, i, cx = gfx.width()  / 2 - 1,
                      cy = gfx.height() / 2 - 1;

  gfx.fillScreen(GFX_BLACK);
  n     = min(cx, cy);
  start = micros();
  for (i = 0; i < n; i += 5) {
    gfx.drawTriangle(
      cx    , cy - i, // peak
      cx - i, cy + i, // bottom left
      cx + i, cy + i, // bottom right
      gfx.color565(0, 0, i));
  }

  return micros() - start;
}

unsigned long testFilledTriangles() {
  unsigned long start, t = 0;
  int           i, cx = gfx.width()  / 2 - 1,
                   cy = gfx.height() / 2 - 1;

  gfx.fillScreen(GFX_BLACK);
  start = micros();
  for (i = min(cx, cy); i > 10; i -= 5) {
    start = micros();
    gfx.fillTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
                     gfx.color565(0, i, i));
    t += micros() - start;
    gfx.drawTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
                     gfx.color565(i, i, 0));
  }

  return t;
}

unsigned long testRoundRects() {
  unsigned long start;
  int           w, i, i2,
                cx = gfx.width()  / 2 - 1,
                cy = gfx.height() / 2 - 1;

  gfx.fillScreen(GFX_BLACK);
  w     = min(gfx.width(), gfx.height());
  start = micros();
  for (i = 0; i < w; i += 6) {
    i2 = i / 2;
    gfx.drawRoundRect(cx - i2, cy - i2, i, i, i / 8, gfx.color565(i, 0, 0));
  }

  return micros() - start;
}

unsigned long testFilledRoundRects() {
  unsigned long start;
  int           i, i2,
                cx = gfx.width()  / 2 - 1,
                cy = gfx.height() / 2 - 1;

  gfx.fillScreen(GFX_BLACK);
  start = micros();
  for (i = min(gfx.width(), gfx.height()); i > 20; i -= 6) {
    i2 = i / 2;
    gfx.fillRoundRect(cx - i2, cy - i2, i, i, i / 8, gfx.color565(0, i, 0));
    yield();
  }

  return micros() - start;
}

/***************************************************
  Original Adafruit text:

  This is an example sketch for the Adafruit 2.2" SPI display.
  This library works with the Adafruit 2.2" TFT Breakout w/SD card
  ----> http://www.adafruit.com/products/1480

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

