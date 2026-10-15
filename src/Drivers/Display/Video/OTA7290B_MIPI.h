#pragma once
#include "../../../Display/VideoControllerTypes.h"

namespace nice_gfx { namespace video {
static const uint8_t OTA7290B_MIPI_data_0[] = {0x00};
static const uint8_t OTA7290B_MIPI_data_1[] = {0x00};
static const InitCommand OTA7290B_MIPI_Init[] = {
  {0x11, OTA7290B_MIPI_data_0, 1, 120},
  {0x29, OTA7290B_MIPI_data_1, 1, 20},
};
static constexpr VideoInitProgram OTA7290B_MIPI_Program(OTA7290B_MIPI_Init, sizeof(OTA7290B_MIPI_Init) / sizeof(InitCommand));
} } // namespace nice_gfx::video
