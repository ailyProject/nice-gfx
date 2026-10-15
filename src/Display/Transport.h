#pragma once
#include "../Core/Status.h"
#include "../Core/Pixel.h"
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include "../Core/Limits.h"

namespace nice_gfx {

struct TransportInfo {
  uint32_t frequency = 0;
  int clock = -1, cs = -1, dc = -1;
  int data[16] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
  int read = -1, write = -1;
  uint8_t lanes = 0;
};

// One call per span/transfer, never a virtual call per pixel. Transport and Panel
// are single-producer objects; serialize access when used from multiple tasks.
class Transport {
 public:
  virtual ~Transport() {}
  virtual Status begin() = 0;
  virtual void end() = 0;
  virtual Status command(uint8_t command, const uint8_t *data, size_t bytes) = 0;
  virtual Status command16(uint16_t, const uint8_t *, size_t) { return Status::Unsupported; }
  virtual Status register16(uint16_t c, uint16_t value, uint8_t commandBits = 8) {
    const uint8_t bytes[] = {uint8_t(value >> 8), uint8_t(value)};
    return commandBits == 16 ? command16(c, bytes, 2) : command(uint8_t(c), bytes, 2);
  }
  virtual Status readWords(uint16_t, uint16_t *, size_t, uint8_t = 1, uint8_t = 8) {
    return Status::Unsupported;
  }
  virtual Status data(const uint8_t *, size_t) { return Status::Unsupported; }
  virtual Status setPixelFormat(PixelFormat format) {
    return format == PixelFormat::RGB565 ? Status::Ok : Status::Unsupported;
  }
  virtual Status read(uint16_t, uint8_t *, size_t, uint8_t dummyBytes = 1, uint8_t commandBits = 8) {
    (void)dummyBytes; (void)commandBits; return Status::Unsupported;
  }
  // Async retains the view until wait() returns or busy() returns false.
  // Even on error, wait() must drain every accepted DMA descriptor.
  virtual Status pixels(uint8_t command, PixelView view, bool async) = 0;
  virtual Status pixels16(uint16_t, PixelView, bool) { return Status::Unsupported; }
  virtual Status dataPixels(PixelView, bool) { return Status::Unsupported; }
  virtual Status fill(uint8_t command, uint16_t color, size_t count) = 0;
  virtual Status fill16(uint16_t, uint16_t, size_t) { return Status::Unsupported; }
  virtual Status dataFill(uint16_t, size_t) { return Status::Unsupported; }
  virtual Status wait() = 0;
  // Commit composed framebuffer writes when the renderer closes a batch.
  virtual Status present() { return wait(); }
  virtual bool busy() const = 0;
  virtual bool asynchronous() const = 0;
  // Can consume retained full-screen/strided surfaces efficiently. Source
  // lifetime follows pixels()/wait(); ordinary transports remain tiled.
  virtual bool supportsFrameBlit() const { return false; }
  // Exact retained-view eligibility, checked before a frame borrows any source.
  // Backends may restrict pixel format, row stride or transaction quantum;
  // a false result selects ordinary tile composition for the entire frame.
  virtual bool canFrameBlit(PixelView view) const { return supportsFrameBlit() && view.valid(); }
  virtual Status setPixelClockHz(uint32_t) { return Status::Unsupported; }
  virtual bool validOutputPin(int pin) const { return pin >= 0; }
  virtual TransportInfo info() const { return TransportInfo(); }
  virtual void output(int pin, bool high) = 0;
  virtual void delayMs(uint32_t ms) = 0;
 protected:
  Transport() {}
 private:
  Transport(const Transport &) = delete;
  Transport &operator=(const Transport &) = delete;
};

} // namespace nice_gfx
