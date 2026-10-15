#pragma once
#include "../../../Display/VideoControllerTypes.h"

namespace nice_gfx { namespace video {
static const uint8_t ST7796_MIPI_data_1[] = {0x48};
static const uint8_t ST7796_MIPI_data_2[] = {0x77};
static const uint8_t ST7796_MIPI_data_3[] = {0xC3};
static const uint8_t ST7796_MIPI_data_4[] = {0x96};
static const uint8_t ST7796_MIPI_data_5[] = {0x02};
static const uint8_t ST7796_MIPI_data_6[] = {0xC6};
static const uint8_t ST7796_MIPI_data_7[] = {0x2F};
static const uint8_t ST7796_MIPI_data_8[] = {0xC0, 0xF0, 0x35};
static const uint8_t ST7796_MIPI_data_9[] = {0x15};
static const uint8_t ST7796_MIPI_data_10[] = {0xAF};
static const uint8_t ST7796_MIPI_data_11[] = {0x09};
static const uint8_t ST7796_MIPI_data_12[] = {0x22};
static const uint8_t ST7796_MIPI_data_13[] = {0x00};
static const uint8_t ST7796_MIPI_data_14[] = {0xE8, 0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33};
static const uint8_t ST7796_MIPI_data_15[] = {0xE0, 0x70, 0x00, 0x05, 0x03, 0x02, 0x20, 0x29, 0x01, 0x45, 0x30, 0x09, 0x07, 0x22, 0x29};
static const uint8_t ST7796_MIPI_data_16[] = {0xE1, 0x70, 0x0C, 0x10, 0x0F, 0x0E, 0x09, 0x35, 0x64, 0x48, 0x3A, 0x14, 0x13, 0x2E, 0x30};
static const uint8_t ST7796_MIPI_data_17[] = {0xE0, 0x70, 0x04, 0x0A, 0x0B, 0x0A, 0x27, 0x31, 0x55, 0x47, 0x29, 0x13, 0x13, 0x29, 0x2D};
static const uint8_t ST7796_MIPI_data_18[] = {0xE1, 0x70, 0x08, 0x0E, 0x09, 0x08, 0x04, 0x33, 0x32, 0x49, 0x36, 0x14, 0x14, 0x2A, 0x2F};
static const uint8_t ST7796_MIPI_data_20[] = {0xC3};
static const uint8_t ST7796_MIPI_data_21[] = {0x96};
static const uint8_t ST7796_MIPI_data_22[] = {0xC3};
static const InitCommand ST7796_MIPI_Init[] = {
  {0x11, nullptr, 0, 120},
  {0x36, ST7796_MIPI_data_1, 1, 0},
  {0x3A, ST7796_MIPI_data_2, 1, 0},
  {0xF0, ST7796_MIPI_data_3, 1, 0},
  {0xF0, ST7796_MIPI_data_4, 1, 0},
  {0xB4, ST7796_MIPI_data_5, 1, 0},
  {0xB7, ST7796_MIPI_data_6, 1, 0},
  {0xB6, ST7796_MIPI_data_7, 1, 0},
  {0x11, ST7796_MIPI_data_8, 3, 0},
  {0xC1, ST7796_MIPI_data_9, 1, 0},
  {0xC2, ST7796_MIPI_data_10, 1, 0},
  {0xC3, ST7796_MIPI_data_11, 1, 0},
  {0xC5, ST7796_MIPI_data_12, 1, 0},
  {0xC6, ST7796_MIPI_data_13, 1, 0},
  {0x11, ST7796_MIPI_data_14, 9, 0},
  {0x11, ST7796_MIPI_data_15, 15, 0},
  {0x11, ST7796_MIPI_data_16, 15, 0},
  {0x11, ST7796_MIPI_data_17, 15, 0},
  {0x11, ST7796_MIPI_data_18, 15, 0},
  {0x21, nullptr, 0, 0},
  {0xF0, ST7796_MIPI_data_20, 1, 0},
  {0xF0, ST7796_MIPI_data_21, 1, 120},
  {0xF0, ST7796_MIPI_data_22, 1, 0},
  {0x29, nullptr, 0, 0},
  {0x2C, nullptr, 0, 0},
};
static constexpr VideoInitProgram ST7796_MIPI_Program(ST7796_MIPI_Init, sizeof(ST7796_MIPI_Init) / sizeof(InitCommand));
} } // namespace nice_gfx::video
