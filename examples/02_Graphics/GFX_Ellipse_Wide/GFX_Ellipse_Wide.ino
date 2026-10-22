/*
  Ellipse drawing example

  This sketch does not use any fonts.
*/

#include <SPI.h>

#include <NICE_GFX.h> // Hardware-specific library
#include "Hardware.h"

ExampleDisplay gfx;       // Invoke custom library


void setup(void) {
  gfx.begin();

  gfx.setRotation(1);

}

void loop() {

  gfx.fillScreen(GFX_BLACK);

  // Draw some random circles
  for (int i = 0; i < 40; i++)
  {
    int rx = random(60);
    int ry = random(60);
    int x = rx + random(480 - rx - rx);
    int y = ry + random(320 - ry - ry);
    gfx.fillEllipse(x, y, rx, ry, random(0xFFFF));
  }

  delay(2000);
  gfx.fillScreen(GFX_BLACK);

  for (int i = 0; i < 40; i++)
  {
    int rx = random(60);
    int ry = random(60);
    int x = rx + random(480 - rx - rx);
    int y = ry + random(320 - ry - ry);
    gfx.drawEllipse(x, y, rx, ry, random(0xFFFF));
  }

  delay(2000);
}



