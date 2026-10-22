#include "CameraCapture.h"
#include "Sc101iotTiming.h"
#include <Arduino.h>
// Discover the Arduino core's ESP_Video dependency. Use its native esp_video
// API here because this camera board has an external XCLK oscillator.
#include <ESP_Video.h>
#include <esp_video_init.h>
#include <esp_video_device.h>
#include <esp_video_ioctl.h>
#include <esp_heap_caps.h>
#include <driver/gpio.h>
#include <soc/lcd_cam_struct.h>
#include <soc/axi_dma_struct.h>
#include <linux/videodev2.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <new>

namespace mosaico_camera {
namespace {
QueueHandle_t freeFrames = nullptr, readyFrames = nullptr;
Frame *storage[kOutputBuffers] = {};
std::atomic<CaptureState> state{CaptureState::Stopped};
std::atomic<bool> detectionEnabled{true};
std::atomic<uint32_t> dqCount{0}, qCount{0}, timeoutCount{0}, qFailures{0};
std::atomic<uint32_t> shortCount{0};
std::atomic<uint32_t> lastIndex{0}, lastBytes{0}, driverSequence{0}, lastFlags{0}, holdUs{0}, maxHoldUs{0};
volatile uint32_t vsyncCount = 0;
void IRAM_ATTR countVsync(void *) { vsyncCount = vsyncCount + 1; }
void dumpCaptureHw() {
  Serial.printf("CAM vsync=%lu ctrl=%08lx ctrl1=%08lx start=%u raw=%08lx ena=%08lx\n",
                (unsigned long)vsyncCount, (unsigned long)LCD_CAM.cam_ctrl.val,
                (unsigned long)LCD_CAM.cam_ctrl1.val, unsigned(LCD_CAM.cam_ctrl1.cam_start),
                (unsigned long)LCD_CAM.lc_dma_int_raw.val, (unsigned long)LCD_CAM.lc_dma_int_ena.val);
  for (unsigned i = 0; i < 3; ++i) {
    const auto &d = AXI_DMA.in[i];
    Serial.printf("AXI RX%u peri=%lu raw=%08lx ena=%08lx state=%08lx link1=%08lx dscr=%08lx eof=%08lx fifo=%08lx\n",
                  i, (unsigned long)d.conf.in_peri_sel.peri_in_sel_chn,
                  (unsigned long)d.intr.raw.val, (unsigned long)d.intr.ena.val,
                  (unsigned long)d.conf.in_state.val, (unsigned long)d.conf.in_link1.val,
                  (unsigned long)d.conf.in_dscr.val, (unsigned long)d.conf.in_suc_eof_des_addr.val,
                  (unsigned long)d.conf.infifo_status.val);
  }
}
char errorMessage[64] = {};

void fail(const char *message, int code = 0) {
  std::snprintf(errorMessage, sizeof(errorMessage), "%s", message);
  Serial.printf("Camera: %s (code %d)\n", message, code);
  state.store(CaptureState::Failed, std::memory_order_release);
}
bool inputFormat(uint32_t fourcc, PixelFormat &format) {
  switch (fourcc) {
    case V4L2_PIX_FMT_RGB565: format = PixelFormat::RGB565LE; return true;
    case V4L2_PIX_FMT_RGB565X: format = PixelFormat::RGB565BE; return true;
    case V4L2_PIX_FMT_RGB24: format = PixelFormat::RGB888; return true;
    case V4L2_PIX_FMT_BGR24: format = PixelFormat::BGR888; return true;
    case V4L2_PIX_FMT_UYVY: format = PixelFormat::UYVY; return true;
    case V4L2_PIX_FMT_YUYV: format = PixelFormat::YUYV; return true;
    case V4L2_PIX_FMT_GREY: format = PixelFormat::Gray8; return true;
    default: return false;
  }
}

class CaptureDevice {
 public:
  ~CaptureDevice() {
    if (vsyncIsr) {
      gpio_intr_disable(static_cast<gpio_num_t>(kCameraVsync));
      gpio_isr_handler_remove(static_cast<gpio_num_t>(kCameraVsync));
    }
    if (fd >= 0) {
      if (streaming) {
        int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(fd, VIDIOC_STREAMOFF, &type);
      }
      for (size_t i = 0; i < count; ++i)
        if (buffers[i].data) munmap(buffers[i].data, buffers[i].bytes);
      struct v4l2_requestbuffers request = {};
      request.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      request.memory = V4L2_MEMORY_MMAP;
      ioctl(fd, VIDIOC_REQBUFS, &request);
      close(fd);
    }
    if (initialized) esp_video_deinit_with_flags(ESP_VIDEO_INIT_FLAGS_DVP);
  }
  bool begin() {
    esp_video_init_dvp_config_t dvp = {};
    dvp.sccb_config.init_sccb = true;
    dvp.sccb_config.i2c_config.port = kSccbPort;
    dvp.sccb_config.i2c_config.sda_pin = static_cast<gpio_num_t>(kSccbSda);
    dvp.sccb_config.i2c_config.scl_pin = static_cast<gpio_num_t>(kSccbScl);
    dvp.sccb_config.freq = 100000;
    dvp.reset_pin = static_cast<gpio_num_t>(kCameraReset);
    dvp.pwdn_pin = static_cast<gpio_num_t>(kCameraPowerDown);
    dvp.dvp_pin.data_width = CAM_CTLR_DATA_WIDTH_8;
    for (auto &pin : dvp.dvp_pin.data_io) pin = GPIO_NUM_NC;
    for (size_t i = 0; i < 8; ++i) dvp.dvp_pin.data_io[i] = static_cast<gpio_num_t>(kCameraData[i]);
    dvp.dvp_pin.vsync_io = static_cast<gpio_num_t>(kCameraVsync);
    dvp.dvp_pin.de_io = static_cast<gpio_num_t>(kCameraHref);
    dvp.dvp_pin.pclk_io = static_cast<gpio_num_t>(kCameraPclk);
    dvp.dvp_pin.xclk_io = GPIO_NUM_NC;
    dvp.xclk_freq = 0; // CameraBoard's own 20/24 MHz oscillator; no MCU XCLK pin.
    esp_video_init_config_t config = {};
    config.dvp = &dvp;
    const esp_err_t result = esp_video_init_with_flags(&config, ESP_VIDEO_INIT_FLAGS_DVP);
    if (result != ESP_OK) { fail("Sensor init failed; check H2/core", int(result)); return false; }
    initialized = true;
    fd = ::open(ESP_VIDEO_DVP_DEVICE_NAME, O_RDWR);
    if (fd < 0) { fail("Cannot open DVP video device", errno); return false; }

    // Preserve the sensor's compiled default mode. S_FMT sets the output format,
    // not an arbitrary sensor resolution. OV3640/SC101IOT have different modes.
    struct v4l2_format format = {};
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_G_FMT, &format) != 0 ||
        ioctl(fd, VIDIOC_S_FMT, &format) != 0 ||
        ioctl(fd, VIDIOC_G_FMT, &format) != 0) {
      fail("Cannot configure capture format", errno); return false;
    }
    if (!applySc101iotTiming(fd)) {
      fail("Cannot apply camera timing", errno); return false;
    }
    if (!inputFormat(format.fmt.pix.pixelformat, pixelFormat)) {
      const uint32_t code = format.fmt.pix.pixelformat;
      Serial.printf("Unsupported FOURCC: %c%c%c%c. Select an RGB565 or UYVY/YUYV sensor default in the core build.\n",
                    int(code & 255), int((code >> 8) & 255), int((code >> 16) & 255), int(code >> 24));
      fail("Unsupported camera pixel format"); return false;
    }
    width = format.fmt.pix.width; height = format.fmt.pix.height;
    stride = format.fmt.pix.bytesperline;
    if (!stride) stride = width * pixelBytes(pixelFormat); // V4L2 zero means packed.
    if (!width || !height || width > 8192 || height > 8192 ||
        stride < width * pixelBytes(pixelFormat) ||
        ((pixelFormat == PixelFormat::UYVY || pixelFormat == PixelFormat::YUYV) && (width & 1))) {
      fail("Invalid camera dimensions/stride"); return false;
    }
    const uint32_t code = format.fmt.pix.pixelformat;
    Serial.printf("Camera %lux%lu %c%c%c%c stride=%lu, preview=%ux%u\n",
                  static_cast<unsigned long>(width), static_cast<unsigned long>(height),
                  int(code & 255), int((code >> 8) & 255), int((code >> 16) & 255), int(code >> 24),
                  static_cast<unsigned long>(stride), unsigned(kPreviewWidth), unsigned(kPreviewHeight));
#ifdef VIDIOC_S_DQBUF_TIMEOUT
    struct timeval timeout = {};
    timeout.tv_usec = 250000;
    if (ioctl(fd, VIDIOC_S_DQBUF_TIMEOUT, &timeout) != 0)
      Serial.println("Camera dequeue timeout unavailable; capture stays on its own task.");
#endif
    struct v4l2_requestbuffers request = {};
    request.count = kCaptureBuffers;
    request.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    request.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &request) != 0 ||
        request.count == 0 || request.count > kCaptureBuffers) {
      fail("Cannot allocate capture buffers", errno); return false;
    }
    count = request.count;
    for (size_t i = 0; i < count; ++i) {
      struct v4l2_buffer buffer = {};
      buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buffer.memory = V4L2_MEMORY_MMAP;
      buffer.index = i;
      if (ioctl(fd, VIDIOC_QUERYBUF, &buffer) != 0) {
        fail("Cannot query capture buffer", errno); return false;
      }
      void *mapped = mmap(nullptr, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buffer.m.offset);
      if (mapped == MAP_FAILED) { fail("Cannot map capture buffer", errno); return false; }
      buffers[i].data = static_cast<uint8_t *>(mapped);
      buffers[i].bytes = buffer.length;
      if (ioctl(fd, VIDIOC_QBUF, &buffer) != 0) {
        fail("Cannot queue capture buffer", errno); return false;
      }
    }
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) != 0) {
      fail("Cannot start camera stream", errno); return false;
    }
    streaming = true;
    const esp_err_t service = gpio_install_isr_service(0);
    if (service == ESP_OK || service == ESP_ERR_INVALID_STATE) {
      if (gpio_isr_handler_add(static_cast<gpio_num_t>(kCameraVsync), countVsync, nullptr) == ESP_OK) {
        vsyncIsr = true;
        gpio_set_intr_type(static_cast<gpio_num_t>(kCameraVsync), GPIO_INTR_POSEDGE);
        gpio_intr_enable(static_cast<gpio_num_t>(kCameraVsync));
      }
    }
    return true;
  }
  bool dequeue(struct v4l2_buffer &buffer) {
    buffer = {};
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    return ioctl(fd, VIDIOC_DQBUF, &buffer) == 0;
  }
  bool requeue(const struct v4l2_buffer &buffer) {
    struct v4l2_buffer queued = {};
    queued.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    queued.memory = V4L2_MEMORY_MMAP;
    queued.index = buffer.index;
    return ioctl(fd, VIDIOC_QBUF, &queued) == 0;
  }
  bool convert(const struct v4l2_buffer &buffer, Frame &frame) const {
    if (buffer.index >= count || !buffer.bytesused || buffer.bytesused > buffers[buffer.index].bytes) return false;
    const SourceFrame source{buffers[buffer.index].data, buffer.bytesused, width, height, stride, pixelFormat};
    return convertFrame(source, frame.pixels, kPreviewWidth, kPreviewHeight, kPreviewPixels);
  }
  size_t width = 0, height = 0;
 private:
  struct Buffer { uint8_t *data = nullptr; size_t bytes = 0; } buffers[kCaptureBuffers];
  int fd = -1;
  size_t count = 0, stride = 0;
  bool initialized = false, streaming = false, vsyncIsr = false;
  PixelFormat pixelFormat = PixelFormat::UYVY;
};

