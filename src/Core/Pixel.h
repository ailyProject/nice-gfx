#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include "Limits.h"
namespace nice_gfx {
enum class PixelOrder : uint8_t { Native, Wire }; // RGB565 words, or MSB-first bytes
enum class PixelFormat : uint8_t { RGB565, RGB666, RGB888 };

struct PixelView {
  const uint8_t *data;
  size_t width, height, stride; // All sizes are in pixels, including stride.
  PixelOrder order;
  PixelView(const void *p, size_t w, size_t h = 1, size_t s = 0,
            PixelOrder o = PixelOrder::Native)
      : data(static_cast<const uint8_t *>(p)), width(w), height(h), stride(s ? s : w), order(o) {}
  bool valid() const {
    if (!data || !width || !height || stride < width || width > SIZE_MAX / 2) return false;
    return height - 1 <= (SIZE_MAX / 2 - width) / stride;
  }
};

// Portable conversion used by DMA staging and host tests. Never mutates input.
inline void copyPixels(uint8_t *dst, const uint8_t *src, size_t count, PixelOrder order) {
  if (!count) return;
  if (order == PixelOrder::Wire) {
    if (dst != src) memcpy(dst, src, count * 2);
    return;
  }
#if defined(_WIN32) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
  // Two pixels per word on 32-bit MCUs. Fixed-size memcpy permits unaligned
  // sources and avoids strict-aliasing violations; no larger staging is needed.
  if (sizeof(size_t) > 2) while (count >= 2) {
    uint32_t pair;
    memcpy(&pair, src, sizeof(pair));
    pair = ((pair & 0x00FF00FFUL) << 8) | ((pair >> 8) & 0x00FF00FFUL);
    memcpy(dst, &pair, sizeof(pair));
    dst += 4; src += 4; count -= 2;
  }
#endif
  while (count--) {
    uint16_t value;
    memcpy(&value, src, sizeof(value));
    dst[0] = static_cast<uint8_t>(value >> 8);
    dst[1] = static_cast<uint8_t>(value);
    dst += 2; src += 2;
  }
}
}
