#pragma once
#include "../../../Display/VideoControllerTypes.h"

namespace nice_gfx { namespace video {
static const uint8_t JD9165_MIPI_data_0[] = {0x00};
static const uint8_t JD9165_MIPI_data_1[] = {0x00};
static const InitCommand JD9165_MIPI_Init[] = {
  {0x11, JD9165_MIPI_data_0, 1, 120},
  {0x29, JD9165_MIPI_data_1, 1, 20},
};
static constexpr VideoInitProgram JD9165_MIPI_Program(JD9165_MIPI_Init, sizeof(JD9165_MIPI_Init) / sizeof(InitCommand));
} } // namespace nice_gfx::video
