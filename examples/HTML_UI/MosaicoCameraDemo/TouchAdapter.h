#pragma once
#include "Hardware.h"
#include <UI/Input.h>
#include <Wire.h>
using nice_gfx::htmlui::TouchSample;
inline bool &cst9220NeedsAck() {
  static bool needsAck = true;
  return needsAck;
}
inline bool writeCst9220Command(uint16_t command) {
  Wire.beginTransmission(uint8_t(90));
  Wire.write(uint8_t(command >> 8)); Wire.write(uint8_t(command));
  return Wire.endTransmission() == 0;
}
inline bool readCst9220Register(uint16_t reg, uint8_t *data, uint8_t length) {
  if (!writeCst9220Command(reg)) return false;
  if (Wire.requestFrom(uint8_t(90), length) != length) {
    while (Wire.available()) Wire.read(); return false;
  }
  for (uint8_t i = 0; i < length; ++i) data[i] = uint8_t(Wire.read());
  return true;
}
inline bool acknowledgeCst9220Report() {
  if (!cst9220NeedsAck()) return true;
  Wire.beginTransmission(uint8_t(90));
  Wire.write(uint8_t(0xD0)); Wire.write(uint8_t(0x00)); Wire.write(uint8_t(0xAB));
  return Wire.endTransmission() == 0;
}
inline TouchSample readCst9220Packet(int32_t &x, int32_t &y) {
  // Two point records occupy a 15-byte burst, with the second at offset 7.
  uint8_t packet[15];
  if (!readCst9220Register(0xD000, packet, sizeof(packet))) return TouchSample::Error;
  // Release the frame before decoding, including empty/release/bad reports.
  if (!acknowledgeCst9220Report()) return TouchSample::Error;
  // Match Espressif's driver: an empty or consumed/not-ready report has no
  // contact. Frequent polling may observe these between real touch frames;
  // treating them as transport faults would cancel every pending click.
  if (packet[0] == 0 || packet[0] == 0xAB || packet[6] != 0xAB) return TouchSample::Released;
  const uint8_t points = packet[5] & 0x7F;
  if (points > 2) return TouchSample::Error;
  const uint16_t nativeWidth = kRotation & 1 ? kUiHeight : kUiWidth;
  const uint16_t nativeHeight = kRotation & 1 ? kUiWidth : kUiHeight;
  bool active = false;
  uint16_t px = 0, py = 0;
  for (uint8_t i = 0; i < points; ++i) {
    const uint8_t *point = packet + (i == 0 ? 0 : 7);
    // Release records retain both coordinates and the record count.
    if (((point[0] & 0x0F) >> 1) != 3) continue;
    const uint16_t pointX = (uint16_t(point[1]) << 4) | (point[3] >> 4);
    const uint16_t pointY = (uint16_t(point[2]) << 4) | (point[3] & 0x0F);
    if (pointX >= nativeWidth || pointY >= nativeHeight) return TouchSample::Error;
    if (!active) { px = pointX; py = pointY; active = true; }
  }
  if (!active) return TouchSample::Released;
  switch (kRotation) {
    case 1: x = py; y = nativeWidth - 1 - px; break;
    case 2: x = nativeWidth - 1 - px; y = nativeHeight - 1 - py; break;
    case 3: x = nativeHeight - 1 - py; y = px; break;
    default: x = px; y = py;
  }
  return TouchSample::Pressed;
}
inline bool beginTouch() { cst9220NeedsAck() = true;
  if (6 >= 0) pinMode(6, INPUT_PULLUP);
  if (-1 >= 0) {
    pinMode(-1, OUTPUT); digitalWrite(-1, LOW); delay(10);
    digitalWrite(-1, HIGH); delay(100);
  }
  if (!Wire.begin(56, 3, 400000)) return false;
  Wire.setTimeOut(20);
  if (!writeCst9220Command(0xD101)) return false;
  delay(10);
  uint8_t identity[4];
  if (!readCst9220Register(0xD204, identity, sizeof(identity)) ||
      (uint16_t(identity[2]) | (uint16_t(identity[3]) << 8)) != 0x9220) {
    writeCst9220Command(0xD109); return false;
  }
  const uint16_t project = uint16_t(identity[0]) | (uint16_t(identity[1]) << 8);
  cst9220NeedsAck() = project != 0x542F;
  if (!cst9220NeedsAck()) {
    if (!writeCst9220Command(0xD109)) return false;
    delay(10);
  }
  return true; }
// Custom touch drivers can replace this function. Return Error on lost input;
// a fault must cancel a gesture instead of activating a button on synthetic up.
inline TouchSample sampleTouch(int32_t &x, int32_t &y) {
  // IRQ announces new data; HIGH does not mean the finger was released.
  // Polling also recovers a missed edge and observes the release-status record.
  return readCst9220Packet(x, y);
}

// Startup and input scheduling are separate from the generated UI and your data.
inline bool beginDisplayHardware() {
  if (kDisplayTePin >= 0) pinMode(kDisplayTePin, INPUT);
  auto &gfx = displayInstance();
  if (kBacklightPin >= 0) { pinMode(kBacklightPin, OUTPUT); digitalWrite(kBacklightPin, HIGH); }
  // Match the BSP: complete LCD reset/init before configuring CST9220.
  if (!gfx.begin() || !beginTouch()) { Serial.println("Hardware initialization failed"); return false; }
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
