#pragma once
#include "../Panel.h"

namespace nice_gfx {

template<class Controller, uint16_t Width, uint16_t Height>
class SizedPanel : public Controller {
  static PanelOptions dimensions(PanelOptions options) {
    if (!options.width) options.width = Width;
    if (!options.height) options.height = Height;
    return options;
  }
 public:
  SizedPanel(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Controller(bus, reset, dimensions(options)) {}
};

} // namespace nice_gfx
