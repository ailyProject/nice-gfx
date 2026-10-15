#pragma once
#include <stdint.h>
#define NICE_GFX_MONO_FALLBACK_VERSION 1
#define NICE_GFX_MONO_COVERAGE_VERSION 1

namespace nice_gfx {
// Constant descriptor; bitmap and sorted Unicode index are stored in Flash.
// Descriptors supplied to setFont() must outlive the display's use of them.
struct MonoFont {
  const uint8_t *bitmap;
  const uint16_t *codepoints; // nullptr for contiguous printable ASCII
  uint16_t count;
  uint8_t width, height, baseline;
  // Optional one-level fallback. Missing glyphs retain the primary line height
  // and use a square advance; existing six-field descriptors remain valid.
  const MonoFont *fallback;
  // Zero (including omitted fields) and 1 retain the original 1bpp rows.
  // 4 stores row-aligned coverage, 0..15, with the left pixel in the high nibble.
  uint8_t bitsPerPixel;
};
namespace detail {
const uint8_t *monoGlyph(const MonoFont &font, uint16_t codepoint);
const uint8_t *monoGlyphExact(const MonoFont &font, uint16_t codepoint);
const MonoFont *monoFallback(const MonoFont &font, uint16_t codepoint);
uint8_t monoAdvance(const MonoFont &font, uint16_t codepoint);
inline uint16_t monoStride(const MonoFont &font) {
  return font.bitsPerPixel == 4 ? (font.width + 1) / 2 : (font.width + 7) / 8;
}
inline MonoFont withMonoFallback(const MonoFont *font, const MonoFont *fallback) {
  MonoFont result = *font; result.fallback = fallback; return result;
}

} // namespace detail
} // namespace nice_gfx
