// JC4880P443C_I_W ST7701 MIPI initialization using NICE_GFX VideoInitCommand.
// Register and timing reference: ST7701 datasheet; panel tuning: JC4880P443C_I_W specifications.
#pragma once
#include "../../../Display/VideoControllerTypes.h"
namespace nice_gfx { namespace video {
static const uint8_t ST7701_JC4880_data_0[] = {0x77, 0x01, 0x00, 0x00, 0x13};
static const uint8_t ST7701_JC4880_data_1[] = {0x08};
static const uint8_t ST7701_JC4880_data_2[] = {0x77, 0x01, 0x00, 0x00, 0x10};
static const uint8_t ST7701_JC4880_data_3[] = {0x63, 0x00};
static const uint8_t ST7701_JC4880_data_4[] = {0x0D, 0x02};
static const uint8_t ST7701_JC4880_data_5[] = {0x10, 0x08};
static const uint8_t ST7701_JC4880_data_6[] = {0x10};
static const uint8_t ST7701_JC4880_data_7[] = {0x80, 0x09, 0x53, 0x0C, 0xD0, 0x07, 0x0C, 0x09, 0x09, 0x28, 0x06, 0xD4, 0x13, 0x69, 0x2B, 0x71};
static const uint8_t ST7701_JC4880_data_8[] = {0x80, 0x94, 0x5A, 0x10, 0xD3, 0x06, 0x0A, 0x08, 0x08, 0x25, 0x03, 0xD3, 0x12, 0x66, 0x6A, 0x0D};
static const uint8_t ST7701_JC4880_data_9[] = {0x77, 0x01, 0x00, 0x00, 0x11};
static const uint8_t ST7701_JC4880_data_10[] = {0x5D};
static const uint8_t ST7701_JC4880_data_11[] = {0x58};
static const uint8_t ST7701_JC4880_data_12[] = {0x87};
static const uint8_t ST7701_JC4880_data_13[] = {0x80};
static const uint8_t ST7701_JC4880_data_14[] = {0x4E};
static const uint8_t ST7701_JC4880_data_15[] = {0x85};
static const uint8_t ST7701_JC4880_data_16[] = {0x21};
static const uint8_t ST7701_JC4880_data_17[] = {0x10, 0x1F};
static const uint8_t ST7701_JC4880_data_18[] = {0x03};
static const uint8_t ST7701_JC4880_data_19[] = {0x00};
static const uint8_t ST7701_JC4880_data_20[] = {0x78};
static const uint8_t ST7701_JC4880_data_21[] = {0x78};
static const uint8_t ST7701_JC4880_data_22[] = {0x88};
static const uint8_t ST7701_JC4880_data_23[] = {0x00, 0x3A, 0x02};
static const uint8_t ST7701_JC4880_data_24[] = {0x04, 0xA0, 0x00, 0xA0, 0x05, 0xA0, 0x00, 0xA0, 0x00, 0x40, 0x40};
static const uint8_t ST7701_JC4880_data_25[] = {0x30, 0x00, 0x40, 0x40, 0x32, 0xA0, 0x00, 0xA0, 0x00, 0xA0, 0x00, 0xA0, 0x00};
static const uint8_t ST7701_JC4880_data_26[] = {0x00, 0x00, 0x33, 0x33};
static const uint8_t ST7701_JC4880_data_27[] = {0x44, 0x44};
static const uint8_t ST7701_JC4880_data_28[] = {0x09, 0x2E, 0xA0, 0xA0, 0x0B, 0x30, 0xA0, 0xA0, 0x05, 0x2A, 0xA0, 0xA0, 0x07, 0x2C, 0xA0, 0xA0};
static const uint8_t ST7701_JC4880_data_29[] = {0x00, 0x00, 0x33, 0x33};
static const uint8_t ST7701_JC4880_data_30[] = {0x44, 0x44};
static const uint8_t ST7701_JC4880_data_31[] = {0x08, 0x2D, 0xA0, 0xA0, 0x0A, 0x2F, 0xA0, 0xA0, 0x04, 0x29, 0xA0, 0xA0, 0x06, 0x2B, 0xA0, 0xA0};
static const uint8_t ST7701_JC4880_data_32[] = {0x00, 0x00, 0x4E, 0x4E, 0x00, 0x00, 0x00};
static const uint8_t ST7701_JC4880_data_33[] = {0x08, 0x01};
static const uint8_t ST7701_JC4880_data_34[] = {0xB0, 0x2B, 0x98, 0xA4, 0x56, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0x65, 0x4A, 0x89, 0xB2, 0x0B};
static const uint8_t ST7701_JC4880_data_35[] = {0x08, 0x08, 0x08, 0x45, 0x3F, 0x54};
static const uint8_t ST7701_JC4880_data_36[] = {0x77, 0x01, 0x00, 0x00, 0x00};
static const uint8_t ST7701_JC4880_data_37[] = {0x00};
static const uint8_t ST7701_JC4880_data_38[] = {0x00};
static const VideoInitCommand ST7701_JC4880_Init[] = {
  {0xFF, ST7701_JC4880_data_0, 5, 0},
  {0xEF, ST7701_JC4880_data_1, 1, 0},
  {0xFF, ST7701_JC4880_data_2, 5, 0},
  {0xC0, ST7701_JC4880_data_3, 2, 0},
  {0xC1, ST7701_JC4880_data_4, 2, 0},
  {0xC2, ST7701_JC4880_data_5, 2, 0},
  {0xCC, ST7701_JC4880_data_6, 1, 0},
  {0xB0, ST7701_JC4880_data_7, 16, 0},
  {0xB1, ST7701_JC4880_data_8, 16, 0},
  {0xFF, ST7701_JC4880_data_9, 5, 0},
  {0xB0, ST7701_JC4880_data_10, 1, 0},
  {0xB1, ST7701_JC4880_data_11, 1, 0},
  {0xB2, ST7701_JC4880_data_12, 1, 0},
  {0xB3, ST7701_JC4880_data_13, 1, 0},
  {0xB5, ST7701_JC4880_data_14, 1, 0},
  {0xB7, ST7701_JC4880_data_15, 1, 0},
  {0xB8, ST7701_JC4880_data_16, 1, 0},
  {0xB9, ST7701_JC4880_data_17, 2, 0},
  {0xBB, ST7701_JC4880_data_18, 1, 0},
  {0xBC, ST7701_JC4880_data_19, 1, 0},
  {0xC1, ST7701_JC4880_data_20, 1, 0},
  {0xC2, ST7701_JC4880_data_21, 1, 0},
  {0xD0, ST7701_JC4880_data_22, 1, 0},
  {0xE0, ST7701_JC4880_data_23, 3, 0},
  {0xE1, ST7701_JC4880_data_24, 11, 0},
  {0xE2, ST7701_JC4880_data_25, 13, 0},
  {0xE3, ST7701_JC4880_data_26, 4, 0},
  {0xE4, ST7701_JC4880_data_27, 2, 0},
  {0xE5, ST7701_JC4880_data_28, 16, 0},
  {0xE6, ST7701_JC4880_data_29, 4, 0},
  {0xE7, ST7701_JC4880_data_30, 2, 0},
  {0xE8, ST7701_JC4880_data_31, 16, 0},
  {0xEB, ST7701_JC4880_data_32, 7, 0},
  {0xEC, ST7701_JC4880_data_33, 2, 0},
  {0xED, ST7701_JC4880_data_34, 16, 0},
  {0xEF, ST7701_JC4880_data_35, 6, 0},
  {0xFF, ST7701_JC4880_data_36, 5, 0},
  {0x11, ST7701_JC4880_data_37, 1, 120},
  {0x29, ST7701_JC4880_data_38, 1, 20}
};
} } // namespace nice_gfx::video
