// A local MonoFont descriptor demonstrates custom bitmaps and Unicode glyphs.
// Its bitmap and sorted index are included with this sketch, without flags.
#include <NICE_GFX.h>
#include "Hardware.h"
#include "MyFont.h"
ExampleDisplay gfx;
void setup() {
  gfx.begin();
  gfx.setRotation(1);
  gfx.setFont(LocalFont);
  gfx.setTextColor(GFX_WHITE, GFX_NAVY);
}
void loop() {
  gfx.fillScreen(GFX_NAVY);
  gfx.setTextDatum(MC_DATUM);
  const uint8_t scales[] = {1, 2, 3};
  for (uint8_t scale : scales) {
    gfx.setTextSize(scale);
    gfx.drawString("A µ ä ?", gfx.width() / 2, 24 + scale * 40);
  }
  gfx.setTextSize(2);
  gfx.setCursor(4, 4);
  gfx.print("A µ ä");
  delay(2500);
}
