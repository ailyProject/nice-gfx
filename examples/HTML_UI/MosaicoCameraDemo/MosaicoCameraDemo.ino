#include <NICE_GFX.h>
#include "Hardware.h"
#include "ui.h"
#include "TouchAdapter.h"
#include "CameraCapture.h"
#include <cstdio>
#include <esp_system.h>
#include <soc/soc.h>
#include <soc/lp_system_reg.h>

using nice_gfx::Status;
using nice_gfx::htmlui::Event;
using nice_gfx::htmlui::Image;
using nice_gfx::htmlui::ImageView;
using nice_gfx::htmlui::Rect;
using namespace mosaico_camera;

nice_gfx::htmlui::View ui(gfx, kTilePixels);
constexpr auto previewId = ui_config::id("preview");
constexpr auto statusId = ui_config::id("status");
constexpr auto fpsId = ui_config::id("fps");
constexpr auto detectId = ui_config::id("detect");
constexpr nice_gfx::htmlui::Id boxIds[] = {
  ui_config::id("box0"), ui_config::id("box1"), ui_config::id("box2"), ui_config::id("box3")};
constexpr nice_gfx::htmlui::Id labelIds[] = {
  ui_config::id("label0"), ui_config::id("label1"), ui_config::id("label2"), ui_config::id("label3")};
bool uiReady = false, detectionVisible = true;
uint32_t framesInstalled = 0, lastStatsAt = 0, lastReceivedAt = 0;
uint32_t lastSequence = 0;
uint8_t lastDetectionMask = 0;
bool reportedFailure = false;

void hideDetections() {
  for (size_t i = 0; i < kDetectionCount; ++i) {
    ui.setVisible(boxIds[i], false); ui.setVisible(labelIds[i], false);
  }
}
void onDetectionChanged(void *, Event &) {
  detectionVisible = ui.value(detectId) != 0;
  enableDetection(detectionVisible);
  if (!detectionVisible) hideDetections();
}
void releasePixels(void *context, const void *) {
  returnFrame(static_cast<Frame *>(context));
}

bool installFrame(Frame *frame) {
  Image image;
  const Status adopted = Image::adopt(
      ImageView(frame->pixels, kPreviewWidth, kPreviewHeight, sizeof(frame->pixels)),
      releasePixels, frame, image);
  if (adopted != Status::Ok) {
    returnFrame(frame);
    ui.setText(statusId, "Cannot adopt preview buffer");
    return false;
  }
  // One logical update: the image and boxes come from the SAME frame. A render
  // already in flight keeps its own old snapshot until all its tiles finish.
  if (!ui.beginUpdate()) return false;
  const Status installed = ui.setImage(previewId, image);
  if (installed == Status::Ok) {
    lastDetectionMask = 0;
    for (size_t i = 0; i < kDetectionCount; ++i) {
      const Detection &detection = frame->detections[i];
      const bool visible = detectionVisible && frame->detectionEnabled && detection.valid;
      if (visible) {
        lastDetectionMask |= uint8_t(1u << i);
        ui.setBounds(boxIds[i], Rect{kPreviewX + detection.x, kPreviewY + detection.y,
                                    detection.width, detection.height});
        int32_t labelX = kPreviewX + detection.x;
        if (labelX > kPreviewX + int32_t(kPreviewWidth) - 80)
          labelX = kPreviewX + int32_t(kPreviewWidth) - 80;
        const int32_t labelY = kPreviewY + detection.y >= kPreviewY + 16
            ? kPreviewY + detection.y - 16 : kPreviewY + detection.y;
        ui.setBounds(labelIds[i], Rect{labelX, labelY, 80, 16});
        ui.setText(labelIds[i], detectionName(i));
      }
      ui.setVisible(boxIds[i], visible); ui.setVisible(labelIds[i], visible);
    }
    char status[48];
    std::snprintf(status, sizeof(status), "%ux%u > 320x240 | %lu ms",
                  unsigned(frame->sourceWidth), unsigned(frame->sourceHeight),
                  static_cast<unsigned long>(frame->processingUs / 1000));
    ui.setText(statusId, status);
    lastReceivedAt = millis();
    lastSequence = frame->sequence;
    ++framesInstalled;
  } else ui.setText(statusId, "Preview update failed");
  const bool ended = ui.endUpdate();
  // 'image' may go out of scope now. The UI/snapshot owns the pixels until its
  // final reference invokes releasePixels. Never return this Frame manually.
  return installed == Status::Ok && ended;
}

