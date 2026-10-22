#pragma once
// Generated from examples/_shared/ExampleDrawing.h. Edit this sketch-local copy.
#include "Hardware.h"
#include <Graphics/Sprite.h>

inline bool exampleBegin(NICE_GFX &display, uint8_t rotation = exampleRotation()) {
  if (display.beginChecked() != nice_gfx::Status::Ok) return false;
  display.setRotation(rotation);
  const int backlight = exampleBacklightPin();
  if (backlight >= 0) {
    pinMode(backlight, OUTPUT);
    digitalWrite(backlight, HIGH);
  }
  return true;
}

// Paint in screen coordinates. Two RGB565 rows keep RAM bounded and allow
// text/lines on panels such as CH13613 that require 2x2 output windows.
// The painter may run many times; keep application state changes outside it.
template<class Painter>
bool exampleDraw(NICE_GFX &display, Painter paint) {
  GFX_Sprite strip(&display);
  strip.setColorDepth(16);
  if (!strip.createSprite(display.width(), 2)) return false;
  for (int y = 0; y < display.height(); y += 2) {
    strip.fillSprite(GFX_BLACK);
    strip.setOrigin(0, -y);
    paint(strip);
    strip.pushSprite(0, y);
    if (display.lastError() != nice_gfx::Status::Ok) return false;
    if (display.wait() != nice_gfx::Status::Ok) return false;
  }
  return true;
}
