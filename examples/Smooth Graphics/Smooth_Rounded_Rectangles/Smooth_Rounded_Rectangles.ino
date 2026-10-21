// Draw random coloured smooth (anti-aliased) rounded rectangles on the TFT

#include <NICE_GFX.h>
#include "Hardware.h"

ExampleDisplay gfx;

void setup(void) {
  gfx.begin();
  gfx.setFont(nice_gfx::fonts::TomThumb4x6);
  gfx.fillScreen(GFX_BLACK); // Background is black
}

void loop() {
  gfx.fillScreen(GFX_BLACK);
  gfx.setCursor(0, 0);

  // Draw some random smooth rounded rectangles
  for (int i = 0; i < 20; i++)
  {
    int radius = random(60);
    int w = random(2 * radius, 160);
    int h = random(2 * radius, 160);
    int t = random(1, radius / 3);
    int x = random(gfx.width() - w);
    int y = random(gfx.height() - h);

    // Random colour is anti-aliased (blended) with background colour (black in this case)
    gfx.drawSmoothRoundRect(x, y, radius, radius - t, w, h, random(0x10000), GFX_BLACK);
  }
  gfx.print("Variable thickness");
  delay(2000);

  gfx.fillScreen(GFX_BLACK);
  gfx.setCursor(0, 0);

  // Draw some random minimum thickness smooth rounded rectangles
  for (int i = 0; i < 20; i++)
  {
    int radius = random(60);
    int w = random(2 * radius, 160);
    int h = random(2 * radius, 160);
    int t = 0;
    int x = random(gfx.width() - w);
    int y = random(gfx.height() - h);

    // Random colour is anti-aliased (blended) with background colour (black in this case)
    gfx.drawSmoothRoundRect(x, y, radius, radius - t, w, h, random(0x10000), GFX_BLACK);
  }
  gfx.print("Minimum thickness");
  delay(2000);
}
