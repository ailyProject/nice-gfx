#pragma once
#include "Esp32Video.h"
#if defined(CONFIG_IDF_TARGET_ESP32P4) && SOC_MIPI_DSI_SUPPORTED && SOC_PPA_SUPPORTED
#include "Rgb565Dma2dCopy.h"
namespace nice_gfx {
// A native landscape render surface, published into the portrait DPI scanout
// with PPA. All source reads finish before present/commit returns. Single
// scanout only: this does not add an atomic VSYNC flip.
class Esp32MipiRotation : public Esp32Mipi {
 public:
  Esp32MipiRotation(const MipiConfig &config,uint8_t rotation)
    :Esp32Mipi(configuration(config)),nativeWidth_(config.dpi.video_timing.h_size),
     nativeHeight_(config.dpi.video_timing.v_size),rotation_(rotation){}
  ~Esp32MipiRotation() override {end();}
  Status begin() override;
  void end() override;
  Status present() override;
  Status beginFrame(const DirtyRect *,size_t) override;
  Status commitFrame() override;
  Status abortFrame() override;
  Status mapNativeFramebuffer(NativeFramebuffer &) override;
  Status copySurfaceRows(const NativeFramebuffer &,int32_t,PixelView) override;
  Status cacheFrameSource(const FrameSource &) override;
  Status blitFrameSource(const DirtyRect &,const FrameSource &,int32_t,int32_t) override;
  Status blendFrame(const DirtyRect &,const FrameSource &,const FrameSource &,uint8_t,uint16_t=0) override;
  Status composeOpaqueFrame(const OpaqueFrameComposition &) override;
  const uint16_t *scanout()const{return scanout_;}
  uint32_t rotations()const{return rotations_;}
  uint64_t rotationMicros()const{return rotationMicros_;}
  uint32_t rotationFailures()const{return rotationFailures_;}
  uint64_t sourceDeltaBlits()const{return sourceDeltaBlits_;}
  uint64_t sourceDeltaPixels()const{return sourceDeltaPixels_;}
  uint64_t sourceRepeatSkips()const{return sourceRepeatSkips_;}
  const platform::rgb565_dma2d::Stats &nativeCopy2dStats()const{return nativeCopy2d_.stats();}
  // Successful synchronous composition calls, independent of presentation.
  uint64_t sourceComposedFrames()const{return sourceComposedFrames_;}
 protected:
  Status drawPixelView(int x,int y,PixelView view) override;
  Status drawFillRect(int x,int y,size_t w,size_t h,uint16_t color) override;
 private:
  static MipiConfig configuration(MipiConfig config){config.useFramebuffer=true;config.usePpa=true;config.dpi.num_fbs=1;return config;}
  void damage(const DirtyRect &);
  Status publish();
  Status rotateView(int x,int y,PixelView view);
  Status materialize();
  struct CachedSource {uint16_t *pixels=nullptr;const uint8_t *source=nullptr;uint64_t owner=0,revision=0,used=0;size_t stride=0;PixelOrder order=PixelOrder::Native;};
  CachedSource sources_[16];uint64_t cacheClock_=0;
  // Metadata only. A proof about the complete physical image, never a lease
  // on caller pixels or on an LRU source-cache slot.
  CachedSource displayed_{};
  bool sourcePresented(const FrameSource &,uint64_t revision)const;
  void rememberSource(const FrameSource &);
  Status copyCachedSource(const DirtyRect &,const CachedSource &,int32_t,int32_t);
  uint64_t sourceDeltaBlits_=0,sourceDeltaPixels_=0,sourceRepeatSkips_=0;
  uint64_t sourceComposedFrames_=0;
  CachedSource *resolve(const FrameSource &,CachedSource *keep=nullptr);
  ppa_client_handle_t blend_=nullptr;
  platform::rgb565_dma2d::Context nativeCopy2d_;
  uint16_t *scanout_=nullptr,*landscape_=nullptr;
  size_t nativeWidth_=0,nativeHeight_=0;
  uint8_t rotation_=1;
  DirtyRect dirty_[16];size_t dirtyCount_=0;
  DirtyRect frameDamage_[16];size_t frameDamageCount_=0;
  bool landscapeCurrent_=true,mapped_=false,explicitFrame_=false;
  uint32_t rotations_=0,rotationFailures_=0;
  uint64_t rotationMicros_=0;
};
} // namespace nice_gfx
#endif
