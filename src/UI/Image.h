#pragma once
#include "Compiled.h"
#include "../Core/Pixel.h"
#include "../Core/Status.h"

namespace nice_gfx { namespace htmlui {

enum class ImageFormat : uint8_t { RGB565, RGB332, Mono1, Mono1Masked };

// A bounded, single-frame pixel source. Width/height and stride are independent
// of the component's authored bounds. RGB565 Native means uint16_t color values;
// Wire means high byte first. RGB332 uses one byte per pixel. Mono1 is row-padded
// MSB-left, white=1; Mono1Masked appends an opaque=1 plane after stride*height.
// A zero stride is
// tightly packed; byteLength must include the last visible row, not its padding.
struct ImageView {
  const void *pixels;
  size_t width, height, byteLength;
  ImageFormat format;
  PixelOrder order;
  size_t strideBytes;
  ImageView(const void *p = nullptr, size_t w = 0, size_t h = 0, size_t bytes = 0,
            ImageFormat f = ImageFormat::RGB565, PixelOrder o = PixelOrder::Native,
            size_t stride = 0)
      : pixels(p), width(w), height(h), byteLength(bytes), format(f), order(o), strideBytes(stride) {}
};

using ImageRelease = void (*)(void *context, const void *pixels);

// Immutable, reference-counted pixels. Handles and render snapshots share the
// same source; replacing a component cannot free pixels still in use by a frame.
// No STL types appear in this public header. Empty handles do not own storage.
class Image {
 public:
  Image();
  ~Image();
  Image(const Image &other);
  Image &operator=(const Image &other);
  Image(Image &&other) noexcept;
  Image &operator=(Image &&other) noexcept;
  bool valid() const;
  ImageView view() const; // Borrowed data; keep this handle alive and unchanged.
  size_t storageBytes() const; // Owned pixels only; includes adopted padding.
  // Failure preserves out. Copy packs rows; the caller may immediately reuse its
  // input on success. Dimensions are 1..4096; source/storage is bounded to 8 MiB.
  static Status copy(const ImageView &source, Image &out);
  // Transfers ownership only on success. Pixels must remain immutable; release
  // runs exactly once when the final handle/state/frame/cache releases them.
  // It may run on the task dropping that reference, so it must be nonblocking.
  // Context must outlive those references; release must not throw or reenter View.
  static Status adopt(const ImageView &source, ImageRelease release, void *context, Image &out);
 private:
  struct Impl;
  Impl *impl_;
  explicit Image(Impl *storage);
  void retain();
  void release();
  friend class View;
};

} }
