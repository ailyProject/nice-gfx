#pragma once
#include "Monospace.h"
namespace nice_gfx { namespace fonts {
// ASCII, GB2312 non-Han characters and 9,000 Han glyphs; use subsets on AVR.
#if !defined(__AVR__)
extern const MonoFont GfxKaiCN24;
#endif
} }
