// The drawing scales with the active display size, including after rotation.
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
  const int side = width < height ? width : height;
  const int margin = side / 12, radius = side / 8;
  if (!exampleDraw(gfx, [=](GFX_Sprite &canvas) {
    canvas.drawRect(margin, margin, width - 2 * margin,
                    height - 2 * margin, GFX_WHITE);
    canvas.fillCircle(width / 3, height / 3, radius, GFX_CYAN);
    canvas.fillTriangle(width * 2 / 3, height / 5,
                        width / 2, height / 2,
                        width - 2 * margin, height / 2, GFX_ORANGE);
    canvas.fillRectHGradient(2 * margin, height * 2 / 3,
                            width - 4 * margin, radius, GFX_BLUE, GFX_MAGENTA);
    canvas.drawLine(margin, height - margin - 1,
                    width - margin - 1, margin, GFX_GREEN);
  })) Serial.println("Drawing failed; check display status and available RAM");
}

void loop() { delay(10); }
