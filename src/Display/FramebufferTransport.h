#pragma once
#include "Transport.h"

namespace nice_gfx {

// Adapts the renderer's sequential DCS window writes to rectangle/framebuffer
// backends. A successful drawBitmap() must have finished using its source.
// The scratch buffer belongs to the backend and remains valid until end().
class FramebufferTransport : public Transport {
 public:
  Status command(uint8_t c, const uint8_t *data, size_t bytes) override {
    if (!ready_) return Status::NotInitialized;
    if (bytes && !data) return Status::InvalidArgument;
    if (c == 0x2A || c == 0x2B) {
      if (bytes != 4) return Status::InvalidArgument;
      const size_t first = (size_t(data[0]) << 8) | data[1];
      const size_t last = (size_t(data[2]) << 8) | data[3];
      const size_t limit = c == 0x2A ? width_ : height_;
      if (first > last || last >= limit) return Status::InvalidArgument;
      if (c == 0x2A) { x0_ = first; x1_ = last; haveX_ = true; }
      else { y0_ = first; y1_ = last; haveY_ = true; }
      cursor_ = 0; started_ = false;
      return Status::Ok;
    }
    if (c == 0x2C) return bytes ? Status::InvalidArgument : startWrite();
    return Status::Unsupported;
  }
  Status pixels(uint8_t c, PixelView view, bool) override {
    if (c != 0x2C) return Status::Unsupported;
    Status s = validateView(view);
    if (s == Status::Ok) s = startWrite();
    return s == Status::Ok ? writeView(view) : s;
  }
  Status dataPixels(PixelView view, bool) override {
    Status s = validateView(view);
    if (s != Status::Ok) return s;
    return started_ ? writeView(view) : Status::InvalidArgument;
  }
  Status fill(uint8_t c, uint16_t color, size_t count) override {
    if (c != 0x2C) return Status::Unsupported;
    Status s = startWrite();
    return s == Status::Ok ? writeFill(color, count) : s;
  }
  Status dataFill(uint16_t color, size_t count) override {
    if (!ready_) return Status::NotInitialized;
    return started_ ? writeFill(color, count) : Status::InvalidArgument;
  }
  bool asynchronous() const override { return false; }
 protected:
  Status startRaster(size_t width, size_t height, uint16_t *scratch, size_t capacity) {
    if (!width || !height || width > INT16_MAX || height > INT16_MAX ||
        height > SIZE_MAX / width || !scratch || !capacity) return Status::InvalidArgument;
    width_ = width; height_ = height; scratch_ = scratch; capacity_ = capacity;
    haveX_ = haveY_ = started_ = false; cursor_ = 0; ready_ = true;
    return Status::Ok;
  }
  void stopRaster() {
    ready_ = haveX_ = haveY_ = started_ = false;
    scratch_ = nullptr; capacity_ = cursor_ = 0;
  }
  bool rasterReady() const { return ready_; }
  size_t rasterWidth() const { return width_; }
  size_t rasterHeight() const { return height_; }
  virtual Status drawBitmap(int x0, int y0, int x1, int y1, const uint16_t *pixels) = 0;
  // Backends with a mapped framebuffer can consume a complete strided tile
  // directly. Other backends preserve bounded staging and prefix semantics.
  virtual bool directPixelView() const { return false; }
  virtual Status drawPixelView(int, int, PixelView) { return Status::Unsupported; }
  // Mapped backends can fill a rectangle without staging one scanline at a
  // time. A successful call has completed its source-independent write.
  virtual bool directFill() const { return false; }
  virtual Status drawFillRect(int, int, size_t, size_t, uint16_t) { return Status::Unsupported; }
 private:
  Status validateView(PixelView view) const {
    if (!ready_) return Status::NotInitialized;
    if (!view.valid() || view.height > SIZE_MAX / view.width ||
        (view.order != PixelOrder::Native && view.order != PixelOrder::Wire))
      return Status::InvalidArgument;
    return Status::Ok;
  }
  Status startWrite() {
    if (!ready_) return Status::NotInitialized;
    if (!haveX_ || !haveY_) return Status::InvalidArgument;
    cursor_ = 0; started_ = true;
    return Status::Ok;
  }
  bool fits(size_t count) const {
    const size_t area = (x1_ - x0_ + 1) * (y1_ - y0_ + 1);
    return cursor_ <= area && count <= area - cursor_;
  }
  Status writeView(PixelView view) {
    const size_t total = view.width * view.height;
    if (!fits(total)) return Status::InvalidArgument;
    const size_t targetWidth = x1_ - x0_ + 1;
    if (directPixelView()) {
      if (view.width == targetWidth && cursor_ % targetWidth == 0) {
        const Status s = drawPixelView(int(x0_), int(y0_ + cursor_ / targetWidth), view);
        if (s == Status::Ok) cursor_ += total;
        return s;
      }
      // A packed stream may cross several destination scanlines even when
      // its source shape differs from the current DCS window (e.g. a glyph).
      // Reshape it into at most a leading fragment, full rows and a tail.
      if (view.height == 1 || view.stride == view.width) {
        size_t consumed = 0;
        while (consumed < total) {
          const size_t targetX = cursor_ % targetWidth, remaining = total - consumed;
          size_t width = targetWidth - targetX, rows = 1;
          if (targetX == 0 && remaining >= targetWidth) rows = remaining / targetWidth;
          else if (width > remaining) width = remaining;
          const size_t chunk = width * rows;
          const Status s = drawPixelView(int(x0_ + targetX), int(y0_ + cursor_ / targetWidth),
            PixelView(view.data + consumed * 2, width, rows, width, view.order));
          if (s != Status::Ok) return s;
          cursor_ += chunk; consumed += chunk;
        }
        return Status::Ok;
      }
    }
    size_t consumed = 0;
    while (consumed < total) {
      const size_t sourceX = consumed % view.width;
      size_t count = view.width - sourceX;
      const size_t targetWidth = x1_ - x0_ + 1;
      const size_t targetX = cursor_ % targetWidth;
      if (count > targetWidth - targetX) count = targetWidth - targetX;
      if (count > capacity_) count = capacity_;
      const uint8_t *source = view.data + ((consumed / view.width) * view.stride + sourceX) * 2;
      if (view.order == PixelOrder::Native) memcpy(scratch_, source, count * 2);
      else for (size_t i = 0; i < count; ++i)
        scratch_[i] = uint16_t((uint16_t(source[i * 2]) << 8) | source[i * 2 + 1]);
      const size_t targetY = cursor_ / targetWidth;
      const Status s = drawBitmap(int(x0_ + targetX), int(y0_ + targetY),
                                 int(x0_ + targetX + count), int(y0_ + targetY + 1), scratch_);
      if (s != Status::Ok) return s;
      cursor_ += count; consumed += count;
    }
    return Status::Ok;
  }
  Status writeFill(uint16_t color, size_t count) {
    if (!fits(count)) return Status::InvalidArgument;
    if (directFill()) {
      const size_t targetWidth = x1_ - x0_ + 1;
      while (count) {
        const size_t targetX = cursor_ % targetWidth;
        size_t width = targetWidth - targetX, rows = 1;
        if (targetX == 0 && count >= targetWidth) rows = count / targetWidth;
        else if (width > count) width = count;
        const size_t chunk = width * rows;
        const Status s = drawFillRect(int(x0_ + targetX), int(y0_ + cursor_ / targetWidth),
                                      width, rows, color);
        if (s != Status::Ok) return s;
        cursor_ += chunk; count -= chunk;
      }
      return Status::Ok;
    }
    while (count) {
      const size_t targetWidth = x1_ - x0_ + 1;
      const size_t targetX = cursor_ % targetWidth;
      size_t chunk = targetWidth - targetX;
      if (chunk > capacity_) chunk = capacity_;
      if (chunk > count) chunk = count;
      for (size_t i = 0; i < chunk; ++i) scratch_[i] = color;
      const size_t targetY = cursor_ / targetWidth;
      const Status s = drawBitmap(int(x0_ + targetX), int(y0_ + targetY),
                                 int(x0_ + targetX + chunk), int(y0_ + targetY + 1), scratch_);
      if (s != Status::Ok) return s;
      cursor_ += chunk; count -= chunk;
    }
    return Status::Ok;
  }
  size_t width_ = 0, height_ = 0, x0_ = 0, x1_ = 0, y0_ = 0, y1_ = 0, cursor_ = 0;
  uint16_t *scratch_ = nullptr;
  size_t capacity_ = 0;
  bool ready_ = false, haveX_ = false, haveY_ = false, started_ = false;
};
} // namespace nice_gfx
