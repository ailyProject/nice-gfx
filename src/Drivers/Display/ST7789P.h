#pragma once
#include "ST7789.h"

namespace nice_gfx { namespace controllers {
inline void rotate_ST7789P_284(PanelSequence &s, uint8_t r) {
  controllers::rotate_ST7789(s, r);
  s.colstart = r & 1 ? 18 : 0; s.rowstart = r & 1 ? 0 : 18;
}
inline ControllerProfile profile_ST7789P_284() {
  ControllerProfile p = controllers::profile_ST7789();
  p.name = "ST7789P"; p.height = 284; p.rotate = rotate_ST7789P_284; return p;
}
} }

namespace nice_gfx {
class ST7789P_240x284 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  ST7789P_240x284(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_ST7789P_284(), reset, options) {}
};
}
