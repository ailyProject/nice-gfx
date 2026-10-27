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
  if(font.compactPunctuation){if(codepoint=='.'||codepoint==','||codepoint==':')return std::max<uint8_t>(1,font.width/3);if(codepoint==' ')return std::max<uint8_t>(1,font.width/2);}
  return font.width;
}


} // namespace detail
} // namespace nice_gfx
