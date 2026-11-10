#pragma once
#include "Image.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#if defined(ESP_PLATFORM) || defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#elif defined(_WIN32)
#include <malloc.h>
#endif

#define NICE_GFX_UI_IMAGE_FRAME_POOL_VERSION 1

namespace nice_gfx { namespace htmlui {

// Optional RGB565 Native buffers for producers publishing immutable UI images.
// Construction allocates nothing. One owner serializes begin/acquire/publish/
// end; a producer may write only its reserved buffer and must finish before
// publish/releaseFailed. The final Image release can run on another task.
//
// The pool MUST outlive every reserved producer, Image, View and render/cache
// snapshot using it. Stop/join producers, clear image overrides, drain View
// work and release other Image handles before end(). A Busy result preserves
// all storage. Destruction while Busy aborts rather than leave dangling leases.
class ImageFramePool {
 public:
  static constexpr uint8_t MaxSlots = 3;
  static constexpr size_t Alignment = 64;
  using Allocate = void *(*)(size_t alignment, size_t bytes);
  using Release = void (*)(void *pixels);

  struct Surface {
    uint16_t *pixels = nullptr;
    uint16_t width = 0, height = 0;
    size_t stride = 0; // Pixels, not bytes.
    bool valid() const {
      return pixels && width && height && width <= 4096 && height <= 4096 &&
        stride >= width && stride <= (8388608u / sizeof(uint16_t)) / height;
    }
    size_t byteLength() const { return valid() ? stride * height * sizeof(uint16_t) : 0; }
    ImageView imageView() const {
      return valid() ? ImageView(pixels, width, height, byteLength(),
        ImageFormat::RGB565, PixelOrder::Native, stride * sizeof(uint16_t)) : ImageView{};
    }
  };

  // Setup-only allocator hooks permit platform ports and deterministic OOM
  // checks. ESP32 defaults require external PSRAM; they do not consume scarce
  // internal RAM when PSRAM allocation fails. Hooks must be a matching pair.
  explicit ImageFramePool(Allocate allocate = allocatePixels, Release release = freePixels)
    : allocate_(allocate), release_(release) {}
  ImageFramePool(const ImageFramePool &) = delete;
  ImageFramePool &operator=(const ImageFramePool &) = delete;
  ImageFramePool(ImageFramePool &&) = delete;
  ImageFramePool &operator=(ImageFramePool &&) = delete;
  ~ImageFramePool() { if (end() == Status::Busy) std::abort(); }

  // Two buffers reduce RAM; three tolerate an additional retained UI frame.
  // Failure rolls back all allocations from this call. Existing ready pools
  // return Busy and retain their original buffers/configuration.
  Status begin(uint16_t width, uint16_t height, uint8_t slots = MaxSlots) {
    if (ready()) return Status::Busy;
    if (!allocate_ || !release_ || slots < 2 || slots > MaxSlots ||
        !width || !height || width > 4096 || height > 4096 ||
        size_t(width) * height > 8388608u / sizeof(uint16_t)) return Status::InvalidArgument;
    const size_t bytes = size_t(width) * height * sizeof(uint16_t);
    for (uint8_t i = 0; i < slots; ++i) {
      auto *pixels = static_cast<uint16_t *>(allocate_(Alignment, bytes));
      if (!pixels) { end(); return Status::NoMemory; }
      if (reinterpret_cast<uintptr_t>(pixels) % Alignment) {
        release_(pixels); end(); return Status::InvalidArgument;
      }
      slots_[i].surface = {pixels, width, height, width};
    }
    next_ = 0;
    count_ = slots;
    return Status::Ok;
  }

  // No allocation, waiting or overwrite. A full pool returns -1 so callers can
  // skip a frame while earlier UI images/snapshots retain their leases.
  int acquire() {
    if (!ready()) return -1;
    for (uint8_t offset = 0; offset < count_; ++offset) {
      const uint8_t index = uint8_t((next_ + offset) % count_);
      uint32_t expected = Free;
      if (slots_[index].state.compare_exchange_strong(expected, Reserved,
            std::memory_order_acquire, std::memory_order_relaxed)) {
        next_ = uint8_t((index + 1) % count_);
        return index;
      }
    }
    return -1;
  }

