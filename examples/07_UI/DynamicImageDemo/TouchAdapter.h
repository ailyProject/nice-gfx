#pragma once
#include "Hardware.h"
#include <UI/Input.h>
#include <Wire.h>
using nice_gfx::htmlui::TouchSample;
#if defined(IRAM_ATTR)
#define GFX_UI_TOUCH_ISR_ATTR IRAM_ATTR
#elif defined(ARDUINO_ISR_ATTR)
#define GFX_UI_TOUCH_ISR_ATTR ARDUINO_ISR_ATTR
#else
#define GFX_UI_TOUCH_ISR_ATTR
#endif
#if defined(__GNUC__)
#define GFX_UI_TOUCH_ALWAYS_INLINE __attribute__((always_inline))
#define GFX_UI_TOUCH_ISR_LINKAGE __attribute__((weak))
#else
#define GFX_UI_TOUCH_ALWAYS_INLINE
#define GFX_UI_TOUCH_ISR_LINKAGE inline
#endif
// C++11 inline function statics share one counter across translation units.
// Inline the accessor into the ISR; Xtensa inline/COMDAT IRAM functions can
// place their literal pool in flash, beyond the ISR's backward l32r reach.
inline GFX_UI_TOUCH_ALWAYS_INLINE volatile uint32_t &touchIrqCounter() {
  static volatile uint32_t generation = 0;
  return generation;
}
// A non-inline weak ISR retains one handler across GCC translation units while
// keeping its literal pool beside the custom IRAM section on Xtensa.
GFX_UI_TOUCH_ISR_LINKAGE GFX_UI_TOUCH_ISR_ATTR void touchDataReady() { ++touchIrqCounter(); }
// Diagnostic readers observe the adapter's IRQ; do not install another ISR.
inline uint32_t touchIrqGeneration() { return touchIrqCounter(); }
#undef GFX_UI_TOUCH_ISR_ATTR
#undef GFX_UI_TOUCH_ALWAYS_INLINE
#undef GFX_UI_TOUCH_ISR_LINKAGE
struct Chsc6417SampleState {
  TouchSample sample = TouchSample::Released;
  int32_t x = 0, y = 0;
  int32_t startupX = 0, startupY = 0;
  uint32_t consumedGeneration = 0, retryAt = 0, pollAt = 0;
  bool initialPending = false, retryPending = false;
  bool startupPoint = false, contactArmed = false;
};
inline Chsc6417SampleState &chsc6417SampleState() {
  static Chsc6417SampleState state;
  return state;
}
inline TouchSample readChsc6417Packet(int32_t &x, int32_t &y, bool &transportError) {
  transportError = false;
  for (uint8_t attempt = 0; attempt < 2; ++attempt) {
    Wire1.beginTransmission(uint8_t(46)); Wire1.write(uint8_t(0xE0));
    if (Wire1.endTransmission(false) == 0 &&
        Wire1.requestFrom(uint8_t(46), uint8_t(3)) == 3) {
      break;
    }
    while (Wire1.available()) Wire1.read();
    if (attempt == 1) { transportError = true; return TouchSample::Error; }
    delay(3); // Retry only transport faults, once, with a fresh E0 transaction.
  }
  uint8_t packet[3]; for (auto &v : packet) v = uint8_t(Wire1.read());
  const uint8_t points = packet[0] & 0x03;
  if (points == 0) return TouchSample::Released;
  if (points != 1) return TouchSample::Error;
  const uint16_t px = uint16_t((packet[0] & 0x40) << 2) | packet[1];
  const uint16_t py = uint16_t((packet[0] & 0x80) << 1) | packet[2];
  if (px >= 480 || py >= 480) return TouchSample::Error;
  switch (kRotation) {
    case 1: x = py; y = 479 - px; break;
    case 2: x = 479 - px; y = 479 - py; break;
    case 3: x = 479 - py; y = px; break;
    default: x = px; y = py;
  }
  return TouchSample::Pressed;
}
inline bool beginTouch() { if (10 >= 0) { detachInterrupt(digitalPinToInterrupt(10)); pinMode(10, INPUT_PULLUP); }
  chsc6417SampleState() = Chsc6417SampleState();
  touchIrqCounter() = 0;
  if (48 >= 0) {
    pinMode(48, OUTPUT); digitalWrite(48, LOW); delay(20);
    digitalWrite(48, HIGH); delay(200);
  }
  if (!Wire1.begin(21, 47, 400000)) return false;
  Wire1.setTimeOut(20);
  Wire1.beginTransmission(uint8_t(46));
  if (Wire1.endTransmission() != 0) return false;
  // E0 may retain an old Pressed packet after reset. Establish its baseline
  // before accepting fallback contacts; a fresh IRQ/LOW notification remains
  // authoritative. Failure here must not delay or prevent the first frame.
  auto &state = chsc6417SampleState();
  bool transportError = false;
  const TouchSample baseline = readChsc6417Packet(state.startupX, state.startupY, transportError);
  state.startupPoint = baseline == TouchSample::Pressed;
  state.contactArmed = baseline == TouchSample::Released;
  state.pollAt = millis() + 20;
  if (transportError) { state.retryPending = true; state.retryAt = millis() + 20; }
  if (10 >= 0) {
    attachInterrupt(digitalPinToInterrupt(10), touchDataReady, FALLING);
    // A controller already holding data-ready low may not generate a new edge.
    chsc6417SampleState().initialPending = digitalRead(10) == LOW;
  }
  return true; }
