#pragma once
#include "../../../Display/VideoControllerTypes.h"

namespace nice_gfx { namespace video {
static const uint8_t ST7701_MIPI_data_0[] = {0x77, 0x01, 0x00, 0x00, 0x13};
static const uint8_t ST7701_MIPI_data_1[] = {0x08};
static const uint8_t ST7701_MIPI_data_2[] = {0x77, 0x01, 0x00, 0x00, 0x10};
static const uint8_t ST7701_MIPI_data_3[] = {0x2C, 0x00};
static const uint8_t ST7701_MIPI_data_4[] = {0x10, 0x0C};
static const uint8_t ST7701_MIPI_data_5[] = {0x21, 0x0A};
static const uint8_t ST7701_MIPI_data_6[] = {0x10};
static const uint8_t ST7701_MIPI_data_7[] = {0x00, 0x0B, 0x12, 0x0D, 0x10, 0x06, 0x02, 0x08, 0x07, 0x1F, 0x04, 0x11, 0x0F, 0x29, 0x31, 0x1E};
static const uint8_t ST7701_MIPI_data_8[] = {0x00, 0x0B, 0x13, 0x0D, 0x11, 0x06, 0x03, 0x08, 0x07, 0x20, 0x04, 0x12, 0x11, 0x29, 0x31, 0x1E};
static const uint8_t ST7701_MIPI_data_9[] = {0x77, 0x01, 0x00, 0x00, 0x11};
static const uint8_t ST7701_MIPI_data_10[] = {0x5D};
static const uint8_t ST7701_MIPI_data_11[] = {0x72};
static const uint8_t ST7701_MIPI_data_12[] = {0x84};
static const uint8_t ST7701_MIPI_data_13[] = {0x80};
static const uint8_t ST7701_MIPI_data_14[] = {0x4D};
static const uint8_t ST7701_MIPI_data_15[] = {0x85};
static const uint8_t ST7701_MIPI_data_16[] = {0x20};
static const uint8_t ST7701_MIPI_data_17[] = {0x78};
static const uint8_t ST7701_MIPI_data_18[] = {0x78};
static const uint8_t ST7701_MIPI_data_19[] = {0x88};
static const uint8_t ST7701_MIPI_data_20[] = {0x80, 0x00, 0x02};
static const uint8_t ST7701_MIPI_data_21[] = {0x05, 0x00, 0x07, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x33, 0x33};
static const uint8_t ST7701_MIPI_data_22[] = {0x00, 0x00, 0x30, 0x30, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
static const uint8_t ST7701_MIPI_data_23[] = {0x00, 0x00, 0x11, 0x11};
static const uint8_t ST7701_MIPI_data_24[] = {0x44, 0x44};
static const uint8_t ST7701_MIPI_data_25[] = {0x0C, 0x78, 0x00, 0xE0, 0x0E, 0x7A, 0x00, 0xE0, 0x08, 0x74, 0x00, 0xE0, 0x0A, 0x76, 0x00, 0xE0};
static const uint8_t ST7701_MIPI_data_26[] = {0x00, 0x00, 0x11, 0x11};
static const uint8_t ST7701_MIPI_data_27[] = {0x44, 0x44};
static const uint8_t ST7701_MIPI_data_28[] = {0x0D, 0x79, 0x00, 0xE0, 0x0F, 0x7B, 0x00, 0xE0, 0x09, 0x75, 0x00, 0xE0, 0x0B, 0x77, 0x00, 0xE0};
static const uint8_t ST7701_MIPI_data_29[] = {0x36, 0x00};
static const uint8_t ST7701_MIPI_data_30[] = {0x00, 0x01, 0xE4, 0xE4, 0x44, 0x88, 0x40};
static const uint8_t ST7701_MIPI_data_31[] = {0xA1, 0xC2, 0xFB, 0x0F, 0x67, 0x45, 0xFF, 0xFF, 0xFF, 0xFF, 0x54, 0x76, 0xF0, 0xBF, 0x2C, 0x1A};
static const uint8_t ST7701_MIPI_data_32[] = {0x10, 0x0D, 0x04, 0x08, 0x3F, 0x1F};
static const uint8_t ST7701_MIPI_data_33[] = {0x77, 0x01, 0x00, 0x00, 0x13};
static const uint8_t ST7701_MIPI_data_34[] = {0x00, 0x0E};
static const uint8_t ST7701_MIPI_data_35[] = {0x00, 0x0C};
static const uint8_t ST7701_MIPI_data_36[] = {0x00, 0x00};
static const uint8_t ST7701_MIPI_data_37[] = {0x77, 0x01, 0x00, 0x00, 0x00};
static const InitCommand ST7701_MIPI_Init[] = {
  {0xFF, ST7701_MIPI_data_0, 5, 0},
  {0xEF, ST7701_MIPI_data_1, 1, 0},
  {0xFF, ST7701_MIPI_data_2, 5, 0},
  {0xC0, ST7701_MIPI_data_3, 2, 0},
  {0xC1, ST7701_MIPI_data_4, 2, 0},
  {0xC2, ST7701_MIPI_data_5, 2, 0},
  {0xCC, ST7701_MIPI_data_6, 1, 0},
  {0xB0, ST7701_MIPI_data_7, 16, 0},
  {0xB1, ST7701_MIPI_data_8, 16, 0},
  {0xFF, ST7701_MIPI_data_9, 5, 0},
  {0xB0, ST7701_MIPI_data_10, 1, 0},
  {0xB1, ST7701_MIPI_data_11, 1, 0},
  {0xB2, ST7701_MIPI_data_12, 1, 0},
  {0xB3, ST7701_MIPI_data_13, 1, 0},
  {0xB5, ST7701_MIPI_data_14, 1, 0},
  {0xB7, ST7701_MIPI_data_15, 1, 0},
  {0xB8, ST7701_MIPI_data_16, 1, 0},
  {0xC1, ST7701_MIPI_data_17, 1, 0},
  {0xC2, ST7701_MIPI_data_18, 1, 0},
  {0xD0, ST7701_MIPI_data_19, 1, 0},
  {0xE0, ST7701_MIPI_data_20, 3, 0},
  {0xE1, ST7701_MIPI_data_21, 11, 0},
  {0xE2, ST7701_MIPI_data_22, 12, 0},
  {0xE3, ST7701_MIPI_data_23, 4, 0},
  {0xE4, ST7701_MIPI_data_24, 2, 0},
  {0xE5, ST7701_MIPI_data_25, 16, 0},
  {0xE6, ST7701_MIPI_data_26, 4, 0},
  {0xE7, ST7701_MIPI_data_27, 2, 0},
  {0xE8, ST7701_MIPI_data_28, 16, 0},
  {0xE9, ST7701_MIPI_data_29, 2, 0},
  {0xEB, ST7701_MIPI_data_30, 7, 0},
  {0xED, ST7701_MIPI_data_31, 16, 0},
  {0xEF, ST7701_MIPI_data_32, 6, 0},
  {0xFF, ST7701_MIPI_data_33, 5, 0},
  {0xE8, ST7701_MIPI_data_34, 2, 0},
  {0xE8, ST7701_MIPI_data_35, 2, 20},
  {0xE8, ST7701_MIPI_data_36, 2, 0},
  {0xFF, ST7701_MIPI_data_37, 5, 0},
  {0x11, nullptr, 0, 120},
  {0x29, nullptr, 0, 0},
};
static constexpr VideoInitProgram ST7701_MIPI_Program(ST7701_MIPI_Init, sizeof(ST7701_MIPI_Init) / sizeof(InitCommand));
} } // namespace nice_gfx::video