  // Borrowed immutable descriptor with writable pixels. It remains valid only
  // for this reservation; never keep/write it after publish or releaseFailed.
  // Invalid indices and nonreserved slots return nullptr.
  const Surface *reservedSurface(int index) const {
    return valid(index) && slots_[index].state.load(std::memory_order_acquire) == Reserved
      ? &slots_[index].surface : nullptr;
  }
  bool releaseFailed(int index) {
    if (!valid(index)) return false;
    uint32_t expected = Reserved;
    return slots_[index].state.compare_exchange_strong(expected, Free,
      std::memory_order_release, std::memory_order_relaxed);
  }

  // ViewTarget normally is htmlui::View; templating avoids pulling the complete
  // View renderer into this optional header. The target must accept setImage
  // (Id, const Image&). Image::adopt allocates its small reference-count handle;
  // pixel data is never copied or allocated here. A target error releases an
  // unretained image, or preserves its lease if the target retained a handle.
  // After this call do NOT releaseFailed(), including on publication failure.
  template<class ViewTarget>
  Status publish(ViewTarget &view, Id id, int index) {
    if (!valid(index)) return Status::InvalidArgument;
    auto &slot = slots_[index];
    uint32_t expected = Reserved;
    if (!slot.state.compare_exchange_strong(expected, Leased,
          std::memory_order_acq_rel, std::memory_order_relaxed)) return Status::Busy;
    Image image;
    const Status adopted = Image::adopt(slot.surface.imageView(), released, &slot, image);
    if (adopted != Status::Ok) {
      slot.state.store(Free, std::memory_order_release);
      return adopted;
    }
    return view.setImage(id, image);
  }

  bool ready() const { return count_ != 0; }
  uint8_t slotCount() const { return count_; }
  bool drained() const {
    for (const auto &slot : slots_)
      if (slot.state.load(std::memory_order_acquire) != Free) return false;
    return true;
  }
  unsigned freeSlots() const {
    unsigned result = 0;
    for (uint8_t i = 0; i < count_; ++i)
      if (slots_[i].state.load(std::memory_order_acquire) == Free) ++result;
    return result;
  }
  // Requested pixel payload only; allocator rounding and Image handle/control
  // storage are separate. Default tight stride is width pixels.
  size_t storageBytes() const {
    size_t bytes = 0;
    for (const auto &slot : slots_) bytes += slot.surface.byteLength();
    return bytes;
  }
  Status end() {
    if (!drained()) return Status::Busy;
    for (auto &slot : slots_) {
      if (slot.surface.pixels) release_(slot.surface.pixels);
      slot.surface = {};
    }
    count_ = next_ = 0;
    return Status::Ok;
  }

 private:
  enum : uint32_t { Free = 0, Reserved = 1, Leased = 2 };
  struct Slot {
    Surface surface;
    std::atomic<uint32_t> state{Free};
  };
  static_assert(std::atomic<uint32_t>::is_always_lock_free,
    "ImageFramePool release callbacks require lock-free 32-bit atomics");
  Allocate allocate_;
  Release release_;
  Slot slots_[MaxSlots];
  uint8_t count_ = 0, next_ = 0;

  bool valid(int index) const { return index >= 0 && index < count_; }
  static void released(void *context, const void *) {
    // This must be the final pool access: its owner may end/destroy the pool
    // immediately after observing Free. No allocation/free, View or SDK calls.
    static_cast<Slot *>(context)->state.store(Free, std::memory_order_release);
  }
  static void *allocatePixels(size_t alignment, size_t bytes) {
#if defined(ESP_PLATFORM) || defined(ARDUINO_ARCH_ESP32)
    return heap_caps_aligned_alloc(alignment, bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#elif defined(_WIN32)
    return _aligned_malloc(bytes, alignment);
#else
    return std::aligned_alloc(alignment, (bytes + alignment - 1) / alignment * alignment);
#endif
  }
  static void freePixels(void *pixels) {
#if defined(ESP_PLATFORM) || defined(ARDUINO_ARCH_ESP32)
    heap_caps_free(pixels);
#elif defined(_WIN32)
    _aligned_free(pixels);
#else
    std::free(pixels);
#endif
  }
};

} } // namespace nice_gfx::htmlui
