#pragma once
#include "../Core/Pixel.h"

namespace nice_gfx {

enum class MonoDither : uint8_t { None, Bayer4 };
struct MonoOptions {
  uint8_t threshold = 128;
  MonoDither dither = MonoDither::None;
};

// The same integer RGB565 conversion is used on every MCU. Dither coordinates
// are physical display coordinates, so tiled rendering never creates seams.
inline uint8_t monoLuminance(uint16_t color) {
  if (!color) return 0;
  if (color == 0xFFFF) return 255;
  const unsigned r = (color >> 11) & 31, g = (color >> 5) & 63, b = color & 31;
  return uint8_t((((r << 3) | (r >> 2)) * 77U +
                  ((g << 2) | (g >> 4)) * 150U +
                  ((b << 3) | (b >> 2)) * 29U + 128U) >> 8);
}
inline bool monoWhite(uint16_t color, const MonoOptions &options, unsigned x, unsigned y) {
  // Preserve exact black and white independently of threshold/dither.
  if (!color) return false;
  if (color == 0xFFFF) return true;
  int threshold = options.threshold;
  if (options.dither == MonoDither::Bayer4) {
    static const uint8_t matrix[16] = {0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5};
    threshold += int(matrix[((y & 3) << 2) | (x & 3)]) * 16 - 120;
    if (threshold < 1) threshold = 1;
    if (threshold > 255) threshold = 255;
  }
  return monoLuminance(color) >= threshold;
}
inline uint16_t monoRead565(const uint8_t *source, PixelOrder order) {
  if (order == PixelOrder::Wire) return uint16_t(uint16_t(source[0]) << 8 | source[1]);
  uint16_t result; memcpy(&result, source, sizeof(result)); return result;
}

} // namespace nice_gfx
