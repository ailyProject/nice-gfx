#pragma once
#include "Hardware.h"
#include <UI/Input.h>

using nice_gfx::htmlui::TouchSample;

inline bool beginTouch() { return true; }
// Custom touch drivers can replace this function. Return Error on lost input;
// a fault must cancel a gesture instead of activating a button on synthetic up.
inline TouchSample sampleTouch(int32_t &x, int32_t &y) {
  (void)x; (void)y; return TouchSample::Released;
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
