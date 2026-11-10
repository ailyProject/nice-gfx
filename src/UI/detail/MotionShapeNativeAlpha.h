#pragma once
#include "../../Display/Transport.h"
#include "../Shapes.h"
#include "Rgb565Blend.h"

namespace nice_gfx { namespace htmlui { namespace detail {

// A bounded native foreground row. Repeated fill/stroke writes overwrite the
// foreground before opacity is applied, exactly like MotionShapeRow. Neither
// transparent pixels nor partially written foregrounds touch the destination.
constexpr int32_t NativeAlphaMotionShapeWidth = 256;
struct NativeAlphaMotionShapeRow {
  uint16_t colors[NativeAlphaMotionShapeWidth];
  uint32_t coverage[NativeAlphaMotionShapeWidth / 32];
  int32_t x = 0, y = 0, width = 0;

  void begin(int32_t left, int32_t top, int32_t count) {
    x = left; y = top; width = count;
    for (auto &bits : coverage) bits = 0;
  }
  void fillRect(int32_t left, int32_t top, int32_t w, int32_t h, uint16_t color) {
    if (top > y || int64_t(top) + h <= y) return;
    const int32_t first = std::max(left, x);
    const int32_t last = int32_t(std::min(int64_t(left) + w, int64_t(x) + width));
    for (int32_t px = first; px < last; ++px) {
      const uint32_t at = uint32_t(px - x);
      colors[at] = color;
      coverage[at / 32] |= uint32_t(1) << (at % 32);
    }
  }
  void composite(uint16_t *target, uint8_t alpha) const {
    const size_t words = (size_t(width) + 31) / 32;
    for (size_t word = 0; word < words; ++word) {
      uint32_t bits = coverage[word];
      while (bits) {
        unsigned bit = 0;
#if defined(__GNUC__) || defined(__clang__)
        bit = unsigned(__builtin_ctz(bits));
#else
        while (!(bits & (uint32_t(1) << bit))) ++bit;
#endif
        bits &= bits - 1;
        const size_t at = word * 32 + bit;
        target[at] = rgb565_blend::color(colors[at], target[at], alpha);
      }
    }
  }
};

// Caller owns an ordinary renderer ROI wholly inside area, with Native565
// destination words and 1..256 visible columns. The same TransformedShape and
// floating scan conversion as the established painter define every pixel.
// A row is rasterized once instead of once per 64-column block. A reusable
// 556-byte stack object avoids heap allocation and full-ROI byte swapping.
inline void paintMotionShapeNativeAlpha(const shape_raster::TransformedShape &shape,
    Rect clipped, const DirtyRect &area, uint16_t *pixels, size_t stride, uint8_t alpha) {
  NativeAlphaMotionShapeRow row;
  for (int32_t y = clipped.y; y < clipped.y + clipped.height; ++y) {
    row.begin(clipped.x, y, clipped.width);
    shape.paint(row, {clipped.x, y, clipped.width, 1});
    row.composite(pixels + size_t(y - area.y) * stride + size_t(clipped.x - area.x), alpha);
  }
}

}}}
