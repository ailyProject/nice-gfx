// Start here: edit Hardware.h, then choose your board in the Arduino IDE.
#include <NICE_GFX.h>
#include "Hardware.h"
#include "ExampleDrawing.h"

ExampleDisplay gfx;

void setup() {
  Serial.begin(115200);
  if (!exampleBegin(gfx)) {
    Serial.println("Display initialization failed; check Hardware.h");
    return;
  }
  const int width = gfx.width(), height = gfx.height();
  if (!exampleDraw(gfx, [=](GFX_Sprite &canvas) {
    canvas.setFont(nice_gfx::fonts::Fixed6x13);
    canvas.setTextDatum(MC_DATUM);
    canvas.setTextColor(GFX_CYAN, GFX_BLACK);
    canvas.drawString("NICE_GFX", width / 2, height / 2 - 12);
    canvas.setTextColor(GFX_WHITE, GFX_BLACK);
    canvas.drawString("Hello!", width / 2, height / 2 + 12);
  })) Serial.println("Drawing failed; check display status and available RAM");
}

void loop() { delay(10); }
