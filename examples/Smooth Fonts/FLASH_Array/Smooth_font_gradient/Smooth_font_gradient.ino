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
  const uint16_t top = GFX_BLUE, bottom = GFX_RED;
  gfx.fillRectVGradient(0, 0, gfx.width(), gfx.height(), top, bottom);
  gfx.setFont(nice_gfx::fonts::Spleen12x24);
  gfx.setTextColor(GFX_WHITE); // Transparent glyphs preserve every background row.
  gfx.setTextDatum(MC_DATUM);
  gfx.drawString("Gradient", gfx.width() / 2, gfx.height() / 2 - 24);
  gfx.setFont(nice_gfx::fonts::GfxTerm8x16);
  gfx.drawString("Transparent text", gfx.width() / 2, gfx.height() / 2 + 16);
  delay(2500);
  gfx.fillRectHGradient(0, 0, gfx.width(), gfx.height(), GFX_DARKGREEN, GFX_NAVY);
  for (int i = 0; i <= 100; ++i) {
    gfx.setTextColor(GFX_YELLOW, GFX_NAVY);
    gfx.setTextPadding(gfx.textWidth("100.00"));
    gfx.drawFloat(i / 10.0, 2, gfx.width() / 2, gfx.height() / 2);
    delay(20);
  }
  gfx.setTextPadding(0);
}