void setup() {
  Serial.begin(115200); // USB-OTG CDC; do not wait indefinitely for a host.
  if (!beginDisplayHardware()) return;
  if (ui_config::load(ui) != Status::Ok ||
      !ui_config::bind_detectChanged(ui, onDetectionChanged)) return;
  uiReady = true;
  // Live frames deliberately use the default tiled renderer without a full-page
  // render cache, keeping the number of retained frame-pool slots bounded.
  detectionVisible = ui.value(detectId) != 0;
  enableDetection(detectionVisible);
  lastStatsAt = lastReceivedAt = millis();
  if (!beginCamera()) ui.setText(statusId, cameraError());
  Serial.println("MosaicoCameraDemo: real camera + color threshold detection.");
}

void loop() {
  if (Serial.available() && Serial.read() == 'B') {
    Serial.println("Entering S31 ROM download mode");
    Serial.flush();
    delay(30);
    REG_SET_BIT(LP_SYSTEM_REG_SYS_CTRL_REG, LP_SYSTEM_REG_FORCE_DOWNLOAD_BOOT);
    esp_restart();
  }
  if (!uiReady) { delay(10); return; }
  Frame *frame = nullptr;
  if (takeLatestFrame(frame)) installFrame(frame);
  const uint32_t now = millis();
  const CaptureStats capture = captureStats();
  if (cameraState() == CaptureState::Failed && !reportedFailure) {
    hideDetections();
    ui.setText(statusId, cameraError());
    reportedFailure = true;
  }
  if (uint32_t(now - lastStatsAt) >= 1000) {
    char fps[24];
    const uint32_t tenths = framesInstalled * 10000 / uint32_t(now - lastStatsAt);
    std::snprintf(fps, sizeof(fps), "%lu.%lu FPS",
                  static_cast<unsigned long>(tenths / 10), static_cast<unsigned long>(tenths % 10));
    ui.setText(fpsId, fps);
    if (cameraState() == CaptureState::Streaming && uint32_t(now - lastReceivedAt) > 2000) {
      hideDetections(); ui.setText(statusId, "Waiting for camera frames...");
    }
    // This counter measures UI image submissions, not LCD scanout/physical FPS.
    Serial.printf("Preview submissions: %s; frame=%lu colors=0x%02x; UI frames=%llu, memory=%lu bytes\n", fps,
                  static_cast<unsigned long>(lastSequence), unsigned(lastDetectionMask),
                  static_cast<unsigned long long>(ui.runtimeStats().frames),
                  static_cast<unsigned long>(ui.memoryStats().totalBytes()));
    Serial.printf("Capture DQ=%lu Q=%lu timeout=%lu Qfail=%lu short=%lu index=%lu bytes=%lu seq=%lu flags=%lx hold=%lu max=%lu us\n",
                  (unsigned long)capture.dequeued, (unsigned long)capture.queued,
                  (unsigned long)capture.timeouts, (unsigned long)capture.queueFailures,
                  (unsigned long)capture.shortFrames,
                  (unsigned long)capture.index, (unsigned long)capture.bytes,
                  (unsigned long)capture.driverSequence, (unsigned long)capture.flags,
                  (unsigned long)capture.holdUs, (unsigned long)capture.maxHoldUs);
    framesInstalled = 0; lastStatsAt = now;
  }
  if (capture.dequeued >= kCaptureOnlyFrames || cameraState() == CaptureState::Failed)
    uiReady = pollUi(ui, now); // Diagnostic warmup keeps LCD/touch idle.
  delay(1);
}
