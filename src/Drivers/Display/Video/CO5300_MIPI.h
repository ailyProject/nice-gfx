#pragma once
#include "../../../Display/VideoControllerTypes.h"

namespace nice_gfx { namespace video {
static const uint8_t CO5300_MIPI_data_0[] = {0x20};
static const uint8_t CO5300_MIPI_data_1[] = {0x5A};
static const uint8_t CO5300_MIPI_data_2[] = {0x59};
static const uint8_t CO5300_MIPI_data_3[] = {0x80};
static const uint8_t CO5300_MIPI_data_4[] = {0x00};
static const uint8_t CO5300_MIPI_data_5[] = {0x00};
static const uint8_t CO5300_MIPI_data_6[] = {0x00};
static const uint8_t CO5300_MIPI_data_7[] = {0x20};
static const uint8_t CO5300_MIPI_data_8[] = {0xFF};
static const uint8_t CO5300_MIPI_data_9[] = {0xAB};
static const uint8_t CO5300_MIPI_data_10[] = {0x00, 0x06, 0x01, 0xD7};
static const uint8_t CO5300_MIPI_data_11[] = {0x00, 0x00, 0x01, 0xD1};
static const InitCommand CO5300_MIPI_Init[] = {
  {0xFE, CO5300_MIPI_data_0, 1, 0},
  {0xF4, CO5300_MIPI_data_1, 1, 0},
  {0xF5, CO5300_MIPI_data_2, 1, 0},
  {0xFE, CO5300_MIPI_data_3, 1, 0},
  {0x03, CO5300_MIPI_data_4, 1, 0},
  {0xFE, CO5300_MIPI_data_5, 1, 0},
  {0x35, CO5300_MIPI_data_6, 1, 0},
  {0x53, CO5300_MIPI_data_7, 1, 0},
  {0x51, CO5300_MIPI_data_8, 1, 0},
  {0x63, CO5300_MIPI_data_9, 1, 0},
  {0x2A, CO5300_MIPI_data_10, 4, 0},
  {0x2B, CO5300_MIPI_data_11, 4, 0},
  {0x11, nullptr, 0, 400},
  {0x29, nullptr, 0, 0},
};
static constexpr VideoInitProgram CO5300_MIPI_Program(CO5300_MIPI_Init, sizeof(CO5300_MIPI_Init) / sizeof(InitCommand));
} } // namespace nice_gfx::video
