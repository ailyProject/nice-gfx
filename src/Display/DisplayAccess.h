#pragma once
#include "Backend.h"

class NICE_GFX;
namespace nice_gfx { namespace detail {
// Internal bridge for optional display services. The public display API never
// exposes its panel or transport; Graphics/Gfx.h defines this bridge.
struct DisplayAccess { static Backend *panel(NICE_GFX &display); };
} }
