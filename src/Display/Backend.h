#pragma once
#include "Transport.h"
#include "Capabilities.h"
#include "NativeStaging.h"

namespace nice_gfx {

// Display-region contract, independent of controller protocol and UI. Existing
// Panel implements this contract without changing its wire encoding. Dynamic
// dispatch happens at region/span boundaries, never inside a pixel loop.
class Backend {
 public:
  virtual ~Backend() {}
  virtual Status begin() = 0;
  virtual void end() = 0;
  virtual bool ready() const = 0;
  virtual uint16_t width() const = 0;
  virtual uint16_t height() const = 0;
  virtual uint8_t rotation() const = 0;
  virtual Status setRotation(uint8_t) = 0;
  virtual uint8_t windowAlignmentX() const { return 1; }
  virtual uint8_t windowAlignmentY() const { return 1; }
  virtual uint32_t id() const { return 0; }
  virtual int resetPin() const { return -1; }
  virtual TransportInfo transportInfo() const { return TransportInfo(); }
  virtual DisplayCapabilities capabilities() const {
    DisplayCapabilities result; result.nativeWidth = width(); result.nativeHeight = height(); return result;
  }
  virtual bool supportsFrameBlit() const { return false; }
  virtual bool canFrameBlit(PixelView view) const { return supportsFrameBlit() && view.valid(); }
  virtual Status mapNativeFramebuffer(NativeFramebuffer &frame) {
    frame = NativeFramebuffer(); return Status::Unsupported;
  }
  virtual bool supportsNativeStage()const{return false;}
  virtual NativeStageTransfer copyNativeStage(const NativeFramebuffer &,const DirtyRect &,const NativeStageSource &)noexcept{return {};}
  virtual NativeStageTransfer drainNativeStage()noexcept{return {};}
  virtual NativeStageProviderStats nativeStageProviderStats()const{return {};}
  virtual Status setPixelClockHz(uint32_t) { return Status::Unsupported; }
  // Optional blocking copy into independent Native565 storage. Whole rows only;
  // never maps/publishes the display or changes frame-source provenance.
  virtual Status copySurfaceRows(const NativeFramebuffer &,int32_t,PixelView) { return Status::Unsupported; }
  virtual Status powerSave(bool) { return Status::Unsupported; }
  virtual Status setContrast(uint8_t) { return Status::Unsupported; }
  virtual Status status() const = 0;
  virtual bool asynchronous() const { return false; }
  // busy() concerns source-buffer ownership. Slow physical refresh never
  // borrows the caller's RGB565 tile after pixels()/flush() has copied it.
  virtual bool busy() const { return false; }
  virtual bool refreshBusy() const { return false; }
  virtual Status poll(uint32_t) { return Status::Ok; }
  virtual bool frameOpen() const { return false; }
  // Explicit write-discard contract. The caller must fully replace EVERY pixel
  // in these regions before any read/alpha blend there, then commit or abort.
  // No generic map, read/modify, transition or partial-cover caller may opt in.
  // Unsupported MUST leave the transaction unopened and output untouched.
  virtual Status beginRetainedRepaintFrame(const DirtyRect *,size_t,const RetainedRepaintRequest *,size_t,RetainedRepaintReceipt *) { return Status::Unsupported; }
  virtual Status mapRetainedRepaintFramebuffer(const RetainedRepaintReceipt *,size_t,NativeFramebuffer &) { return Status::Unsupported; }
  virtual size_t retainedRepaintStorageBytes()const{return 0;}
  virtual Status validateRetainedRepaintFrame(const RetainedRepaintReceipt *,size_t) { return Status::Unsupported; }
  virtual Status certifyRetainedRepaintFrame(const RetainedRepaintReceipt *,size_t) { return Status::Unsupported; }
  virtual Status beginRepaintFrame(const DirtyRect *,size_t) { return Status::Unsupported; }
  virtual Status beginFrame(const DirtyRect *, size_t) { return Status::Ok; }
  virtual Status commitFrame() { return finish(); }
  virtual Status abortFrame() { return finish(); }
  // Optional write batching. finish() publishes CPU writes before returning;
  // source buffers are still consumed by the individual drawing operations.
  virtual Status beginWrite() { return Status::Ok; }
  virtual Status cacheFrameSource(const FrameSource &) { return Status::Unsupported; }
  virtual Status blitFrameSource(const DirtyRect &,const FrameSource &,int32_t,int32_t) { return Status::Unsupported; }
  virtual Status blendFrame(const DirtyRect &,const FrameSource &,const FrameSource &,uint8_t,uint16_t=0) { return Status::Unsupported; }
  virtual Status command(uint16_t, const uint8_t * = nullptr, size_t = 0) { return Status::Unsupported; }
  virtual Status command16(uint16_t c, const uint8_t *data = nullptr, size_t bytes = 0) {
    return command(c, data, bytes);
  }
  virtual Status data(const uint8_t *, size_t) { return Status::Unsupported; }
  virtual Status writeRegister16(uint16_t c, uint16_t value) {
    const uint8_t bytes[] = {uint8_t(value >> 8), uint8_t(value)}; return command(c, bytes, 2);
  }
  virtual Status window(int32_t, int32_t, int32_t, int32_t) = 0;
  virtual Status pixels(PixelView, bool async = false) = 0;
  virtual Status fill(uint16_t, uint32_t) = 0;
  virtual void append(uint16_t color) { fill(color, 1); }
  virtual Status append(uint16_t color, uint32_t count) { return fill(color, count); }
  // Finish drains writes and source reads. For e-paper it may enqueue an
  // update; poll() advances physical completion without blocking UI input.
  virtual Status finish() = 0;
  virtual Status flush(int32_t x, int32_t y, PixelView view, bool async = true) {
    if (!view.valid() || view.width > INT32_MAX || view.height > INT32_MAX || x < 0 || y < 0 ||
        view.width > width() || view.height > height() || size_t(x) > width() - view.width || size_t(y) > height() - view.height)
      return Status::InvalidArgument;
    const Status result = window(x, y, x + int32_t(view.width) - 1, y + int32_t(view.height) - 1);
    return result == Status::Ok ? pixels(view, async) : result;
  }
  virtual Status readCommand(uint16_t, uint8_t *, size_t, uint8_t = 1, uint8_t = 8) { return Status::Unsupported; }
  virtual Status readRect(int32_t, int32_t, size_t, size_t, uint16_t *) { return Status::Unsupported; }
  virtual Status readPixel(int32_t x, int32_t y, uint16_t &out) { return readRect(x, y, 1, 1, &out); }
  virtual Status readIndexedCommand(uint16_t, uint8_t, uint8_t &) { return Status::Unsupported; }
  virtual Status invert(bool) { return Status::Unsupported; }
  // Opt-in native drawing uses an independent sealed image and presentation
  // receipt. Existing commitFrame()/finish() retain their completion contract.
  virtual bool supportsNativePresentationPipeline()const{return false;}
  virtual Status beginNativeDrawing(const DirtyRect *,size_t,bool,NativeDrawingLease &lease){lease={};return Status::Unsupported;}
  virtual Status mapNativeDrawing(const NativeDrawingLease &,NativeFramebuffer &frame){frame={};return Status::Unsupported;}
  virtual NativeStageTransfer copyNativeDrawingStage(const NativeDrawingLease &,const NativeFramebuffer &,const DirtyRect &,const NativeStageSource &)noexcept{return {};}
  virtual Status syncNativeDrawing(const NativeDrawingLease &)noexcept{return Status::Unsupported;}
  virtual Status sealNativeDrawing(const NativeDrawingLease &)noexcept{return Status::Unsupported;}
  virtual Status abortNativeDrawing(const NativeDrawingLease &)noexcept{return Status::Unsupported;}
  virtual NativePresentationTransfer submitNativePresentation(const NativeDrawingLease &)noexcept{return {};}
  virtual NativePresentationTransfer pollNativePresentation(const NativePresentationReceipt &,bool)noexcept{return {};}
#if NICE_GFX_P4_NATIVE_PRESENTATION_OPAQUE_FILL
  virtual NativeStageTransfer fillNativeDrawing(const NativeDrawingLease &,const NativeFramebuffer &,const DirtyRect &,uint16_t)noexcept{return {};}
#endif
  virtual Status composeOpaqueFrame(const OpaqueFrameComposition &) { return Status::Unsupported; }
 protected:
  Backend() {}
 private:
  Backend(const Backend &) = delete;
  Backend &operator=(const Backend &) = delete;
};

} // namespace nice_gfx
