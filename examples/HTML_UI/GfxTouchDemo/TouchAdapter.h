#pragma once
#include "Hardware.h"
#include <UI/Input.h>
#include <Wire.h>
using nice_gfx::htmlui::TouchSample;

inline bool beginTouch() { pinMode(D2, OUTPUT); digitalWrite(D2, HIGH); // Disable the shared-bus SD card.
  pinMode(D7, INPUT_PULLUP);
  Wire.begin(D4, D5, 100000); Wire.setTimeOut(20);
  return true; }
// Custom touch drivers can replace this function. Return Error on lost input;
// a fault must cancel a gesture instead of activating a button on synthetic up.
inline TouchSample sampleTouch(int32_t &x, int32_t &y) {
  if (digitalRead(D7) != LOW) return TouchSample::Released;
  if (Wire.requestFrom(uint8_t(46), uint8_t(5)) != 5) {
    while (Wire.available()) Wire.read(); return TouchSample::Error;
  }
  uint8_t packet[5]; for (auto &v : packet) v = uint8_t(Wire.read());
  if (packet[0] == 0) return TouchSample::Released;
  if (packet[0] != 1 || packet[2] >= 240 || packet[4] >= 240) return TouchSample::Error;
  uint16_t px = packet[2], py = packet[4];
  switch (kRotation) {
    case 1: x = py; y = 239 - px; break;
    case 2: x = 239 - px; y = 239 - py; break;
    case 3: x = 239 - py; y = px; break;
    default: x = px; y = py;
  }
  return TouchSample::Pressed;
}

// Startup and input scheduling are separate from the generated UI and your data.
inline bool beginDisplayHardware() {
  if (kDisplayTePin >= 0) pinMode(kDisplayTePin, INPUT);
  auto &gfx = displayInstance();
  if (kBacklightPin >= 0) { pinMode(kBacklightPin, OUTPUT); digitalWrite(kBacklightPin, HIGH); }
  if (!beginTouch() || !gfx.begin()) { Serial.println("Hardware initialization failed"); return false; }
  gfx.setRotation(kRotation);
  if (gfx.width() != kUiWidth || gfx.height() != kUiHeight) { Serial.println("Screen dimensions do not match compiled UI"); return false; }
  return true;
}

// One view per sketch. Brief release gaps retain the contact. Errors cancel it
// and suppress the remaining finger movement until a stable release.
inline bool pollUi(nice_gfx::htmlui::View &ui, uint32_t now, uint32_t *inputMicros = nullptr) {
  static nice_gfx::htmlui::TouchInput touchInput(30);
  static uint32_t lastSample = 0;
  const uint32_t inputAt = inputMicros ? micros() : 0;
  if (now - lastSample >= 5) {
    lastSample = now;
    int32_t x = 0, y = 0;
    const auto sample = sampleTouch(x, y);
    touchInput.update(ui, sample, x, y, now);
#ifdef NICE_GFX_TOUCH_SAMPLE_HOOK
    NICE_GFX_TOUCH_SAMPLE_HOOK(ui, sample, x, y, now);
#endif
  }
  if (inputMicros) *inputMicros += uint32_t(micros() - inputAt);
  const auto status = ui.tick(now); // At most one tile; input is not gated by full frames.

  if (status != nice_gfx::Status::Ok && status != nice_gfx::Status::Busy) {
    touchInput.cancel(ui); Serial.println("UI display transfer failed"); return false;
  }
  return true;
}
