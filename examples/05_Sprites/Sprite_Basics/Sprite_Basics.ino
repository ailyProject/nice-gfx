// A small reusable RGB565 sprite; no full-screen framebuffer is needed.
#include <NICE_GFX.h>
#include <Graphics/Sprite.h>
#include "Hardware.h"
#include "ExampleDrawing.h"

ExampleDisplay gfx;
GFX_Sprite badge(&gfx);
bool ready = false;
int badgeWidth = 0, badgeY = 0, previousX = 0;
uint32_t frame = 0;

void setup() {
  Serial.begin(115200);
  if (!exampleBegin(gfx)) {
    Serial.println("Display initialization failed; check Hardware.h");
    return;
  }
  if (gfx.width() < 64 || gfx.height() < 32) {
    Serial.println("Sprite_Basics needs at least 64 x 32 logical pixels");
    return;
  }
  badgeWidth = (gfx.width() < 96 ? gfx.width() : 96) & ~1;
  badgeY = ((gfx.height() - 32) / 2) & ~1;
  badge.setColorDepth(16);
  if (!badge.createSprite(badgeWidth, 32)) {
    Serial.println("Sprite allocation failed");
    return;
  }
  badge.setFont(nice_gfx::fonts::Fixed6x13);
  badge.setTextDatum(MC_DATUM);
  badge.setTextColor(GFX_WHITE, GFX_NAVY);
  gfx.fillScreen(GFX_BLACK);
  ready = true;
}

void loop() {
  if (!ready) { delay(10); return; }
  static uint32_t lastUpdate = 0;
  const uint32_t now = millis();
  if (uint32_t(now - lastUpdate) < 100) { delay(1); return; }
  lastUpdate = now;
  // Even coordinates and sizes also satisfy CH13613's 2x2 window alignment.
  const int positions = (gfx.width() - badgeWidth) / 2 + 1;
  const int x = int(frame % positions) * 2;
  gfx.fillRect(previousX, badgeY, badgeWidth, 32, GFX_BLACK);
  badge.fillSprite(GFX_NAVY);
  badge.drawNumber(frame++, badgeWidth / 2, 16);
  badge.pushSprite(x, badgeY);
  const nice_gfx::Status submitted = gfx.lastError();
  const nice_gfx::Status completed = gfx.wait();
  if (submitted != nice_gfx::Status::Ok || completed != nice_gfx::Status::Ok) {
    Serial.println("Sprite transfer failed");
    ready = false;
  }
  previousX = x;
}
