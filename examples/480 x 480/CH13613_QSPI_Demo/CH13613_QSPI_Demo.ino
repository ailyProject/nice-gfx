/*
  CH13613 QSPI example for ESP32-S3 and the OSPTEK 1.6-inch 480x480 AMOLED.

  Connections:
    CS  = GPIO45    SCLK = GPIO17
    D0  = GPIO8     D1   = GPIO16
    D2  = GPIO15    D3   = GPIO18
    RST = GPIO7     TE   = GPIO38

  Pins are supplied through the display constructor below.
*/

#include <Arduino.h>

#if !defined(CONFIG_IDF_TARGET_ESP32S3)
  #error "The CH13613 QSPI example requires an ESP32-S3 board"
#endif

#include <NICE_GFX.h>

// One display object owns the panel and DMA bus. CS, RST, CLK, D0, D1, D2, D3.
CH13613_480x480_QSPI gfx(45, 7, 17, 8, 16, 15, 18);
constexpr int tearingPin = 38;

static uint32_t frameCount = 0;
static uint32_t lastUpdate = 0;

static void drawStaticScreen()
{
  gfx.fillScreen(GFX_BLACK);

  gfx.setTextDatum(TC_DATUM);
  gfx.setTextColor(GFX_GREEN, GFX_BLACK);
  gfx.setTextSize(3);
  gfx.drawString("CH13613 QSPI", 240, 24);

  gfx.setTextColor(GFX_LIGHTGREY, GFX_BLACK);
  gfx.setTextSize(2);
  gfx.drawString("ESP32-S3  |  480 x 480  |  40 MHz", 240, 62);

  gfx.drawRoundRect(18, 96, 444, 300, 12, GFX_WHITE);
  gfx.fillRoundRect(38, 116, 190, 70, 10, GFX_RED);
  gfx.fillRoundRect(252, 116, 190, 70, 10, GFX_GREEN);

  gfx.fillCircle(133, 246, 44, GFX_BLUE);
  gfx.drawCircle(347, 246, 44, GFX_YELLOW);
  gfx.drawCircle(347, 246, 43, GFX_YELLOW);

  gfx.fillRectHGradient(40, 318, 400, 34, GFX_MAGENTA, GFX_CYAN);
  gfx.fillTriangle(72, 382, 128, 326, 184, 382, GFX_ORANGE);
  gfx.drawEllipse(350, 366, 72, 24, GFX_CYAN);

  gfx.setTextDatum(BC_DATUM);
  gfx.setTextColor(GFX_YELLOW, GFX_BLACK);
  gfx.setTextSize(2);
  gfx.drawString("QSPI frame counter", 240, 432);
}

void setup()
{
  Serial.begin(115200);

  pinMode(tearingPin, INPUT);

  if (!gfx.begin()) {
    Serial.println("Display initialization failed");
    return;
  }
  gfx.setFont(nice_gfx::fonts::GfxTerm8x16);
  gfx.setRotation(0);
  drawStaticScreen();
}

void loop()
{
  const uint32_t now = millis();
  if (now - lastUpdate < 1000) return;
  lastUpdate = now;

  ++frameCount;
  gfx.setTextDatum(BC_DATUM);
  gfx.setTextColor(GFX_YELLOW, GFX_BLACK);
  gfx.setTextSize(3);
  gfx.setTextPadding(180);
  gfx.drawNumber(frameCount, 240, 474);
}
