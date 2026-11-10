// JC1060P470C_I_W_Y V3 JD9165 MIPI initialization using NICE_GFX VideoInitCommand.
// Register and timing reference: JD9165 datasheet; panel tuning: JC1060P470C_I_W_Y V3 specifications.
#pragma once
#include "../../../Display/VideoControllerTypes.h"
namespace nice_gfx { namespace video {
static const uint8_t JD9165_JC1060_data_0[] = {0x00};
static const uint8_t JD9165_JC1060_data_1[] = {0x49, 0x61, 0x02, 0x00};
static const uint8_t JD9165_JC1060_data_2[] = {0x01};
static const uint8_t JD9165_JC1060_data_3[] = {0x0C};
static const uint8_t JD9165_JC1060_data_4[] = {0x08};
static const uint8_t JD9165_JC1060_data_5[] = {0x11};
static const uint8_t JD9165_JC1060_data_6[] = {0x04};
static const uint8_t JD9165_JC1060_data_7[] = {0x05};
static const uint8_t JD9165_JC1060_data_8[] = {0x38};
static const uint8_t JD9165_JC1060_data_9[] = {0x18};
static const uint8_t JD9165_JC1060_data_10[] = {0x29};
static const uint8_t JD9165_JC1060_data_11[] = {0x01};
static const uint8_t JD9165_JC1060_data_12[] = {0x29};
static const uint8_t JD9165_JC1060_data_13[] = {0x01};
static const uint8_t JD9165_JC1060_data_14[] = {0x02};
static const uint8_t JD9165_JC1060_data_15[] = {0x05};
static const uint8_t JD9165_JC1060_data_16[] = {0x22};
static const uint8_t JD9165_JC1060_data_17[] = {0x08};
static const uint8_t JD9165_JC1060_data_18[] = {0x12};
static const uint8_t JD9165_JC1060_data_19[] = {0x16};
static const uint8_t JD9165_JC1060_data_20[] = {0x64};
static const uint8_t JD9165_JC1060_data_21[] = {0x00};
static const uint8_t JD9165_JC1060_data_22[] = {0x00};
static const uint8_t JD9165_JC1060_data_23[] = {0x78};
static const uint8_t JD9165_JC1060_data_24[] = {0x00};
static const uint8_t JD9165_JC1060_data_25[] = {0x04};
static const uint8_t JD9165_JC1060_data_26[] = {0x16, 0x17, 0x0B, 0x0D, 0x0D, 0x0D, 0x11, 0x10, 0x07, 0x07, 0x09};
static const uint8_t JD9165_JC1060_data_27[] = {0x09, 0x1E, 0x1E, 0x1C, 0x1C, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D};
static const uint8_t JD9165_JC1060_data_28[] = {0x0A, 0x05, 0x0B, 0x0D, 0x0D, 0x0D, 0x11, 0x10, 0x06, 0x06, 0x08};
static const uint8_t JD9165_JC1060_data_29[] = {0x08, 0x1F, 0x1F, 0x1D, 0x1D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D};
static const uint8_t JD9165_JC1060_data_30[] = {0x0A, 0x05, 0x0D, 0x0B, 0x0D, 0x0D, 0x11, 0x10, 0x1D, 0x1D, 0x1F};
static const uint8_t JD9165_JC1060_data_31[] = {0x1F, 0x08, 0x08, 0x06, 0x06, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D};
static const uint8_t JD9165_JC1060_data_32[] = {0x16, 0x17, 0x0D, 0x0B, 0x0D, 0x0D, 0x11, 0x10, 0x1C, 0x1C, 0x1E};
static const uint8_t JD9165_JC1060_data_33[] = {0x1E, 0x09, 0x09, 0x07, 0x07, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D};
static const uint8_t JD9165_JC1060_data_34[] = {0x00, 0x00, 0x00, 0x00};
static const uint8_t JD9165_JC1060_data_35[] = {0x00, 0x00, 0x41, 0x41};
static const uint8_t JD9165_JC1060_data_36[] = {0x00, 0x00, 0x00, 0x00};
static const uint8_t JD9165_JC1060_data_37[] = {0x00};
static const uint8_t JD9165_JC1060_data_38[] = {0x85};
static const uint8_t JD9165_JC1060_data_39[] = {0x06, 0x09};
static const uint8_t JD9165_JC1060_data_40[] = {0x05, 0x08};
static const uint8_t JD9165_JC1060_data_41[] = {0x0A, 0x04};
static const uint8_t JD9165_JC1060_data_42[] = {0x00};
static const uint8_t JD9165_JC1060_data_43[] = {0x00};
static const uint8_t JD9165_JC1060_data_44[] = {0x06};
static const uint8_t JD9165_JC1060_data_45[] = {0x3F, 0x25, 0x27, 0x35, 0x1D, 0x1B, 0x1B, 0x1A, 0x18, 0x0A, 0x2A, 0x21, 0x19, 0x30};
static const uint8_t JD9165_JC1060_data_46[] = {0x3F, 0x26, 0x27, 0x35, 0x1E, 0x1C, 0x1C, 0x1A, 0x18, 0x0B, 0x2A, 0x21, 0x19, 0x30};
static const uint8_t JD9165_JC1060_data_47[] = {0x0A};
static const uint8_t JD9165_JC1060_data_48[] = {0x4F};
static const uint8_t JD9165_JC1060_data_49[] = {0x40};
static const uint8_t JD9165_JC1060_data_50[] = {0x0D};
static const uint8_t JD9165_JC1060_data_51[] = {0x04};
static const uint8_t JD9165_JC1060_data_52[] = {0x05};
static const uint8_t JD9165_JC1060_data_53[] = {0x0C};
static const uint8_t JD9165_JC1060_data_54[] = {0x05};
static const uint8_t JD9165_JC1060_data_55[] = {0x0C};
static const uint8_t JD9165_JC1060_data_56[] = {0x00};
static const uint8_t JD9165_JC1060_data_57[] = {0x55};
static const uint8_t JD9165_JC1060_data_58[] = {0x00};
static const uint8_t JD9165_JC1060_data_59[] = {0x00};
static const VideoInitCommand JD9165_JC1060_Init[] = {
  {0x30, JD9165_JC1060_data_0, 1, 0},
  {0xF7, JD9165_JC1060_data_1, 4, 0},
  {0x30, JD9165_JC1060_data_2, 1, 0},
  {0x04, JD9165_JC1060_data_3, 1, 0},
  {0x05, JD9165_JC1060_data_4, 1, 0},
  {0x0B, JD9165_JC1060_data_5, 1, 0},
  {0x20, JD9165_JC1060_data_6, 1, 0},
  {0x1F, JD9165_JC1060_data_7, 1, 0},
  {0x23, JD9165_JC1060_data_8, 1, 0},
  {0x28, JD9165_JC1060_data_9, 1, 0},
  {0x29, JD9165_JC1060_data_10, 1, 0},
  {0x2A, JD9165_JC1060_data_11, 1, 0},
  {0x2B, JD9165_JC1060_data_12, 1, 0},
  {0x2C, JD9165_JC1060_data_13, 1, 0},
  {0x30, JD9165_JC1060_data_14, 1, 0},
  {0x00, JD9165_JC1060_data_15, 1, 0},
  {0x01, JD9165_JC1060_data_16, 1, 0},
  {0x02, JD9165_JC1060_data_17, 1, 0},
  {0x03, JD9165_JC1060_data_18, 1, 0},
  {0x04, JD9165_JC1060_data_19, 1, 0},
  {0x05, JD9165_JC1060_data_20, 1, 0},
  {0x06, JD9165_JC1060_data_21, 1, 0},
  {0x07, JD9165_JC1060_data_22, 1, 0},
  {0x08, JD9165_JC1060_data_23, 1, 0},
  {0x09, JD9165_JC1060_data_24, 1, 0},
  {0x0A, JD9165_JC1060_data_25, 1, 0},
  {0x0B, JD9165_JC1060_data_26, 11, 0},
  {0x0C, JD9165_JC1060_data_27, 11, 0},
  {0x0D, JD9165_JC1060_data_28, 11, 0},
  {0x0E, JD9165_JC1060_data_29, 11, 0},
  {0x0F, JD9165_JC1060_data_30, 11, 0},
  {0x10, JD9165_JC1060_data_31, 11, 0},
  {0x11, JD9165_JC1060_data_32, 11, 0},
  {0x12, JD9165_JC1060_data_33, 11, 0},
  {0x13, JD9165_JC1060_data_34, 4, 0},
  {0x14, JD9165_JC1060_data_35, 4, 0},
  {0x15, JD9165_JC1060_data_36, 4, 0},
  {0x17, JD9165_JC1060_data_37, 1, 0},
  {0x18, JD9165_JC1060_data_38, 1, 0},
  {0x19, JD9165_JC1060_data_39, 2, 0},
  {0x1A, JD9165_JC1060_data_40, 2, 0},
  {0x1B, JD9165_JC1060_data_41, 2, 0},
  {0x26, JD9165_JC1060_data_42, 1, 0},
  {0x27, JD9165_JC1060_data_43, 1, 0},
  {0x30, JD9165_JC1060_data_44, 1, 0},
  {0x12, JD9165_JC1060_data_45, 14, 0},
  {0x13, JD9165_JC1060_data_46, 14, 0},
  {0x30, JD9165_JC1060_data_47, 1, 0},
  {0x02, JD9165_JC1060_data_48, 1, 0},
  {0x0B, JD9165_JC1060_data_49, 1, 0},
  {0x30, JD9165_JC1060_data_50, 1, 0},
  {0x0D, JD9165_JC1060_data_51, 1, 0},
  {0x10, JD9165_JC1060_data_52, 1, 0},
  {0x11, JD9165_JC1060_data_53, 1, 0},
  {0x12, JD9165_JC1060_data_54, 1, 0},
  {0x13, JD9165_JC1060_data_55, 1, 0},
  {0x30, JD9165_JC1060_data_56, 1, 0},
  {0x3A, JD9165_JC1060_data_57, 1, 0},
  {0x11, JD9165_JC1060_data_58, 1, 120},
  {0x29, JD9165_JC1060_data_59, 1, 20}
};
} } // namespace nice_gfx::video
