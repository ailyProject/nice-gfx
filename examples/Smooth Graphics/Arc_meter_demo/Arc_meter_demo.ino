// Random-size antialiased arc meters with fixed-width numeric Sprite labels.
// Digit spacing stays constant while values change. No external font renderer.
#include <NICE_GFX.h>
#include "Hardware.h"
ExampleDisplay gfx;
GFX_Sprite spr(&gfx);
#define DRAW_DIGITS
constexpr uint16_t DARKER_GREY = 0x18E3;
constexpr uint32_t LOOP_DELAY = 0;
uint32_t runTime = 0;
bool initMeter = true;

void ringMeter(int x, int y, int r, int value, const char *units) {
  static uint16_t lastAngle = 30;
  if (initMeter) {
    initMeter = false;
    lastAngle = 30;
    gfx.fillCircle(x, y, r, DARKER_GREY);
    gfx.drawSmoothCircle(x, y, r, GFX_SILVER, DARKER_GREY);
    const int outer = r - 3;
    gfx.drawArc(x, y, outer, outer - outer / 5, 30, 330, GFX_BLACK, DARKER_GREY);
  }
  r -= 3;
  const int angle = map(value, 0, 100, 30, 330);
  if (lastAngle == angle) return;
#ifdef DRAW_DIGITS
  if (r >= 25) {
    spr.setFont(nice_gfx::fonts::Spleen12x24);
    spr.setTextSize(uint8_t(constrain(r / 32, 1, 4)));
    const int width = spr.textWidth("100"), height = spr.fontHeight();
    if (spr.createSprite(width, height)) {
      spr.fillSprite(DARKER_GREY);
      spr.setTextColor(GFX_WHITE, DARKER_GREY);
      spr.setTextDatum(MC_DATUM);
      spr.drawNumber(value, width / 2, height / 2);
      spr.pushSprite(x - width / 2, y - height / 2);
      spr.deleteSprite();
    }
    gfx.setFont(nice_gfx::fonts::ProFont6x12);
    gfx.setTextSize(1);
    gfx.setTextDatum(TC_DATUM);
    gfx.setTextColor(GFX_GOLD, DARKER_GREY);
    gfx.drawString(units, x, y + r / 2);
  }
#endif
  const int thickness = r < 25 ? r / 3 : r / 5;
  if (angle > lastAngle)
    gfx.drawArc(x, y, r, r - thickness, lastAngle, angle, GFX_SKYBLUE, GFX_BLACK);
  else
    gfx.drawArc(x, y, r, r - thickness, angle, lastAngle, GFX_BLACK, DARKER_GREY);
  lastAngle = angle;
}
void setup() {
  Serial.begin(115200);
  gfx.begin();
  gfx.setRotation(1);
  gfx.fillScreen(GFX_NAVY);
  gfx.setFont(nice_gfx::fonts::ProFont6x12);
  spr.setFont(nice_gfx::fonts::Spleen12x24);
}
void loop() {
  const int maximumRadius = min(gfx.width(), gfx.height()) * 3 / 10;
  const int radius = maximumRadius > 20 ? random(20, maximumRadius) : 20;
  const int x = gfx.width() / 2, y = gfx.height() / 2;
  gfx.fillScreen(GFX_NAVY);
  initMeter = true;
  for (int cycle = 0; cycle < 2; ++cycle) {
    const int direction = cycle == 0 ? 1 : -1;
    for (int step = 0; step <= 100; ++step) {
      while (uint32_t(millis() - runTime) < LOOP_DELAY) yield();
      runTime = millis();
      ringMeter(x, y, radius, direction > 0 ? step : 100 - step, "Watts");
    }
    delay(1000);
  }
}
