#pragma once
#include "../../Core/Status.h"
#include <stdint.h>

namespace nice_gfx {

struct RawTouch {
  uint16_t x = 0, y = 0, pressure = 0;
};

struct TouchCalibration {
  uint16_t xMin = 300, xSpan = 3600, yMin = 300, ySpan = 3600;
  bool swapXY = true, invertX = true, invertY = false;
  uint8_t rotation = 0; // Display rotation at calibration time (0..7).

  bool valid() const {
    return xSpan && ySpan && rotation < 8 &&
           uint32_t(xMin) + xSpan <= UINT16_MAX &&
           uint32_t(yMin) + ySpan <= UINT16_MAX;
  }
  Status decodeWords(const uint16_t *data, uint8_t referenceRotation = 0) {
    if (!data || !data[1] || !data[3] || (data[4] & ~7U) || referenceRotation > 7)
      return Status::InvalidArgument;
    TouchCalibration c;
    c.xMin = data[0]; c.xSpan = data[1]; c.yMin = data[2]; c.ySpan = data[3];
    c.swapXY = (data[4] & 1) != 0;
    c.invertX = (data[4] & 2) != 0; c.invertY = (data[4] & 4) != 0;
    c.rotation = referenceRotation;
    if (!c.valid()) return Status::InvalidArgument;
    *this = c;
    return Status::Ok;
  }
  void encodeWords(uint16_t *data) const {
    if (!data) return;
    data[0] = xMin; data[1] = xSpan; data[2] = yMin; data[3] = ySpan;
    data[4] = uint16_t(uint16_t(swapXY) | (uint16_t(invertX) << 1) | (uint16_t(invertY) << 2));
  }
  // Four averaged raw positions at top-left, bottom-left, top-right,
  // bottom-right. Callers own the calibration UI and sampling timeout.
  Status fromCorners(const RawTouch *corners, uint8_t referenceRotation = 0) {
    if (!corners || referenceRotation > 7) return Status::InvalidArgument;
    TouchCalibration c;
    c.rotation = referenceRotation;
    const int32_t dx = int32_t(corners[0].x) - corners[1].x;
    const int32_t dy = int32_t(corners[0].y) - corners[1].y;
    c.swapXY = (dx < 0 ? -dx : dx) > (dy < 0 ? -dy : dy);
    uint32_t left = 0, right = 0, top = 0, bottom = 0;
    for (uint8_t i = 0; i < 4; ++i) {
      const uint16_t x = c.swapXY ? corners[i].y : corners[i].x;
      const uint16_t y = c.swapXY ? corners[i].x : corners[i].y;
      (i < 2 ? left : right) += x;
      (i & 1 ? bottom : top) += y;
    }
    left /= 2; right /= 2; top /= 2; bottom /= 2;
    if (left == right || top == bottom) return Status::InvalidArgument;
    c.invertX = left > right; c.invertY = top > bottom;
    c.xMin = uint16_t(c.invertX ? right : left);
    c.xSpan = uint16_t(c.invertX ? left - right : right - left);
    c.yMin = uint16_t(c.invertY ? bottom : top);
    c.ySpan = uint16_t(c.invertY ? top - bottom : bottom - top);
    *this = c;
    return Status::Ok;
  }
  bool convert(uint16_t &x, uint16_t &y, uint16_t width, uint16_t height,
               uint8_t displayRotation = 0) const {
    if (!valid() || !width || !height || displayRotation > 7) return false;
    // Controllers disagree on the meaning of mirror rotations 4..7. Those
    // modes use calibration taken in that exact mode, never a guessed mirror.
    if ((rotation >= 4 || displayRotation >= 4) && rotation != displayRotation) return false;
    const int32_t rawX = swapXY ? y : x, rawY = swapXY ? x : y;
    if (rawX < xMin || rawY < yMin || rawX > int32_t(xMin) + xSpan ||
        rawY > int32_t(yMin) + ySpan) return false;
    const uint8_t delta = (displayRotation + 4 - rotation) & 3;
    const uint16_t w = delta & 1 ? height : width, h = delta & 1 ? width : height;
    uint16_t px = uint16_t(uint64_t(rawX - xMin) * (w - 1) / xSpan);
    uint16_t py = uint16_t(uint64_t(rawY - yMin) * (h - 1) / ySpan);
    if (invertX) px = uint16_t(w - 1 - px);
    if (invertY) py = uint16_t(h - 1 - py);
    switch (delta) {
      case 0: x = px; y = py; break;
      case 1: x = py; y = uint16_t(w - 1 - px); break;
      case 2: x = uint16_t(w - 1 - px); y = uint16_t(h - 1 - py); break;
      default: x = uint16_t(h - 1 - py); y = px; break;
    }
    return true;
  }
};

} // namespace nice_gfx
