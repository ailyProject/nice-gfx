// Font descriptors live in Flash and are selected explicitly with setFont().
// The existing example path is retained; these examples need no font files.
#include <NICE_GFX.h>
#include <Display/Presets/ExampleBoard.h>

ExampleDisplay gfx;
void setup() {
  Serial.begin(115200);
  gfx.begin();
  gfx.setRotation(1);
  gfx.setFont(nice_gfx::fonts::GfxTerm8x16);
  gfx.setTextSize(1);
}

void loop() {
  gfx.fillScreen(GFX_BLACK);
  gfx.setFont(nice_gfx::fonts::GfxSansCN16);
  gfx.setTextColor(GFX_WHITE, GFX_BLACK);
  gfx.setCursor(4, 4);
  gfx.println("Unicode / UTF-8");
  gfx.println("中文界面");
  gfx.println("温度 25 度");
  gfx.println("设备正常");
  gfx.println("ASCII: ABC xyz 0123");
  // Both streams and direct Unicode glyphs use the same descriptor.
  gfx.drawChar(0x4E2D, 8, gfx.height() - gfx.fontHeight() - 4);
  delay(2500);
  gfx.setFont(nice_gfx::fonts::UnifontCN16);
  gfx.fillScreen(GFX_NAVY);
  gfx.setCursor(4, 4);
  gfx.println("UnifontCN16");
  gfx.println("你好，世界！");
  gfx.println("Missing glyph: 😀");
  delay(2500);
}