void captureLoop() {
  CaptureDevice capture;
  if (!capture.begin()) return;
  state.store(CaptureState::Streaming, std::memory_order_release);
  uint32_t sequence = 0, lastFrameAt = millis(), lastGoodFrameAt = millis();
  uint32_t lastProcessedAt = millis() - kFrameIntervalMs;
  for (;;) {
    struct v4l2_buffer buffer = {};
    if (!capture.dequeue(buffer)) {
      timeoutCount.fetch_add(1, std::memory_order_relaxed);
      if (timeoutCount.load() == 1) dumpCaptureHw();
      if (uint32_t(millis() - lastFrameAt) > kNoFrameTimeoutMs) {
        dumpCaptureHw();
        fail("Camera timeout; reset to retry", errno); return;
      }
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }
    const uint32_t now = millis();
    const uint32_t heldAt = micros();
    const uint32_t received = dqCount.fetch_add(1, std::memory_order_relaxed) + 1;
    lastIndex.store(buffer.index); lastBytes.store(buffer.bytesused);
    driverSequence.store(buffer.sequence); lastFlags.store(buffer.flags);
    lastFrameAt = now;
    if (buffer.bytesused) lastGoodFrameAt = now;
    else shortCount.fetch_add(1, std::memory_order_relaxed);
    Frame *frame = nullptr;
    // The patched DVP driver marks incomplete startup frames with bytesused=0.
    // Return those buffers immediately and wait for a complete frame.
    if (buffer.bytesused && received > kCaptureOnlyFrames && uint32_t(now - lastProcessedAt) >= kFrameIntervalMs)
      xQueueReceive(freeFrames, &frame, 0);
    bool converted = true;
    if (frame) {
      const uint32_t started = micros();
      converted = capture.convert(buffer, *frame);
      frame->sequence = ++sequence;
      frame->capturedAt = now;
      frame->sourceWidth = uint16_t(capture.width); frame->sourceHeight = uint16_t(capture.height);
      frame->detectionEnabled = detectionEnabled.load(std::memory_order_relaxed);
      // Replace this recognizer with your model. Its results must describe THESE
      // pixels in preview coordinates, and remain attached to this same Frame.
      if (converted && frame->detectionEnabled)
        detectColors(frame->pixels, kPreviewWidth, kPreviewHeight, frame->detections);
      else for (auto &detection : frame->detections) detection = Detection{};
      frame->processingUs = uint32_t(micros() - started);
      lastProcessedAt = now;
    }
    // Driver mmap memory is always returned before the UI sees an output frame.
    if (!capture.requeue(buffer)) {
      qFailures.fetch_add(1, std::memory_order_relaxed);
      if (frame) returnFrame(frame);
      fail("Cannot return capture buffer", errno); return;
    }
    qCount.fetch_add(1, std::memory_order_relaxed);
    const uint32_t held = uint32_t(micros() - heldAt);
    holdUs.store(held); if (held > maxHoldUs.load()) maxHoldUs.store(held);
    if (!buffer.bytesused && uint32_t(now - lastGoodFrameAt) > kNoFrameTimeoutMs) {
      fail("Camera only delivered short frames"); return;
    }
    if (!converted) {
      returnFrame(frame);
      fail("Invalid or truncated camera frame"); return;
    }
    if (frame) {
      Frame *superseded = nullptr;
      if (xQueueReceive(readyFrames, &superseded, 0) == pdTRUE) returnFrame(superseded);
      if (xQueueSend(readyFrames, &frame, 0) != pdTRUE) returnFrame(frame);
    }
    vTaskDelay(1);
  }
}
void captureTask(void *) {
  captureLoop(); // Unwind CaptureDevice/stop the driver before deleting this task.
  vTaskDelete(nullptr);
}
void destroyUnusedPool() {
  for (auto &frame : storage) {
    if (frame) { frame->~Frame(); heap_caps_free(frame); frame = nullptr; }
  }
  if (freeFrames) { vQueueDelete(freeFrames); freeFrames = nullptr; }
  if (readyFrames) { vQueueDelete(readyFrames); readyFrames = nullptr; }
}
} // namespace

