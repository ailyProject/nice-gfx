// Check RGB color order, orientation and display dimensions on any preset.
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
    const uint16_t colors[] = {GFX_RED, GFX_GREEN, GFX_BLUE};
    for (int i = 0; i < 3; ++i) {
      const int left = i * width / 3, right = (i + 1) * width / 3;
      canvas.fillRect(left, 0, right - left, height / 2, colors[i]);
    }
    canvas.setFont(nice_gfx::fonts::Fixed6x13);
    canvas.setTextDatum(MC_DATUM);
    canvas.setTextColor(GFX_WHITE, GFX_BLACK);
    canvas.drawString("RGB", width / 2, height * 2 / 3);
    char dimensions[24];
    snprintf(dimensions, sizeof(dimensions), "%d x %d", width, height);
    canvas.drawString(dimensions, width / 2, height * 2 / 3 + 18);
  })) Serial.println("Drawing failed; check display status and available RAM");
  Serial.print("Display: "); Serial.print(width); Serial.print(" x "); Serial.println(height);
}

void loop() { delay(10); }
