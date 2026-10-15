#pragma once
#include "Monospace.h"
namespace nice_gfx { namespace fonts {
// The full font exceeds AVR's near-PROGMEM limits. Use a generated subset there.
#if !defined(__AVR__)
extern const MonoFont UnifontCN16;
#endif
} }
