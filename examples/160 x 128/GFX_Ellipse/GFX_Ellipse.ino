/*
  Ellipse drawing example
  
  This sketch does not use any fonts.
*/

#include <NICE_GFX.h> // Hardware-specific library
#include "Hardware.h"
#include <SPI.h>

ExampleDisplay gfx;       // Invoke custom library

void setup(void) {
  gfx.begin();

  gfx.setRotation(1);

}

void loop() {

  gfx.fillScreen(GFX_BLACK);

  // Draw some random filled ellipses
  for (int i = 0; i < 20; i++)
  {
    int rx = random(40);
    int ry = random(40);
    int x = rx + random(160 - rx - rx);
    int y = ry + random(128 - ry - ry);
    gfx.fillEllipse(x, y, rx, ry, random(0xFFFF));
  }

  delay(2000);
  gfx.fillScreen(GFX_BLACK);

  // Draw some random outline ellipses
  for (int i = 0; i < 20; i++)
  {
    int rx = random(40);
    int ry = random(40);
    int x = rx + random(160 - rx - rx);
    int y = ry + random(128 - ry - ry);
    gfx.drawEllipse(x, y, rx, ry, random(0xFFFF));
  }

  delay(2000);
}



