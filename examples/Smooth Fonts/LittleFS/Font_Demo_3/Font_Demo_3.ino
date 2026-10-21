// Font descriptors live in Flash and are selected explicitly with setFont().
// The existing example path is retained; these examples need no font files.
#include <NICE_GFX.h>
#include "Hardware.h"

ExampleDisplay gfx;
GFX_Sprite spr(&gfx);
void setup() {
  Serial.begin(115200);
  gfx.begin();
  gfx.setRotation(1);
  gfx.setFont(nice_gfx::fonts::GfxTerm8x16);
  gfx.setTextSize(1);
}

void loop() {
  gfx.fillScreen(GFX_DARKGREY);
  spr.setColorDepth(16);
  spr.setFont(nice_gfx::fonts::GfxTerm8x16);
  spr.createSprite(100, 50);
  spr.fillSprite(GFX_BLUE);
  spr.drawRect(0, 0, 100, 50, GFX_WHITE);
  spr.setTextColor(GFX_YELLOW);
  spr.setTextDatum(MC_DATUM);
  spr.drawString("Sprite", 50, 25);
  spr.pushSprite(10, 10);
  spr.pushSprite(10, 70, GFX_BLUE); // Blue pixels leave the display background intact.
  delay(1800);

  spr.setFont(nice_gfx::fonts::Spleen12x24);
  spr.setTextColor(GFX_WHITE, GFX_BLUE);
  for (int value = 0; value <= 200; ++value) {
    spr.fillSprite(GFX_BLUE);
    spr.drawFloat(value / 100.0, 2, 50, 25);
    spr.pushSprite(10, 10);
    delay(20);
  }
  spr.deleteSprite();

  // Reuse the same Sprite for small, fixed-width numbered tiles.
  spr.setFont(nice_gfx::fonts::ProFont6x12);
  spr.createSprite(36, 24);
  for (int row = 0; row < 4; ++row) for (int column = 0; column < 4; ++column) {
    spr.fillSprite(GFX_NAVY);
    spr.drawRoundRect(0, 0, 36, 24, 4, GFX_CYAN);
    spr.drawNumber(row * 4 + column + 1, 18, 12);
    spr.pushSprite(120 + column * 40, 20 + row * 28);
  }
  delay(1800);
  spr.deleteSprite();
}
