#include "../Fonts/Monospace.h"

namespace nice_gfx {
namespace detail {
const uint8_t *monoGlyphExact(const MonoFont &font, uint16_t codepoint) {
  if (codepoint < 32 || codepoint == 127) return nullptr;
  uint16_t index = 0;
  if (!font.codepoints) {
    if (codepoint > 126 || codepoint - 32 >= font.count) return nullptr;
    index = codepoint - 32;
  } else {
    uint16_t lo = 0, hi = font.count;
    while (lo < hi) {
      const uint16_t mid = lo + (hi - lo) / 2;
      if (pgm_read_word(font.codepoints + mid) < codepoint) lo = mid + 1;
      else hi = mid;
    }
    if (lo == font.count || pgm_read_word(font.codepoints + lo) != codepoint)
      return nullptr;
    index = lo;
  }
  return font.bitmap + uint32_t(index) * monoStride(font) * font.height;
}
const uint8_t *monoGlyph(const MonoFont &font, uint16_t codepoint) {
  if (codepoint < 32 || codepoint == 127) return nullptr;
  const uint8_t *glyph = monoGlyphExact(font, codepoint);
  return glyph ? glyph : monoGlyphExact(font, '?');
}
const MonoFont *monoFallback(const MonoFont &font, uint16_t codepoint) {
  return font.fallback && !monoGlyphExact(font, codepoint) &&
    monoGlyphExact(*font.fallback, codepoint) ? font.fallback : nullptr;
}
uint8_t monoAdvance(const MonoFont &font, uint16_t codepoint) {
  if (codepoint < 32 || codepoint == 127) return 0;
  if(monoFallback(font,codepoint))return font.height;
  if (font.advances) {
    const uint8_t *glyph = monoGlyph(font, codepoint);

    if (glyph) {
      const uint32_t index = uint32_t(glyph - font.bitmap) / (monoStride(font) * font.height);

      return pgm_read_byte(font.advances + index);
    }
  }

  const uint8_t width=(font.compactPunctuation&MonoHalfAscii)&&codepoint<=126?std::max<uint8_t>(1,font.width/2):font.width;
  if(font.compactPunctuation&MonoCompactPunctuation){if(codepoint=='.'||codepoint==','||codepoint==':')return std::max<uint8_t>(1,width/3);if(codepoint==' ')return std::max<uint8_t>(1,width/2);}
  return width;
}

uint8_t monoTextInkTop(const MonoFont &font, const char *text, int32_t width, uint8_t scale) {
  if (!text || !font.bitmap || !font.advances) return 0;

  uint8_t top = font.height;
  int32_t advance = 0;
  const uint16_t stride = monoStride(font);

  for (const auto *at = reinterpret_cast<const uint8_t *>(text); *at; ++at) {
    if (*at < 32 || *at > 126) return 0;

    advance += monoAdvance(font, *at) * scale;

    if (advance > width) return 0;

    const uint8_t *glyph = monoGlyphExact(font, *at);

    if (!glyph) return 0;

    for (uint8_t y = 0; y < top; ++y) {
      bool ink = false;

      for (uint8_t x = 0; x < font.width; ++x) {
        const uint8_t byte = pgm_read_byte(glyph + y * stride + (font.bitsPerPixel == 4 ? x / 2 : x / 8));

        if (font.bitsPerPixel == 4 ? ((byte >> (x & 1 ? 0 : 4)) & 15) : (byte & (128 >> (x & 7)))) { ink = true; break; }
      }

      if (ink) { top = y; break; }
    }
  }

  return top == font.height ? 0 : top;
}


} // namespace detail
} // namespace nice_gfx
