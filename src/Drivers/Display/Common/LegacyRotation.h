#pragma once
#include "../../../Display/ControllerProfile.h"

namespace nice_gfx { namespace controllers {

inline void rotate_SeeedLegacy(PanelSequence &s, uint8_t) {
  s.colstart = s.rowstart = 0;
  s.writeRegister16(3,0x1030);
}

} } // namespace nice_gfx::controllers
