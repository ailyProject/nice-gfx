#pragma once
#include "Pixel.h"
#include "Status.h"
namespace nice_gfx {
inline size_t wirePixelBytes(PixelFormat format) { return format == PixelFormat::RGB565 ? 2 : 3; }
inline void encodePixels(uint8_t *out, const uint8_t *in, size_t count,
                         PixelOrder order, PixelFormat format) {
  if (format == PixelFormat::RGB565) { copyPixels(out, in, count, order); return; }
  while (count--) {
    uint16_t color;
    if (order == PixelOrder::Wire) color = uint16_t(in[0]) << 8 | in[1];
    else memcpy(&color, in, sizeof(color));
    in += 2;
    uint8_t r = uint8_t((color >> 8) & 0xF8), g = uint8_t((color >> 3) & 0xFC), b = uint8_t(color << 3);
    // Replicate low bits before quantizing; red/blue 31 must become 63 in RGB666.
    r |= r >> 5; g |= g >> 6; b |= b >> 5;
    if (format == PixelFormat::RGB666) { r &= 0xFC; g &= 0xFC; b &= 0xFC; }
    *out++ = r; *out++ = g; *out++ = b;
  }
}
}
