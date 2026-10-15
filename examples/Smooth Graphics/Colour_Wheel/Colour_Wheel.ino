// Arc drawing example - draw a colour wheel

#include <NICE_GFX.h>       // Include the graphics library
#include <Display/Presets/ExampleBoard.h>
ExampleDisplay gfx;  // Create object "gfx"

uint16_t colors[12];

// -------------------------------------------------------------------------
// Setup
// -------------------------------------------------------------------------
void setup(void) {
  Serial.begin(115200);
  gfx.begin();
  gfx.fillScreen(GFX_BLACK);

  // Create the outer ring colours
  for (uint8_t c = 0; c < 2; c++) {
    colors[c + 10] = gfx.alphaBlend(128 + c * 127, GFX_RED,     GFX_MAGENTA);
    colors[c +  8] = gfx.alphaBlend(128 + c * 127, GFX_MAGENTA, GFX_BLUE);
    colors[c +  6] = gfx.alphaBlend(128 + c * 127, GFX_BLUE,    GFX_GREEN);
    colors[c +  4] = gfx.alphaBlend(128 + c * 127, GFX_GREEN,   GFX_YELLOW);
    colors[c +  2] = gfx.alphaBlend(128 + c * 127, GFX_YELLOW,  GFX_ORANGE);
    colors[c +  0] = gfx.alphaBlend(128 + c * 127, GFX_ORANGE,  GFX_RED);
  }
}

// -------------------------------------------------------------------------
// Main loop
// -------------------------------------------------------------------------
void loop() {
  uint16_t rDelta = (gfx.width() - 1) / 10;
  uint16_t x = gfx.width() / 2;
  uint16_t y = gfx.height() / 2;
  bool smooth = true;

  // Draw rings as a series of arcs, increasingly blend colour with white towards middle
  for (uint16_t i = 5; i > 0; i--) {
    for (uint16_t angle = 0; angle <= 330; angle += 30) {
      uint16_t radius = i * rDelta;
      uint16_t wheelColor = gfx.alphaBlend((i * 255.0)/5.0, colors[angle / 30], GFX_WHITE);
      gfx.drawArc(x, y, radius, radius - rDelta, angle, angle + 30, wheelColor, GFX_BLACK, smooth);
    }
    smooth = false;  // Only outer ring is smooth
  }

  while (1) delay(100);
}
