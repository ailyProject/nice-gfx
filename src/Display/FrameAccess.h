#pragma once
#include "../Graphics/Gfx.h"

namespace nice_gfx {
// Public, non-owning access to the optional native frame interface. The display
// retains ownership. A null result means this sink has no display backend.
// Callers serialize begin/map/flush/commit or abort with other display users;
// framebuffer borrows end at commit/abort and must never outlive the display.
inline Backend *frameBackend(NICE_GFX &display) {
  return detail::DisplayAccess::panel(display);
}
} // namespace nice_gfx
