/*
  Ellipse drawing example

  This sketch does not use any fonts.
*/

#include <NICE_GFX.h> // Hardware-specific library
#include <Display/Presets/ExampleBoard.h>
#include <SPI.h>

ExampleDisplay gfx;       // Invoke custom library

void setup(void) {
  gfx.begin();

  gfx.setRotation(1);

}

void loop() {

  gfx.fillScreen(GFX_BLACK);

  // Draw some random ellipses
  for (int i = 0; i < 40; i++)
  {
    int rx = random(60);
    int ry = random(60);
    int x = rx + random(320 - rx - rx);
    int y = ry + random(240 - ry - ry);
    gfx.fillEllipse(x, y, rx, ry, random(0xFFFF));
  }

  delay(2000);
  gfx.fillScreen(GFX_BLACK);

  for (int i = 0; i < 40; i++)
  {
    int rx = random(60);
    int ry = random(60);
    int x = rx + random(320 - rx - rx);
    int y = ry + random(240 - ry - ry);
    gfx.drawEllipse(x, y, rx, ry, random(0xFFFF));
  }

  delay(2000);
}



