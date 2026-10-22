#pragma once
#include <sdkconfig.h>
#include <stddef.h>
#include <stdint.h>

#if !defined(CONFIG_IDF_TARGET_ESP32S31) || !CONFIG_IDF_TARGET_ESP32S31
#error "Select the ESP-Mosaico V1.2 board in an Arduino-ESP32 4.x core with ESP32-S31 support."
#endif
#if !defined(ARDUINO_ESP_MOSAICO)
#error "Select Espressif ESP-Mosaico V1.2 so its variant performs the peripheral power sequence."
#endif
#if !defined(CONFIG_ESP_VIDEO_ENABLE_DVP_VIDEO_DEVICE) || !CONFIG_ESP_VIDEO_ENABLE_DVP_VIDEO_DEVICE
#error "This Arduino core must include esp_video with CONFIG_ESP_VIDEO_ENABLE_DVP_VIDEO_DEVICE enabled."
#endif

namespace mosaico_camera {
// CoreBoard V1.2 + official CameraBoard in LEFT H2 slot. CameraBoard V1.2
// (OV3640) and V1.4 (SC101IOT) have the same DVP wiring and an onboard oscillator.
// Mainboard touch is Wire/I2C0 on GPIO56/3. Camera SCCB uses I2C1 on GPIO0/1.
constexpr int kSccbPort = 1, kSccbSda = 0, kSccbScl = 1;
constexpr int kCameraReset = 53, kCameraPowerDown = 48;
constexpr int kCameraVsync = 55, kCameraHref = 19, kCameraPclk = 17;
constexpr int kCameraData[8] = {16, 15, 33, 4, 14, 12, 18, 13};
constexpr int kCameraFlash = 34; // Active low; keep OFF, open-drain HIGH.
constexpr size_t kPreviewWidth = 320, kPreviewHeight = 240;
constexpr size_t kPreviewPixels = kPreviewWidth * kPreviewHeight;
constexpr int kPreviewX = 80, kPreviewY = 96;
constexpr size_t kCaptureBuffers = 2, kOutputBuffers = 3;
constexpr uint32_t kTargetFps = 10; // Cap processing; actual sensor/display rate varies.
constexpr uint32_t kFrameIntervalMs = 1000 / kTargetFps;
constexpr uint32_t kNoFrameTimeoutMs = 5000;
// Temporary diagnostic: keep LCD/touch idle until pure DQ/Q captures succeed.
constexpr uint32_t kCaptureOnlyFrames = 45;
}
