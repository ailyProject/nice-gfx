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
  gfx.fillScreen(GFX_NAVY);
  gfx.setFont(nice_gfx::fonts::GfxTerm8x16);
  gfx.setTextColor(GFX_WHITE, GFX_NAVY);
  const int x = gfx.width() / 2, y = gfx.height() / 2;
  const uint8_t datums[] = {TL_DATUM, TC_DATUM, TR_DATUM, ML_DATUM,
    MC_DATUM, MR_DATUM, BL_DATUM, BC_DATUM, BR_DATUM,
    L_BASELINE, C_BASELINE, R_BASELINE};
  for (uint8_t datum : datums) {
    gfx.fillScreen(GFX_NAVY);
    gfx.drawFastHLine(0, y, gfx.width(), GFX_DARKGREY);
    gfx.drawFastVLine(x, 0, gfx.height(), GFX_DARKGREY);
    gfx.setTextDatum(datum);
    gfx.drawString("Aligned text", x, y);
    delay(600);
  }
  gfx.setTextDatum(TC_DATUM);
  gfx.fillScreen(GFX_NAVY);
  gfx.setFont(nice_gfx::fonts::Spleen12x24);
  gfx.drawString("Measured padding", x, 8);
  gfx.setTextPadding(gfx.textWidth("999.99"));
  for (int value = 0; value <= 200; ++value) {
    gfx.drawFloat(value / 10.0, 2, x, 48);
    delay(20);
  }
  gfx.setTextPadding(0);
  gfx.setTextDatum(TL_DATUM);
}
