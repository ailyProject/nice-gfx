#pragma once
#include "../Core/BuildConfig.h"
#include "../Core/Status.h"
#include "../Core/Pixel.h"
#include "NativeStaging.h"
#include "NativePresentation.h"
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include "../Core/Limits.h"

#ifndef NICE_GFX_FRAME_REPAINT_INHERIT
#define NICE_GFX_FRAME_REPAINT_INHERIT 0
#endif

namespace nice_gfx {

struct DirtyRect {
  int32_t x, y, width, height;
  DirtyRect(int32_t left = 0, int32_t top = 0, int32_t w = 0, int32_t h = 0)
      : x(left), y(top), width(w), height(h) {}
  uint64_t pixels() const { return uint64_t(width) * uint64_t(height); }
};

#ifndef NICE_GFX_UI_RETAINED_CHART_CPU_CONTEXT
#define NICE_GFX_UI_RETAINED_CHART_CPU_CONTEXT 0
#endif
#ifndef NICE_GFX_UI_RETAINED_CHART_INK
#define NICE_GFX_UI_RETAINED_CHART_INK 0
#endif
struct RetainedRepaintRequest {uint64_t owner=0,revision=0;DirtyRect rect{};};
struct RetainedRepaintReceipt {uintptr_t physicalBuffer=0;uint64_t allocationEpoch=0,previousToken=0,nextToken=0;bool retained=false;};

struct TransportInfo {
  uint32_t frequency = 0;
  int clock = -1, cs = -1, dc = -1;
  int data[16] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
  int read = -1, write = -1;
  uint8_t lanes = 0;
};

// Optional writable, native RGB565 scanout storage. This is borrowed from the
// transport: never free it or retain it after end(). Access remains serialized
// with every other transport operation. DMA writers must complete and make
// their output CPU-coherent before submitting this same surface via pixels().
// Mapping does not imply a second buffer, VSYNC synchronization or tear freedom.
struct NativeFramebuffer {
  uint16_t *pixels = nullptr;
  size_t width = 0, height = 0, stride = 0, bytes = 0;
  // Only the P4 MIPI transport sets this for its fenced, open atomic current
  // back allocation. Authorizes synchronous accelerated RMW over complete
  // owned cache lines during this mapping's serialized paint scope only.
  // Generic/partial/unknown/guard-page native views keep the default false.
  bool fencedAtomicMipiRmw = false;
};

// Immutable full picture for optional retained hardware composition. Copies
// are owned by the backend. The caller changes revision whenever any pixel
// changes and uses a fresh owner namespace for each resource-owner lifetime.
struct FrameSource {
  PixelView pixels;
  uint64_t owner=0,revision=0;
  // Optional exact delta from baseRevision. Borrowed for this blocking call;
  // a cache without that exact base must rebuild the complete picture.
  uint64_t baseRevision=0;
  const DirtyRect *damage=nullptr;size_t damageCount=0;
  FrameSource(PixelView image=PixelView(nullptr,0),uint64_t identity=0,uint64_t version=0)
    :pixels(image),owner(identity),revision(version){}
};

// Optional synchronous full-screen composition from immutable full logical
// pictures. The foreground ROI is opaque and samples sourceX/sourceY; pixels
// outside it use background blended against black with backgroundKeep. Color
// precision follows blendFrame(), including backend hardware quantization;
// this does not promise the UI backdrop's exact floor-channel arithmetic.
// Unsupported leaves visible output untouched. Once output writes begin, any
// failure is a real error. Sources are released before the operation returns.
struct OpaqueFrameComposition {
  FrameSource background,foreground;
  DirtyRect foregroundArea;
  int32_t sourceX=0,sourceY=0;
  uint8_t backgroundKeep=255;
};

// One call per span/transfer, never a virtual call per pixel. Transport and Panel
// are single-producer objects; serialize access when used from multiple tasks.
class Transport {
 public:
  virtual ~Transport() {}
  virtual Status begin() = 0;
  virtual void end() = 0;
  virtual Status command(uint8_t command, const uint8_t *data, size_t bytes) = 0;
  // OLED command arguments remain D/C=command, unlike TFT register arguments.
  // Implementations can batch this entire stream in one bus transaction.
  virtual Status commands(const uint8_t *bytes, size_t count) {
    if (count && !bytes) return Status::InvalidArgument;
    for (size_t i=0;i<count;++i) {
      const Status result=command(bytes[i],nullptr,0);
      if(result!=Status::Ok)return result;
    }
    return Status::Ok;
  }
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
  virtual Status beginWrite() { return Status::Ok; }
  virtual Status cacheFrameSource(const FrameSource &) { return Status::Unsupported; }
  virtual Status blitFrameSource(const DirtyRect &,const FrameSource &,int32_t,int32_t) { return Status::Unsupported; }
  // Writes the whole requested region synchronously. An empty background
  // selects solidColor. Unsupported must leave the visible output untouched.
  virtual Status blendFrame(const DirtyRect &,const FrameSource &,const FrameSource &,uint8_t,uint16_t=0) { return Status::Unsupported; }
  // Optional whole-frame transaction. Regions are logical, validated dirty
  // rectangles and are borrowed only during beginFrame(). A backend may copy
  // preserved pixels into a back buffer here. wait()/present() inside an open
  // frame must only drain intermediate writes; commitFrame() publishes once.
  // The default commit preserves framebuffer transports that override present.
  // Explicit write-discard contract. The caller must fully replace EVERY pixel
  // in these regions before any read/alpha blend there, then commit or abort.
  // No generic map, read/modify, transition or partial-cover caller may opt in.
  // Unsupported MUST leave the transaction unopened and output untouched.
  // Separate retained contract: damage minus retained centers is fully
  // replaced; centers are restored from exact old-ink certificates. The
  // exclusive mapped lease forbids other writers until certify+fenced commit.
  // Unsupported leaves the transaction and receipts untouched.
  virtual Status beginRetainedRepaintFrame(const DirtyRect *,size_t,const RetainedRepaintRequest *,size_t,RetainedRepaintReceipt *) { return Status::Unsupported; }
  virtual Status mapRetainedRepaintFramebuffer(const RetainedRepaintReceipt *,size_t,NativeFramebuffer &) { return Status::Unsupported; }
  virtual size_t retainedRepaintStorageBytes()const{return 0;}
  virtual Status validateRetainedRepaintFrame(const RetainedRepaintReceipt *,size_t) { return Status::Unsupported; }
  virtual Status certifyRetainedRepaintFrame(const RetainedRepaintReceipt *,size_t) { return Status::Unsupported; }
  virtual Status beginRepaintFrame(const DirtyRect *,size_t) { return Status::Unsupported; }
  virtual Status beginFrame(const DirtyRect *, size_t) { return Status::Ok; }
  virtual Status commitFrame() { return present(); }
  virtual Status abortFrame() { return Status::Ok; }
  virtual bool busy() const = 0;
  virtual bool asynchronous() const = 0;
  // Can consume retained full-screen/strided surfaces efficiently. Source
  // lifetime follows pixels()/wait(); ordinary transports remain tiled.
  virtual bool supportsFrameBlit() const { return false; }
  // Exact retained-view eligibility, checked before a frame borrows any source.
  // Backends may restrict pixel format, row stride or transaction quantum;
  // a false result selects ordinary tile composition for the entire frame.
  virtual bool canFrameBlit(PixelView view) const { return supportsFrameBlit() && view.valid(); }
  // A successful map has drained outstanding source reads. The normal frame
  // submission/present lifecycle still applies after writing the mapped pixels.
  // Unsupported probes leave the output empty and allocate no memory.
  virtual Status mapNativeFramebuffer(NativeFramebuffer &frame) {
    frame = NativeFramebuffer(); return Status::Unsupported;
  }
  virtual Status setPixelClockHz(uint32_t) { return Status::Unsupported; }
  virtual bool supportsNativeStage()const{return false;}
  virtual NativeStageTransfer copyNativeStage(const NativeFramebuffer &,const DirtyRect &,const NativeStageSource &)noexcept{return {};}
  virtual NativeStageTransfer drainNativeStage()noexcept{return {};}
  virtual NativeStageProviderStats nativeStageProviderStats()const{return {};}
  virtual Status copySurfaceRows(const NativeFramebuffer &,int32_t,PixelView) { return Status::Unsupported; }
  virtual bool validOutputPin(int pin) const { return pin >= 0; }
  virtual TransportInfo info() const { return TransportInfo(); }
  virtual void output(int pin, bool high) = 0;
  virtual void delayMs(uint32_t ms) = 0;
  // Appended capability slots preserve every existing virtual slot. Public
  // wait/present/commit behavior stays synchronous for existing callers.
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
  Transport() {}
 private:
  Transport(const Transport &) = delete;
  Transport &operator=(const Transport &) = delete;
};

} // namespace nice_gfx
