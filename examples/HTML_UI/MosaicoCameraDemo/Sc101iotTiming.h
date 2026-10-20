// SPDX-License-Identifier: Apache-2.0
// SC101IOT PCLK bits follow the ESP-Mosaico BSP's sensor timing patch:
// https://github.com/esp-mosaico/esp-mosaico-bsp/blob/master/components/
// mosaico_module_camera/patches/esp-cam-sensor-sc101iot-pclk-delay.patch
#pragma once

#include <Arduino.h>
#include <esp_cam_sensor_types.h>
#include <esp_video_ioctl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdint.h>

namespace mosaico_camera {
namespace sc101iot_timing_detail {

inline bool sensorIoctl(int fd, uint32_t command, void *payload,
                        uint32_t bytes, bool write) {
  struct v4l2_ext_control control = {};
  control.id = command;
  control.size = bytes;
  control.p_u8 = static_cast<uint8_t *>(payload);
  struct v4l2_ext_controls controls = {};
  controls.ctrl_class = V4L2_CTRL_CLASS_ESP_CAM_IOCTL;
  controls.count = 1;
  controls.controls = &control;
  if (ioctl(fd, write ? VIDIOC_S_EXT_CTRLS : VIDIOC_G_EXT_CTRLS, &controls) == 0)
    return true;
  const int savedErrno = errno;
  Serial.printf("SC101IOT timing: sensor ioctl 0x%08lx failed, errno=%d\n",
                static_cast<unsigned long>(command), savedErrno);
  return false;
}

inline bool readReg(int fd, uint8_t address, uint8_t &value) {
  esp_cam_sensor_reg_val_t reg = {};
  reg.regaddr = address;
  if (!sensorIoctl(fd, ESP_CAM_SENSOR_IOC_G_REG, &reg, sizeof(reg), false))
    return false;
  if (reg.value > 0xffu) {
    Serial.printf("SC101IOT timing: reg 0x%02x has invalid value 0x%08lx\n",
                  unsigned(address), static_cast<unsigned long>(reg.value));
    return false;
  }
  value = static_cast<uint8_t>(reg.value);
  return true;
}

inline bool writeReg(int fd, uint8_t address, uint8_t value) {
  esp_cam_sensor_reg_val_t reg = {};
  reg.regaddr = address;
  reg.value = value;
  return sensorIoctl(fd, ESP_CAM_SENSOR_IOC_S_REG, &reg, sizeof(reg), true);
}
}

// Invoke on the capture/control task, after the final sensor-format selection
// and before STREAMON. No concurrent sensor/SCCB operations may run during this
// page-based sequence. Unknown sensors are left untouched and return true.
inline bool applySc101iotTiming(int fd) {
  using namespace sc101iot_timing_detail;
  esp_cam_sensor_id_t identity = {};
  if (!sensorIoctl(fd, ESP_CAM_SENSOR_IOC_G_CHIP_ID,
                   &identity, sizeof(identity), false)) return false;
  const bool isSc101iot = identity.pid == 0xda4au;
  Serial.printf("Camera timing: PID=0x%04x, %s\n", unsigned(identity.pid),
                isSc101iot ? "SC101IOT" : "other sensor; SC101IOT tuning skipped");
  if (!isSc101iot) return true;

  // The SC101IOT driver's G_CHIP_ID selects page 0x31 while reading identity.
  // Preserve the current page after that identity query; never issue raw
  // register accesses to an unidentified sensor (including OV3640).
  uint8_t originalPage = 0;
  if (!readReg(fd, 0xf0u, originalPage)) return false;

  uint8_t oldValue = 0, newValue = 0, readback = 0;
  bool oldValid = false, readbackValid = false;
  bool okay = writeReg(fd, 0xf0u, 0x36u);
  if (okay) {
    okay = readReg(fd, 0x40u, oldValue);
    oldValid = okay;
  }
  if (okay) {
    // PCLK delay occupies ONLY bits [1:0], in steps of 2 ns. Keep bits [7:2].
    newValue = static_cast<uint8_t>(oldValue & 0xfcu);
    if (newValue != oldValue) okay = writeReg(fd, 0x40u, newValue);
  }
  if (okay) {
    okay = readReg(fd, 0x40u, readback);
    readbackValid = okay;
    if (okay && readback != newValue) {
      Serial.println("SC101IOT timing: PCLK register readback mismatch");
      okay = false;
    }
  }
  if (oldValid) {
    Serial.printf("SC101IOT timing: page36/reg40 old=0x%02x (%u ns), target=0x%02x (0 ns), readback=%s0x%02x\n",
                  unsigned(oldValue), unsigned(oldValue & 0x03u) * 2u,
                  unsigned(newValue), readbackValid ? "" : "unavailable/",
                  unsigned(readback));
  }

  // Always try to restore, even when page selection, reading, writing or the
  // PCLK verification failed. Do not short-circuit this cleanup with 'okay'.
  bool restored = writeReg(fd, 0xf0u, originalPage);
  if (restored) {
    uint8_t restoredPage = 0;
    restored = readReg(fd, 0xf0u, restoredPage) && restoredPage == originalPage;
  }
  if (!restored)
    Serial.printf("SC101IOT timing: failed to restore page 0x%02x\n", unsigned(originalPage));
  return okay && restored;
}
}
