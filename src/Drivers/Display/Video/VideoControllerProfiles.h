#pragma once
#include "ST7701_RGB.h"
#include "ST7701_MIPI.h"
#include "ILI9881C_MIPI.h"
#include "EK79007_MIPI.h"
#include "ST77922_RGB.h"
#include "ST77922_MIPI.h"
#include "JD9365_MIPI.h"
#include "GC9503_RGB.h"
#include "ST7123_MIPI.h"
#include "ST7703_MIPI.h"
#include "JD9165_MIPI.h"
#include "ST7121_MIPI.h"
#include "ST77903_RGB.h"
#include "HX8399_MIPI.h"
#include "NV3052_RGB.h"
#include "ER88577_MIPI.h"
#include "HX8394_MIPI.h"
#include "ST7102_MIPI.h"
#include "ST7102_RGB.h"
#include "OTA7290B_MIPI.h"
#include "AXS15260D_MIPI.h"
#include "JD9366_MIPI.h"
#include "HI8561_MIPI.h"
#include "SD5207_MIPI.h"
#include "CO5300_MIPI.h"
#include "ST77916_MIPI.h"
#include "ST7796_MIPI.h"
#include "AXS15231B_MIPI.h"
#include "EK9716B_RGB.h"
#include "ST7262_RGB.h"
namespace nice_gfx {
inline const VideoControllerProfile *videoControllerProfile(VideoController controller, VideoInterface interface) {
  if (controller == VideoController::ST7701 && interface == VideoInterface::RGB) {
    static const VideoControllerProfile profile = {"ST7701_RGB", true, true, false, 0, 0, ((uint32_t(1) << 16) | (uint32_t(1) << 18) | (uint32_t(1) << 24)), (uint32_t(1) << 16 | uint32_t(1) << 18 | uint32_t(1) << 24), 0, 10, 120, 120, false, false, true, video::ST7701_RGB_Program};
    return &profile;
  }
  if (controller == VideoController::ST7701 && interface == VideoInterface::MIPI) {
    static const VideoControllerProfile profile = {"ST7701_MIPI", true, true, false, 4, 2, ((uint32_t(1) << 16) | (uint32_t(1) << 18) | (uint32_t(1) << 24)), (uint32_t(1) << 16 | uint32_t(1) << 18 | uint32_t(1) << 24), 0, 10, 10, 20, false, false, true, video::ST7701_MIPI_Program};
    return &profile;
  }
  if (controller == VideoController::ILI9881C && interface == VideoInterface::MIPI) {
    static const VideoControllerProfile profile = {"ILI9881C_MIPI", false, true, false, 28, 2, 0, (uint32_t(1) << 16 | uint32_t(1) << 18 | uint32_t(1) << 24), 0, 10, 10, 20, false, false, true, video::ILI9881C_MIPI_Program};
    return &profile;
  }
  if (controller == VideoController::EK79007 && interface == VideoInterface::MIPI) {
    static const VideoControllerProfile profile = {"EK79007_MIPI", false, true, false, 20, 2, 0, (uint32_t(1) << 16), 0, 10, 20, 20, false, false, false, video::EK79007_MIPI_Program};
    return &profile;
  }
  if (controller == VideoController::ST77922 && interface == VideoInterface::RGB) {
    static const VideoControllerProfile profile = {"ST77922_RGB", true, true, false, 0, 0, ((uint32_t(1) << 16) | (uint32_t(1) << 18) | (uint32_t(1) << 24)), (uint32_t(1) << 16 | uint32_t(1) << 18 | uint32_t(1) << 24), 0, 10, 120, 120, false, false, true, video::ST77922_RGB_Program};
    return &profile;
  }
  if (controller == VideoController::ST77922 && interface == VideoInterface::MIPI) {
    static const VideoControllerProfile profile = {"ST77922_MIPI", true, true, false, 2, 1, ((uint32_t(1) << 16) | (uint32_t(1) << 18) | (uint32_t(1) << 24)), (uint32_t(1) << 16 | uint32_t(1) << 18 | uint32_t(1) << 24), 0, 10, 120, 120, false, false, true, video::ST77922_MIPI_Program};
    return &profile;
  }
  if (controller == VideoController::JD9365 && interface == VideoInterface::MIPI) {
    static const VideoControllerProfile profile = {"JD9365_MIPI", false, true, false, 30, 2, 0, (uint32_t(1) << 16 | uint32_t(1) << 18 | uint32_t(1) << 24), 5, 10, 120, 120, false, false, true, video::JD9365_MIPI_Program};
    return &profile;
  }
  if (controller == VideoController::GC9503 && interface == VideoInterface::RGB) {
    static const VideoControllerProfile profile = {"GC9503_RGB", true, false, false, 0, 0, ((uint32_t(1) << 16) | (uint32_t(1) << 18) | (uint32_t(1) << 24)), 0, 0, 10, 120, 120, false, false, true, video::GC9503_RGB_Program};
    return &profile;
  }
  if (controller == VideoController::ST7123 && interface == VideoInterface::MIPI) {
    static const VideoControllerProfile profile = {"ST7123_MIPI", false, true, false, 4, 2, 0, (uint32_t(1) << 16), 5, 10, 120, 120, false, false, false, video::ST7123_MIPI_Program};
    return &profile;
  }
  if (controller == VideoController::ST7703 && interface == VideoInterface::MIPI) {
    static const VideoControllerProfile profile = {"ST7703_MIPI", false, true, false, 4, 2, 0, (uint32_t(1) << 16 | uint32_t(1) << 18 | uint32_t(1) << 24), 5, 10, 120, 120, true, false, true, video::ST7703_MIPI_Program};
    return &profile;
  }
  if (controller == VideoController::JD9165 && interface == VideoInterface::MIPI) {
    static const VideoControllerProfile profile = {"JD9165_MIPI", false, true, false, 4, 2, 0, (uint32_t(1) << 16), 5, 10, 120, 120, false, false, true, video::JD9165_MIPI_Program};
    return &profile;
  }
  if (controller == VideoController::ST7121 && interface == VideoInterface::MIPI) {
    static const VideoControllerProfile profile = {"ST7121_MIPI", false, true, false, 4, 2, 0, (uint32_t(1) << 16), 5, 10, 120, 120, false, false, false, video::ST7121_MIPI_Program};
    return &profile;
  }
  if (controller == VideoController::ST77903 && interface == VideoInterface::RGB) {
    static const VideoControllerProfile profile = {"ST77903_RGB", true, false, false, 0, 0, ((uint32_t(1) << 16) | (uint32_t(1) << 18) | (uint32_t(1) << 24)), 0, 10, 10, 120, 120, false, false, true, video::ST77903_RGB_Program};
    return &profile;
  }
  if (controller == VideoController::HX8399 && interface == VideoInterface::MIPI) {
    static const VideoControllerProfile profile = {"HX8399_MIPI", false, true, false, 30, 2, 0, (uint32_t(1) << 16 | uint32_t(1) << 18 | uint32_t(1) << 24), 0, 10, 10, 120, false, false, true, video::HX8399_MIPI_Program};
    return &profile;
  }
  if (controller == VideoController::NV3052 && interface == VideoInterface::RGB) {
    static const VideoControllerProfile profile = {"NV3052_RGB", true, false, false, 0, 0, ((uint32_t(1) << 16) | (uint32_t(1) << 18) | (uint32_t(1) << 24)), 0, 10, 10, 120, 120, false, false, true, video::NV3052_RGB_Program};
    return &profile;
  }
  if (controller == VideoController::ER88577 && interface == VideoInterface::MIPI) {
    static const VideoControllerProfile profile = {"ER88577_MIPI", false, true, false, 4, 2, 0, (uint32_t(1) << 16 | uint32_t(1) << 18 | uint32_t(1) << 24), 0, 20, 120, 120, false, false, true, video::ER88577_MIPI_Program};
    return &profile;
  }
  if (controller == VideoController::HX8394 && interface == VideoInterface::MIPI) {
    static const VideoControllerProfile profile = {"HX8394_MIPI", false, true, false, 30, 2, 0, (uint32_t(1) << 16 | uint32_t(1) << 18 | uint32_t(1) << 24), 0, 10, 10, 120, false, false, true, video::HX8394_MIPI_Program};
    return &profile;
  }
  if (controller == VideoController::ST7102 && interface == VideoInterface::MIPI) {
    static const VideoControllerProfile profile = {"ST7102_MIPI", true, true, false, 4, 2, ((uint32_t(1) << 16) | (uint32_t(1) << 18) | (uint32_t(1) << 24)), (uint32_t(1) << 16 | uint32_t(1) << 18 | uint32_t(1) << 24), 0, 10, 120, 120, false, false, true, video::ST7102_MIPI_Program};
    return &profile;
  }
  if (controller == VideoController::ST7102 && interface == VideoInterface::RGB) {
    static const VideoControllerProfile profile = {"ST7102_RGB", true, true, false, 0, 0, ((uint32_t(1) << 16) | (uint32_t(1) << 18) | (uint32_t(1) << 24)), (uint32_t(1) << 16 | uint32_t(1) << 18 | uint32_t(1) << 24), 0, 10, 120, 120, false, false, true, video::ST7102_RGB_Program};
    return &profile;
  }
  if (controller == VideoController::OTA7290B && interface == VideoInterface::MIPI) {
    static const VideoControllerProfile profile = {"OTA7290B_MIPI", false, true, false, 4, 2, 0, (uint32_t(1) << 16), 5, 10, 120, 120, false, false, true, video::OTA7290B_MIPI_Program};
    return &profile;
  }
  if (controller == VideoController::AXS15260D && interface == VideoInterface::MIPI) {
    static const VideoControllerProfile profile = {"AXS15260D_MIPI", false, true, false, 4, 2, 0, (uint32_t(1) << 16 | uint32_t(1) << 18 | uint32_t(1) << 24), 5, 10, 120, 120, false, false, false, video::AXS15260D_MIPI_Program};
    return &profile;
  }
  if (controller == VideoController::JD9366 && interface == VideoInterface::MIPI) {
    static const VideoControllerProfile profile = {"JD9366_MIPI", false, true, false, 4, 2, 0, (uint32_t(1) << 16), 5, 10, 120, 120, false, false, true, video::JD9366_MIPI_Program};
    return &profile;
  }
  if (controller == VideoController::HI8561 && interface == VideoInterface::MIPI) {
    static const VideoControllerProfile profile = {"HI8561_MIPI", false, true, false, 4, 2, 0, (uint32_t(1) << 16), 5, 10, 120, 120, false, false, true, video::HI8561_MIPI_Program};
    return &profile;
  }
  if (controller == VideoController::SD5207 && interface == VideoInterface::MIPI) {
    static const VideoControllerProfile profile = {"SD5207_MIPI", false, true, false, 4, 2, 0, (uint32_t(1) << 16), 5, 10, 120, 120, false, false, true, video::SD5207_MIPI_Program};
    return &profile;
  }
  if (controller == VideoController::CO5300 && interface == VideoInterface::MIPI) {
    static const VideoControllerProfile profile = {"CO5300_MIPI", false, true, false, 2, 1, 0, (uint32_t(1) << 16 | uint32_t(1) << 18 | uint32_t(1) << 24), 5, 10, 120, 120, false, false, true, video::CO5300_MIPI_Program};
    return &profile;
  }
  if (controller == VideoController::ST77916 && interface == VideoInterface::MIPI) {
    static const VideoControllerProfile profile = {"ST77916_MIPI", false, true, false, 2, 1, 0, (uint32_t(1) << 16 | uint32_t(1) << 24), 5, 10, 120, 120, false, false, true, video::ST77916_MIPI_Program};
    return &profile;
  }
  if (controller == VideoController::ST7796 && interface == VideoInterface::MIPI) {
    static const VideoControllerProfile profile = {"ST7796_MIPI", false, true, false, 2, 1, 0, (uint32_t(1) << 16 | uint32_t(1) << 18 | uint32_t(1) << 24), 0, 10, 120, 120, false, true, true, video::ST7796_MIPI_Program};
    return &profile;
  }
  if (controller == VideoController::AXS15231B && interface == VideoInterface::MIPI) {
    static const VideoControllerProfile profile = {"AXS15231B_MIPI", false, true, false, 2, 1, 0, (uint32_t(1) << 16 | uint32_t(1) << 18 | uint32_t(1) << 24), 130, 130, 220, 120, true, false, true, video::AXS15231B_MIPI_Program};
    return &profile;
  }
  if (controller == VideoController::EK9716B && interface == VideoInterface::RGB) {
    static const VideoControllerProfile profile = {"EK9716B_RGB", true, false, true, 0, 0, uint32_t(1) << 16, 0, 0, 10, 120, 0, false, false, true, video::EK9716B_RGB_Program};
    return &profile;
  }
  if (controller == VideoController::ST7262 && interface == VideoInterface::RGB) {
    static const VideoControllerProfile profile = {"ST7262_RGB", true, false, true, 0, 0, uint32_t(1) << 16, 0, 0, 10, 120, 0, false, false, true, video::ST7262_RGB_Program};
    return &profile;
  }
  return nullptr;
}
} // namespace nice_gfx
