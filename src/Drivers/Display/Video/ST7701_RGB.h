#pragma once
#include "../../../Display/VideoControllerTypes.h"

namespace nice_gfx { namespace video {
static const uint8_t ST7701_RGB_data_0[] = {0x77, 0x01, 0x00, 0x00, 0x13};
static const uint8_t ST7701_RGB_data_1[] = {0x08};
static const uint8_t ST7701_RGB_data_2[] = {0x77, 0x01, 0x00, 0x00, 0x10};
static const uint8_t ST7701_RGB_data_3[] = {0x3B, 0x00};
static const uint8_t ST7701_RGB_data_4[] = {0x10, 0x02};
static const uint8_t ST7701_RGB_data_5[] = {0x20, 0x06};
static const uint8_t ST7701_RGB_data_6[] = {0x10};
static const uint8_t ST7701_RGB_data_7[] = {0x00, 0x13, 0x5A, 0x0F, 0x12, 0x07, 0x09, 0x08, 0x08, 0x24, 0x07, 0x13, 0x12, 0x6B, 0x73, 0xFF};
static const uint8_t ST7701_RGB_data_8[] = {0x00, 0x13, 0x5A, 0x0F, 0x12, 0x07, 0x09, 0x08, 0x08, 0x24, 0x07, 0x13, 0x12, 0x6B, 0x73, 0xFF};
static const uint8_t ST7701_RGB_data_9[] = {0x77, 0x01, 0x00, 0x00, 0x11};
static const uint8_t ST7701_RGB_data_10[] = {0x8D};
static const uint8_t ST7701_RGB_data_11[] = {0x48};
static const uint8_t ST7701_RGB_data_12[] = {0x89};
static const uint8_t ST7701_RGB_data_13[] = {0x80};
static const uint8_t ST7701_RGB_data_14[] = {0x49};
static const uint8_t ST7701_RGB_data_15[] = {0x85};
static const uint8_t ST7701_RGB_data_16[] = {0x32};
static const uint8_t ST7701_RGB_data_17[] = {0x78};
static const uint8_t ST7701_RGB_data_18[] = {0x78};
static const uint8_t ST7701_RGB_data_19[] = {0x88};
static const uint8_t ST7701_RGB_data_20[] = {0x00, 0x00, 0x02};
static const uint8_t ST7701_RGB_data_21[] = {0x05, 0xC0, 0x07, 0xC0, 0x04, 0xC0, 0x06, 0xC0, 0x00, 0x44, 0x44};
static const uint8_t ST7701_RGB_data_22[] = {0x00, 0x00, 0x33, 0x33, 0x01, 0xC0, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x00, 0x00};
static const uint8_t ST7701_RGB_data_23[] = {0x00, 0x00, 0x11, 0x11};
static const uint8_t ST7701_RGB_data_24[] = {0x44, 0x44};
static const uint8_t ST7701_RGB_data_25[] = {0x0D, 0xF1, 0x10, 0x98, 0x0F, 0xF3, 0x10, 0x98, 0x09, 0xED, 0x10, 0x98, 0x0B, 0xEF, 0x10, 0x98};
static const uint8_t ST7701_RGB_data_26[] = {0x00, 0x00, 0x11, 0x11};
static const uint8_t ST7701_RGB_data_27[] = {0x44, 0x44};
static const uint8_t ST7701_RGB_data_28[] = {0x0C, 0xF0, 0x10, 0x98, 0x0E, 0xF2, 0x10, 0x98, 0x08, 0xEC, 0x10, 0x98, 0x0A, 0xEE, 0x10, 0x98};
static const uint8_t ST7701_RGB_data_29[] = {0x00, 0x01, 0xE4, 0xE4, 0x44, 0x88, 0x00};
static const uint8_t ST7701_RGB_data_30[] = {0xFF, 0x04, 0x56, 0x7F, 0xBA, 0x2F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF2, 0xAB, 0xF7, 0x65, 0x40, 0xFF};
static const uint8_t ST7701_RGB_data_31[] = {0x10, 0x0D, 0x04, 0x08, 0x3F, 0x1F};
static const uint8_t ST7701_RGB_data_32[] = {0x77, 0x01, 0x00, 0x00, 0x00};
static const InitCommand ST7701_RGB_Init[] = {
  {0xFF, ST7701_RGB_data_0, 5, 0},
  {0xEF, ST7701_RGB_data_1, 1, 0},
  {0xFF, ST7701_RGB_data_2, 5, 0},
  {0xC0, ST7701_RGB_data_3, 2, 0},
  {0xC1, ST7701_RGB_data_4, 2, 0},
  {0xC2, ST7701_RGB_data_5, 2, 0},
  {0xCC, ST7701_RGB_data_6, 1, 0},
  {0xB0, ST7701_RGB_data_7, 16, 0},
  {0xB1, ST7701_RGB_data_8, 16, 0},
  {0xFF, ST7701_RGB_data_9, 5, 0},
  {0xB0, ST7701_RGB_data_10, 1, 0},
  {0xB1, ST7701_RGB_data_11, 1, 0},
  {0xB2, ST7701_RGB_data_12, 1, 0},
  {0xB3, ST7701_RGB_data_13, 1, 0},
  {0xB5, ST7701_RGB_data_14, 1, 0},
  {0xB7, ST7701_RGB_data_15, 1, 0},
  {0xB8, ST7701_RGB_data_16, 1, 0},
  {0xC1, ST7701_RGB_data_17, 1, 0},
  {0xC2, ST7701_RGB_data_18, 1, 0},
  {0xD0, ST7701_RGB_data_19, 1, 100},
  {0xE0, ST7701_RGB_data_20, 3, 0},
  {0xE1, ST7701_RGB_data_21, 11, 0},
  {0xE2, ST7701_RGB_data_22, 13, 0},
  {0xE3, ST7701_RGB_data_23, 4, 0},
  {0xE4, ST7701_RGB_data_24, 2, 0},
  {0xE5, ST7701_RGB_data_25, 16, 0},
  {0xE6, ST7701_RGB_data_26, 4, 0},
  {0xE7, ST7701_RGB_data_27, 2, 0},
  {0xE8, ST7701_RGB_data_28, 16, 0},
  {0xEB, ST7701_RGB_data_29, 7, 0},
  {0xED, ST7701_RGB_data_30, 16, 0},
  {0xEF, ST7701_RGB_data_31, 6, 0},
  {0xFF, ST7701_RGB_data_32, 5, 0},
  {0x11, nullptr, 0, 120},
  {0x29, nullptr, 0, 0},
};
static constexpr VideoInitProgram ST7701_RGB_Program(ST7701_RGB_Init, sizeof(ST7701_RGB_Init) / sizeof(InitCommand));
} } // namespace nice_gfx::video

