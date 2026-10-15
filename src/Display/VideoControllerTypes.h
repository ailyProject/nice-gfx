#pragma once
#include "Transport.h"

namespace nice_gfx {

// NICE_GFX controller identifiers for RGB/MIPI register programs.
enum class VideoController : uint8_t {
  ST7701, ILI9881C, EK79007, ST77922, JD9365, GC9503, ST7123,
  ST7703, JD9165, ST7121, ST77903, HX8399, NV3052, ER88577,
  HX8394, ST7102, OTA7290B, AXS15260D, JD9366, HI8561, SD5207,
  CO5300, ST77916, ST7796, AXS15231B, EK9716B, ST7262
};
enum class VideoInterface : uint8_t { RGB, MIPI };

namespace video {
struct InitCommand {
  uint8_t command;
  const uint8_t *data;
  uint16_t dataBytes;
  uint16_t delayMs;
};
}
typedef video::InitCommand VideoInitCommand;
struct VideoInitProgram {
  const VideoInitCommand *commands;
  size_t count;
  constexpr VideoInitProgram(const VideoInitCommand *c = nullptr, size_t n = 0) : commands(c), count(n) {}
};
// Commands are copied synchronously, never modified. Their storage must remain
// valid through begin(). A non-null pointer replaces the module sequence after
// the controller's required prefix; the caller then owns all mode registers.
struct VideoInitOverride {
  const VideoInitCommand *commands;
  size_t count;
  constexpr VideoInitOverride(const VideoInitCommand *c = nullptr, size_t n = 0) : commands(c), count(n) {}
};
typedef Status (*VideoWrite)(uint8_t command, const uint8_t *data, size_t bytes, void *context);
typedef void (*VideoDelay)(uint32_t milliseconds, void *context);

struct VideoControllerProfile {
  const char *name;
  bool supportsRgb, supportsMipi, noCommand;
  uint8_t laneMask, defaultLanes;
  uint32_t rgbBitsMask, mipiBitsMask;
  uint16_t resetPreMs, resetActiveMs, resetIdleMs, softResetMs;
  bool initRequiresVideo, defaultBgr, colorOrderSupported;
  VideoInitProgram program;
};

} // namespace nice_gfx
