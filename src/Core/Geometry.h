#pragma once
#include <stdint.h>

namespace nice_gfx {
struct ClippedRect {
  int32_t x, y, width, height, sourceX, sourceY;
};
// All additions/subtractions use 64 bits before narrowing. A very distant
// image/origin must clip away, never wrap around into a visible destination.
inline bool clipRect(int32_t x, int32_t y, int32_t width, int32_t height,
                     int32_t originX, int32_t originY, int32_t left,
                     int32_t top, int32_t right, int32_t bottom, ClippedRect &out) {
  if (width <= 0 || height <= 0 || right <= left || bottom <= top) return false;
  const int64_t sx = int64_t(x) + originX, sy = int64_t(y) + originY;
  const int64_t x0 = sx < left ? left : sx, y0 = sy < top ? top : sy;
  const int64_t x1 = sx + width > right ? right : sx + width;
  const int64_t y1 = sy + height > bottom ? bottom : sy + height;
  if (x1 <= x0 || y1 <= y0) return false;
  out = {int32_t(x0), int32_t(y0), int32_t(x1-x0), int32_t(y1-y0),
         int32_t(x0-sx), int32_t(y0-sy)};
  return true;
}
}
