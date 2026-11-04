#pragma once
#include "../Controls.h"
#include <vector>

namespace nice_gfx { namespace htmlui {
// Copy-on-write like InstrumentState: raster frames retain their own values.
// Only configured controls allocate this state; NodeDesc remains unchanged.
struct ControlState {
  int32_t upper=0;
  uint8_t part=0;
  std::vector<uint8_t> qr;
};
} }
