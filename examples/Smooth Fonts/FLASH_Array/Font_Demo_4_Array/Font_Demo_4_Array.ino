// Font descriptors live in Flash and are selected explicitly with setFont().
// The existing example path is retained; these examples need no font files.
#include <NICE_GFX.h>
#include <Display/Presets/ExampleBoard.h>

ExampleDisplay gfx;
GFX_Sprite spr(&gfx);

// Size the buffer from the selected descriptor, draw once, then release it.
// The display cursor does not move when the Sprite is pushed.
void pushText(const String &text, int x, int y) {
  const int width = spr.textWidth(text), height = spr.fontHeight();
  if (width <= 0 || height <= 0 || !spr.createSprite(width, height)) return;
  spr.fillSprite(GFX_BLUE);
  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(GFX_WHITE, GFX_BLUE);
  spr.drawString(text, 0, 0);
  spr.pushSprite(x, y);
  spr.deleteSprite();
}
void setup() {
  Serial.begin(115200);
  gfx.begin();
  gfx.setRotation(1);
  gfx.setFont(nice_gfx::fonts::GfxTerm8x16);
  gfx.setTextSize(1);
}

void loop() {
  gfx.fillScreen(GFX_BLACK);
  spr.setColorDepth(16);
  spr.setFont(nice_gfx::fonts::GfxTerm8x16);
  pushText("Small Sprite text", 8, 12);
  spr.setFont(nice_gfx::fonts::Spleen12x24);
  pushText("Measured buffer", 8, 48);
  for (int value = 0; value <= 200; ++value) {
    pushText(String(" ") + String(value / 100.0, 2) + " ", 8, 100);
    delay(20);
  }
  delay(1800);
}
