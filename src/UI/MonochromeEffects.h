#pragma once
#include <stdint.h>
#define NICE_GFX_UI_MONO_EFFECTS_VERSION 1

namespace nice_gfx { namespace htmlui {

// Quantize only the final composited scanout tile. Retained RGB565 surfaces
// keep their precision, so opacity is never dithered twice during a transition.
enum class MonochromeEffects : uint8_t { Threshold, Bayer4 };

} }
