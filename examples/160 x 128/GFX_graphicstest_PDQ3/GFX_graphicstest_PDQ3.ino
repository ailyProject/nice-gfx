/*
 Adapted from the Adafruit and Xark's PDQ graphicstest sketch.

 This sketch uses the TomThumb4x6 bitmap font only.

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

// These are used to get information about static SRAM and flash memory sizes
extern "C" char __data_start[];		// start of SRAM data
extern "C" char _end[];			// end of SRAM data (used to check amount of SRAM this program's variables use)
extern "C" char __data_load_end[];	// end of FLASH (used to check amount of Flash this program's code and data uses)


void setup()
{
  Serial.begin(115200);

  Serial.println(F("TFT 1.8\" SPI TFT Test!     ")); 
 
  gfx.begin();
  gfx.setFont(nice_gfx::fonts::TomThumb4x6);            // initialize LCD
}

// NOTE: This demo should run, but may look odd on 128x128 LCD (vs 128x160)

void loop(void)
{
  
  Serial.print(F(__DATE__ " " __TIME__ " - Flash=0x"));
  // These are not compatible with the ESP8266 core library
  //Serial.print((uint16_t)__data_load_end, HEX);
  //Serial.print(F(" RAM=0x"));
  //Serial.println((uint16_t)_end - (uint16_t)__data_start, HEX);
  Serial.println(F("Benchmark                Time (microseconds)"));

  uint32_t usecHaD = testHaD();
  Serial.print(F("HaD pushColor             "));
  Serial.println(usecHaD);
  delay(100);


  uint32_t usecFillScreen = testFillScreen();
  Serial.print(F("Screen fill              "));
  Serial.println(usecFillScreen);
  delay(100);

  gfx.setRotation(1);
  uint32_t usecText = testText();
  Serial.print(F("Text                     "));
  Serial.println(usecText);
  delay(100);
  gfx.setRotation(0);
  
  uint32_t usecPixels = testPixels();
  Serial.print(F("Pixels                   "));
  Serial.println(usecPixels);
  delay(100);

  uint32_t usecLines = testLines(GFX_BLUE);
  Serial.print(F("Lines                    "));
  Serial.println(usecLines);
  delay(100);

  uint32_t usecFastLines = testFastLines(GFX_RED, GFX_BLUE);
  Serial.print(F("Horiz/Vert Lines         "));
  Serial.println(usecFastLines);
  delay(100);

  uint32_t usecRects = testRects(GFX_GREEN);
  Serial.print(F("Rectangles (outline)     "));
  Serial.println(usecRects);
  delay(100);

  uint32_t usecFilledRects = testFilledRects(GFX_YELLOW, GFX_MAGENTA);
  Serial.print(F("Rectangles (filled)      "));
  Serial.println(usecFilledRects);
  delay(100);

  uint32_t usecFilledCircles = testFilledCircles(10, GFX_MAGENTA);
  Serial.print(F("Circles (filled)         "));
  Serial.println(usecFilledCircles);
  delay(100);

  uint32_t usecCircles = testCircles(10, GFX_WHITE);
  Serial.print(F("Circles (outline)        "));
  Serial.println(usecCircles);
  delay(100);

  uint32_t usecTriangles = testTriangles();
  Serial.print(F("Triangles (outline)      "));
  Serial.println(usecTriangles);
  delay(100);

  uint32_t usecFilledTrangles = testFilledTriangles();
  Serial.print(F("Triangles (filled)       "));
  Serial.println(usecFilledTrangles);
  delay(100);

  uint32_t usecRoundRects = testRoundRects();
  Serial.print(F("Rounded rects (outline)  "));
  Serial.println(usecRoundRects);
  delay(100);

  uint32_t usedFilledRoundRects = testFilledRoundRects();
  Serial.print(F("Rounded rects (filled)   "));
  Serial.println(usedFilledRoundRects);
  delay(100);

  Serial.println(F("Done!"));
  
  uint16_t c = 4;
  int8_t d = 1;
  for (int32_t i = 0; i < gfx.height(); i++)
  {
    gfx.drawFastHLine(0, i, gfx.width(), c);
    c += d;
    if (c <= 4 || c >= 11)
      d = -d;
  }
  
  gfx.setCursor(0, 0);
  gfx.setTextColor(GFX_MAGENTA);

  gfx.println(F("NICE_GFX"));

  gfx.setTextSize(1);
  gfx.setTextColor(GFX_WHITE);
  gfx.println(F("SPI TFT on ESP8266"));
  gfx.println(F(""));
  gfx.setTextSize(1);
  gfx.setTextColor(gfx.color565(0x80, 0x80, 0x80));

  // These are not compatible with the ESP8266 core library
  //gfx.print(F("Flash=0x"));
  //gfx.print((uint16_t)__data_load_end, HEX);
  //gfx.print(F(" RAM=0x"));
  //gfx.print((uint16_t)_end - (uint16_t)__data_start, HEX);

  gfx.setTextColor(GFX_GREEN);
  gfx.print(F("Benchmark        usec"));

  gfx.setTextColor(GFX_CYAN);
  gfx.print(F("HaD logo   "));
  gfx.setTextColor(GFX_YELLOW);
  printnice(usecHaD);

  gfx.setTextColor(GFX_CYAN);
  gfx.print(F("Clear      "));
  gfx.setTextColor(GFX_YELLOW);
  printnice(usecFillScreen);

  gfx.setTextColor(GFX_CYAN);
  gfx.print(F("Text       "));
  gfx.setTextColor(GFX_YELLOW);
  printnice(usecText);

  gfx.setTextColor(GFX_CYAN);
  gfx.print(F("Pixels     "));
  gfx.setTextColor(GFX_YELLOW);
  printnice(usecPixels);

  gfx.setTextColor(GFX_CYAN);
  gfx.print(F("Lines      "));
  gfx.setTextColor(GFX_YELLOW);
  printnice(usecLines);

  gfx.setTextColor(GFX_CYAN);
  gfx.print(F("H/V Lines  "));
  gfx.setTextColor(GFX_YELLOW);
  printnice(usecFastLines);

  gfx.setTextColor(GFX_CYAN);
  gfx.print(F("Rectangles "));
  gfx.setTextColor(GFX_YELLOW);
  printnice(usecRects);

  gfx.setTextColor(GFX_CYAN);
  gfx.print(F("Rects-Fill "));
  gfx.setTextColor(GFX_YELLOW);
  printnice(usecFilledRects);

  gfx.setTextColor(GFX_CYAN);
  gfx.print(F("Circles    "));
  gfx.setTextColor(GFX_YELLOW);
  printnice(usecCircles);

  gfx.setTextColor(GFX_CYAN);
  gfx.print(F("CircleFill "));
  gfx.setTextColor(GFX_YELLOW);
  printnice(usecFilledCircles);

  gfx.setTextColor(GFX_CYAN);
  gfx.print(F("Triangles  "));
  gfx.setTextColor(GFX_YELLOW);
  printnice(usecTriangles);

  gfx.setTextColor(GFX_CYAN);
  gfx.print(F("Tris-Fill  "));
  gfx.setTextColor(GFX_YELLOW);
  printnice(usecFilledTrangles);

  gfx.setTextColor(GFX_CYAN);
  gfx.print(F("Rnd-Rects  "));
  gfx.setTextColor(GFX_YELLOW);
  printnice(usecRoundRects);

  gfx.setTextColor(GFX_CYAN);
  gfx.print(F("RRects-Fill"));
  gfx.setTextColor(GFX_YELLOW);
  printnice(usedFilledRoundRects);

  gfx.setTextSize(1);
  gfx.println(F(""));
  gfx.setTextColor(GFX_GREEN);
  gfx.print(F("Benchmark Completed!"));

  delay(15 * 1000L);
}

void printnice(int32_t v)
{
  char  str[32] = { 0 };
  sprintf(str, "%d", v);
  for (char *p = (str+strlen(str))-3; p > str; p -= 3)
  {
    memmove(p+1, p, strlen(p)+1);
    *p = ',';
    
  }
  while (strlen(str) < 10)
  {
    memmove(str+1, str, strlen(str)+1);
    *str = ' ';
  }
  gfx.print(str);
}

static inline uint32_t micros_start() __attribute__ ((always_inline));
static inline uint32_t micros_start()
{
  uint8_t oms = millis();
  while ((uint8_t)millis() == oms)
    ;
  return micros();
}

uint32_t testHaD()
{
  // pseudo-code for cheesy RLE
  // start with color1
  // while more input data remaining
  //  count =  0nnnnnnn = 1 byte or 1nnnnnnn nnnnnnnn 2 bytes (0 - 32767)
  //  repeat color count times
  //  toggle color1/color2
  static const uint8_t HaD_128x160[] PROGMEM =
  {
    0x85, 0x91, 0x09, 0x4b, 0x09, 0x24, 0x0a, 0x47, 0x09, 0x27, 0x0a, 0x44, 0x0a, 0x29, 0x0a, 0x42, 
    0x0a, 0x2b, 0x0a, 0x41, 0x0a, 0x2c, 0x0a, 0x3e, 0x0b, 0x2f, 0x09, 0x3d, 0x09, 0x32, 0x08, 0x3c, 
    0x09, 0x33, 0x09, 0x3b, 0x08, 0x33, 0x0a, 0x3a, 0x0a, 0x31, 0x0b, 0x3a, 0x0c, 0x1d, 0x01, 0x10, 
    0x0d, 0x39, 0x0c, 0x1d, 0x01, 0x10, 0x0d, 0x39, 0x0d, 0x0f, 0x01, 0x0c, 0x03, 0x0d, 0x0e, 0x39, 
    0x0e, 0x0c, 0x03, 0x0c, 0x04, 0x0b, 0x0f, 0x39, 0x0f, 0x0a, 0x04, 0x0c, 0x05, 0x09, 0x10, 0x39, 
    0x10, 0x08, 0x05, 0x0c, 0x06, 0x07, 0x11, 0x39, 0x11, 0x06, 0x06, 0x0d, 0x07, 0x04, 0x13, 0x37, 
    0x12, 0x05, 0x07, 0x0d, 0x08, 0x02, 0x15, 0x34, 0x15, 0x03, 0x08, 0x0d, 0x20, 0x32, 0x20, 0x0e, 
    0x21, 0x31, 0x20, 0x0f, 0x21, 0x2e, 0x22, 0x10, 0x22, 0x2b, 0x22, 0x12, 0x22, 0x12, 0x05, 0x12, 
    0x22, 0x14, 0x22, 0x0c, 0x0f, 0x0c, 0x22, 0x16, 0x22, 0x08, 0x15, 0x08, 0x22, 0x18, 0x22, 0x05, 
    0x19, 0x05, 0x21, 0x1c, 0x1f, 0x04, 0x1c, 0x05, 0x1f, 0x1f, 0x1c, 0x04, 0x1e, 0x04, 0x1d, 0x2b, 
    0x11, 0x04, 0x21, 0x03, 0x12, 0x36, 0x0f, 0x03, 0x24, 0x03, 0x10, 0x38, 0x0d, 0x03, 0x26, 0x03, 
    0x0d, 0x3b, 0x0b, 0x03, 0x28, 0x03, 0x0b, 0x3d, 0x0a, 0x03, 0x29, 0x03, 0x09, 0x40, 0x07, 0x03, 
    0x2b, 0x03, 0x07, 0x42, 0x05, 0x03, 0x2c, 0x04, 0x04, 0x45, 0x04, 0x03, 0x2d, 0x03, 0x04, 0x46, 
    0x02, 0x03, 0x2e, 0x03, 0x03, 0x48, 0x01, 0x03, 0x2f, 0x03, 0x01, 0x4c, 0x31, 0x4e, 0x32, 0x4e, 
    0x33, 0x4c, 0x34, 0x4c, 0x34, 0x4c, 0x35, 0x4b, 0x35, 0x4a, 0x0e, 0x03, 0x14, 0x04, 0x0d, 0x4a, 
    0x0b, 0x09, 0x0e, 0x0a, 0x0a, 0x4a, 0x0a, 0x0b, 0x0c, 0x0c, 0x09, 0x4a, 0x09, 0x0d, 0x0a, 0x0e, 
    0x09, 0x49, 0x08, 0x0f, 0x09, 0x0e, 0x09, 0x49, 0x08, 0x0f, 0x09, 0x0f, 0x08, 0x49, 0x08, 0x0f, 
    0x09, 0x0f, 0x08, 0x49, 0x07, 0x0f, 0x0a, 0x0f, 0x08, 0x49, 0x07, 0x0f, 0x0b, 0x0e, 0x08, 0x49, 
    0x07, 0x0d, 0x0e, 0x0d, 0x08, 0x49, 0x07, 0x0b, 0x13, 0x0a, 0x08, 0x49, 0x08, 0x07, 0x18, 0x08, 
    0x08, 0x49, 0x08, 0x06, 0x1b, 0x06, 0x08, 0x49, 0x09, 0x04, 0x1c, 0x05, 0x08, 0x4a, 0x09, 0x04, 
    0x1d, 0x04, 0x08, 0x4a, 0x0a, 0x03, 0x1d, 0x03, 0x09, 0x4b, 0x19, 0x02, 0x1a, 0x4b, 0x19, 0x03, 
    0x19, 0x4b, 0x18, 0x04, 0x18, 0x4d, 0x17, 0x05, 0x17, 0x4a, 0x01, 0x02, 0x17, 0x05, 0x16, 0x4a, 
    0x02, 0x02, 0x17, 0x05, 0x16, 0x02, 0x03, 0x44, 0x03, 0x03, 0x16, 0x02, 0x01, 0x02, 0x16, 0x02, 
    0x03, 0x43, 0x05, 0x03, 0x15, 0x01, 0x03, 0x01, 0x15, 0x03, 0x04, 0x41, 0x06, 0x03, 0x15, 0x01, 
    0x03, 0x01, 0x14, 0x03, 0x07, 0x3d, 0x09, 0x03, 0x2d, 0x03, 0x08, 0x3b, 0x0a, 0x04, 0x2b, 0x03, 
    0x0a, 0x39, 0x0c, 0x03, 0x2a, 0x04, 0x0b, 0x37, 0x0e, 0x03, 0x28, 0x03, 0x0e, 0x2e, 0x04, 0x03, 
    0x10, 0x03, 0x27, 0x03, 0x10, 0x03, 0x03, 0x24, 0x19, 0x03, 0x26, 0x03, 0x1a, 0x1e, 0x1d, 0x03, 
    0x24, 0x03, 0x1e, 0x19, 0x20, 0x04, 0x21, 0x03, 0x20, 0x17, 0x22, 0x04, 0x1f, 0x03, 0x22, 0x15, 
    0x22, 0x04, 0x21, 0x04, 0x21, 0x13, 0x22, 0x05, 0x15, 0x01, 0x0b, 0x05, 0x21, 0x12, 0x21, 0x06, 
    0x15, 0x01, 0x0b, 0x06, 0x21, 0x10, 0x21, 0x07, 0x0a, 0x01, 0x0a, 0x01, 0x0b, 0x07, 0x21, 0x0e, 
    0x20, 0x0a, 0x09, 0x02, 0x09, 0x02, 0x09, 0x09, 0x20, 0x0e, 0x08, 0x02, 0x15, 0x0b, 0x08, 0x03, 
    0x08, 0x03, 0x08, 0x0b, 0x15, 0x03, 0x08, 0x0d, 0x07, 0x04, 0x13, 0x0d, 0x06, 0x05, 0x06, 0x06, 
    0x05, 0x0d, 0x14, 0x04, 0x07, 0x0c, 0x07, 0x06, 0x11, 0x38, 0x12, 0x06, 0x06, 0x0c, 0x06, 0x08, 
    0x10, 0x39, 0x10, 0x08, 0x05, 0x0c, 0x04, 0x0b, 0x0f, 0x39, 0x0f, 0x0a, 0x04, 0x0c, 0x03, 0x0d, 
    0x0e, 0x39, 0x0e, 0x0c, 0x03, 0x0c, 0x02, 0x0e, 0x0e, 0x39, 0x0d, 0x0f, 0x01, 0x0c, 0x01, 0x10, 
    0x0d, 0x39, 0x0d, 0x0f, 0x01, 0x1e, 0x0c, 0x39, 0x0c, 0x30, 0x0a, 0x3a, 0x0a, 0x33, 0x09, 0x3b, 
    0x08, 0x34, 0x09, 0x3b, 0x09, 0x32, 0x09, 0x3c, 0x0a, 0x2f, 0x0a, 0x3e, 0x0a, 0x2d, 0x0b, 0x3f, 
    0x0a, 0x2b, 0x0b, 0x41, 0x0a, 0x29, 0x0b, 0x43, 0x0a, 0x27, 0x0a, 0x46, 0x0a, 0x25, 0x0a, 0x49, 
    0x09, 0x23, 0x08, 0x4e, 0x08, 0x96, 0x12 
  };

  gfx.fillScreen(GFX_BLACK);

  uint32_t start = micros_start();

  gfx.startWrite();

  for (int i = 0; i < 0x10; i++)
  {
    gfx.setAddrWindow(0, 0, gfx.width(), gfx.height());

    uint16_t cnt = 0;
    uint16_t color = gfx.color565((i << 4) | i, (i << 4) | i, (i << 4) | i);
    uint16_t curcolor = 0;

    const uint8_t *cmp = &HaD_128x160[0];
    gfx.startWrite();
    while (cmp < &HaD_128x160[sizeof(HaD_128x160)])
    {
      cnt = pgm_read_byte(cmp++);
      if (cnt & 0x80)
        cnt = ((cnt & 0x7f) << 8) | pgm_read_byte(cmp++);

      gfx.pushBlock(curcolor, cnt); // PDQ_GFX has count

      curcolor ^= color;
    }
    gfx.endWrite();
  }

  gfx.endWrite();

  uint32_t t = micros() - start;

  gfx.setTextColor(GFX_YELLOW);
  gfx.setCursor(0, 145);
  gfx.print(F(" http://hackaday.io/"));
  gfx.print(F("         Xark"));

  delay(3 * 1000L);
  
  return t;
}

uint32_t testFillScreen()
{
  uint32_t start = micros_start();

  for (uint8_t i = 0; i < 12; i++)
  {
    gfx.fillScreen(GFX_WHITE);
    gfx.fillScreen(GFX_RED);
    gfx.fillScreen(GFX_GREEN);
    gfx.fillScreen(GFX_BLUE);
    gfx.fillScreen(GFX_BLACK);
  }

  return micros() - start;
}

uint32_t testText() {
  gfx.fillScreen(GFX_BLACK);
  uint32_t start = micros_start();
  gfx.setCursor(0, 0);
  gfx.setTextColor(GFX_WHITE); gfx.setTextSize(1);
  gfx.println(F("Hello World!"));
  gfx.setTextColor(gfx.color565(0xff, 0x00, 0x00));
  gfx.print(F("RED "));
  gfx.setTextColor(gfx.color565(0x00, 0xff, 0x00));
  gfx.print(F("GREEN "));
  gfx.setTextColor(gfx.color565(0x00, 0x00, 0xff));
  gfx.println(F("BLUE"));
  gfx.setTextColor(GFX_YELLOW);
  gfx.println(1234.56);
  gfx.setTextColor(GFX_RED);
  gfx.println(0xDEADBEEF, HEX);
  gfx.setTextColor(GFX_GREEN);
  gfx.setTextSize(2);
  gfx.println(F("Groop"));
  gfx.println(F("I implore thee,"));
  gfx.setTextSize(1);
  gfx.println(F("my foonting turlingdromes."));
  gfx.println(F("And hooptiously drangle me"));
  gfx.println(F("with crinkly bindlewurdles,"));
  gfx.println(F("Or I will rend thee"));
  gfx.println(F("in the gobberwarts"));
  gfx.println(F("with my blurglecruncheon,"));
  gfx.println(F("see if I don't!"));
  gfx.println(F(""));
  gfx.setTextColor(GFX_MAGENTA);
  gfx.println(F("Woot!"));
  uint32_t t = micros() - start;
  delay(1000);
  return t;
}

uint32_t testPixels()
{
  int32_t w = gfx.width();
  int32_t h = gfx.height();

  uint32_t start = micros_start();
  gfx.startWrite();
  for (uint16_t y = 0; y < h; y++)
  {
    for (uint16_t x = 0; x < w; x++)
    {
      gfx.drawPixel(x, y, gfx.color565(x<<3, y<<3, x*y));
    }
  }
  gfx.endWrite();
  return micros() - start;
}


uint32_t testLines(uint16_t color)
{
  uint32_t start, t;
  int32_t x1, y1, x2, y2;
  int32_t w = gfx.width();
  int32_t h = gfx.height();

  gfx.fillScreen(GFX_BLACK);

  x1 = y1 = 0;
  y2 = h - 1;

  start = micros_start();

  for (x2 = 0; x2 < w; x2 += 6)
  {
    gfx.drawLine(x1, y1, x2, y2, color);
  }

  x2 = w - 1;

  for (y2 = 0; y2 < h; y2 += 6)
  {
    gfx.drawLine(x1, y1, x2, y2, color);
  }

  t = micros() - start; // fillScreen doesn't count against timing

  gfx.fillScreen(GFX_BLACK);

  x1 = w - 1;
  y1 = 0;
  y2 = h - 1;

  start = micros_start();

  for (x2 = 0; x2 < w; x2 += 6)
  {
    gfx.drawLine(x1, y1, x2, y2, color);
  }

  x2 = 0;
  for (y2 = 0; y2 < h; y2 += 6)
  {
    gfx.drawLine(x1, y1, x2, y2, color);
  }

  t += micros() - start;

  gfx.fillScreen(GFX_BLACK);

  x1 = 0;
  y1 = h - 1;
  y2 = 0;

  start = micros_start();

  for (x2 = 0; x2 < w; x2 += 6)
  {
    gfx.drawLine(x1, y1, x2, y2, color);
  }
  x2 = w - 1;
  for (y2 = 0; y2 < h; y2 += 6)
  {
    gfx.drawLine(x1, y1, x2, y2, color);
  }
  t += micros() - start;

  gfx.fillScreen(GFX_BLACK);

  x1 = w - 1;
  y1 = h - 1;
  y2 = 0;

  start = micros_start();

  for (x2 = 0; x2 < w; x2 += 6)
  {
    gfx.drawLine(x1, y1, x2, y2, color);
  }

  x2 = 0;
  for (y2 = 0; y2 < h; y2 += 6)
  {
    gfx.drawLine(x1, y1, x2, y2, color);
  }

  t += micros() - start;

  return t;
}

uint32_t testFastLines(uint16_t color1, uint16_t color2)
{
  uint32_t start;
  int32_t x, y;
  int32_t w = gfx.width();
  int32_t h = gfx.height();

  gfx.fillScreen(GFX_BLACK);

  start = micros_start();

  for (y = 0; y < h; y += 5)
    gfx.drawFastHLine(0, y, w, color1);
  for (x = 0; x < w; x += 5)
    gfx.drawFastVLine(x, 0, h, color2);

  return micros() - start;
}

uint32_t testRects(uint16_t color)
{
  uint32_t start;
  int32_t n, i, i2;
  int32_t cx = gfx.width() / 2;
  int32_t cy = gfx.height() / 2;

  gfx.fillScreen(GFX_BLACK);
  n = min(gfx.width(), gfx.height());
  start = micros_start();
  for (i = 2; i < n; i += 6)
  {
    i2 = i / 2;
    gfx.drawRect(cx-i2, cy-i2, i, i, color);
  }

  return micros() - start;
}

uint32_t testFilledRects(uint16_t color1, uint16_t color2)
{
  uint32_t start, t = 0;
  int32_t n, i, i2;
  int32_t cx = gfx.width() / 2 - 1;
  int32_t cy = gfx.height() / 2 - 1;

  gfx.fillScreen(GFX_BLACK);
  n = min(gfx.width(), gfx.height());
  for (i = n; i > 0; i -= 6)
  {
    i2 = i / 2;

    start = micros_start();

    gfx.fillRect(cx-i2, cy-i2, i, i, color1);

    t += micros() - start;

    // Outlines are not included in timing results
    gfx.drawRect(cx-i2, cy-i2, i, i, color2);
  }

  return t;
}

uint32_t testFilledCircles(uint8_t radius, uint16_t color)
{
  uint32_t start;
  int32_t x, y, w = gfx.width(), h = gfx.height(), r2 = radius * 2;

  gfx.fillScreen(GFX_BLACK);

  start = micros_start();

  for (x = radius; x < w; x += r2)
  {
    for (y = radius; y < h; y += r2)
    {
      gfx.fillCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

uint32_t testCircles(uint8_t radius, uint16_t color)
{
  uint32_t start;
  int32_t x, y, r2 = radius * 2;
  int32_t w = gfx.width() + radius;
  int32_t h = gfx.height() + radius;

  // Screen is not cleared for this one -- this is
  // intentional and does not affect the reported time.
  start = micros_start();

  for (x = 0; x < w; x += r2)
  {
    for (y = 0; y < h; y += r2)
    {
      gfx.drawCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

uint32_t testTriangles()
{
  uint32_t start;
  int32_t n, i;
  int32_t cx = gfx.width()/ 2 - 1;
  int32_t cy = gfx.height() / 2 - 1;

  gfx.fillScreen(GFX_BLACK);
  n = min(cx, cy);

  start = micros_start();

  for (i = 0; i < n; i += 5)
  {
    gfx.drawTriangle(
      cx    , cy - i, // peak
      cx - i, cy + i, // bottom left
      cx + i, cy + i, // bottom right
      gfx.color565(0, 0, i));
  }

  return micros() - start;
}

uint32_t testFilledTriangles()
{
  uint32_t start, t = 0;
  int32_t i;
  int32_t cx = gfx.width() / 2 - 1;
  int32_t cy = gfx.height() / 2 - 1;

  gfx.fillScreen(GFX_BLACK);

  start = micros_start();

  for (i = min(cx,cy); i > 10; i -= 5) {
    start = micros_start();
    gfx.fillTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
      gfx.color565(0, i, i));
    t += micros() - start;
    gfx.drawTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
      gfx.color565(i, i, 0));
  }

  return t;
}

uint32_t testRoundRects()
 {
  uint32_t start;
  int32_t w, i, i2;
  int32_t cx = gfx.width() / 2 - 1;
  int32_t cy = gfx.height() / 2 - 1;

  gfx.fillScreen(GFX_BLACK);
  
  w = min(gfx.width(), gfx.height());
  
  start = micros_start();

  for (i = 0; i < w; i += 6)
  {
    i2 = i / 2;
    gfx.drawRoundRect(cx-i2, cy-i2, i, i, i/8, gfx.color565(i, 0, 0));
  }

  return micros() - start;
}

uint32_t testFilledRoundRects()
{
  uint32_t start;
  int32_t i, i2;
  int32_t cx = gfx.width() / 2 - 1;
  int32_t cy = gfx.height() / 2 - 1;

  gfx.fillScreen(GFX_BLACK);

  start = micros_start();

  for (i = min(gfx.width(), gfx.height()); i > 20; i -= 6)
  {
    i2 = i / 2;
    gfx.fillRoundRect(cx-i2, cy-i2, i, i, i/8, gfx.color565(0, i, 0));
  }

  return micros() - start;
}
