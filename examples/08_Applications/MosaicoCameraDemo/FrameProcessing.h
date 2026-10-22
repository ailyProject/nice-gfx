#pragma once
#include <stddef.h>
#include <stdint.h>
#include <limits>

// Portable camera-to-UI processing. No Arduino, camera driver or View dependency.
namespace mosaico_camera {

enum class PixelFormat { RGB565LE, RGB565BE, RGB888, BGR888, UYVY, YUYV, Gray8 };
struct SourceFrame {
  const uint8_t *data;
  size_t bytes, width, height, stride;
  PixelFormat format;
};
struct Detection {
  bool valid = false;
  uint16_t x = 0, y = 0, width = 0, height = 0;
  uint32_t samples = 0;
};
constexpr size_t kDetectionCount = 4;
inline const char *detectionName(size_t index) {
  static const char *const names[] = {"RED", "GREEN", "BLUE", "YELLOW"};
  return index < kDetectionCount ? names[index] : "OBJECT";
}
inline uint8_t clampChannel(int value) {
  return uint8_t(value < 0 ? 0 : value > 255 ? 255 : value);
}
inline uint16_t pack565(uint8_t r, uint8_t g, uint8_t b) {
  return uint16_t((uint16_t(r & 248) << 8) | (uint16_t(g & 252) << 3) | (b >> 3));
}
inline uint16_t yuvTo565(uint8_t y, uint8_t u, uint8_t v) {
  // BT.601 limited range, the YUV interpretation used by this example.
  const int c = int(y) - 16, d = int(u) - 128, e = int(v) - 128;
  return pack565(clampChannel((298 * c + 409 * e + 128) / 256),
                 clampChannel((298 * c - 100 * d - 208 * e + 128) / 256),
                 clampChannel((298 * c + 516 * d + 128) / 256));
}
inline size_t pixelBytes(PixelFormat format) {
  switch (format) {
    case PixelFormat::RGB888: case PixelFormat::BGR888: return 3;
    case PixelFormat::Gray8: return 1;
    default: return 2;
  }
}
inline bool validSource(const SourceFrame &source) {
  switch (source.format) {
    case PixelFormat::RGB565LE: case PixelFormat::RGB565BE:
    case PixelFormat::RGB888: case PixelFormat::BGR888:
    case PixelFormat::UYVY: case PixelFormat::YUYV: case PixelFormat::Gray8: break;
    default: return false;
  }
  if (!source.data || !source.width || !source.height ||
      source.width > 8192 || source.height > 8192) return false;
  if ((source.format == PixelFormat::UYVY || source.format == PixelFormat::YUYV) &&
      (source.width & 1)) return false;
  const size_t row = source.width * pixelBytes(source.format);
  if (source.stride < row || source.bytes < row) return false;
  if (source.height > 1 && source.stride >
      ((std::numeric_limits<size_t>::max)() - row) / (source.height - 1)) return false;
  return source.bytes >= (source.height - 1) * source.stride + row;
}
inline uint16_t sourcePixel(const SourceFrame &source, size_t x, size_t y) {
  const uint8_t *row = source.data + y * source.stride;
  switch (source.format) {
    case PixelFormat::RGB565LE: return uint16_t(row[2*x] | (uint16_t(row[2*x+1]) << 8));
    case PixelFormat::RGB565BE: return uint16_t((uint16_t(row[2*x]) << 8) | row[2*x+1]);
    case PixelFormat::RGB888: return pack565(row[3*x], row[3*x+1], row[3*x+2]);
    case PixelFormat::BGR888: return pack565(row[3*x+2], row[3*x+1], row[3*x]);
    case PixelFormat::Gray8: return pack565(row[x], row[x], row[x]);
    case PixelFormat::UYVY: {
      const uint8_t *pair = row + (x & ~size_t(1)) * 2;
      return yuvTo565(pair[(x & 1) ? 3 : 1], pair[0], pair[2]);
    }
    case PixelFormat::YUYV: {
      const uint8_t *pair = row + (x & ~size_t(1)) * 2;
      return yuvTo565(pair[(x & 1) ? 2 : 0], pair[1], pair[3]);
    }
  }
  return 0;
}

// Nearest-neighbor, aspect-preserving fit with black letterboxing. Destination
// pixels are native uint16_t color values, independent of the source byte order.
inline bool convertFrame(const SourceFrame &source, uint16_t *destination,
                         size_t width, size_t height, size_t destinationPixels) {
  if (!validSource(source) || !destination || !width || !height ||
      width > 1024 || height > 1024 || destinationPixels < width * height) return false;
  for (size_t i = 0; i < width * height; ++i) destination[i] = 0;
  size_t drawWidth = width, drawHeight = height;
  if (source.width * height > source.height * width)
    drawHeight = source.height * width / source.width;
  else
    drawWidth = source.width * height / source.height;
  if (!drawWidth) drawWidth = 1;
  if (!drawHeight) drawHeight = 1;
  const size_t left = (width - drawWidth) / 2, top = (height - drawHeight) / 2;
  for (size_t y = 0; y < drawHeight; ++y) {
    const size_t sy = y * source.height / drawHeight;
    for (size_t x = 0; x < drawWidth; ++x)
      destination[(top + y) * width + left + x] =
          sourcePixel(source, x * source.width / drawWidth, sy);
  }
  return true;
}

inline int colorClass(uint16_t pixel) {
  const int r = ((pixel >> 11) & 31) * 255 / 31;
  const int g = ((pixel >> 5) & 63) * 255 / 63;
  const int b = (pixel & 31) * 255 / 31;
  if (r >= 140 && g >= 120 && b * 2 < r && b * 2 < g &&
      r * 10 < g * 16 && g * 10 < r * 16) return 3;
  if (r >= 100 && r * 10 > g * 15 && r * 10 > b * 15) return 0;
  if (g >= 90 && g * 10 > r * 14 && g * 10 > b * 14) return 1;
  if (b >= 100 && b * 10 > r * 14 && b * 10 > g * 13) return 2;
  return -1;
}

// Actual color-threshold recognition, not model inference. One bounding box per
// color: separate objects of the same color are deliberately merged. 'samples'
// is a count of matching samples, never an AI confidence score.
inline void detectColors(const uint16_t *pixels, size_t width, size_t height,
                         Detection (&output)[kDetectionCount], size_t step = 4,
                         uint32_t minimumSamples = 24) {
  for (auto &detection : output) detection = Detection{};
  if (!pixels || !width || !height || width > 1024 || height > 1024 || !step) return;
  size_t minX[kDetectionCount], minY[kDetectionCount], maxX[kDetectionCount] = {}, maxY[kDetectionCount] = {};
  for (size_t i = 0; i < kDetectionCount; ++i) { minX[i] = width; minY[i] = height; }
  for (size_t y = 0; y < height; y += step) for (size_t x = 0; x < width; x += step) {
    const int kind = colorClass(pixels[y * width + x]);
    if (kind < 0) continue;
    ++output[kind].samples;
    if (x < minX[kind]) minX[kind] = x;
    if (y < minY[kind]) minY[kind] = y;
    if (x > maxX[kind]) maxX[kind] = x;
    if (y > maxY[kind]) maxY[kind] = y;
  }
  for (size_t i = 0; i < kDetectionCount; ++i) {
    auto &detection = output[i];
    if (!detection.samples || detection.samples < minimumSamples) continue;
    const size_t right = maxX[i] + (step < width - maxX[i] ? step : width - maxX[i]);
    const size_t bottom = maxY[i] + (step < height - maxY[i] ? step : height - maxY[i]);
    detection.valid = true;
    detection.x = uint16_t(minX[i]); detection.y = uint16_t(minY[i]);
    detection.width = uint16_t(right - minX[i]); detection.height = uint16_t(bottom - minY[i]);
  }
}

} // namespace mosaico_camera
