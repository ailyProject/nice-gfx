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
  gfx.fillScreen(GFX_BLACK);
  gfx.setFont(nice_gfx::fonts::GfxTerm8x16);
  gfx.setTextColor(GFX_WHITE, GFX_BLACK);
  gfx.setCursor(4, 4);
  gfx.println("GfxTerm8x16");
  gfx.setTextColor(GFX_YELLOW);
  gfx.println("Transparent text");
  gfx.setTextColor(GFX_GREEN, GFX_BLUE);
  gfx.println("RGB backgrounds");
  gfx.println(12345);
  gfx.println(3.14159, 3);
  gfx.println("Print wraps at the display edge and supports newlines.");
  delay(2500);

  gfx.fillScreen(GFX_NAVY);
  gfx.setFont(nice_gfx::fonts::Spleen12x24);
  gfx.setTextColor(GFX_WHITE, GFX_NAVY);
  gfx.setCursor(4, 4);
  gfx.println("Spleen12x24");
  gfx.println("0123456789");
  gfx.setTextSize(2);
  gfx.println("Scale 2");
  gfx.setTextSize(1);
  delay(2500);
}
