#include "../Fonts/Monospace.h"

namespace nice_gfx {
namespace detail {
#if NICE_GFX_ENABLE_PINYIN
namespace {
constexpr uint16_t rankHeader = 257;
constexpr uint16_t rankPage = 24;

uint16_t bitCount(uint16_t value) {
  value = uint16_t(value - ((value >> 1) & 0x5555u));
  value = uint16_t((value & 0x3333u) + ((value >> 2) & 0x3333u));
  value = uint16_t((value + (value >> 4)) & 0x0f0fu);
  return uint16_t((value + (value >> 8)) & 31u);
}

// One directory entry per Unicode page. Each 32-code-point group stores
// its first glyph offset and two 16-bit presence masks.
bool monoRank(const uint16_t *table, uint16_t codepoint, uint16_t &index) {
  const uint16_t page = pgm_read_word(table + 1 + (codepoint >> 8));
  if (!page) return false;
  const uint16_t low = codepoint & 255u;
  const uint16_t *entry = table + page + (low >> 5) * 3u;
  const uint16_t bit = uint16_t(1u << (low & 15u));
  const uint16_t mask = pgm_read_word(entry + 1 + ((low >> 4) & 1u));
  if (!(mask & bit)) return false;
  index = pgm_read_word(entry) + bitCount(uint16_t(mask & (bit - 1)));
  if (low & 16u) index += bitCount(pgm_read_word(entry + 1));
  return true;
}
} // namespace

uint32_t monoIndexBytes(const MonoFont &font) {
  if (!font.codepoints) return 0;
  const uint16_t words = font.compactPunctuation & MonoRankIndex ? pgm_read_word(font.codepoints) : font.count;
  return uint32_t(words) * sizeof(uint16_t);
}

bool validMonoRankIndex(const MonoFont &font) {
  const auto *table = font.codepoints;
  if (!table) return false;
  const uint16_t words = pgm_read_word(table);
  if (words < rankHeader || words > rankHeader + 256 * rankPage) return false;
  uint16_t next = rankHeader;
  uint32_t count = 0;
  for (uint16_t page = 0; page < 256; ++page) {
    const uint16_t offset = pgm_read_word(table + 1 + page);
    if (!offset) continue;
    if (offset != next || uint32_t(offset) + rankPage > words) return false;
    for (uint16_t group = 0; group < rankPage; group += 3) {
      const auto *entry = table + offset + group;
      if (pgm_read_word(entry) != count) return false;
      count += bitCount(pgm_read_word(entry + 1)) + bitCount(pgm_read_word(entry + 2));
    }
    next += rankPage;
  }
  uint16_t replacement;
  return next == words && count == font.count && monoRank(table, '?', replacement);
}
#else
uint32_t monoIndexBytes(const MonoFont &font) {
  return font.codepoints ? uint32_t(font.count) * sizeof(uint16_t) : 0;
}
bool validMonoRankIndex(const MonoFont &) { return false; }
#endif

const uint8_t *monoGlyphExact(const MonoFont &font, uint16_t codepoint) {
  if (codepoint < 32 || codepoint == 127) return nullptr;
  uint16_t index = 0;
  if (!font.codepoints) {
    if (codepoint > 126 || codepoint - 32 >= font.count) return nullptr;
    index = codepoint - 32;
  } else if (font.compactPunctuation & MonoRankIndex) {
#if NICE_GFX_ENABLE_PINYIN
    if (!monoRank(font.codepoints, codepoint, index)) return nullptr;
#else
    return nullptr;
#endif
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
  const uint8_t width=(font.compactPunctuation&MonoHalfAscii)&&codepoint<=126?std::max<uint8_t>(1,font.width/2):font.width;
  if(font.compactPunctuation&MonoCompactPunctuation){if(codepoint=='.'||codepoint==','||codepoint==':')return std::max<uint8_t>(1,width/3);if(codepoint==' ')return std::max<uint8_t>(1,width/2);}
  return width;
}


} // namespace detail
} // namespace nice_gfx
