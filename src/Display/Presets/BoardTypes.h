#pragma once
#include "../../Core/Status.h"
#include <stddef.h>
#include <string.h>

namespace nice_gfx {
enum class BoardSupport : uint8_t { Ready, Pending, Unsupported };
struct BoardPins {
  int16_t cs,dc,reset,clock,mosi,miso,backlight,power,sda,scl,busy;
  int16_t data[16];
};
struct BoardTouchPins {
  const char *controller;
  int16_t sda,scl,reset,irq;
  uint8_t address;
};
struct BoardPresetInfo {
  const char *id,*label,*displayClass,*fqbn,*display;
  BoardSupport support;
  bool generic;
  uint16_t width,height;
  uint8_t rotation;
  uint32_t frequency;
  BoardPins pins;
  const char *pinLabels; // Original editor labels, including unresolved analog pins.
  BoardTouchPins touch;
  const char *notes;
};
namespace detail {
template<class> struct BoardUnavailable { static constexpr bool value=false; };
}
} // namespace nice_gfx
