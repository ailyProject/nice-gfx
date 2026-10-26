#pragma once
#include <stdint.h>

namespace nice_gfx {

// Storage/color and physical refresh are independent axes. An e-paper target
// need not be monochrome; an OLED need not use a slow physical refresh.
enum class DisplayColorMode : uint8_t { Rgb, Monochrome };
enum class DisplayRefreshMode : uint8_t { Immediate, Epaper };

struct DisplayCapabilities {
  DisplayColorMode colorMode = DisplayColorMode::Rgb;
  DisplayRefreshMode refreshMode = DisplayRefreshMode::Immediate;
  uint8_t bitsPerPixel = 16;
  uint16_t nativeWidth = 0, nativeHeight = 0;
  uint32_t minRefreshMs = 0;
  uint32_t recommendedFrameMs = 0;
  bool partialRefresh = true;
  bool monochrome() const { return colorMode == DisplayColorMode::Monochrome; }
  bool slowRefresh() const { return refreshMode == DisplayRefreshMode::Epaper; }
};

} // namespace nice_gfx
