#pragma once
#include "CameraConfig.h"
#include "FrameProcessing.h"

namespace mosaico_camera {
struct Frame {
  uint16_t pixels[kPreviewPixels];
  Detection detections[kDetectionCount];
  uint32_t sequence = 0, capturedAt = 0, processingUs = 0;
  uint16_t sourceWidth = 0, sourceHeight = 0;
  bool detectionEnabled = false;
};
enum class CaptureState { Stopped, Starting, Streaming, Failed };
struct CaptureStats {
  uint32_t dequeued, queued, timeouts, queueFailures, shortFrames;
  uint32_t index, bytes, driverSequence, flags, holdUs, maxHoldUs;
};
CaptureStats captureStats();

// This module owns the capture task, V4L2 buffers and a bounded PSRAM frame pool.
// Only trivial Frame* values cross FreeRTOS queues; never memcpy an Image handle.
bool beginCamera();
void enableDetection(bool enabled);
CaptureState cameraState();
const char *cameraError(); // Stable after Failed; detailed errors also go to Serial.
bool takeLatestFrame(Frame *&frame); // Transfers ownership to the UI task.
void returnFrame(Frame *frame);      // Nonblocking; safe as an Image release callback.
}
