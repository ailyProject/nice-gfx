#include "../UnifontCN16.h"
#include "../GfxMonoCN16.h"
#if !defined(__AVR__)
#include "../FontStorage.h"
namespace nice_gfx { namespace fonts {
#include "UnifontCN.inc"
// Both descriptors share one constant bitmap/index, with no dynamic initializer.
const MonoFont GfxMonoCN16 = { unifontCNBitmap, unifontCNCodepoints, 7540, 16, 16, 14, nullptr, 1, MonoHalfAscii };
} }
#endif
