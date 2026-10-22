// Font descriptors live in Flash and are selected explicitly with setFont().
// The existing example path is retained; these examples need no font files.
#include <NICE_GFX.h>
#include "Hardware.h"

ExampleDisplay gfx;
void setup() {
  Serial.begin(115200);
  gfx.begin();
  gfx.setRotation(1);
  gfx.setFont(nice_gfx::fonts::GfxTerm8x16);
  gfx.setTextSize(1);
}

void loop() {
  gfx.fillRectVGradient(0, 0, gfx.width(), gfx.height(), GFX_NAVY, GFX_GREEN);
  for (int r = 15; r < gfx.width() / 2; r += 18)
    gfx.drawCircle(gfx.width() / 2, gfx.height() / 2, r, GFX_SKYBLUE);
  uint16_t background = gfx.readPixel(gfx.width() / 2, gfx.height() / 2);
  if (gfx.lastError() != nice_gfx::Status::Ok) background = GFX_NAVY;
  const uint32_t rgb = gfx.color16to24(background);
  const unsigned brightness = ((rgb >> 16) & 255) + ((rgb >> 8) & 255) + (rgb & 255);
  gfx.setFont(nice_gfx::fonts::GfxTermBold8x16);
  gfx.setTextColor(brightness > 384 ? GFX_BLACK : GFX_WHITE);
  gfx.setTextDatum(MC_DATUM);
  gfx.drawString("Text over graphics", gfx.width() / 2, gfx.height() / 2);
  // Glyph backgrounds stay transparent; no framebuffer copy is needed.
  delay(3000);
}
