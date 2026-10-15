#pragma once
#include <stdint.h>

namespace nice_gfx {
// Row-major mask, each row padded to whole bytes. Zero coverage is transparent.
// bitsPerPixel=4 uses high nibble first (0..15 coverage); 0/1 retains the legacy
// MSB-first binary mask. Bitmap may reside in PROGMEM; descriptor is const.
struct MonoIcon { const uint8_t *bitmap; uint16_t width, height, bytes; uint8_t bitsPerPixel; };
}
