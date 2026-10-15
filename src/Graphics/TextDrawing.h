#pragma once
#include "Gfx.h"

// Draw with explicit fonts and integer scaling. A temporary selection
// preserves the surrounding font, size and alignment settings.
namespace nice_gfx { namespace text {
class Selection {
 public:
  Selection(NICE_GFX &target, const nice_gfx::MonoFont &font, uint8_t scale)
      : target_(target), font_(target.getFont()), size_(target.textsize),
        datum_(target.getTextDatum()) {
    target_.setFont(font);
    target_.setTextSize(scale);
  }
  ~Selection() {
    target_.setFont(font_);
    target_.setTextSize(size_);
    target_.setTextDatum(datum_);
  }
  Selection(const Selection &) = delete;
  Selection &operator=(const Selection &) = delete;
 private:
  NICE_GFX &target_;
  const nice_gfx::MonoFont *font_;
  uint8_t size_, datum_;
};
inline void select(NICE_GFX &target, const nice_gfx::MonoFont &font, uint8_t scale = 1) {
  target.setFont(font);
  target.setTextSize(scale);
}
template<class Text>
int16_t draw(NICE_GFX &target, const Text &text, int32_t x, int32_t y,
             const nice_gfx::MonoFont &font, uint8_t scale = 1) {
  Selection selection(target, font, scale);
  return target.drawString(text, x, y);
}
template<class Text>
int16_t center(NICE_GFX &target, const Text &text, int32_t x, int32_t y,
               const nice_gfx::MonoFont &font, uint8_t scale = 1) {
  Selection selection(target, font, scale);
  target.setTextDatum(TC_DATUM);
  return target.drawString(text, x, y);
}
template<class Text>
int16_t right(NICE_GFX &target, const Text &text, int32_t x, int32_t y,
              const nice_gfx::MonoFont &font, uint8_t scale = 1) {
  Selection selection(target, font, scale);
  target.setTextDatum(TR_DATUM);
  return target.drawString(text, x, y);
}
inline int16_t number(NICE_GFX &target, long value, int32_t x, int32_t y,
                      const nice_gfx::MonoFont &font, uint8_t scale = 1) {
  Selection selection(target, font, scale);
  return target.drawNumber(value, x, y);
}
inline int16_t decimal(NICE_GFX &target, float value, uint8_t precision,
                       int32_t x, int32_t y, const nice_gfx::MonoFont &font,
                       uint8_t scale = 1) {
  Selection selection(target, font, scale);
  return target.drawFloat(value, precision, x, y);
}
inline int16_t glyph(NICE_GFX &target, uint16_t codepoint, int32_t x, int32_t y,
                     const nice_gfx::MonoFont &font, uint8_t scale = 1) {
  Selection selection(target, font, scale);
  return target.drawChar(codepoint, x, y);
}
template<class Text>
int16_t width(NICE_GFX &target, const Text &text, const nice_gfx::MonoFont &font,
              uint8_t scale = 1) {
  Selection selection(target, font, scale);
  return target.textWidth(text);
}
inline int16_t height(NICE_GFX &target, const nice_gfx::MonoFont &font,
                      uint8_t scale = 1) {
  Selection selection(target, font, scale);
  return target.fontHeight();
}
inline void cursor(NICE_GFX &target, int16_t x, int16_t y,
                    const nice_gfx::MonoFont &font, uint8_t scale = 1) {
  select(target, font, scale);
  target.setCursor(x, y);
}
} } // namespace nice_gfx::text
