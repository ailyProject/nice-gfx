#pragma once
#include <stddef.h>

namespace nice_gfx {
// Keep nested transfer staging and glyph batches within small MCU RAM budgets.
// Larger address spaces retain the bulk sizes used by the accelerated backends.
static constexpr size_t pixelBufferPixels = sizeof(size_t) <= 2 ? 16 : 128;
static constexpr size_t transferBufferBytes = sizeof(size_t) <= 2 ? 48 : 384;
}
