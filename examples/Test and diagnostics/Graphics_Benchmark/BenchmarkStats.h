#pragma once
#include <algorithm>
#include <stdint.h>
#include <stddef.h>

namespace benchmark {

// Frame intervals include HUD, scheduler and idle time. Work timings do not.
// Keep a bounded rolling latency window rather than allocating per frame.
struct Stats {
  static constexpr size_t Window = 256;
  uint32_t frames = 0, errors = 0;
  uint64_t intervalUs = 0, workUs = 0, drawUs = 0, ioUs = 0, pixels = 0;
  uint32_t maxUs = 0, recent[Window] = {};
  size_t samples = 0, cursor = 0;

  void add(uint32_t interval, uint32_t work, uint32_t draw, uint32_t io, uint64_t pixelCount) {
    ++frames;
    intervalUs += interval;
    workUs += work;
    drawUs += draw;
    ioUs += io;
    pixels += pixelCount;
    maxUs = std::max(maxUs, interval);
    recent[cursor] = interval;
    cursor = (cursor + 1) % Window;
    samples = std::min(samples + 1, Window);
  }
  double fps(uint32_t wallUs) const { return wallUs ? double(frames) * 1000000.0 / wallUs : 0; }
  double meanMs() const { return frames ? double(intervalUs) / frames / 1000.0 : 0; }
  double drawMs() const { return frames ? double(drawUs) / frames / 1000.0 : 0; }
  double ioMs() const { return frames ? double(ioUs) / frames / 1000.0 : 0; }
  uint32_t p95Us() const {
    if (!samples) return 0;
    uint32_t sorted[Window];
    std::copy(recent, recent + samples, sorted);
    std::sort(sorted, sorted + samples);
    // Nearest-rank percentile over the most recent <=256 completed frames.
    return sorted[(samples * 95 + 99) / 100 - 1];
  }
};

} // namespace benchmark
