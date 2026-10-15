#pragma once
#include "Monospace.h"
namespace nice_gfx { namespace fonts {
// Full GB2312 + ASCII; on AVR generate a small application-specific subset.
#if !defined(__AVR__)
extern const MonoFont GfxSerifCN24;
#endif
} }
