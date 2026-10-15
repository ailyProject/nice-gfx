#pragma once
#include <stdint.h>

namespace nice_gfx {
// Decode whole Unicode sequences, returning U+FFFD for unsupported non-BMP
// characters and malformed sequences. Zero means more bytes are required.
inline uint16_t utf8Byte(uint8_t c, uint8_t &state, uint32_t &value) {
  if (state && (c & 0xC0) == 0x80) {
    value = (value << 6) | (c & 0x3F);
    --state;
    if (state & 3) return 0;
    const uint8_t length = state >> 2;
    state = 0;
    if ((length == 2 && value < 0x80) || (length == 3 && value < 0x800) ||
        (length == 4 && value < 0x10000) || value > 0xFFFF ||
        (value >= 0xD800 && value <= 0xDFFF)) return 0xFFFD;
    return uint16_t(value);
  }
  // Resynchronize at a new leading byte; never swallow the following ASCII.
  state = 0;
  if (c < 0x80) return c;
  if (c >= 0xC2 && c <= 0xDF) { value = c & 0x1F; state = (2 << 2) | 1; }
  else if (c >= 0xE0 && c <= 0xEF) { value = c & 0x0F; state = (3 << 2) | 2; }
  else if (c >= 0xF0 && c <= 0xF4) { value = c & 7; state = (4 << 2) | 3; }
  else return 0xFFFD;
  return 0;
}

inline uint16_t utf8Next(const uint8_t *buf, uint16_t &index, uint16_t remaining) {
  if (!buf || !remaining) return 0;
  uint8_t state = 0;
  uint32_t value = 0;
  while (remaining--) {
    const uint16_t code = utf8Byte(buf[index++], state, value);
    if (!state) return code;
  }
  return 0xFFFD; // Truncated string; do not read past its end.
}
} // namespace nice_gfx
