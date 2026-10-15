#pragma once
#include "../../../Display/VideoControllerTypes.h"

namespace nice_gfx { namespace video {
static const uint8_t ST77903_RGB_data_0[] = {0xC3};
static const uint8_t ST77903_RGB_data_1[] = {0x96};
static const uint8_t ST77903_RGB_data_2[] = {0xA5};
static const uint8_t ST77903_RGB_data_3[] = {0x66, 0x07, 0x4B, 0x0B};
static const uint8_t ST77903_RGB_data_4[] = {0x66, 0x07, 0x4B, 0x0B};
static const uint8_t ST77903_RGB_data_5[] = {0x44, 0x04, 0x44, 0x04};
static const uint8_t ST77903_RGB_data_6[] = {0x44, 0x04, 0x44, 0x04};
static const uint8_t ST77903_RGB_data_7[] = {0xBF, 0x80};
static const uint8_t ST77903_RGB_data_8[] = {0x09};
static const uint8_t ST77903_RGB_data_9[] = {0x04};
static const uint8_t ST77903_RGB_data_10[] = {0xD2, 0x09, 0x0C, 0x07, 0x06, 0x23, 0x2E, 0x43, 0x46, 0x17, 0x13, 0x13, 0x2D, 0x33};
static const uint8_t ST77903_RGB_data_11[] = {0xD2, 0x09, 0x0C, 0x07, 0x05, 0x23, 0x2E, 0x33, 0x46, 0x17, 0x13, 0x13, 0x2D, 0x33};
static const uint8_t ST77903_RGB_data_12[] = {0x5A, 0xF5, 0x64, 0x33, 0x22, 0x25, 0x10, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77};
static const uint8_t ST77903_RGB_data_13[] = {0x5A, 0xF5, 0x64, 0x33, 0x22, 0x25, 0x10, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77};
static const uint8_t ST77903_RGB_data_14[] = {0x00, 0x55, 0x00, 0x00, 0x00, 0x88};
static const uint8_t ST77903_RGB_data_15[] = {0x09};
static const uint8_t ST77903_RGB_data_16[] = {0x01};
static const uint8_t ST77903_RGB_data_17[] = {0x01};
static const uint8_t ST77903_RGB_data_18[] = {0xC0, 0x63};
static const uint8_t ST77903_RGB_data_19[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x2A, 0xBA, 0x02};
static const uint8_t ST77903_RGB_data_20[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x2A, 0xBA, 0x02};
static const uint8_t ST77903_RGB_data_21[] = {0x5A, 0x09, 0x43, 0x00, 0x23, 0x01, 0x00};
static const uint8_t ST77903_RGB_data_22[] = {0x00, 0x24, 0x00, 0x25, 0x83, 0x87, 0x18, 0x00};
static const uint8_t ST77903_RGB_data_23[] = {0x00, 0x24, 0x00, 0x25, 0x83, 0x87, 0x18, 0x00};
static const uint8_t ST77903_RGB_data_24[] = {0x33, 0x44, 0xFF, 0xFF, 0x98, 0xA7, 0x7A, 0x89, 0x52, 0xFF, 0x06};
static const uint8_t ST77903_RGB_data_25[] = {0xC3};
static const uint8_t ST77903_RGB_data_26[] = {0x40, 0x0F, 0x2F};
static const uint8_t ST77903_RGB_data_27[] = {0x00, 0x06, 0x06};
static const uint8_t ST77903_RGB_data_28[] = {0x00};
static const InitCommand ST77903_RGB_Init[] = {
  {0xF0, ST77903_RGB_data_0, 1, 0},
  {0xF0, ST77903_RGB_data_1, 1, 0},
  {0xF0, ST77903_RGB_data_2, 1, 0},
  {0xC1, ST77903_RGB_data_3, 4, 0},
  {0xC2, ST77903_RGB_data_4, 4, 0},
  {0xC3, ST77903_RGB_data_5, 4, 0},
  {0xC4, ST77903_RGB_data_6, 4, 0},
  {0xC5, ST77903_RGB_data_7, 2, 0},
  {0xD6, ST77903_RGB_data_8, 1, 0},
  {0xD7, ST77903_RGB_data_9, 1, 0},
  {0xE0, ST77903_RGB_data_10, 14, 0},
  {0xE1, ST77903_RGB_data_11, 14, 0},
  {0xE5, ST77903_RGB_data_12, 14, 0},
  {0xE6, ST77903_RGB_data_13, 14, 0},
  {0xEC, ST77903_RGB_data_14, 6, 0},
  {0xB2, ST77903_RGB_data_15, 1, 0},
  {0xB3, ST77903_RGB_data_16, 1, 0},
  {0xB4, ST77903_RGB_data_17, 1, 0},
  {0xA4, ST77903_RGB_data_18, 2, 0},
  {0xA5, ST77903_RGB_data_19, 9, 0},
  {0xA6, ST77903_RGB_data_20, 9, 0},
  {0xBA, ST77903_RGB_data_21, 7, 0},
  {0xBB, ST77903_RGB_data_22, 8, 0},
  {0xBC, ST77903_RGB_data_23, 8, 0},
  {0xBD, ST77903_RGB_data_24, 11, 0},
  {0xED, ST77903_RGB_data_25, 1, 0},
  {0xE4, ST77903_RGB_data_26, 3, 0},
  {0xA0, ST77903_RGB_data_27, 3, 0},
  {0x35, ST77903_RGB_data_28, 1, 0},
  {0x21, nullptr, 0, 0},
  {0x11, nullptr, 0, 120},
  {0x29, nullptr, 0, 120},
};
static constexpr VideoInitProgram ST77903_RGB_Program(ST77903_RGB_Init, sizeof(ST77903_RGB_Init) / sizeof(InitCommand));
} } // namespace nice_gfx::video