// Custom touch drivers can replace this function. Return Error on lost input;
// a fault must cancel a gesture instead of activating a button on synthetic up.
inline TouchSample sampleTouch(int32_t &x, int32_t &y) {
  auto &state = chsc6417SampleState();
  const uint32_t generation = touchIrqGeneration(); // Acknowledge only this snapshot.
  const uint32_t now = millis();
  const bool notified = generation != state.consumedGeneration || state.initialPending;
  if (10 >= 0 && generation == state.consumedGeneration && !state.initialPending &&
      (!state.retryPending || int32_t(now - state.retryAt) < 0) && int32_t(now - state.pollAt) < 0) {
    // IRQ means new data, not contact level. HIGH must retain the last contact;
    // a zero-point packet releases it. Periodic recovery reads also handle an
    // absent IRQ connection or a missed release notification.
    if (state.sample == TouchSample::Pressed) { x = state.x; y = state.y; }
    return state.sample;
  }
  state.initialPending = state.retryPending = false;
  const int32_t previousX = x, previousY = y;
  bool transportError = false;
  state.sample = readChsc6417Packet(x, y, transportError);
  if (10 >= 0 && state.sample == TouchSample::Pressed && !notified && !state.contactArmed) {
    if (!state.startupPoint) {
      state.startupPoint = true; state.startupX = x; state.startupY = y;
    } else if (x != state.startupX || y != state.startupY) {
      state.contactArmed = true;
    }
    if (!state.contactArmed) { state.sample = TouchSample::Released; x = previousX; y = previousY; }
  } else if (state.sample == TouchSample::Released || state.sample == TouchSample::Pressed) {
    state.contactArmed = true;
  }
  state.consumedGeneration = generation; // Preserve edges arriving during I2C/retry.
  if (state.sample == TouchSample::Pressed) { state.x = x; state.y = y; }
  // Poll slowly while idle, promptly while held. IRQ notifications bypass this
  // bound, and a configured -1 IRQ keeps explicit every-call polling semantics.
  state.pollAt = millis() + (state.sample == TouchSample::Pressed ? 10 : 20);
  if (transportError && 10 >= 0) {
    // A missed data-ready edge after a bus fault must not strand the adapter.
    // Error cancels the gesture; a bounded probe can recover without IRQ spam.
    state.retryPending = true; state.retryAt = millis() + 20;
  }
  return state.sample;
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
