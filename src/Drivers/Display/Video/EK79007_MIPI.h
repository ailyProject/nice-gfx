#pragma once
#include "../../../Display/VideoControllerTypes.h"

namespace nice_gfx { namespace video {
static const uint8_t EK79007_MIPI_data_0[] = {0x8B};
static const uint8_t EK79007_MIPI_data_1[] = {0x78};
static const uint8_t EK79007_MIPI_data_2[] = {0x84};
static const uint8_t EK79007_MIPI_data_3[] = {0x88};
static const uint8_t EK79007_MIPI_data_4[] = {0xA8};
static const uint8_t EK79007_MIPI_data_5[] = {0xE3};
static const uint8_t EK79007_MIPI_data_6[] = {0x88};
static const InitCommand EK79007_MIPI_Init[] = {
  {0x80, EK79007_MIPI_data_0, 1, 0},
  {0x81, EK79007_MIPI_data_1, 1, 0},
  {0x82, EK79007_MIPI_data_2, 1, 0},
  {0x83, EK79007_MIPI_data_3, 1, 0},
  {0x84, EK79007_MIPI_data_4, 1, 0},
  {0x85, EK79007_MIPI_data_5, 1, 0},
  {0x86, EK79007_MIPI_data_6, 1, 0},
  {0x11, nullptr, 0, 120},
};
static constexpr VideoInitProgram EK79007_MIPI_Program(EK79007_MIPI_Init, sizeof(EK79007_MIPI_Init) / sizeof(InitCommand));
} } // namespace nice_gfx::video