// Type-1 480x480 glass tuning, as used by Waveshare 4B. Host RGB565
// wiring uses the panel RGB666 DPI mode (COLMOD 0x60). Board pins are not
// part of the controller program. Reference: waveshareteam/ESP32-S3-Touch-LCD-4B.
namespace nice_gfx { namespace video {
static const uint8_t ST7701_RGB_Type1_data_1[] = {0x77, 0x01, 0x00, 0x00, 0x10};
static const uint8_t ST7701_RGB_Type1_data_2[] = {0x3B, 0x00};
static const uint8_t ST7701_RGB_Type1_data_3[] = {0x0D, 0x02};
static const uint8_t ST7701_RGB_Type1_data_4[] = {0x21, 0x08};
static const uint8_t ST7701_RGB_Type1_data_5[] = {0x08};
static const uint8_t ST7701_RGB_Type1_data_6[] = {0x00, 0x11, 0x18, 0x0E, 0x11, 0x06, 0x07, 0x08, 0x07, 0x22, 0x04, 0x12, 0x0F, 0xAA, 0x31, 0x18};
static const uint8_t ST7701_RGB_Type1_data_7[] = {0x00, 0x11, 0x19, 0x0E, 0x12, 0x07, 0x08, 0x08, 0x08, 0x22, 0x04, 0x11, 0x11, 0xA9, 0x32, 0x18};
static const uint8_t ST7701_RGB_Type1_data_8[] = {0x77, 0x01, 0x00, 0x00, 0x11};
static const uint8_t ST7701_RGB_Type1_data_9[] = {0x60};
static const uint8_t ST7701_RGB_Type1_data_10[] = {0x30};
static const uint8_t ST7701_RGB_Type1_data_11[] = {0x87};
static const uint8_t ST7701_RGB_Type1_data_12[] = {0x80};
static const uint8_t ST7701_RGB_Type1_data_13[] = {0x49};
static const uint8_t ST7701_RGB_Type1_data_14[] = {0x85};
static const uint8_t ST7701_RGB_Type1_data_15[] = {0x21};
static const uint8_t ST7701_RGB_Type1_data_16[] = {0x78};
static const uint8_t ST7701_RGB_Type1_data_17[] = {0x78};
static const uint8_t ST7701_RGB_Type1_data_18[] = {0x00, 0x1B, 0x02};
static const uint8_t ST7701_RGB_Type1_data_19[] = {0x08, 0xA0, 0x00, 0x00, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x44, 0x44};
static const uint8_t ST7701_RGB_Type1_data_20[] = {0x11, 0x11, 0x44, 0x44, 0xED, 0xA0, 0x00, 0x00, 0xEC, 0xA0, 0x00, 0x00};
static const uint8_t ST7701_RGB_Type1_data_21[] = {0x00, 0x00, 0x11, 0x11};
static const uint8_t ST7701_RGB_Type1_data_22[] = {0x44, 0x44};
static const uint8_t ST7701_RGB_Type1_data_23[] = {0x0A, 0xE9, 0xD8, 0xA0, 0x0C, 0xEB, 0xD8, 0xA0, 0x0E, 0xED, 0xD8, 0xA0, 0x10, 0xEF, 0xD8, 0xA0};
static const uint8_t ST7701_RGB_Type1_data_24[] = {0x00, 0x00, 0x11, 0x11};
static const uint8_t ST7701_RGB_Type1_data_25[] = {0x44, 0x44};
static const uint8_t ST7701_RGB_Type1_data_26[] = {0x09, 0xE8, 0xD8, 0xA0, 0x0B, 0xEA, 0xD8, 0xA0, 0x0D, 0xEC, 0xD8, 0xA0, 0x0F, 0xEE, 0xD8, 0xA0};
static const uint8_t ST7701_RGB_Type1_data_27[] = {0x02, 0x00, 0xE4, 0xE4, 0x88, 0x00, 0x40};
static const uint8_t ST7701_RGB_Type1_data_28[] = {0x3C, 0x00};
static const uint8_t ST7701_RGB_Type1_data_29[] = {0xAB, 0x89, 0x76, 0x54, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x20, 0x45, 0x67, 0x98, 0xBA};
static const uint8_t ST7701_RGB_Type1_data_30[] = {0x77, 0x01, 0x00, 0x00, 0x00};
static const uint8_t ST7701_RGB_Type1_data_31[] = {0x00};
static const uint8_t ST7701_RGB_Type1_data_32[] = {0x60};
static const VideoInitCommand ST7701_RGB_Type1_Init[] = {
  {0x11, nullptr, 0, 120},
  {0xFF, ST7701_RGB_Type1_data_1, 5, 0},
  {0xC0, ST7701_RGB_Type1_data_2, 2, 0},
  {0xC1, ST7701_RGB_Type1_data_3, 2, 0},
  {0xC2, ST7701_RGB_Type1_data_4, 2, 0},
  {0xCD, ST7701_RGB_Type1_data_5, 1, 0},
  {0xB0, ST7701_RGB_Type1_data_6, 16, 0},
  {0xB1, ST7701_RGB_Type1_data_7, 16, 0},
  {0xFF, ST7701_RGB_Type1_data_8, 5, 0},
  {0xB0, ST7701_RGB_Type1_data_9, 1, 0},
  {0xB1, ST7701_RGB_Type1_data_10, 1, 0},
  {0xB2, ST7701_RGB_Type1_data_11, 1, 0},
  {0xB3, ST7701_RGB_Type1_data_12, 1, 0},
  {0xB5, ST7701_RGB_Type1_data_13, 1, 0},
  {0xB7, ST7701_RGB_Type1_data_14, 1, 0},
  {0xB8, ST7701_RGB_Type1_data_15, 1, 0},
  {0xC1, ST7701_RGB_Type1_data_16, 1, 0},
  {0xC2, ST7701_RGB_Type1_data_17, 1, 20},
  {0xE0, ST7701_RGB_Type1_data_18, 3, 0},
  {0xE1, ST7701_RGB_Type1_data_19, 11, 0},
  {0xE2, ST7701_RGB_Type1_data_20, 12, 0},
  {0xE3, ST7701_RGB_Type1_data_21, 4, 0},
  {0xE4, ST7701_RGB_Type1_data_22, 2, 0},
  {0xE5, ST7701_RGB_Type1_data_23, 16, 0},
  {0xE6, ST7701_RGB_Type1_data_24, 4, 0},
  {0xE7, ST7701_RGB_Type1_data_25, 2, 0},
  {0xE8, ST7701_RGB_Type1_data_26, 16, 0},
  {0xEB, ST7701_RGB_Type1_data_27, 7, 0},
  {0xEC, ST7701_RGB_Type1_data_28, 2, 0},
  {0xED, ST7701_RGB_Type1_data_29, 16, 0},
  {0xFF, ST7701_RGB_Type1_data_30, 5, 0},
  {0x36, ST7701_RGB_Type1_data_31, 1, 0},
  {0x3A, ST7701_RGB_Type1_data_32, 1, 0},
  {0x21, nullptr, 0, 120},
  {0x29, nullptr, 0, 0},
};
} }
