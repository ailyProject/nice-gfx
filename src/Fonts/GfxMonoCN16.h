#pragma once
#include "Monospace.h"
namespace nice_gfx { namespace fonts {
// Reuses UnifontCN16 data under its existing SIL OFL 1.1 notice.
// CJK stays 16px; printable ASCII uses the original centered 8px of each cell.
#if !defined(__AVR__)
extern const MonoFont GfxMonoCN16;
#endif
} }