bool beginCamera() {
  if (state.load(std::memory_order_acquire) != CaptureState::Stopped) return false;
  state.store(CaptureState::Starting, std::memory_order_release);
  // Core variant already enabled GPIO60 with its required soft-start. Never
  // power-cycle it here: that would also reset the running LCD and touch device.
  // Preload HIGH before enabling this active-low, open-drain output. Arduino
  // digitalWrite-before-pinMode is ignored by this core and could light it.
  gpio_config_t flash = {};
  flash.pin_bit_mask = uint64_t(1) << kCameraFlash;
  flash.mode = GPIO_MODE_OUTPUT_OD;
  flash.pull_up_en = GPIO_PULLUP_DISABLE;
  flash.pull_down_en = GPIO_PULLDOWN_DISABLE;
  flash.intr_type = GPIO_INTR_DISABLE;
  if (gpio_set_level(static_cast<gpio_num_t>(kCameraFlash), 1) != ESP_OK ||
      gpio_config(&flash) != ESP_OK) {
    fail("Cannot configure camera flash pin"); return false;
  }
  freeFrames = xQueueCreate(kOutputBuffers, sizeof(Frame *));
  readyFrames = xQueueCreate(1, sizeof(Frame *));
  if (!freeFrames || !readyFrames) {
    destroyUnusedPool(); fail("Cannot allocate frame queues"); return false;
  }
  for (size_t i = 0; i < kOutputBuffers; ++i) {
    void *memory = heap_caps_malloc(sizeof(Frame), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!memory) { destroyUnusedPool(); fail("PSRAM required for frame pool"); return false; }
    storage[i] = new(memory) Frame{};
    xQueueSend(freeFrames, &storage[i], 0);
  }
  // UI stays on the Arduino loop task. Capture/dequeue/processing never calls View.
  if (xTaskCreate(captureTask, "mosaico-camera", 8192, nullptr, 1, nullptr) != pdPASS) {
    destroyUnusedPool(); fail("Cannot create capture task"); return false;
  }
  return true;
}
void enableDetection(bool enabled) { detectionEnabled.store(enabled, std::memory_order_relaxed); }
CaptureStats captureStats() {
  return {dqCount.load(), qCount.load(), timeoutCount.load(), qFailures.load(), shortCount.load(),
          lastIndex.load(), lastBytes.load(), driverSequence.load(), lastFlags.load(),
          holdUs.load(), maxHoldUs.load()};
}
CaptureState cameraState() { return state.load(std::memory_order_acquire); }
const char *cameraError() { return cameraState() == CaptureState::Failed ? errorMessage : ""; }
bool takeLatestFrame(Frame *&frame) {
  frame = nullptr;
  return readyFrames && xQueueReceive(readyFrames, &frame, 0) == pdTRUE;
}
void returnFrame(Frame *frame) {
  if (frame && freeFrames) {
    // There are exactly kOutputBuffers owners distributed over these queues,
    // the worker, and immutable UI Images. The free queue always has room.
    const BaseType_t returned = xQueueSend(freeFrames, &frame, 0);
    configASSERT(returned == pdTRUE);
    (void)returned;
  }
}
} // namespace mosaico_camera
