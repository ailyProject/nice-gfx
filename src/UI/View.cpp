#include "../Core/BuildConfig.h"
#include "detail/GeometryIram.h"
#include "View.h"
#ifndef NICE_GFX_UI_KEYED_PIE
#define NICE_GFX_UI_KEYED_PIE 0
#endif
#if NICE_GFX_UI_KEYED_PIE
#if defined(NICE_GFX_P4_PIE_DIAGNOSTIC_SATURATION_PROBE) && NICE_GFX_P4_PIE_DIAGNOSTIC_SATURATION_PROBE
#error Live UI must not enable the diagnostic saturation probe
#endif
#include "../Platform/ESP32/Rgb565P4BlendSafe.h"
#endif
#ifndef NICE_GFX_UI_TEXT_LAYOUT_MEMO
#define NICE_GFX_UI_TEXT_LAYOUT_MEMO 1
#endif
#ifndef NICE_GFX_P4_MIPI_INHERIT_PREFETCH
#define NICE_GFX_P4_MIPI_INHERIT_PREFETCH 0
#endif
// Arduino compiles every library translation unit, including on AVR targets
// whose toolchains have no STL containers. Keep basic display use available;
// the compiled UI requires a platform with a full C++ standard library.
#if !defined(ARDUINO_ARCH_AVR) && !defined(__AVR__)
#include "../Graphics/TileCanvas.h"
#include "../Display/RetainedRepaint.h"
#include "DisplayPolicy.h"
#include "Animation.h"
#include "detail/MenuMotion.h"
#include "../Graphics/MonoConvert.h"
#include "detail/RangeGeometry.h"
#include "detail/MonochromeControls.h"
#include "MotionGeometry.h"
#include "Transition.h"
#include "Instruments.h"
#include "Shapes.h"
#include "detail/RasterOcclusion.h"
#include "detail/Rgb565Blend.h"
#include "detail/ContainerTemplateNative.h"
#include "detail/BehaviorBackdrop.h"
#include "detail/SystemModalDimNative.h"
#include "detail/RenderBackend.h"
#include "detail/Rle565.h"
#include "detail/SystemModalRleSeek.h"
#include "detail/SystemModalRleParallel.h"
#include "detail/RetainedStorage.h"
#include "detail/KeyboardIcons.h"
#include <vector>
#include <string>
#include <memory>
#include <new>
#include <exception>
#include <algorithm>
#include <atomic>
#include <limits.h>
#include <string.h>
#include <math.h>
#ifndef NICE_GFX_UI_FLAT_TEXT_ALPHA
#define NICE_GFX_UI_FLAT_TEXT_ALPHA 1
#endif
#if defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32P4)
#include <esp_memory_utils.h>
#include <esp_heap_caps.h>
#endif
#include "../Platform/Common/UiMemory.h"
#include "detail/ImageStorage.h"
#include "detail/MediaPixel.h"
#include "detail/JpegMedia.h"
#include "detail/JpegPrefetch.h"
#include "../Platform/ESP32/Rgb565NativeCopy.h"
#ifndef NICE_GFX_UI_JPEG_PREFETCH_COPY
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#define NICE_GFX_UI_JPEG_PREFETCH_COPY 3
#else
#define NICE_GFX_UI_JPEG_PREFETCH_COPY 0
#endif
#endif
#include "detail/KeyedMediaPpa.h"
#ifndef NICE_GFX_UI_AXIS_MEDIA_CACHE
#define NICE_GFX_UI_AXIS_MEDIA_CACHE 1
#endif
#if NICE_GFX_UI_AXIS_MEDIA_CACHE
#include "detail/MotionAxisCache.h"
#endif
#ifndef NICE_GFX_UI_NATIVE_CACHE_REPAIR
#if defined(CONFIG_IDF_TARGET_ESP32P4) || defined(CONFIG_IDF_TARGET_ESP32S31)
#define NICE_GFX_UI_NATIVE_CACHE_REPAIR 1
#else
#define NICE_GFX_UI_NATIVE_CACHE_REPAIR 0
#endif
#endif
#ifndef NICE_GFX_UI_NATIVE_CAPTURE_SCRATCH
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#define NICE_GFX_UI_NATIVE_CAPTURE_SCRATCH 1
#else
#define NICE_GFX_UI_NATIVE_CAPTURE_SCRATCH 0
#endif
#endif
#if NICE_GFX_UI_NATIVE_CACHE_REPAIR
#include "detail/Rgb565MediaCache.h"
#endif
#ifndef NICE_GFX_UI_NATIVE_CACHE_WIRE_COPY
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#define NICE_GFX_UI_NATIVE_CACHE_WIRE_COPY 1
#else
#define NICE_GFX_UI_NATIVE_CACHE_WIRE_COPY 0
#endif
#endif
#if NICE_GFX_UI_NATIVE_CACHE_WIRE_COPY
#include "detail/NativeWireCopy.h"
#endif
#ifndef NICE_GFX_UI_NATIVE_SURFACES
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#define NICE_GFX_UI_NATIVE_SURFACES 1
#else
#define NICE_GFX_UI_NATIVE_SURFACES 0
#endif
#endif
#ifndef NICE_GFX_UI_FIXED_CHROME_PPA
#define NICE_GFX_UI_FIXED_CHROME_PPA 0
#endif
#if NICE_GFX_UI_FIXED_CHROME_PPA
#include "detail/FixedChromeCache.h"
#include "detail/FixedChromeKey.h"
#endif
#ifndef NICE_GFX_UI_INTERNAL_CONTROL
#if defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32P4)
#define NICE_GFX_UI_INTERNAL_CONTROL 1
#else
#define NICE_GFX_UI_INTERNAL_CONTROL 0
#endif
#endif
#include "detail/ShapeOverlayCache.h"
#include "detail/MotionShapeNativeAlpha.h"
#include "detail/MotionShapeOutlinedMask.h"
#ifndef NICE_GFX_UI_SHAPE_NATIVE_SPANS
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#define NICE_GFX_UI_SHAPE_NATIVE_SPANS 1
#else
#define NICE_GFX_UI_SHAPE_NATIVE_SPANS 0
#endif
#endif
#ifndef NICE_GFX_UI_SHAPE_WIRE_SPANS
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#define NICE_GFX_UI_SHAPE_WIRE_SPANS 1
#else
#define NICE_GFX_UI_SHAPE_WIRE_SPANS 0
#endif
#endif
#ifndef NICE_GFX_UI_SHAPE_ALPHA_PPA
#if defined(CONFIG_IDF_TARGET_ESP32P4) && !CONFIG_SPIRAM_ECC_ENABLE
#define NICE_GFX_UI_SHAPE_ALPHA_PPA 1
#else
#define NICE_GFX_UI_SHAPE_ALPHA_PPA 0
#endif
#endif
#ifndef NICE_GFX_UI_SHAPE_ALPHA_PPA_MIN_PIXELS
#define NICE_GFX_UI_SHAPE_ALPHA_PPA_MIN_PIXELS 16384
#endif
#ifndef NICE_GFX_UI_CONTAINER_ALPHA_PPA
#if defined(CONFIG_IDF_TARGET_ESP32P4) && !CONFIG_SPIRAM_ECC_ENABLE
#define NICE_GFX_UI_CONTAINER_ALPHA_PPA 1
#else
#define NICE_GFX_UI_CONTAINER_ALPHA_PPA 0
#endif
#endif
#ifndef NICE_GFX_UI_SHAPE_INK_DAMAGE
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#define NICE_GFX_UI_SHAPE_INK_DAMAGE 1
#else
#define NICE_GFX_UI_SHAPE_INK_DAMAGE 0
#endif
#endif
#ifndef NICE_GFX_UI_NATIVE_AFFINE_MEDIA
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#define NICE_GFX_UI_NATIVE_AFFINE_MEDIA 1
#else
#define NICE_GFX_UI_NATIVE_AFFINE_MEDIA 0
#endif
#endif
#ifndef NICE_GFX_UI_OFFSCREEN_GDMA
#define NICE_GFX_UI_OFFSCREEN_GDMA 1
#endif
#if NICE_GFX_UI_SHAPE_NATIVE_SPANS || NICE_GFX_UI_SHAPE_WIRE_SPANS
#include "detail/ShapeNativeSpans.h"
#endif
#ifndef NICE_GFX_UI_SHAPE_OVERLAY_PPA
#if defined(CONFIG_IDF_TARGET_ESP32S31) || (defined(CONFIG_IDF_TARGET_ESP32P4) && !CONFIG_SPIRAM_ECC_ENABLE)
#define NICE_GFX_UI_SHAPE_OVERLAY_PPA 1
#else
#define NICE_GFX_UI_SHAPE_OVERLAY_PPA 0
#endif
#endif
#include "detail/FrameContext.h"
#include "detail/ComponentPaintIndex.h"
#include "detail/ContainerAlphaNative.h"
#include "detail/SystemModalCopyNative.h"
#ifndef NICE_GFX_UI_SYSTEM_MODAL_ROW_COPY
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#define NICE_GFX_UI_SYSTEM_MODAL_ROW_COPY 1
#else
#define NICE_GFX_UI_SYSTEM_MODAL_ROW_COPY 0
#endif
#endif
#ifndef NICE_GFX_UI_CACHED_VIEWPORT_FADE
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#define NICE_GFX_UI_CACHED_VIEWPORT_FADE 1
#else
#define NICE_GFX_UI_CACHED_VIEWPORT_FADE 0
#endif
#endif
#ifndef NICE_GFX_UI_TEXT_MASK_NATIVE
#if defined(CONFIG_IDF_TARGET_ESP32P4) || defined(CONFIG_IDF_TARGET_ESP32S31)
#define NICE_GFX_UI_TEXT_MASK_NATIVE 1
#else
#define NICE_GFX_UI_TEXT_MASK_NATIVE 0
#endif
#endif
#ifndef NICE_GFX_UI_SYSTEM_PANEL_TEXT_PREPARE
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#define NICE_GFX_UI_SYSTEM_PANEL_TEXT_PREPARE 1
#else
#define NICE_GFX_UI_SYSTEM_PANEL_TEXT_PREPARE 0
#endif
#endif
#ifndef NICE_GFX_UI_TEXT_ALPHA_PPA
#define NICE_GFX_UI_TEXT_ALPHA_PPA 0
#endif
#ifndef NICE_GFX_UI_BUTTON_TEXT_MASK
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#define NICE_GFX_UI_BUTTON_TEXT_MASK 1
#else
#define NICE_GFX_UI_BUTTON_TEXT_MASK 0
#endif
#endif
#ifndef NICE_GFX_UI_VIEWPORT_ANIMATION_ATLAS
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#define NICE_GFX_UI_VIEWPORT_ANIMATION_ATLAS 1
#else
#define NICE_GFX_UI_VIEWPORT_ANIMATION_ATLAS 0
#endif
#endif
#ifndef NICE_GFX_UI_CONTAINER_PRIMITIVE_TEMPLATE
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#define NICE_GFX_UI_CONTAINER_PRIMITIVE_TEMPLATE 1
#else
#define NICE_GFX_UI_CONTAINER_PRIMITIVE_TEMPLATE 0
#endif
#endif
#ifndef NICE_GFX_UI_CONTAINER_ALPHA_INTERIOR
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#define NICE_GFX_UI_CONTAINER_ALPHA_INTERIOR 1
#else
#define NICE_GFX_UI_CONTAINER_ALPHA_INTERIOR 0
#endif
#endif
#ifndef NICE_GFX_UI_SYSTEM_MODAL_BASE_LEASE
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#define NICE_GFX_UI_SYSTEM_MODAL_BASE_LEASE 1
#else
#define NICE_GFX_UI_SYSTEM_MODAL_BASE_LEASE NICE_GFX_P4_SYSTEM_MODAL_ACCELERATION_DEFAULT
#endif
#endif
#ifndef NICE_GFX_UI_SYSTEM_MODAL_PANEL_CACHE
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#define NICE_GFX_UI_SYSTEM_MODAL_PANEL_CACHE 1
#else
#define NICE_GFX_UI_SYSTEM_MODAL_PANEL_CACHE NICE_GFX_P4_SYSTEM_MODAL_ACCELERATION_DEFAULT
#endif
#endif
#ifndef NICE_GFX_UI_TEXT_MASK_INK_BOUNDS
#define NICE_GFX_UI_TEXT_MASK_INK_BOUNDS 1
#endif
#ifndef NICE_GFX_UI_TEXT_MASK_PPA
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#define NICE_GFX_UI_TEXT_MASK_PPA 1
#else
#define NICE_GFX_UI_TEXT_MASK_PPA 0
#endif
#endif
#if NICE_GFX_UI_TEXT_MASK_PPA || NICE_GFX_UI_TEXT_MASK_NATIVE
#include "detail/TextMaskCache.h"
#endif
#if NICE_GFX_UI_TEXT_MASK_NATIVE
#include "detail/TextMaskNative.h"
#endif
#ifndef NICE_GFX_UI_NATIVE_RESOURCE_PREWARM
#define NICE_GFX_UI_NATIVE_RESOURCE_PREWARM 0
#endif
#ifndef NICE_GFX_UI_OPTIMIZE_O3
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#define NICE_GFX_UI_OPTIMIZE_O3 1
#else
#define NICE_GFX_UI_OPTIMIZE_O3 0
#endif
#endif
#if NICE_GFX_UI_OPTIMIZE_O3 && defined(__GNUC__) && !defined(__clang__)
#pragma GCC optimize("O3")
#endif

namespace nice_gfx { namespace htmlui {
namespace {
uint64_t nextRetainedChartOwner(){static std::atomic<uint64_t> owner{1};uint64_t current=owner.load(std::memory_order_relaxed);
  while(current&&current<UINT64_MAX){if(owner.compare_exchange_weak(current,current+1,std::memory_order_relaxed))return current;}return 0;}
uint64_t nextFrameSourceOwner(){static std::atomic<uint32_t> owner{0};return uint64_t(owner.fetch_add(1,std::memory_order_relaxed))+1;}
// Owning tiles are repeatedly read by the display DMA. Generic ESP32 malloc
// may place these buffers in PSRAM, forcing every submission through a second
// internal staging copy. Prefer DMA-capable internal RAM, while retaining the
// original ordinary-heap allocation and zero initialization as a fallback.
class OwnedUiTile {
 public:
  OwnedUiTile() = default;
  explicit OwnedUiTile(size_t pixels) {
    if(!pixels)return;
    dma_ = static_cast<uint16_t *>(platform::allocateUiTile(pixels));
    if (dma_) pixels_ = pixels;
    else fallback_.resize(pixels);
  }
  ~OwnedUiTile() {
    if (dma_) platform::freeUiMemory(dma_);
  }
  OwnedUiTile(const OwnedUiTile &) = delete;
  OwnedUiTile &operator=(const OwnedUiTile &) = delete;
  uint16_t *data() { return dma_ ? dma_ : fallback_.data(); }
  size_t size() const { return dma_ ? pixels_ : fallback_.size(); }
  void swap(OwnedUiTile &other) noexcept {
    fallback_.swap(other.fallback_);
    std::swap(dma_,other.dma_);std::swap(pixels_,other.pixels_);
  }
 private:
  std::vector<uint16_t> fallback_;
  uint16_t *dma_ = nullptr;
  size_t pixels_ = 0;
};

bool validMedia(const MediaDesc &m) {
  if(mediaJpeg(m))return detail::validJpegMedia(m);
  if(!m.pixels||!m.width||!m.height||m.width>4096||m.height>4096||!m.frames||m.frames>300||
     !m.fps||m.fps>30||(m.bytesPerPixel!=1&&m.bytesPerPixel!=2&&!mediaColorKeyed(m)&&!mediaPacked(m)))return false;
  const uint64_t bytes=uint64_t(mediaFrameBytes(m))*m.frames+(mediaColorKeyed(m)?2:0);
  return bytes<=8388608&&reinterpret_cast<uintptr_t>(m.pixels)<=UINTPTR_MAX-size_t(bytes);
}
int32_t clamp32(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : (v > hi ? hi : v); }
bool contains(Rect r, int32_t x, int32_t y) { return r.width > 0 && r.height > 0 && x >= r.x && y >= r.y && int64_t(x) < int64_t(r.x)+r.width && int64_t(y) < int64_t(r.y)+r.height; }
Rect intersect(Rect a, Rect b) {
  int32_t x = std::max<int32_t>(a.x,b.x), y = std::max<int32_t>(a.y,b.y);
  int32_t r = int32_t(std::min<int64_t>(int64_t(a.x)+a.width,int64_t(b.x)+b.width));
  int32_t d = int32_t(std::min<int64_t>(int64_t(a.y)+a.height,int64_t(b.y)+b.height));
  return {x,y,std::max<int32_t>(0,r-x),std::max<int32_t>(0,d-y)};
}
bool control(Kind k) { return k==Kind::InputField || k==Kind::Spinbox || k==Kind::Button || k==Kind::Range || k==Kind::Checkbox || k==Kind::Switch; }
bool scrolling(Kind k) { return k==Kind::Scroll || k==Kind::Pager; }
bool hasText(Kind k) { return k==Kind::InputField || instrumentKind(k) || k==Kind::Text || k==Kind::Label || k==Kind::Button || k==Kind::Checkbox; }
uint16_t switchColor(uint16_t from,uint16_t to,uint16_t position) {
  const uint32_t remaining=1000-position;
  const uint16_t r=uint16_t((((from>>11)&31)*remaining+((to>>11)&31)*position+500)/1000);
  const uint16_t g=uint16_t((((from>>5)&63)*remaining+((to>>5)&63)*position+500)/1000);
  const uint16_t b=uint16_t(((from&31)*remaining+(to&31)*position+500)/1000);
  return uint16_t((r<<11)|(g<<5)|b);
}
bool validFont(const MonoFont &font) {
  if(!font.bitmap||!font.count||!font.width||!font.height||font.baseline>font.height)return false;
  if(!font.codepoints)return font.count>=95;
  // monoGlyph uses binary search and falls back to '?'. A malformed index must
  // not enter that fallback recursively. Validate each unique font once/mount.
  bool fallback=false;uint16_t previous=0;
  for(uint16_t i=0;i<font.count;++i) {
    const uint16_t cp=pgm_read_word(font.codepoints+i);
    if(i&&cp<=previous)return false;
    if(cp=='?')fallback=true;
    previous=cp;
  }
  return fallback;
}
bool validGradient(const GradientDesc *g,Rect r) {
  if(!g)return true;
  if(!g->colors||g->dither>1||reinterpret_cast<uintptr_t>(g->colors)%alignof(uint16_t))return false;
  // Generated coordinates span approximately 0..255 in Q16. Reject malicious
  // resources before the 32-bit hot loops; allow half a palette bin at edges.
  const int64_t limit=255LL<<16;
  if(g->stepX < -limit || g->stepX > limit || g->stepY < -limit || g->stepY > limit)return false;
  const int64_t x=int64_t(std::max<int32_t>(0,r.width-1))*g->stepX;
  const int64_t y=int64_t(std::max<int32_t>(0,r.height-1))*g->stepY;
  const int64_t corners[]={g->origin,g->origin+x,g->origin+y,g->origin+x+y};
  for(int64_t q:corners)if(q < -65536 || q > limit+65536)return false;
  return true;
}
Rect contentBox(Rect r,const Style &s) {
  const int32_t left=int32_t(s.paddingLeft)+s.borderWidth,right=int32_t(s.paddingRight)+s.borderWidth;
  const int32_t top=int32_t(s.paddingTop)+s.borderWidth,bottom=int32_t(s.paddingBottom)+s.borderWidth;
  r.x+=left;r.y+=top;r.width=std::max<int32_t>(0,r.width-left-right);r.height=std::max<int32_t>(0,r.height-top-bottom);return r;
}
// Images follow the content edge of the authored border radius, independently
// of overflow. Subtract each side's border/padding on its own axis. Preserve
// the common CSS overlap reduction as a fraction for narrow content boxes.
struct MediaRoundClip {
  struct Corner { int32_t rx,ry; } corners[4]; // TL, TR, BR, BL
  Rect content;
  int32_t numerator=1,denominator=1;
  MediaRoundClip(Rect box,const Style &style):content(contentBox(box,style)) {
    const int32_t radius=std::max<int32_t>(0,std::min<int32_t>(style.radius,std::min(box.width,box.height)/2));
    const int32_t left=std::max<int32_t>(0,radius-style.borderWidth-style.paddingLeft);
    const int32_t right=std::max<int32_t>(0,radius-style.borderWidth-style.paddingRight);
    const int32_t top=std::max<int32_t>(0,radius-style.borderWidth-style.paddingTop);
    const int32_t bottom=std::max<int32_t>(0,radius-style.borderWidth-style.paddingBottom);
    corners[0]={left,top};corners[1]={right,top};corners[2]={right,bottom};corners[3]={left,bottom};
    for(auto &corner:corners)if(!corner.rx||!corner.ry)corner={0,0};
    reduce(content.width,corners[0].rx+corners[1].rx);
    reduce(content.width,corners[3].rx+corners[2].rx);
    reduce(content.height,corners[0].ry+corners[3].ry);
    reduce(content.height,corners[1].ry+corners[2].ry);
  }
  void reduce(int32_t extent,int32_t sum) {
    if(sum&&int64_t(extent)*denominator<int64_t(numerator)*sum){numerator=extent;denominator=sum;}
  }
  bool rounded() const {
    return numerator&&(corners[0].rx||corners[1].rx||corners[2].rx||corners[3].rx);
  }
  bool cornerContains(const Corner &corner,int32_t x,int32_t y) const {
    if(!corner.rx)return true;
    const int64_t dx=(int64_t(x)*2+1)*denominator-int64_t(2)*corner.rx*numerator;
    const int64_t dy=(int64_t(y)*2+1)*denominator-int64_t(2)*corner.ry*numerator;
    if(dx>=0||dy>=0)return true;
    const int64_t rx2=int64_t(corner.rx)*corner.rx,ry2=int64_t(corner.ry)*corner.ry;
    return dx*dx*ry2+dy*dy*rx2<=4*rx2*ry2*numerator*numerator;
  }
  bool contains(int32_t x,int32_t y) const {
    x-=content.x;y-=content.y;
    if(x<0||y<0||x>=content.width||y>=content.height)return false;
    const int32_t right=content.width-1-x,bottom=content.height-1-y;
    return cornerContains(corners[0],x,y)&&cornerContains(corners[1],right,y)&&
      cornerContains(corners[2],right,bottom)&&cornerContains(corners[3],x,bottom);
  }
  bool cornerContainsFixed(const Corner &corner,int64_t x,int64_t y) const {
    if(!corner.rx)return true;
    const int64_t dx=x*denominator-int64_t(corner.rx)*numerator*65536;
    const int64_t dy=y*denominator-int64_t(corner.ry)*numerator*65536;
    if(dx>=0||dy>=0)return true;
    // Transformed samples have fractional local coordinates. Only rounded
    // corner samples need these divisions; all square-image kernels stay fast.
    const double nx=double(dx)/corner.rx,ny=double(dy)/corner.ry,limit=double(numerator)*65536;
    return nx*nx+ny*ny<=limit*limit;
  }
  bool containsFixed(int64_t x,int64_t y) const {
    x-=int64_t(content.x)*65536;y-=int64_t(content.y)*65536;
    const int64_t width=int64_t(content.width)*65536,height=int64_t(content.height)*65536;
    if(x<0||y<0||x>=width||y>=height)return false;
    return cornerContainsFixed(corners[0],x,y)&&cornerContainsFixed(corners[1],width-x,y)&&
      cornerContainsFixed(corners[2],width-x,height-y)&&cornerContainsFixed(corners[3],x,height-y);
  }
  int32_t inset(const Corner &corner,int32_t y) const {
    if(!corner.rx||int64_t(2*y+1)*denominator>=int64_t(2)*corner.ry*numerator)return 0;
    int32_t first=0,last=(corner.rx*numerator+denominator-1)/denominator;
    while(first<last){const int32_t middle=first+(last-first)/2;
      if(cornerContains(corner,middle,y))last=middle;else first=middle+1;}
    return first;
  }
  void span(int32_t y,int32_t &first,int32_t &end) const {
    const int32_t row=y-content.y,bottom=content.height-1-row;
    if(row<0||row>=content.height){end=first;return;}
    const int32_t left=std::max(inset(corners[0],row),inset(corners[3],bottom));
    const int32_t right=std::max(inset(corners[1],row),inset(corners[2],bottom));
    first=std::max(first,content.x+left);end=std::min(end,content.x+content.width-right);
  }
};
}

struct View::Impl {
  bool repaintDiscardFrame=false;
#if NICE_GFX_UI_INTERNAL_CONTROL && (defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32P4))
  // Fixed control/cache metadata is touched beside every scanout span. Keep
  // it in internal SRAM while large decoded surfaces remain in PSRAM. The
  // ordinary allocation is still available when internal SRAM is exhausted.
  static void *operator new(size_t bytes){
    if(void *memory=heap_caps_malloc(bytes,MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT))return memory;
    return ::operator new(bytes);
  }
  static void *operator new(size_t bytes,const std::nothrow_t &tag)noexcept{
    if(void *memory=heap_caps_malloc(bytes,MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT))return memory;
    return ::operator new(bytes,tag);
  }
  static void operator delete(void *memory)noexcept{heap_caps_free(memory);}
  static void operator delete(void *memory,const std::nothrow_t &)noexcept{heap_caps_free(memory);}
#endif
  struct TextValue {
    const char *compiled="";
    std::shared_ptr<std::string> owned;
    uint64_t revision=0; // Exact mutation identity; retained raster keys need no old string owner.
    const char *c_str() const { return owned?owned->c_str():compiled; }
    void init(const char *s) { compiled=s?s:"";owned.reset();++revision; }
    bool set(const char *s) { s=s?s:"";if(!strcmp(c_str(),s))return false;if(owned&&owned.use_count()==1)owned->assign(s);else owned=std::make_shared<std::string>(s);++revision;return true; }
  };
  // Immutable traversal metadata fits existing State padding; no node-table copy.
  struct State { uint16_t mediaFrame=0, parent=NoParent; uint32_t mediaAt=0; bool mediaPending=true,mediaPaused=false; Kind kind=Kind::Container; TextValue text; int32_t value=0, scrollX=0, scrollY=0; uint16_t flags=0, styleIndex=NoParent; int32_t animationX=0, animationY=0; uint32_t animationAt=0; uint8_t opacity=255; bool animating=false, animationPending=false;
    Image image;
    bool imageOverride=false;
    int8_t focusOverride=-1;
    std::shared_ptr<const InputFieldDesc> implicitInput;
    std::shared_ptr<InstrumentState> instrument;
    std::shared_ptr<const DynamicNode> dynamic;
    uint32_t switchAt=0; uint16_t switchPosition=0,switchFrom=0,switchTo=0; bool switchPending=false;
    uint16_t motionSlot=NoParent;
    uint32_t motionRevision=0;
  };
  #include "detail/MotionRuntime.inc.h"
  struct Listener { Id id; EventType type; EventHandler callback; void *context; bool capture; bool defaultAction=false; bool scoped=false; bool uiAction=false; };
  enum class Drag { None, Range, Scroll, Pager, Back, Home, Group, Gesture, Custom, Consumed, Media };
  struct MediaTween { const PageDesc *page;uint16_t node,from,to;uint32_t at,duration;bool pending; };
  std::vector<MediaPlaybackDesc> mediaPolicies;
  std::vector<MediaTween> mediaTweens;
  struct SharedMediaPolicy { SharedMediaDesc desc; uint16_t node; };
  struct SharedMediaCarry { Id groupId; uint16_t frame; Image image; bool imageOverride; };
  std::vector<SharedMediaPolicy> sharedMediaPolicies;
  std::vector<SharedMediaCarry> sharedMediaCarry;
  int mediaDragTarget=-1;
  uint16_t mediaDragFrom=0;
  int32_t mediaDragWidth=1;
  bool mediaDragLoop=true,mediaDragReverse=false;
  InteractionVersion interactionVersion=InteractionVersion::Legacy;
  std::vector<InteractionDesc> interactionPolicies;
  int pointerClaim=-1;
  GestureAxis pointerClaimAxis=GestureAxis::None;
  bool pointerDispatch=false, defaultsBlocked=false, customCompatibility=false;
  bool ownerStarted=false, ownerFinished=false;
  int contactModal=-1;
  const GesturePolicy &policy(int i) const {
    static const GesturePolicy automatic{};
    if(i<0)return automatic;
    const Id id=page().nodes[i].id;
    const auto at=std::lower_bound(interactionPolicies.begin(),interactionPolicies.end(),id,[](const InteractionDesc &a,Id b){return a.id<b;});
    return at!=interactionPolicies.end()&&at->id==id?at->policy:automatic;
  }
  bool coordinated() const {return interactionVersion==InteractionVersion::Coordinated;}
  bool descendant(int i,int root) const {for(;i>=0;i=parent(i))if(i==root)return true;return false;}
  int modalRoot() const {
    if(!project)return -1;
    int result=-1;
    if(coordinated())for(const auto &entry:interactionPolicies)if(entry.policy.modal){const int i=find(entry.id);if(i>=0&&i>result&&visible(page(),state(),i,true))result=i;}
    int overlay=-1;uint16_t depth=0;
    for(Id id:viewportOverlayIds){const int i=find(id);if(i<0||!visible(page(),state(),i,true))continue;const uint16_t level=viewportOverlayDepth(page(),state(),i);if(level>depth||(level==depth&&i>overlay)){overlay=i;depth=level;}}
    if(overlay>=0)result=overlay;
    for(const auto &entry:behaviors)if(entry.open&&entry.page==history.back()&&behaviorModal(entry)){const int i=find(entry.desc.id);if(i>result&&visible(page(),state(),i,true))result=i;}
    return result;
  }
  bool inInputLayer(int i,int modal) const {if(modal<0||descendant(i,modal))return true;
    bool systemModal=false;for(int a=modal;a>=0;a=parent(a))if(std::binary_search(systemPanelRoots.begin(),systemPanelRoots.end(),page().nodes[a].id)){systemModal=true;break;}
    if(!systemModal)for(int a=i;a>=0;a=parent(a))if(std::binary_search(systemBarRoots.begin(),systemBarRoots.end(),page().nodes[a].id))return true;
    return false;}
  bool inInputLayer(int i) const {return inInputLayer(i,modalRoot());}
  bool acceptsEvent(const Event &event) const {
    if(!coordinated())return true;
    const auto scope=policy(find(event.currentTarget)).eventScope;
    return scope==EventScope::Subtree||event.target==event.currentTarget||
      (scope==EventScope::Component&&event.component==event.currentTarget);
  }
  NICE_GFX &display;
  OwnedUiTile first, second;
  TileCanvas canvas;
  RefreshScheduler<16> *refresh;
  size_t capacity;
  size_t tileCount=2;
  const ProjectDesc *project=nullptr;
  std::vector<RetainedVector<State>> pages;
  std::vector<uint16_t> history;
  std::shared_ptr<std::vector<Listener>> listeners=std::make_shared<std::vector<Listener>>();
  std::vector<std::vector<int>> eventPaths;
  size_t eventDepth=0;
  mutable std::vector<uint16_t> lookup;
  mutable const PageDesc *lookupPage=nullptr;
  const ComponentDesc *components=nullptr;
  size_t componentCount=0;
  struct ComponentLink {const ComponentDesc *desc;int root,value,text,pager,panel;};
  std::vector<Id> viewportOverlayIds;
  uint16_t viewportOverlayLevels=0;
  std::vector<Id> systemBarRoots,systemPanelRoots;
  uint16_t componentOverlayLevels=0;
  struct SystemChromePair {uint16_t page,bar,panel,handle;};
  std::vector<SystemChromePair> systemChromePairs;
  int systemChromePanel=-1;bool systemChromeOpening=false,systemChromeDragging=false,systemChromeDirect=false,systemChromeRejected=false;
  bool systemChromeInitialOpen=false,systemChromeHandle=false,systemChromeVelocityFresh=false;
  uint16_t systemChromePage=NoParent;uint32_t systemChromeRevision=0;
  int32_t systemChromeStartProgress=0;Id systemChromeSavedFocus=0;
  struct SystemPanelMember {uint16_t page,node,sourcePage,sourceNode;};
  std::vector<SystemPanelMember> systemPanelMembers;
  struct PageTextOverride {uint16_t page,node;TextValue text;};
  struct PageProgressOverride {uint16_t page,node;int32_t value;};
  std::vector<PageTextOverride> pageTextOverrides;
  std::vector<PageProgressOverride> pageProgressOverrides;
  bool systemPanelShareVisibility(uint16_t pageIndex,uint16_t node){
    const auto &page=project->pages[pageIndex];const auto *b=behavior(page.nodes[node].id);
    return !viewportOverlayRoot(page,node)&&!(b&&behaviorPanel(*b));
  }
  static void copySystemPanelValue(const State &source,State &target,const NodeDesc &desc,bool visibility){
    target.text=source.text;target.value=source.value;target.scrollX=source.scrollX;target.scrollY=source.scrollY;
    target.flags=(target.flags&~uint16_t(Hidden|Disabled))|(source.flags&Disabled)|((visibility?source.flags:desc.flags)&Hidden);
    target.focusOverride=source.focusOverride;target.image=source.image;target.imageOverride=source.imageOverride;target.dynamic=source.dynamic;target.instrument=source.instrument;
    target.mediaFrame=source.mediaFrame;target.mediaPaused=source.mediaPaused;target.mediaAt=0;target.mediaPending=true;
    if(desc.kind==Kind::Switch){target.switchPosition=target.switchFrom=target.switchTo=source.value?1000:0;target.switchPending=false;}
  }
  void saveSystemPanelState(uint16_t from){
    bool changed=false;for(const auto &link:systemPanelMembers)if(link.page==from&&link.page!=link.sourcePage){initializePage(link.sourcePage);copySystemPanelValue(pages[from][link.node],pages[link.sourcePage][link.sourceNode],project->pages[link.sourcePage].nodes[link.sourceNode],systemPanelShareVisibility(from,link.node));changed=true;}if(changed){surfaceWarmPending=!surfaces.empty();++surfaceMutationEpoch;}
  }
  void restoreSystemPanelState(uint16_t to){
    bool changed=false;for(const auto &link:systemPanelMembers)if(link.page==to&&link.page!=link.sourcePage&&!pages[link.sourcePage].empty()){copySystemPanelValue(pages[link.sourcePage][link.sourceNode],pages[to][link.node],project->pages[to].nodes[link.node],systemPanelShareVisibility(to,link.node));changed=true;}if(changed){surfaceWarmPending=!surfaces.empty();++surfaceMutationEpoch;}
  }
  bool systemPanelBlank(int target,int root) {
    if(target<0||!descendant(target,root))return false;
    for(int node=target;node>=0&&node!=root;node=parent(node)){
      const auto &desc=page().nodes[node];
      if(control(desc.kind)||scrolling(desc.kind)||desc.routeAction!=RouteAction::None||desc.targetId||mediaScrubTarget(node)>=0||behavior(desc.id)||componentFor(node))return false;
      for(const auto &listener:*listeners)if(listener.id==desc.id&&(listener.type==EventType::Click||listener.type==EventType::LongPress||listener.type==EventType::PointerDown||listener.type==EventType::PointerMove))return false;
    }
    return true;
  }
  mutable const PageDesc *componentsPage=nullptr;
  mutable std::vector<ComponentLink> componentLinks;
  mutable std::vector<int32_t> componentNodes;
  std::vector<uint16_t> underlay; // Allocated only when an animation uses opacity.
  mutable detail::JpegMediaCache jpegCache;
  mutable detail::JpegPrefetch jpegPrefetch;
  // Painters finish their source reads synchronously. Keep the exact borrowed
  // JPEG output alive until the next decode, and release it before its worker.
  mutable detail::JpegPrefetch::Lease jpegPaintLease;
#if NICE_GFX_UI_NATIVE_CACHE_REPAIR
  detail::Rgb565MediaCache rgb565MediaCache;
#endif
#if NICE_GFX_UI_KEYED_MEDIA_PPA
  detail::KeyedMediaPpaCache keyedMediaPpaCache;
#endif
#if NICE_GFX_UI_AXIS_MEDIA_CACHE
  detail::MotionAxisCache motionAxisCache;
#endif
#if NICE_GFX_UI_SHAPE_OVERLAY_PPA || NICE_GFX_UI_SHAPE_NATIVE_SPANS || NICE_GFX_UI_SHAPE_WIRE_SPANS
  detail::ShapeOverlayCache shapeOverlayCache;
  uint64_t shapeTextureHitsBase=0,shapeTextureMissesBase=0,shapeTextureEvictionsBase=0;
#endif
#if NICE_GFX_UI_TEXT_MASK_PPA || NICE_GFX_UI_TEXT_MASK_NATIVE
  detail::TextMaskCache textMaskCache;
#endif
#if NICE_GFX_UI_OUTLINED_MASK
  detail::MotionShapeOutlinedMask motionOutlinedMask;
#endif
  mutable Status mediaDecodeStatus=Status::Ok;
  std::vector<uint16_t> mediaRow; // Reusable wire-order scanline, never a framebuffer.
  detail::FrameContext<State,MotionRuntime> rasterFrame;
  using TextLine=detail::FrameTextLine;
  using Geometry=detail::FrameGeometry;
  using FrameCache=detail::FrameGeometryCache;
  FrameCache *paintCache=&rasterFrame.current.geometry;
  mutable RuntimeStats stats;
  mutable detail::ComponentPaintIndex componentPaintIndices[2];
  mutable uint64_t componentPaintUsed[2]={0,0},componentPaintClock=0;
  mutable const PageDesc *tickPage=nullptr;
  mutable std::vector<uint16_t> animationNodes,mediaNodes;
#ifndef NICE_GFX_UI_SUBTREE_DAMAGE_RANGES
#define NICE_GFX_UI_SUBTREE_DAMAGE_RANGES 1
#endif
  const PageDesc *subtreeRangePage=nullptr;const NodeDesc *subtreeRangeNodes=nullptr;
  uint32_t subtreeRangeEpoch=0;uint16_t subtreeRangeCount=0;bool subtreeRangesValid=false;
  std::unique_ptr<uint16_t[]> subtreeEnds;size_t subtreeEndsCapacity=0;
  bool prepareSubtreeRanges(){
    const auto &p=page();
    if(subtreeRangePage==&p&&subtreeRangeNodes==p.nodes&&subtreeRangeEpoch==inputLayoutEpoch&&subtreeRangeCount==p.count)return subtreeRangesValid;
    subtreeRangePage=&p;subtreeRangeNodes=p.nodes;subtreeRangeEpoch=inputLayoutEpoch;subtreeRangeCount=p.count;subtreeRangesValid=false;
    if(!p.count||p.count>4096||state().size()!=p.count)return false;
    for(uint16_t i=0;i<p.count;++i){const auto &value=state()[i];const uint16_t parent=p.nodes[i].parent;
      if(value.dynamic||value.parent!=parent||(parent!=NoParent&&parent>=i))return false;
    }
    if(subtreeEndsCapacity<p.count){std::unique_ptr<uint16_t[]> next(new(std::nothrow) uint16_t[p.count]);if(!next)return false;subtreeEnds=std::move(next);subtreeEndsCapacity=p.count;}
    std::unique_ptr<uint16_t[]> counts(new(std::nothrow) uint16_t[p.count]);if(!counts)return false;
    for(uint16_t i=0;i<p.count;++i){counts[i]=1;subtreeEnds[i]=uint16_t(i+1);}
    for(size_t end=p.count;end;--end){const uint16_t i=uint16_t(end-1),parent=p.nodes[i].parent;if(parent==NoParent)continue;
      counts[parent]=uint16_t(counts[parent]+counts[i]);subtreeEnds[parent]=std::max(subtreeEnds[parent],subtreeEnds[i]);
    }
    // Prove each node independently. A hoisted shared-media root may make its
    // ancestors noncontiguous while ordinary business subtrees remain exact.
    for(uint16_t i=0;i<p.count;++i)if(counts[i]!=subtreeEnds[i]-i)subtreeEnds[i]=NoParent;
    subtreeRangesValid=true;++stats.subtreeRangeBuilds;return true;
  }
  void ensureTickNodes() const {
    if(!project||tickPage==&page())return;
    tickPage=&page();animationNodes.clear();mediaNodes.clear();
    for(uint16_t i=0;i<page().count;++i){const auto &n=page().nodes[i];
      if(n.animation.effect!=AnimationEffect::None||n.motion||n.exitAnimation||n.kind==Kind::Switch||instrumentKind(n.kind))animationNodes.push_back(i);
      if(n.kind==Kind::Media||n.media)mediaNodes.push_back(i);
    }
  }
  uint32_t (*profilingClock)()=nullptr;
  RenderBudget renderBudget;
  bool ppaBlendEnabled=true;
  size_t rasterCapacity() const {return renderBudget.maxPixelsPerTick?std::min(capacity,renderBudget.maxPixelsPerTick):capacity;}
  uint32_t workClock() const {return profilingClock?profilingClock():platform::micros();}
  struct TickTimer {
    Impl &owner;uint32_t started;bool enabled;
    explicit TickTimer(Impl &s):owner(s),started(0),enabled(s.profilingClock||s.renderBudget.maxMicrosPerTick){if(enabled)started=s.workClock();}
    bool expired() const {return owner.renderBudget.maxMicrosPerTick&&uint32_t(owner.workClock()-started)>=owner.renderBudget.maxMicrosPerTick;}
    ~TickTimer(){if(enabled){const uint32_t elapsed=owner.workClock()-started;owner.stats.tickMicros+=elapsed;owner.stats.maxTickMicros=std::max(owner.stats.maxTickMicros,elapsed);}}
  };
  struct RenderTimer {
    Impl &owner;uint32_t started;
    explicit RenderTimer(Impl &s):owner(s),started(s.profilingClock?s.profilingClock():0){}
    ~RenderTimer(){if(owner.profilingClock){const uint32_t elapsed=owner.profilingClock()-started;owner.stats.renderMicros+=elapsed;owner.stats.maxRenderMicros=std::max(owner.stats.maxRenderMicros,elapsed);}}
  };
  using ProfileCounter=uint64_t RuntimeStats::*;
  struct ProfileTimer {
    uint64_t *counter;uint32_t (*clock)();uint32_t started;
    ProfileTimer(Impl &s,ProfileCounter field,bool enabled=true):
      counter(enabled&&s.profilingClock?&(s.stats.*field):nullptr),
      clock(counter?s.profilingClock:nullptr),started(clock?clock():0){}
    void stop(){if(clock){*counter+=uint32_t(clock()-started);clock=nullptr;}}
    ~ProfileTimer(){stop();}
  };
// BEGIN ROI_PREFIX_TIMING nested-scope
#if NICE_GFX_UI_ROI_PREFIX_TIMING
  struct RoiPrefixTimer;
  mutable RoiPrefixTimer *roiPrefixTimingCurrent=nullptr;
  struct RoiPrefixTimer {
    const Impl &owner;RoiPrefixTimer *parent=nullptr;uint32_t (*clock)()=nullptr;
    uint32_t started=0,children=0;RoiPrefixTimingPhase phase;bool accepted=false;
    RoiPrefixTimer(const Impl &s,RoiPrefixTimingPhase p):owner(s),clock(s.profilingClock),phase(p){
      if(clock){parent=s.roiPrefixTimingCurrent;s.roiPrefixTimingCurrent=this;started=clock();++s.stats.roiPrefixTimingCalls[unsigned(p)];}
    }
    RoiPrefixTimer(const RoiPrefixTimer&)=delete;RoiPrefixTimer&operator=(const RoiPrefixTimer&)=delete;
    void result(Status s){if(!clock||phase<RoiPrefixTimingPhase::TransportCopy||phase>RoiPrefixTimingPhase::CanvasCopy)return;
      const unsigned i=unsigned(phase)-unsigned(RoiPrefixTimingPhase::TransportCopy);
      if(s==Status::Unsupported)++owner.stats.roiPrefixTimingCopyUnsupported[i];else if(s!=Status::Ok)++owner.stats.roiPrefixTimingCopyErrors[i];}
    void stop(){if(clock){const uint32_t elapsed=uint32_t(clock()-started);clock=nullptr;
      owner.stats.roiPrefixTimingMicros[unsigned(phase)]+=elapsed;
      owner.stats.roiPrefixTimingSelfMicros[unsigned(phase)]+=uint32_t(elapsed-children);
      owner.roiPrefixTimingCurrent=parent;if(parent)parent->children+=elapsed;
      if(phase==RoiPrefixTimingPhase::Resolve){if(accepted)++owner.stats.roiPrefixTimingAccepted;else ++owner.stats.roiPrefixTimingRejected;}
    }}
    ~RoiPrefixTimer(){stop();}
  };
#endif
// END ROI_PREFIX_TIMING nested-scope
  unsigned geometryProfileDepth=0;
  struct GeometryTimer {
    Impl &owner;bool entered;ProfileTimer timer;
    explicit GeometryTimer(Impl &s):owner(s),entered(s.profilingClock!=nullptr),
      timer(s,&RuntimeStats::geometryMicros,entered&&s.geometryProfileDepth++==0){}
    ~GeometryTimer(){if(entered)--owner.geometryProfileDepth;}
  };
  static ProfileCounter paintProfileCounter(Kind kind) {
    if(kind==Kind::Media||kind==Kind::Icon)return &RuntimeStats::mediaPaintMicros;
    if(kind==Kind::Shape)return &RuntimeStats::shapePaintMicros;
    if(kind==Kind::Text||kind==Kind::Label||kind==Kind::Button||kind==Kind::InputField||kind==Kind::Checkbox)return &RuntimeStats::textPaintMicros;
    return &RuntimeStats::otherPaintMicros;
  }
#if NICE_GFX_UI_SHAPE_PATH_PROFILE
  uint8_t activeShapePaintPath=0;bool shapePaintPathActive=false;
#endif
  void markShapePaintPath(ShapePaintPath path){
#if NICE_GFX_UI_SHAPE_PATH_PROFILE
    if(profilingClock&&shapePaintPathActive)activeShapePaintPath=uint8_t(path);
#else
    (void)path;
#endif
  }
#if NICE_GFX_UI_NODE_KIND_PROFILE || NICE_GFX_UI_SHAPE_PATH_PROFILE
  struct NodePaintTimer {
    uint64_t *group=nullptr,*kindMicros=nullptr;
    uint32_t (*clock)()=nullptr;uint32_t started=0;
#if NICE_GFX_UI_SHAPE_PATH_PROFILE
    Impl *shapeOwner=nullptr;uint8_t savedPath=0;bool savedActive=false;
#endif
    NodePaintTimer(Impl &s,Kind kind):clock(s.profilingClock){
      if(!clock)return;group=&(s.stats.*paintProfileCounter(kind));
#if NICE_GFX_UI_NODE_KIND_PROFILE
      const unsigned index=unsigned(kind);if(index<20){kindMicros=&s.stats.nodeKindPaintMicros[index];++s.stats.nodeKindPaintCalls[index];}
#endif
#if NICE_GFX_UI_SHAPE_PATH_PROFILE
      if(kind==Kind::Shape){shapeOwner=&s;savedPath=s.activeShapePaintPath;savedActive=s.shapePaintPathActive;s.activeShapePaintPath=0;s.shapePaintPathActive=true;}
#endif
      started=clock();
    }
    ~NodePaintTimer(){if(clock){const uint32_t elapsed=clock()-started;*group+=elapsed;if(kindMicros)*kindMicros+=elapsed;
#if NICE_GFX_UI_SHAPE_PATH_PROFILE
      if(shapeOwner){const unsigned path=shapeOwner->activeShapePaintPath<uint8_t(ShapePaintPath::Count)?shapeOwner->activeShapePaintPath:0;++shapeOwner->stats.shapePathCalls[path];shapeOwner->stats.shapePathMicros[path]+=elapsed;shapeOwner->activeShapePaintPath=savedPath;shapeOwner->shapePaintPathActive=savedActive;}
#endif
    }}
  };
  static_assert(unsigned(Kind::InputField)<20,"fixed kind diagnostic slots");
#endif
  uint16_t updateDepth=0;
  int32_t snapshotBackOffset=0, paintOffsetX=0, paintOffsetY=0, backOffset=0;
  enum class TransitionPhase : uint8_t { Paired, Components, Exit, Enter };
  struct Transition {
    Rect viewport{};
    const PageDesc *from=nullptr;
    RetainedVector<State> outgoing;
    std::vector<MotionRuntime> outgoingMotions;
    uint32_t started=0;
    bool active=false,pending=false,reverse=false;
    TransitionPhase phase=TransitionPhase::Paired;
    TransitionDesc config{};
    const TransitionDesc *incoming=nullptr;
    int destination=-1;
    RouteAction action=RouteAction::None;
    uint16_t background=0;
    bool sequential=false,sharedComponents=false;
    TransitionSample sample{0,0,0,0,255,false};
  } transition;
  bool snapshotTransition=false;
  TransitionPhase snapshotPhase=TransitionPhase::Paired;
  uint16_t snapshotBackground=0;
  uint32_t transitionEpoch=0,snapshotTransitionEpoch=0,completedTransitionEpoch=0;
  TransitionSample transitionFrame{0,0,0,0,255,false};
  Rect snapshotMotionRect{};
  bool renderingViewport=false;
  std::vector<uint16_t> transitionUnderlay; // At most one tile, never a framebuffer.
  rgb565_ppa::Context ppaFade; // One optional S31 full-page scratch surface.
  bool nativeGpuPaintAllowed()const{
#if NICE_GFX_UI_RETAINED_CHART_CPU_CONTEXT
    return !ppaFade.cpuOnlyNativePaint();
#else
    return true;
#endif
  }
  int snapshotPressed=-1;
  const InputConfig *input=nullptr;
  const FocusStyleDesc *focusStyles=nullptr;size_t focusStyleCount=0;Id snapshotFocusId=0;
  mutable const InputConfig *focusIndexConfig=nullptr;
  mutable std::vector<size_t> focusIndex;
  mutable std::vector<int> focusItems;
  const NavigationDesc *navigation=nullptr;
  const GestureBindingDesc *gesture=nullptr;
  int navigationDot=-1;bool navigationFooterContact=false;
  struct GroupMotion {
    RetainedVector<State> outgoing;
    std::vector<MotionRuntime> outgoingMotions;
    int destination=-1;int32_t offset=0,from=0,to=0;uint32_t started=0,updated=0;
    bool active=false,settling=false,commit=false,prepared=false,sharedBackdrop=false;
    bool vertical=false,follow=false;int sign=-1;
    RouteAction action=RouteAction::Replace;
    TransitionDesc config{TransitionEffect::SlideLeft,AnimationEasing::EaseOut,180};
    uint16_t duration=180;
  } groupMotion;
  NavigationGroupDesc snapshotGroup{};int snapshotGroupIndex=-1;
  std::vector<Id> pageFocus;
  int focusNode=-1, focusScope=-1, editNode=-1, snapshotFocus=-1;
  bool keyInput=false, snapshotKeyInput=false;
  bool snapshotInput=false;uint16_t snapshotFocusColor=0;uint8_t snapshotFocusWidth=0;
  int32_t editValue=0;
  struct SoftKey { std::string id,label,value,action;Rect rect; };
  struct SoftInput {
    int node=-1, mode=0, group=-1, keyFocus=0;
    size_t cursor=0;
    bool visible=false, shift=false, reveal=false, fullscreen=false, picker=false;
    std::string draft,error,pressed;
    InputFieldDesc descriptor{};
    KeyboardTheme theme=KeyboardTheme::Light;
    Rect rect{0,0,0,0},fieldRect{0,0,0,0},labelRect{0,0,0,0};
    std::shared_ptr<const std::vector<SoftKey>> keys;
    int builtNode=-2,builtMode=-1,builtGroup=-2,builtNext=-2;
    bool builtShift=false,builtReveal=false,builtRound=false;
    uint32_t builtEpoch=0;
  } softInput, snapshotKeyboard;
  std::unique_ptr<SoftInput> suspendedSystemInput;
  uint16_t suspendedSystemInputPage=NoParent;
  // Small, immutable text window prepared once per frame, shared by DMA tiles.
  // Its size follows the visible field rather than the maximum draft length.
  struct KeyboardTextFrame {std::string draft,title;size_t cursor=0;uint8_t titleFontSize=1;} keyboardTextFrame;
  bool circularKeyboard=false, keyboardPointer=false;
  int keyboardThemeOverride=-1;
  bool keyboardFieldPointer=false,keyboardSpacePointer=false,keyboardTrackpad=false,keyboardPickerPointer=false,keyboardPickerVertical=false;
  size_t keyboardPointerCursor=0;
  int32_t keyboardPointerX=0,keyboardPointerY=0,keyboardPointerCell=1;
  std::string keyboardPressed;
  bool keyboardBackspaceRepeat=false,keyboardBackspacePaused=false;
  uint32_t keyboardBackspaceStarted=0,keyboardBackspaceAt=0,keyboardBackspaceDelay=400,keyboardBackspaceEpoch=0;
  uint32_t inputEpoch=0;
  uint32_t inputLayoutEpoch=0;
  RouteGuard guard=nullptr;
  void *guardContext=nullptr;
  bool processing=false, guarding=false, pendingNavigation=false, edgeBack=false, swipeHome=false, backPrepared=false;
  std::string pendingPath;
  RouteAction pendingAction=RouteAction::None;
  TransitionDesc pendingTransition{};
  bool pendingExplicitTransition=false,pendingInstant=false;
  int target=-1, activeControl=-1, owner=-1;
  Drag drag=Drag::None;
  bool down=false, canceled=false, changed=false, longPressed=false, allowX=true, allowY=true, moved=false;
  int32_t x=0,y=0,startX=0,startY=0,startScrollX=0,startScrollY=0,startValue=0;
  int32_t rangeGrabOffset=0;bool rangeThumbContact=false;
  uint32_t downAt=0,spinRepeatAt=0;
  int interruptedPager=-1;
  int32_t lastX=0, velocityX=0;
  int32_t lastY=0, velocityY=0;
  uint32_t lastAt=0;
  struct Animation { int node=-1;int32_t from=0,to=0,index=0;uint32_t started=0;bool active=false; } animation;
  struct Inertia { int node=-1;int32_t velocity=0,remainder=0;uint32_t last=0;bool active=false; } inertia;
  struct BackAnimation { int32_t from=0,to=0;uint32_t started=0;bool active=false,commit=false; } backAnimation;

  // Synchronous transports finish reading each tile before returning; only
  // asynchronous DMA can overlap two tiles and needs the second allocation.
  Impl(NICE_GFX &d, size_t n):display(d),first(n?n:DisplayPolicy::from(d.displayCapabilities()).defaultTilePixels(d.width())),second(d.asynchronous()?first.size():0),canvas(d.width(),d.height()),capacity(first.size()) {
#if NICE_GFX_UI_CHART_COLUMN_STROKES
    canvas.enableChartColumnStrokesForExactCanvas(fillNativeCanvas,this);
#endif
    tileBuffers[0]=first.data();tileBuffers[1]=second.size()?second.data():nullptr;
    tileCount=second.size()?2:1;
    refresh=new RefreshScheduler<16>(d,first.data(),second.data(),capacity);
  }
  Impl(NICE_GFX &d,uint16_t *a,uint16_t *b,size_t n):display(d),canvas(d.width(),d.height()),capacity(n) {
#if NICE_GFX_UI_CHART_COLUMN_STROKES
    canvas.enableChartColumnStrokesForExactCanvas(fillNativeCanvas,this);
#endif
    tileBuffers[0]=a;tileBuffers[1]=b;
    tileCount=b?2:1;
    refresh=new RefreshScheduler<16>(d,a,b,n);
  }
  ~Impl() { refresh->cancel(); delete refresh; }
  DisplayPolicy displayPolicy=DisplayPolicy::from(display.displayCapabilities());
  MonochromeEffects monochromeEffects=MonochromeEffects::Threshold;
  MonochromeEffects snapshotMonochromeEffects=MonochromeEffects::Threshold;
  uint32_t lastDisplayFrameAt=0;
  bool displayFrameStarted=false;
  const PageDesc &page() const { return project->pages[history.back()]; }
  size_t resourceCacheBudget=0,resourceReserve=65536;
  const ProjectDesc *resourceSource=nullptr;ProjectDesc resourceProject{};
  struct ResourcePageFree {void operator()(PageDesc *p) const {platform::freeUiMemory(p);}};
  std::unique_ptr<PageDesc[],ResourcePageFree> resourcePages;size_t resourcePageCount=0;
  struct ResourceSlot {
    NodeDesc *nodes=nullptr;size_t capacity=0;uint16_t page=NoParent;uint32_t age=0;
    ~ResourceSlot(){platform::freeUiMemory(nodes);}
  } resourceSlots[2];
  uint32_t resourceAge=0;
  size_t resourceCacheBytes() const {
    size_t bytes=resourcePageCount*sizeof(PageDesc);
    for(const auto &slot:resourceSlots)bytes+=slot.capacity*sizeof(NodeDesc);return bytes;
  }
  void attachResourceProject(const ProjectDesc &p) {
    resourceSource=&p;project=&p;
    if(!resourceCacheBudget||size_t(p.count)*sizeof(PageDesc)>=resourceCacheBudget)return;
    auto *storage=static_cast<PageDesc *>(platform::allocateUiMetadata(size_t(p.count)*sizeof(PageDesc),resourceReserve));
    if(!storage)return;
    for(size_t i=0;i<p.count;++i)new(storage+i) PageDesc(p.pages[i]);
    resourcePages.reset(storage);resourcePageCount=p.count;resourceProject=p;
    resourceProject.pages=storage;project=&resourceProject;
  }
  void cacheResourcePage(uint16_t index) {
    if(!resourcePages||index>=resourcePageCount)return;
    for(auto &slot:resourceSlots)if(slot.page==index){slot.age=++resourceAge;return;}
    const auto &source=resourceSource->pages[index];const size_t bytes=size_t(source.count)*sizeof(NodeDesc);
    ResourceSlot *chosen=nullptr;
    for(auto &slot:resourceSlots){
      const PageDesc *old=slot.page==NoParent?nullptr:&resourcePages[slot.page];
      if(old&&(old==&page()||(transition.active&&transition.from==old)||
        (refresh->busy()&&(rasterFrame.current.page==old||rasterFrame.preview.page==old))))continue;
      const size_t replacementBytes=std::max(slot.capacity,size_t(source.count))*sizeof(NodeDesc);
      if(replacementBytes>resourceCacheBudget||resourceCacheBytes()-slot.capacity*sizeof(NodeDesc)>resourceCacheBudget-replacementBytes)continue;
      if(!chosen||slot.page==NoParent||slot.age<chosen->age)chosen=&slot;
    }
    if(!chosen)return;
    if(chosen->page!=NoParent)resourcePages[chosen->page].nodes=resourceSource->pages[chosen->page].nodes;
    chosen->page=NoParent;
    if(!chosen->nodes||chosen->capacity<source.count){
      platform::freeUiMemory(chosen->nodes);chosen->nodes=nullptr;chosen->capacity=0;
      chosen->nodes=static_cast<NodeDesc *>(platform::allocateUiMetadata(bytes,resourceReserve));
      if(!chosen->nodes)return;
      chosen->capacity=source.count;
    }
    for(size_t i=0;i<source.count;++i)new(chosen->nodes+i) NodeDesc(source.nodes[i]);
    chosen->page=index;chosen->age=++resourceAge;
    resourcePages[index].nodes=chosen->nodes;
  }

  RetainedVector<State> &state() { return pages[history.back()]; }
  const RetainedVector<State> &state() const { return pages[history.back()]; }
  const MediaDesc *media(const NodeDesc &node,const State &value) const {
    return value.imageOverride?(value.image.impl_?&value.image.impl_->media:nullptr):node.media;
  }
  uint64_t jpegInstance(Id node) const {
    uint16_t index=history.empty()?0:history.back();const PageDesc *drawing=rasterFrame.current.page;
    if(project&&drawing)for(uint16_t p=0;p<project->count;++p)if(&project->pages[p]==drawing||!strcmp(project->pages[p].path,drawing->path)){index=p;break;}
    for(const auto &entry:sharedMediaPolicies)if(entry.desc.pageIndex==index&&entry.desc.nodeId==node)return (uint64_t(entry.desc.groupId)<<1)|1;
    return ((uint64_t(index)<<32)|node)<<1;
  }
  bool decodeJpeg(const MediaDesc &resource,uint16_t frame,JpegDecodeTarget &target,Id node) const {
#if NICE_GFX_UI_HAS_JPEG_PREFETCH
    detail::JpegMediaCache::FramebufferPlan plan;
    const bool canPrefetch=jpegCache.planFramebuffer(resource,frame,plan);
    if(canPrefetch){detail::JpegPrefetch::Lease next;
      if(jpegPrefetch.acquire(plan,next)){
        jpegPaintLease=std::move(next);
        target={const_cast<uint16_t*>(jpegPaintLease.pixels),jpegPaintLease.capacityBytes,jpegPaintLease.strideBytes,jpegPaintLease.width,jpegPaintLease.height};
        requestJpegAhead(resource,frame);return true;
      }
    }
#endif
    const Status status=jpegCache.resolve(resource,frame,target,jpegInstance(node));
#if NICE_GFX_UI_HAS_JPEG_PREFETCH
    jpegPaintLease.reset();
    if(status==Status::Ok&&canPrefetch)requestJpegAhead(resource,frame);
#endif
    if(status!=Status::Ok&&mediaDecodeStatus==Status::Ok)mediaDecodeStatus=status;
    return status==Status::Ok;
  }
  Status imageTarget(Id id,int &index) const {
    if(!project)return Status::NotInitialized;
    index=find(id);
    return index>=0&&page().nodes[index].kind==Kind::Media?Status::Ok:Status::InvalidArgument;
  }
  void replaceImage(int index,const Image &image,bool overrideSource) {
    auto &value=state()[index];value.image=image;value.imageOverride=overrideSource;
    resetMediaPlayback(index);invalidate(index);
  }
#include "detail/MediaPlaybackRuntime.inc.h"
  InstrumentState &instrument(int i) { auto &v=state()[i].instrument;if(v.use_count()!=1)v=std::make_shared<InstrumentState>(*v);return *v; }
  int routeIndex(const char *path) const {
    if(!project||!path)return -1;
    for(uint16_t i=0;i<project->count;++i)if(!strcmp(project->pages[i].path,path))return i;
    return -1;
  }
  const NavigationGroupDesc *navigationGroup(const char *path,int *index=nullptr) const {
    if(!navigation||!path)return nullptr;
    for(uint16_t i=0;i<navigation->groupCount;++i){const auto &g=navigation->groups[i];
      for(uint16_t j=0;j<g.count;++j)if(!strcmp(g.paths[j],path)){if(index)*index=j;return &g;}
    }
    return nullptr;
  }
  Rect navigationViewport(const char *from,const char *to=nullptr) const {
    const auto *g=navigationGroup(from);
    Rect viewport=!g||(to&&navigationGroup(to)!=g)?Rect{0,0,page().width,page().height}:Rect{g->insetLeft,g->insetTop,int32_t(page().width)-g->insetLeft-g->insetRight,int32_t(page().height)-g->insetTop-g->insetBottom};
    const auto barBottom=[&](const char *path)->int32_t{const int index=routeIndex(path);if(index<0||systemBarRoots.empty())return 0;const auto &p=project->pages[index];
      for(uint16_t i=0;i<p.count;++i)if(std::binary_search(systemBarRoots.begin(),systemBarRoots.end(),p.nodes[i].id)){
        const auto &node=p.nodes[i];const bool hidden=pages[index].empty()?(node.flags&Hidden)!=0:(pages[index][i].flags&Hidden)!=0;
        if(!hidden&&node.rect.y<=0&&node.rect.x<=0&&node.rect.width>=p.width)return node.rect.y+node.rect.height;
      }return 0;};
    const int32_t sourceBar=barBottom(from),targetBar=to?barBottom(to):sourceBar;
    if(sourceBar>0&&targetBar>0){const int32_t bottom=viewport.y+viewport.height;viewport.y=std::min(bottom,std::max(viewport.y,std::max(sourceBar,targetBar)));viewport.height=bottom-viewport.y;}
    return viewport;
  }
  Rect transitionViewport() const {return transition.viewport.width?transition.viewport:Rect{0,0,page().width,page().height};}
  void invalidateMotion(Rect r){refresh->invalidate(DirtyRect(r.x,r.y,r.width,r.height));}
  Rect navigationFooter(const NavigationGroupDesc &g) const {
    return {int32_t(g.insetLeft)+g.indicatorInset,int32_t(page().height)-g.insetBottom-g.bottom-g.indicatorHeight,int32_t(page().width)-g.insetLeft-g.insetRight-2*g.indicatorInset,g.indicatorHeight};
  }
  int navigationDotAt(const NavigationGroupDesc &g,int32_t px,int32_t py) const {
    const Rect r=navigationFooter(g);if(!contains(r,px,py))return -1;
    const int step=int(g.dotSize)+g.dotGap,left=r.x+(r.width-(int(g.count)*step-g.dotGap))/2;
    if(px<left-int(g.dotGap)/2||px>=left+int(g.count)*step-int(g.dotGap)/2)return -1;
    return (px-left+int(g.dotGap)/2)/step;
  }
  bool validNavigation(const NavigationDesc &config) const {
    if(!project||config.groupCount>32||config.gestureCount>128||(config.groupCount&&!config.groups)||(config.gestureCount&&!config.gestures))return false;
    std::vector<Id> ids;std::vector<const char *> usedPaths;
    for(uint16_t i=0;i<config.groupCount;++i){const auto &g=config.groups[i];
      if(!g.id||std::find(ids.begin(),ids.end(),g.id)!=ids.end()||!g.paths||g.count<2||g.count>128||g.count>project->count)return false;ids.push_back(g.id);
      if(g.indicatorHeight&&(!g.dotSize||g.indicatorHeight<g.dotSize))return false;
      if(uint32_t(g.insetLeft)+g.insetRight>=page().width||uint32_t(g.insetTop)+g.insetBottom>=page().height)return false;
      const uint32_t width=page().width-g.insetLeft-g.insetRight,height=page().height-g.insetTop-g.insetBottom;
      if(uint32_t(g.indicatorInset)*2>=width||uint32_t(g.indicatorHeight)+g.bottom>=height||(g.indicatorHeight&&uint32_t(g.count)*(g.dotSize+g.dotGap)-g.dotGap>width-2*g.indicatorInset))return false;
      for(uint16_t j=0;j<g.count;++j){if(routeIndex(g.paths[j])<0)return false;for(const char *p:usedPaths)if(!strcmp(p,g.paths[j]))return false;usedPaths.push_back(g.paths[j]);}
    }
    for(uint16_t i=0;i<config.gestureCount;++i){const auto &g=config.gestures[i];
      if(!g.id||std::find(ids.begin(),ids.end(),g.id)!=ids.end()||uint8_t(g.direction)>uint8_t(SwipeDirection::Down)||uint8_t(g.edge)>uint8_t(SwipeEdge::Bottom)||g.action==RouteAction::None||uint8_t(g.action)>uint8_t(RouteAction::Home)||!g.threshold||!g.edgeSize)return false;ids.push_back(g.id);
      if(g.overrideControls&&g.edge==SwipeEdge::Any)return false;
      if(uint8_t(g.interaction)>uint8_t(GestureInteraction::Instant)||(g.transition&&!validTransition(*g.transition)))return false;
      if(g.interaction==GestureInteraction::Follow&&g.transition&&g.transition->effect!=swipeEffect(g.direction))return false;
      if(g.source&&routeIndex(g.source)<0)return false;
      if((g.action==RouteAction::Push||g.action==RouteAction::Replace)&&routeIndex(g.route)<0)return false;
      if((g.action==RouteAction::Back||g.action==RouteAction::Home)&&g.route&&*g.route)return false;
      for(uint16_t j=0;j<i;++j){const auto &old=config.gestures[j];if(old.direction==g.direction&&old.edge==g.edge&&((!old.source&&!g.source)||(old.source&&g.source&&!strcmp(old.source,g.source))))return false;}
    }
    return true;
  }
  const GestureBindingDesc *swipeBinding(SwipeDirection direction,bool explicitEdge) const {
    if(!navigation)return nullptr;const GestureBindingDesc *fallback=nullptr;
    for(uint16_t i=0;i<navigation->gestureCount;++i){const auto &g=navigation->gestures[i];
      if(g.direction!=direction||(g.edge!=SwipeEdge::Any)!=explicitEdge||(g.source&&strcmp(g.source,page().path)))continue;
      bool match=g.edge==SwipeEdge::Any||(g.edge==SwipeEdge::Left&&startX<g.edgeSize)||(g.edge==SwipeEdge::Right&&startX>=int32_t(page().width)-g.edgeSize)||(g.edge==SwipeEdge::Top&&startY<g.edgeSize)||(g.edge==SwipeEdge::Bottom&&startY>=int32_t(page().height)-g.edgeSize);
      if(!match||startX<0||startY<0||startX>=page().width||startY>=page().height)continue;
      if(g.source)return &g;if(!fallback)fallback=&g;
    }
    return fallback;
  }
  void initializePage(uint16_t index) {
    if(!pages[index].empty())return;const uint16_t previous=history.back();history.back()=index;initializeActive();restoreSystemPanelState(index);history.back()=previous;
  }
  static bool verticalSwipe(SwipeDirection direction) {return direction==SwipeDirection::Up||direction==SwipeDirection::Down;}
  static int swipeSign(SwipeDirection direction) {return direction==SwipeDirection::Left||direction==SwipeDirection::Up?-1:1;}
  static TransitionEffect swipeEffect(SwipeDirection direction) {
    return direction==SwipeDirection::Left?TransitionEffect::SlideLeft:direction==SwipeDirection::Right?TransitionEffect::SlideRight:direction==SwipeDirection::Up?TransitionEffect::SlideUp:TransitionEffect::SlideDown;
  }
  int gestureDestination(const GestureBindingDesc &g) const {
    if(g.action==RouteAction::Home)return project->initial;
    if(g.action==RouteAction::Back)return history.size()>1?history[history.size()-2]:-1;
    if(g.action==RouteAction::Push&&history.size()>=64)return -1;
    return routeIndex(g.route);
  }
  int32_t motionDistance() const {const Rect v=navigationViewport(page().path,groupMotion.destination>=0?project->pages[groupMotion.destination].path:nullptr);return groupMotion.vertical?v.height:v.width;}
  bool prepareMotion(int destination,uint32_t now,RouteAction action,bool vertical,int sign,const TransitionDesc &config,bool follow) {
    if(destination<0||destination==history.back())return false;
    groupMotion=GroupMotion{};groupMotion.destination=destination;groupMotion.started=groupMotion.updated=now;groupMotion.active=true;
    groupMotion.action=action;groupMotion.vertical=vertical;groupMotion.sign=sign;groupMotion.config=config;groupMotion.follow=follow;
    groupMotion.sharedBackdrop=sharesMediaWith(uint16_t(destination));
    const char *from=page().path,*to=project->pages[destination].path;
    bool allowed=routeEvent(EventType::BeforeNavigate,from,to,action);
    if(allowed&&down&&guard){guarding=true;allowed=guard(guardContext,from,to,action);guarding=false;}
    if(!down){groupMotion=GroupMotion{};routeEvent(EventType::NavigationCancel,from,to,action);return false;}
    groupMotion.prepared=allowed;
    if(allowed&&!groupMotion.sharedBackdrop){
      saveSystemPanelState(history.back());
      // Follow gestures compose two immutable page poses, just like ordinary
      // paired transitions. Live animation/setter changes must not make the
      // outgoing page miss its retained surface at every pointer sample.
      groupMotion.outgoing=state();
      groupMotion.outgoingMotions=pageMotions[history.back()];
      initializePage(uint16_t(destination));
      // Interactive previews show the complete target, even when a previous
      // visit left its entry/exit animation frozen at a transparent pose.
      const uint16_t previous=history.back();history.back()=uint16_t(destination);
      enterAnimations(false);history.back()=previous;refreshMotionPending();
    }else if(!allowed){routeEvent(EventType::NavigationCancel,from,to,action);if(follow){groupMotion=GroupMotion{};return false;}}
    return down;
  }
  bool prepareGroup(int destination,uint32_t now) {
    int sourceIndex=-1,destinationIndex=-1;navigationGroup(page().path,&sourceIndex);
    if(destination>=0)navigationGroup(project->pages[destination].path,&destinationIndex);
    const int sign=destinationIndex>sourceIndex?-1:1;
    const TransitionDesc config{sign<0?TransitionEffect::SlideLeft:TransitionEffect::SlideRight,AnimationEasing::EaseOut,180};
    return prepareMotion(destination,now,RouteAction::Replace,false,sign,config,false);
  }
  void claimGesture(const GestureBindingDesc *binding,uint32_t now) {
    gesture=binding;drag=Drag::Gesture;cancelTarget();
    if(!down||binding->interaction!=GestureInteraction::Follow)return;
    const TransitionDesc config=binding->transition?*binding->transition:TransitionDesc{swipeEffect(binding->direction),AnimationEasing::EaseOut,180};
    prepareMotion(gestureDestination(*binding),now,binding->action,verticalSwipe(binding->direction),swipeSign(binding->direction),config,true);
  }
  void settleMotion(bool commit,uint32_t now) {
    groupMotion.commit=commit&&groupMotion.prepared;
    if(groupMotion.sharedBackdrop){
      if(!groupMotion.commit){abortGroup();return;}
      // Retain gesture thresholds/cancelability, then commit on the next UI
      // tick after pointer-up cleanup. No full-page surface moves the backdrop.
      groupMotion.offset=groupMotion.from=groupMotion.to=0;groupMotion.started=now;
      groupMotion.duration=0;groupMotion.settling=true;return;
    }
    groupMotion.from=groupMotion.offset;groupMotion.to=groupMotion.commit?groupMotion.sign*motionDistance():0;
    groupMotion.started=groupMotion.updated=now;groupMotion.settling=true;
    const uint32_t distance=uint32_t(abs(groupMotion.to-groupMotion.from));
    groupMotion.duration=groupMotion.follow?uint16_t(std::max<uint32_t>(1,(uint32_t(groupMotion.config.duration)*distance+motionDistance()-1)/motionDistance())):180;
    refresh->invalidateAll();
  }
  void abortGroup(bool notify=true) {
    if(!groupMotion.active)return;
    const int destination=groupMotion.destination;const bool prepared=groupMotion.prepared;const RouteAction action=groupMotion.action;groupMotion=GroupMotion{};refresh->invalidateAll();
    if(notify&&prepared)routeEvent(EventType::NavigationCancel,page().path,project->pages[destination].path,action);
  }
  void finishGroup() {
    const int destination=groupMotion.destination;const bool commit=groupMotion.commit&&groupMotion.prepared;
    if(!commit){abortGroup();return;}
    const char *from=page().path;const RouteAction action=groupMotion.action;
    groupMotion=GroupMotion{};processing=true;
    // The interactive slide has already revealed the target's complete pose.
    commitRoute(destination,action,from,false,true);endProcessing();
  }
  int find(Id id) const {
    ++stats.lookupQueries;if(!project||!id)return -1;
    const auto &p=page();
    if(lookupPage!=&p) {
      lookup.resize(p.count);for(uint16_t i=0;i<p.count;++i)lookup[i]=i;
      std::sort(lookup.begin(),lookup.end(),[&](uint16_t a,uint16_t b){return p.nodes[a].id<p.nodes[b].id;});lookupPage=&p;
    }
    size_t first=0,last=lookup.size();
    while(first<last){const size_t middle=first+(last-first)/2;++stats.lookupComparisons;const Id candidate=p.nodes[lookup[middle]].id;if(candidate<id)first=middle+1;else last=middle;}
    if(first<lookup.size()){++stats.lookupComparisons;if(p.nodes[lookup[first]].id==id)return lookup[first];}
    return -1;
  }
  int parent(int i) const { return page().nodes[i].parent==NoParent ? -1 : int(page().nodes[i].parent); }
  void prepareComponents() const {
    if(!project||componentsPage==&page())return;
    componentLinks.clear();componentNodes.clear();componentsPage=&page();if(!componentCount)return;
    for(size_t c=0;c<componentCount;++c){const auto &d=components[c];const int root=find(d.id);if(root<0)continue;
      componentLinks.push_back({&d,root,d.valueId?find(d.valueId):-1,d.textId?find(d.textId):-1,d.pageId?find(d.pageId):-1,d.expandedId?find(d.expandedId):-1});}
    if(componentLinks.empty())return;
    componentNodes.assign(page().count,-1);
    for(size_t c=0;c<componentLinks.size();++c){const auto &link=componentLinks[c];
      for(int index:{link.root,link.value,link.text,link.pager,link.panel})if(index>=0)componentNodes[size_t(index)]=int32_t(c);
      for(uint8_t n=0;n<link.desc->optionCount;++n){const int index=find(link.desc->options[n].id);if(index>=0)componentNodes[size_t(index)]=int32_t(c);}
    }
  }
  const ComponentLink *componentFor(int index,bool ancestors=false) const {
    if(!componentCount||index<0)return nullptr;prepareComponents();
    for(int at=index;at>=0;at=ancestors?parent(at):-1)if(size_t(at)<componentNodes.size()&&componentNodes[size_t(at)]>=0)return &componentLinks[size_t(componentNodes[size_t(at)])];
    return nullptr;
  }
  const ComponentLink *publicComponent(Id id) const {
    if(!componentCount)return nullptr;
    const int index=find(id);const auto *link=componentFor(index);return link&&link->root==index?link:nullptr;
  }
  int valueTarget(Id id) const {const auto *link=publicComponent(id);return link?(link->value>=0?link->value:link->pager>=0?link->pager:(link->desc->kind==ComponentKind::Select||link->desc->kind==ComponentKind::Choice)?link->root:-1):find(id);}
  int textTarget(Id id) const {const auto *link=publicComponent(id);return link?link->text:find(id);}
  int pageTarget(Id id) const {const auto *link=publicComponent(id);return link&&link->pager>=0?link->pager:find(id);}
  int componentOption(const ComponentLink &link,int index) const {
    for(uint8_t n=0;n<link.desc->optionCount;++n)if(find(link.desc->options[n].id)==index)return n;return -1;
  }
  void updateReadout(int index) {
    const auto *link=componentFor(index);if(!link||link->value!=index||!link->desc->valueFormat||link->text<0)return;
    const char *format=link->desc->valueFormat,*placeholder=strstr(format,"{value}");if(!placeholder)return;
    char text[160];snprintf(text,sizeof(text),"%.*s%ld%s",int(placeholder-format),format,long(state()[index].value),placeholder+7);
    if(state()[link->text].text.set(text))invalidate(link->text);
  }
  void revealComponentSelection(const PageDesc &p,RetainedVector<State> &values,const ComponentDesc &component) {
    if((component.kind!=ComponentKind::Choice&&component.kind!=ComponentKind::Tabs)||values.size()!=p.count)return;
    int root=-1,value=-1,selected=-1;const Id valueId=component.kind==ComponentKind::Tabs?component.pageId:component.id;
    for(uint16_t n=0;n<p.count;++n){if(p.nodes[n].id==component.id)root=n;if(p.nodes[n].id==valueId)value=n;}
    if(root<0||value<0||values[value].value<0||values[value].value>=component.optionCount)return;
    const Id optionId=component.options[values[value].value].id;
    for(uint16_t n=0;n<p.count;++n)if(p.nodes[n].id==optionId){selected=n;break;}
    if(selected<0)return;
    // Selection changes reveal only the hidden edge. A user's later scroll
    // stays untouched because painting never invokes this helper.
    for(uint16_t ancestor=p.nodes[selected].parent;ancestor!=NoParent;ancestor=p.nodes[ancestor].parent)
      if(p.nodes[ancestor].kind==Kind::Scroll&&(values[ancestor].flags&Horizontal)){
        const Rect option=rect(p,values,selected),box=contentBox(rect(p,values,ancestor),styleFor(p,values,ancestor));
        if(box.width<=0)continue;
        const int32_t dx=option.width>=box.width||option.x<box.x?option.x-box.x:option.x+option.width>box.x+box.width?option.x+option.width-box.x-box.width:0;
        int32_t maxX,maxY;extent(p,values,ancestor,maxX,maxY);
        const int32_t next=clamp32(values[ancestor].scrollX+dx,0,maxX);
        if(next!=values[ancestor].scrollX){values[ancestor].scrollX=next;
          if(&p==&page())invalidate(ancestor);else {surfaceWarmPending=!surfaces.empty();++surfaceMutationEpoch;}}
      }
  }
  bool selectOption(const ComponentLink &link,int32_t index) {
    if(index<0||index>=link.desc->optionCount)return false;
    const auto kind=link.desc->kind;if(kind!=ComponentKind::Select&&kind!=ComponentKind::Choice)return false;
    if(state()[link.root].value!=index){state()[link.root].value=index;invalidate(link.root);
      for(uint8_t option=0;option<link.desc->optionCount;++option)invalidate(find(link.desc->options[option].id));
      revealComponentSelection(page(),state(),*link.desc);}
    const int selected=find(link.desc->options[index].id);
    if(link.text>=0&&state()[link.text].text.set(state()[selected].text.c_str()))invalidate(link.text);
    return true;
  }
  int focusTarget(Id id,InputAction action=InputAction::Focus) const {
    const auto *link=publicComponent(id);if(!link)return find(id);
    if(action==InputAction::PagePrevious||action==InputAction::PageNext)return link->pager;
    if(action==InputAction::Increment||action==InputAction::Decrement)return link->value;
    if(link->value>=0)return link->value;
    if(link->desc->kind==ComponentKind::Select)return link->text;
    if(link->desc->kind==ComponentKind::Tabs)return link->pager;
    for(int index=link->root;index<int(page().count);++index){int ancestor=index;while(ancestor>=0&&ancestor!=link->root)ancestor=parent(ancestor);if(ancestor==link->root&&focusable(index,false))return index;}
    return -1;
  }
  bool inSubtree(int index,int root) const {for(int at=index;at>=0;at=parent(at))if(at==root)return true;return false;}
  void expansionFocus(const ComponentLink &link,bool shown) {
    if(link.desc->kind==ComponentKind::Select)invalidate(link.text);
    const bool overlay=(link.desc->flags&ComponentViewportOverlay)!=0;
    if(!input||link.panel<0||(focusNode<0&&!overlay))return;
    int next=-1;
    if(!shown&&inSubtree(focusNode,link.panel))next=focusTarget(link.desc->id);
    else if(shown&&(overlay||inSubtree(focusNode,link.root))) {
      if(link.desc->kind==ComponentKind::Select&&link.desc->optionCount){const int index=clamp32(state()[link.root].value,0,link.desc->optionCount-1);next=find(link.desc->options[index].id);}
      else for(int index=link.panel+1;index<int(page().count);++index)if(inSubtree(index,link.panel)&&focusable(index,false)){next=index;break;}
    }
    if(next<0||!focusable(next,false))return;
    const bool nested=processing;processing=true;setFocus(next);if(!nested)endProcessing();
  }
  // Bounded shared hot styles: reuse common immutable styles, never copy each
  // page/node table back into RAM. Addresses stay stable until unmount.
  static constexpr size_t compiledStyleCapacity=64;
  std::unique_ptr<Style[]> compiledStyles;
  size_t compiledStyleCount=0;uint16_t compiledStyleLimit=0;bool compiledStyleAttempted=false;
  static bool sameCompiledStyle(const Style &a,const Style &b) {
    return a.background==b.background&&a.color==b.color&&a.borderColor==b.borderColor&&a.trackColor==b.trackColor&&a.fillColor==b.fillColor&&a.thumbColor==b.thumbColor&&a.pressedColor==b.pressedColor&&a.disabledColor==b.disabledColor&&a.borderWidth==b.borderWidth&&a.radius==b.radius&&a.fontSize==b.fontSize&&a.textAlign==b.textAlign&&a.paddingTop==b.paddingTop&&a.paddingRight==b.paddingRight&&a.paddingBottom==b.paddingBottom&&a.paddingLeft==b.paddingLeft&&a.trackSize==b.trackSize&&a.thumbWidth==b.thumbWidth&&a.thumbHeight==b.thumbHeight&&a.thumbRadius==b.thumbRadius&&a.font==b.font&&a.gradient==b.gradient&&a.trackRadius==b.trackRadius;
  }
  uint16_t cacheCompiledStyle(const Style &style) {
    if(resourceCacheBudget||displayPolicy.monochrome||displayPolicy.slowRefresh)return NoParent;
    if(!compiledStyleAttempted){
      compiledStyleAttempted=true;size_t nodes=0;
      for(size_t i=0;i<project->count&&nodes<compiledStyleCapacity;++i)nodes+=project->pages[i].count;
      compiledStyleLimit=uint16_t(std::min(nodes,compiledStyleCapacity));
      if(compiledStyleLimit)compiledStyles.reset(new(std::nothrow) Style[compiledStyleLimit]);
    }
    if(!compiledStyles)return NoParent;
    for(size_t i=0;i<compiledStyleCount;++i)if(sameCompiledStyle(compiledStyles[i],style))return uint16_t(i);
    if(compiledStyleCount==compiledStyleLimit)return NoParent;
    compiledStyles[compiledStyleCount]=style;return uint16_t(compiledStyleCount++);
  }
  const Style &styleFor(const PageDesc &p,const RetainedVector<State> &s,int i) const {
    if(s[i].dynamic&&s[i].dynamic->styleOverride)return s[i].dynamic->style;
    return s[i].styleIndex<compiledStyleCount?compiledStyles[s[i].styleIndex]:p.nodes[i].style;
  }
  bool styledFocus() const {return !buildingSurface&&focusStyleCount&&snapshotFocusId;}
  bool focusStyleOwner(Id id) const {for(size_t i=0;i<focusStyleCount;++i)if(focusStyles[i].owner==id)return true;return false;}
  const FocusStyleDesc *focusStyle(int i) const {
    if(!styledFocus())return nullptr;
    for(int node=i;node>=0;node=rasterFrame.current.values[node].parent==NoParent?-1:int(rasterFrame.current.values[node].parent))if(rasterFrame.current.values[node].flags&Disabled)return nullptr;
    const Id id=rasterFrame.current.page->nodes[i].id;size_t first=0,last=focusStyleCount;
    while(first<last){const size_t middle=first+(last-first)/2;if(focusStyles[middle].target<id)first=middle+1;else last=middle;}
    for(;first<focusStyleCount&&focusStyles[first].target==id;++first)if(focusStyles[first].owner==snapshotFocusId)return focusStyles+first;
    return nullptr;
  }
  static void applyFocusStyle(const FocusStyleDesc *focus,Style &style,uint16_t &flags) {
    if(!focus)return;const uint16_t f=focus->flags;
    if(f&FocusBackground){style.background=focus->background;flags=(flags&~Transparent)|((f&FocusTransparent)?Transparent:0);}
    if(f&FocusActiveBackground){style.pressedColor=focus->activeBackground;flags&=~GradientActive;flags=(flags&~ActiveTransparent)|((f&FocusActiveTransparent)?ActiveTransparent:0);}
    if(f&FocusColor)style.color=focus->color;
    if(f&FocusBorder)style.borderColor=focus->borderColor;
  }
  static void applyFocusShape(const FocusStyleDesc *focus,ShapeDesc &shape) {
    if(!focus)return;const uint16_t f=focus->flags;
    if(f&FocusFill){shape.fillColor=focus->fillColor;shape.flags=(shape.flags&~ShapeFill)|((f&FocusFillNone)?0:ShapeFill);}
    if(f&FocusStroke){shape.strokeColor=focus->strokeColor;shape.flags=(shape.flags&~ShapeStroke)|((f&FocusStrokeNone)?0:ShapeStroke);}
  }
  bool viewportOverlayRoot(const PageDesc &p,int i) const {
    const Id id=p.nodes[i].id;
    return (componentCount&&!viewportOverlayIds.empty()&&std::binary_search(viewportOverlayIds.begin(),viewportOverlayIds.end(),id))||
      std::binary_search(systemBarRoots.begin(),systemBarRoots.end(),id)||std::binary_search(systemPanelRoots.begin(),systemPanelRoots.end(),id);
  }
  int gestureParent(int i) const {return viewportOverlayRoot(page(),i)?-1:parent(i);}
  uint16_t viewportOverlayDepth(const PageDesc &p,const RetainedVector<State> &values,int i) const {
    uint16_t depth=0,base=0;if(viewportOverlayIds.empty()&&systemBarRoots.empty()&&systemPanelRoots.empty())return depth;
    for(int a=i;a>=0;a=values[a].parent==NoParent?-1:values[a].parent){const Id id=p.nodes[a].id;
      if(componentCount&&std::binary_search(viewportOverlayIds.begin(),viewportOverlayIds.end(),id))++depth;
      if(std::binary_search(systemBarRoots.begin(),systemBarRoots.end(),id))base=std::max<uint16_t>(base,componentOverlayLevels+1);
      if(std::binary_search(systemPanelRoots.begin(),systemPanelRoots.end(),id))base=std::max<uint16_t>(base,2*(componentOverlayLevels+1));
    }
    return base+depth;
  }
  void updateOverlayLevels() {
    componentOverlayLevels=0;
    if(project&&componentCount)for(uint16_t p=0;p<project->count;++p){const auto &page=project->pages[p];for(uint16_t i=0;i<page.count;++i){uint16_t depth=0;for(uint16_t a=i;a!=NoParent;a=page.nodes[a].parent)if(std::binary_search(viewportOverlayIds.begin(),viewportOverlayIds.end(),page.nodes[a].id))++depth;componentOverlayLevels=std::max(componentOverlayLevels,depth);}}
    viewportOverlayLevels=componentOverlayLevels;
    if(!systemBarRoots.empty())viewportOverlayLevels=std::max<uint16_t>(viewportOverlayLevels,2*componentOverlayLevels+1);
    if(!systemPanelRoots.empty())viewportOverlayLevels=std::max<uint16_t>(viewportOverlayLevels,3*componentOverlayLevels+2);
  }
  Rect baseRect(const PageDesc &p,const RetainedVector<State> &s,int i) const {
    Rect r=p.nodes[i].rect;const auto &d=s[i].dynamic;
    if(d&&d->boundsOverride){r.x+=d->offsetX;r.y+=d->offsetY;r.width=d->width;r.height=d->height;}return r;
  }
  Rect layoutRect(const PageDesc &p,const RetainedVector<State> &s,int i) const {
    Rect r=baseRect(p,s,i);
    for(uint16_t child=i,a=s[i].parent;a!=NoParent;child=a,a=s[a].parent){if(viewportOverlayRoot(p,child))break;if(s[a].dynamic&&s[a].dynamic->boundsOverride){r.x+=s[a].dynamic->offsetX;r.y+=s[a].dynamic->offsetY;}}return r;
  }
  Rect childBox(const PageDesc &p,const RetainedVector<State> &s,int i,Rect r) const {
    r=contentBox(r,styleFor(p,s,i));if(s[i].kind==Kind::Pager)r.height=std::max<int32_t>(0,r.height-p.nodes[i].pager.indicatorHeight);return r;
  }
  void dynamicChanged(int i) {
    ++inputLayoutEpoch;
    if(softInput.visible&&i==softInput.node){const Rect r=softInput.rect;refresh->invalidate(DirtyRect(r.x,r.y,r.width,r.height));}
    if(buildKeyboard(softInput))invalidateKeyboardKeys();
  }
  void initializeActive() {
    cacheResourcePage(history.back());
    ensureTickNodes();
    if(!state().empty())return;
#if NICE_GFX_UI_TEXT_LAYOUT_MEMO
    clearTextLayoutMemoPage(&page());
#endif
    state().resize(page().count);
    initializeMotionPage(history.back());
    for(uint16_t i=0;i<page().count;++i){const auto &n=page().nodes[i];auto &v=state()[i];v.text.init(n.kind==Kind::InputField&&n.inputField?n.inputField->value:n.text);v.value=n.value;if(n.kind==Kind::InputField&&n.inputField&&n.inputField->type==InputFieldType::Number){v.value=0;parseInputNumber(v.text.c_str(),n.inputField->decimals,v.value);}if(n.kind==Kind::Spinbox&&!n.inputField)v.implicitInput=std::make_shared<const InputFieldDesc>(spinInputConfig(n));v.flags=n.flags;v.parent=n.parent;v.kind=n.kind;v.styleIndex=cacheCompiledStyle(n.style);if(n.instrument){v.instrument=std::make_shared<InstrumentState>();v.instrument->init(n);}}
    for(uint16_t i=0;i<page().count;++i){if(page().nodes[i].kind==Kind::Pager)setPage(i,state()[i].value,false);else state()[i].value=normalized(i,state()[i].value);auto &v=state()[i];v.switchPosition=v.switchFrom=v.switchTo=v.value?1000:0;}
    for(const auto &entry:pageTextOverrides)if(entry.page==history.back())state()[entry.node].text=entry.text;
    for(const auto &entry:pageProgressOverrides)if(entry.page==history.back())state()[entry.node].value=entry.value;
    for(size_t component=0;component<componentCount;++component)revealComponentSelection(page(),state(),components[component]);
  }
  bool renderingFullFrameSurface=false;
#include "detail/RenderCache.inc.h"
#include "detail/AnimationLayers.inc.h"
#include "detail/TextLayoutMemo.inc.h"
#include "detail/FrameGeometryMemo.inc.h"
#include "detail/NativeRender.inc.h"
#include "detail/SystemModalMappedRows.inc.h"
#include "detail/SystemModalSourceComposition.inc.h"
  bool nativeRequiresLegacySystemPaint()const {
    if(systemChromePanel>=0||systemChromeDragging||systemModalBaseLive.owner||systemModalBaseFrame.owner||
       systemModalPanelLive.owner||systemModalPanelFrame.owner||systemModalPanelBuilding||systemModalPanelOverlap.active||
       !behaviorVisuals.empty()||behaviorVisualMoving())return true;
    if(!project||history.empty())return true;
    for(const auto &pair:systemChromePairs)if(pair.page==history.back())return true;
    return false;
  }
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
#include "detail/NativePresentation.inc.h"
#endif
  Rect poseRect(const PageDesc &p,const RetainedVector<State> &s,int i) const {
    Rect r=layoutRect(p,s,i);r.x+=s[i].animationX;r.y+=s[i].animationY;
    for(uint16_t child=i,a=s[i].parent;a!=NoParent;child=a,a=s[a].parent) {if(viewportOverlayRoot(p,child))break;r.x+=s[a].animationX-s[a].scrollX;r.y+=s[a].animationY-s[a].scrollY;}
    return r;
  }
  Rect rect(const PageDesc &p,const RetainedVector<State> &s,int i) const {
    const Rect r=poseRect(p,s,i);if(!p.nodes[i].motion)return r;
    const auto &m=motionPose(p,s,i);return motion_geometry::bounds(r,m.scale,m.rotation,p.nodes[i].motion->pivotX,p.nodes[i].motion->pivotY,s[i].kind==Kind::Shape);
  }
  Rect rect(int i) const { return rect(page(),state(),i); }
  int32_t pagerStride(int i) const { return contentBox(baseRect(page(),state(),i),styleFor(page(),state(),i)).width; }
  Rect clip(const PageDesc &p,const RetainedVector<State> &s,int i) const {
    Rect result{0,0,p.width,p.height};
    for(uint16_t child=i,a=s[i].parent;a!=NoParent;child=a,a=s[a].parent)
      {if(viewportOverlayRoot(p,child))break;if((s[a].flags&Clip)||scrolling(s[a].kind))result=intersect(result,childBox(p,s,a,poseRect(p,s,a)));
       if(p.nodes[a].motion){const auto &m=motionPose(p,s,a);if(m.reveal<1024)result=intersect(result,motion_geometry::reveal(poseRect(p,s,a),m.reveal,p.nodes[a].motion->flags));}}
    if(p.nodes[i].motion){const auto &m=motionPose(p,s,i);if(m.reveal<1024&&m.scale==1024&&m.rotation%360==0)result=intersect(result,motion_geometry::reveal(poseRect(p,s,i),m.reveal,p.nodes[i].motion->flags));}
    return result;
  }
  uint8_t opacity(const PageDesc &p,const RetainedVector<State> &s,int i) const {
    uint32_t result=255;
    for(int a=i;a>=0;a=s[a].parent==NoParent?-1:s[a].parent){
      result=result*s[a].opacity/255;
      const Id id=p.nodes[a].id;
      if(std::binary_search(systemBarRoots.begin(),systemBarRoots.end(),id)||
         std::binary_search(systemPanelRoots.begin(),systemPanelRoots.end(),id))break;
    }
    return uint8_t(result);
  }
  bool startAnimation(int i,uint32_t now,bool pending=false) {
    if(i<0)return false;
    const NodeDesc &n=page().nodes[i];
    if(n.animation.effect==AnimationEffect::None&&(!n.motion||!n.motion->count))return false;
    invalidateSubtree(i);
    State &v=state()[i];resetMotion(i);
    const MotionSample sample=n.motion?sampleMotion(n.animation,n.motion,0):MotionSample{0,0,1024,0,1024,255,false};
    const AnimationSample legacy=n.motion?AnimationSample{sample.x,sample.y,sample.opacity,sample.active}:sampleAnimation(n.animation,0);
    auto *m=liveMotion(i);if(m&&(m->scale!=sample.scale||m->rotation!=sample.rotation||m->reveal!=sample.reveal)){
      m->scale=sample.scale;m->rotation=sample.rotation;m->reveal=sample.reveal;++v.motionRevision;
    }
    v.animationX=legacy.x;v.animationY=legacy.y;v.opacity=legacy.opacity;v.animationAt=now;v.animating=true;v.animationPending=pending;
    invalidateSubtree(i);return true;
  }
  void enterAnimations(bool playEnter=true) {
    motionCompletionPending=false;
    mediaTweens.clear();
    for(uint16_t i=0;i<page().count;++i) {
      State &v=state()[i];resetMediaPlayback(i);v.animationX=v.animationY=0;v.opacity=255;v.animating=v.animationPending=false;
      resetMotion(i);
      v.switchPosition=v.switchFrom=v.switchTo=v.value?1000:0;v.switchPending=false;if(v.instrument){auto &a=instrument(i);a.visual=v.value;a.active=false;}
      if(playEnter&&page().nodes[i].animation.trigger==AnimationTrigger::Enter)startAnimation(i,0,true);
    }
  }
  bool animationsRunning() const {
    if(!project)return false;
    ensureTickNodes();
    for(uint16_t i:animationNodes)if((state()[i].animating||(state()[i].instrument&&state()[i].instrument->active)||state()[i].switchPosition!=state()[i].switchTo)&&visible(page(),state(),i))return true;
    return false;
  }
  Rect shapePaintBounds(const PageDesc &page,const RetainedVector<State> &values,int i,Rect full,bool live=false)const{
#if NICE_GFX_UI_SHAPE_INK_DAMAGE
    if(i>=0&&size_t(i)<values.size()){
      const auto &node=page.nodes[i];const auto &value=values[i];const auto *source=node.shape;
      if(node.kind!=Kind::Shape||value.kind!=Kind::Shape||!source)return full;
      // A completed cache key alone does not own its old motion pose. Narrow
      // retained geometry only while it has the corresponding raster snapshot;
      // idle builders without that proof keep the original full rectangle.
      if(node.motion&&!live && &values!=&rasterFrame.current.values && &values!=&rasterFrame.preview.values && &values!=&transition.outgoing)return full;
      bool disabled=(value.flags&Disabled)!=0;
      for(uint16_t parent=value.parent;parent!=NoParent&&!disabled;parent=values[parent].parent)disabled=(values[parent].flags&Disabled)!=0;
      const auto &style=styleFor(page,values,i);
      if(!disabled&&(value.flags&Transparent)&&!node.instrument&&!value.instrument&&!style.borderWidth&&
         (source->type==ShapeType::Line||source->type==ShapeType::Polyline||source->type==ShapeType::Polygon)){
        const Rect box=poseRect(page,values,i);const auto &pose=motionPose(page,values,i);
        // Bound float arithmetic to the supported ordinary display domain;
        // unusual authored geometry keeps the original conservative rectangle.
        if(box.x>=-4096&&box.y>=-4096&&box.width>0&&box.height>0&&box.width<=4096&&box.height<=4096&&
           int64_t(box.x)+box.width<=8192&&int64_t(box.y)+box.height<=8192&&pose.scale){
          const Rect content=contentBox(box,style);const auto *motion=node.motion;
          const shape_raster::TransformedShape shape(*source,content,box,pose.scale,pose.rotation,style.radius,
            motion?motion->pivotX:50,motion?motion->pivotY:50);
          if(shape.count){
            float left=shape.points[0].x,right=left,top=shape.points[0].y,bottom=top;
            for(uint8_t p=1;p<shape.count;++p){left=std::min(left,shape.points[p].x);right=std::max(right,shape.points[p].x);top=std::min(top,shape.points[p].y);bottom=std::max(bottom,shape.points[p].y);}
            // The renderer uses these exact transformed vertices/capsules.
            // Two extra pixels cover integer scan conversion and float error.
            const float pad=shape.stroke+2;
            if(left-pad>=-32768&&top-pad>=-32768&&right+pad<=32768&&bottom+pad<=32768){
              const int32_t x=int32_t(floorf(left-pad)),y=int32_t(floorf(top-pad));
              const Rect ink{x,y,int32_t(ceilf(right+pad))+1-x,int32_t(ceilf(bottom+pad))+1-y};
              return intersect(full,ink);
            }
          }
        }
      }
    }
#endif
    return full;
  }
  void invalidateAnimationSubtree(int i){
#if NICE_GFX_UI_SHAPE_INK_DAMAGE && NICE_GFX_UI_SUBTREE_DAMAGE_RANGES
    DirtyRect pending;
    if(refresh->pendingRegions()==1&&refresh->pendingRegion(0,pending)&&!pending.x&&!pending.y&&pending.width==page().width&&pending.height==page().height){invalidateSubtree(i);return;}
    // Other setters retain their original full old/new rectangles, including
    // changes to focus or style. Only a pure animated vector leaf is narrowed.
    if(i>=0&&size_t(i)<state().size()&&prepareSubtreeRanges()&&subtreeEnds[i]==i+1&&
       !behaviorVisualMoving()&&!styledFocus()&&snapshotPressed!=i&&!focusStyle(i)){
      const Rect complete=rect(i),clipped=clip(page(),state(),i),full=intersect(complete,clipped);
      const Rect damage=intersect(shapePaintBounds(page(),state(),i,complete,true),clipped);
      if(uint64_t(damage.width)*damage.height<uint64_t(full.width)*full.height){
        surfaceWarmPending=!surfaces.empty();++surfaceMutationEpoch;++stats.shapeDamageClips;
        stats.shapeDamageSavedPixels+=uint64_t(full.width)*full.height-uint64_t(damage.width)*damage.height;
        invalidatePageRect(damage);return;
      }
    }
#endif
    invalidateSubtree(i);
  }
  void advanceAnimations(uint32_t now) {
    ProfileTimer animationTimer(*this,&RuntimeStats::animationMicros);
    ensureTickNodes();
    for(uint16_t i:animationNodes) {
      State &v=state()[i];
      if(v.instrument&&v.instrument->active){
        if(displayPolicy.slowRefresh){auto &a=instrument(i);a.visual=v.value;a.active=false;invalidate(i);}
        else if(instrument(i).advance(page().nodes[i],v.value,now))invalidate(i);
      }
      if(v.switchPosition!=v.switchTo) {
        if(v.switchPending){v.switchAt=now;v.switchPending=false;}
        const uint32_t elapsed=uint32_t(now-v.switchAt);
        const int64_t remaining=displayPolicy.slowRefresh||elapsed>=180?0:180-elapsed;
        const uint16_t position=uint16_t(v.switchTo+(int64_t(v.switchFrom)-v.switchTo)*remaining*remaining*remaining/(180*180*180));
        if(v.switchPosition!=position){v.switchPosition=position;invalidate(i);}
      }
      if(!v.animating||!visible(page(),state(),i))continue;
      if(v.animationPending){v.animationAt=now;v.animationPending=false;}
      auto *m=liveMotion(i);
      const NodeDesc &n=page().nodes[i];
      AnimationDesc a=m&&m->hiding?*n.exitAnimation:n.animation;
      // Sampling an infinite animation at its terminal pose does not finish it.
      const bool finiteCompletion=a.repeat!=0;
      uint32_t elapsed=uint32_t(now-v.animationAt);
      const bool terminal=menuMotionConfig.preset==MenuPreset::Reduced||displayPolicy.terminalAnimation(a,n.motion)&&
        !(displayPolicy.monochrome&&!displayPolicy.slowRefresh&&monochromeEffects==MonochromeEffects::Bayer4);
      if(terminal){if(!a.repeat)a.repeat=1;elapsed=uint32_t(a.delay)+uint32_t(a.duration)*a.repeat;}
      // Rebase infinite cycles after their one-time delay, surviving clock rollover.
      if(!terminal&&!a.repeat&&a.duration&&elapsed>=uint32_t(a.delay)+a.duration) {
        uint32_t cycles=(elapsed-a.delay)/a.duration;
        if(n.motion&&(n.motion->flags&MotionAlternate))cycles&=~uint32_t(1);
        v.animationAt+=cycles*a.duration;elapsed=uint32_t(now-v.animationAt);
      }
      MotionSample sample;
      if(m&&m->hiding){const auto exit=sampleAnimation(a,elapsed);sample={m->baseX+exit.x,m->baseY+exit.y,m->scale,m->rotation,m->reveal,uint8_t(uint32_t(m->baseOpacity)*exit.opacity/255),exit.active};}
      else if(n.motion)sample=sampleMotion(a,n.motion,elapsed);
      else {const auto legacy=sampleAnimation(a,elapsed);sample={legacy.x,legacy.y,1024,0,1024,legacy.opacity,legacy.active};}
      const bool poseChanged=m&&(m->scale!=sample.scale||m->rotation!=sample.rotation||m->reveal!=sample.reveal);
      const bool changed=v.animationX!=sample.x||v.animationY!=sample.y||v.opacity!=sample.opacity||poseChanged;
      if(changed)invalidateAnimationSubtree(i);
      v.animationX=sample.x;v.animationY=sample.y;v.opacity=sample.opacity;v.animating=sample.active;
      if(poseChanged){m->scale=sample.scale;m->rotation=sample.rotation;m->reveal=sample.reveal;++v.motionRevision;}
      if(changed)invalidateAnimationSubtree(i);
      if(!sample.active&&((m&&m->hiding)||(finiteCompletion&&hasAnimationEndListener(i))))finishMotionSample(i);
    }
  }
  bool visible(const PageDesc &p,const RetainedVector<State> &s,int i,bool input=false) const {
    for(int a=i;a>=0;a=s[a].parent==NoParent?-1:s[a].parent)
      if((s[a].flags&Hidden)||(input&&(s[a].flags&Disabled))) return false;
    return true;
  }
  bool pointerAvailable() const {
    // Labels may activate a control in a different subtree. Both ends, and
    // the drag owner, must remain enabled and visible for the whole sequence.
    const int modal=modalRoot();bool layer=!coordinated()||contactModal==modal;
    if(systemChromePanel>=0&&systemChromePage==history.back()){
      if(modal==systemChromePanel&&(contactModal<0||contactModal==systemChromePanel))layer=true;
      else if(systemChromeDragging)layer=false;
    }
    return layer&&
      (target<0||visible(page(),state(),target,true))&&
      (activeControl<0||visible(page(),state(),activeControl,true))&&
      (owner<0||visible(page(),state(),owner,true));
  }
  void invalidatePageRect(Rect r) {
    // Keep business/animation state and cached-surface keys current, but do
    // not transfer pixels hidden by the current modal keyboard. Hide/cancel
    // already invalidate the whole page, including updates received during DMA.
    if(softInput.visible&&softInput.node>=0&&softInput.rect.width==page().width&&softInput.rect.height>0&&
       !transition.active&&!groupMotion.active&&!backPrepared)
      r=intersect(r,{0,0,page().width,softInput.rect.y});
    if(r.width>0&&r.height>0)refresh->invalidate(DirtyRect(r.x,r.y,r.width,r.height));
  }
#include "detail/MediaOcclusion.inc.h"
  bool systemBarDamageOccluded(int index,Rect area) {
    if(!project||history.empty()||systemChromePairs.empty()||renderBudget.maxPixelsPerTick||refresh->busy()||processing||down||buildingSurface||foregroundCapture||
       transition.active||groupMotion.active||backPrepared||backAnimation.active||
       softInput.visible||softInput.node>=0||suspendedSystemInput||editNode>=0||focusStyleCount||
       index<0||area.width<=0||area.height<=0)return false;
    const auto &p=page();const auto &values=state();
    if(values.size()!=p.count||size_t(index)>=values.size())return false;
    const auto &node=p.nodes[index];const auto &value=values[index];
    if(node.kind!=value.kind||(value.kind!=Kind::Text&&value.kind!=Kind::Label&&value.kind!=Kind::Progress)||
       node.motion||node.instrument||value.instrument||value.imageOverride)return false;
    for(const auto &pair:systemChromePairs)if(pair.page==history.back()&&pair.bar<p.count&&pair.panel<p.count&&descendant(index,pair.bar)){
      if(componentFor(index,true))return false;
      const auto *motion=behavior(p.nodes[pair.panel].id);
      if(!motion||!motion->open||motion->closing||motion->dragging||motion->pending||motion->paused||
         !systemPanelMotionSafe(*motion))return false;
      if(p.nodes[pair.bar].parent!=0||values[pair.bar].parent!=0||
         p.nodes[pair.bar].kind!=Kind::Container||values[pair.bar].kind!=Kind::Container||
         !visible(p,values,pair.bar)||!visible(p,values,pair.panel)||opacity(p,values,pair.panel)!=255)return false;
      for(int at=index;at>=0;at=parent(at)){
        if(values[at].parent!=p.nodes[at].parent||p.nodes[at].motion)return false;
        if(at==pair.bar)break;
        if(viewportOverlayRoot(p,at))return false;
      }
      for(int at=pair.panel;at>=0;at=parent(at))if(values[at].parent!=p.nodes[at].parent||
        (values[at].flags&(Hidden|Disabled))||p.nodes[at].motion)return false;
      const auto &panel=values[pair.panel];const auto &desc=p.nodes[pair.panel];
      const auto &style=styleFor(p,values,pair.panel);
      if(panel.flags&(Transparent|ActiveTransparent|DisabledTransparent)||desc.instrument||panel.instrument||
         desc.animation.effect!=AnimationEffect::None||desc.exitAnimation||style.radius||style.gradient||style.borderWidth)return false;
      const Rect full{0,0,p.width,p.height},bounds=rect(pair.panel);
      if(!modalBaseSameRect(bounds,full)||!modalBaseSameRect(motion->visual,full)||
         !opaqueCoverContains(clip(p,values,pair.panel),DirtyRect(0,0,p.width,p.height),0,0)||
         !opaqueCoverContains(full,DirtyRect(area.x,area.y,area.width,area.height),0,0))return false;
      const auto panelLayer=viewportOverlayDepth(p,values,pair.panel);
      if(viewportOverlayDepth(p,values,pair.bar)>=panelLayer||viewportOverlayDepth(p,values,index)>=panelLayer)return false;
      return true;
    }
    return false;
  }
  void invalidate(int i) {
    if(i<0) return;
    surfaceWarmPending=!surfaces.empty();++surfaceMutationEpoch;
    Rect r=intersect(rect(i),clip(page(),state(),i));
    // Hidden status fields still update state and retained keys. The ordinary
    // panel movement/close damage exposes their current values when needed.
    if(systemBarDamageOccluded(i,r))return;
    if(!invalidateVisibleMedia(i,r))invalidatePageRect(r);
  }
  void invalidateSubtree(int i) {
    if(i<0)return;
    surfaceWarmPending=!surfaces.empty();++surfaceMutationEpoch;
    size_t end=page().count;bool ranged=false;
#if NICE_GFX_UI_SUBTREE_DAMAGE_RANGES
    DirtyRect pending;
    if(refresh->pendingRegions()==1&&refresh->pendingRegion(0,pending)&&!pending.x&&!pending.y&&pending.width==page().width&&pending.height==page().height){++stats.subtreeFullSkips;return;}
    if(size_t(i)<end&&prepareSubtreeRanges()&&subtreeEnds[i]!=NoParent){ranged=true;end=subtreeEnds[i];++stats.subtreeRangeHits;}
#endif
    if(!ranged)++stats.subtreeRangeFallbacks;
    // One bounding region also covers children extending beyond the parent.
    Rect bounds{0,0,0,0};bool any=false;
    for(uint16_t j=uint16_t(i);j<end;++j){++stats.subtreeVisitedNodes;
      if(!ranged){int a=j;while(a>=0&&a!=i)a=parent(a);if(a!=i)continue;}
      if(!visible(page(),state(),j))continue;
      Rect r=intersect(rect(j),clip(page(),state(),j));if(r.width<=0||r.height<=0)continue;
      if(!any){bounds=r;any=true;}else{const int32_t right=std::max(bounds.x+bounds.width,r.x+r.width),bottom=std::max(bounds.y+bounds.height,r.y+r.height);bounds.x=std::min(bounds.x,r.x);bounds.y=std::min(bounds.y,r.y);bounds.width=right-bounds.x;bounds.height=bottom-bounds.y;}
    }
    if(any)invalidatePageRect(bounds);
  }
  int hit(int32_t px,int32_t py) const {
    const int modal=modalRoot();
    for(int level=viewportOverlayLevels;level>=0;--level)for(int i=int(page().count)-1;i>=0;--i)
      if(viewportOverlayDepth(page(),state(),i)==level)
      if(inInputLayer(i,modal)&&policy(i).pointerEvents&&visible(page(),state(),i,true)&&opacity(page(),state(),i)>0&&contains(rect(i),px,py)&&contains(clip(page(),state(),i),px,py)) {
        if(page().nodes[i].motion){const auto &m=motionPose(page(),state(),i);if(!motion_geometry::contains(poseRect(page(),state(),i),m.scale,m.rotation,px,py,m.reveal,page().nodes[i].motion->flags,page().nodes[i].motion->pivotX,page().nodes[i].motion->pivotY,page().nodes[i].kind==Kind::Shape))continue;}
        return i;
      }
    return -1;
  }
  bool nodeContains(int i,int32_t px,int32_t py) const {
    if(i<0||!contains(rect(i),px,py)||!contains(clip(page(),state(),i),px,py))return false;
    if(!page().nodes[i].motion)return true;
    const auto &m=motionPose(page(),state(),i);
    return motion_geometry::contains(poseRect(page(),state(),i),m.scale,m.rotation,px,py,m.reveal,page().nodes[i].motion->flags,page().nodes[i].motion->pivotX,page().nodes[i].motion->pivotY,page().nodes[i].kind==Kind::Shape);
  }
  int nearestControl(int i) const { for(;i>=0;i=parent(i)) { if(page().nodes[i].kind==Kind::Label&&page().nodes[i].targetId) { int target=find(page().nodes[i].targetId);return target>=0&&visible(page(),state(),target,true)?target:-1; } if(control(page().nodes[i].kind)) return i; if(const auto *link=componentFor(i))if(link->root==i&&link->desc->kind==ComponentKind::ImageButton)return i; } return -1; }
  int activationBoundary(int i) const {
    for(;i>=0;i=parent(i)){
      const auto &n=page().nodes[i];
      if(control(n.kind)||(n.kind==Kind::Label&&n.targetId))return i;
      if(const auto *link=componentFor(i))if(link->root==i&&link->desc->kind==ComponentKind::ImageButton)return i;
    }
    return -1;
  }
  int nearestNavigation(int i) const {
    const int boundary=coordinated()?activationBoundary(i):-1;
    for(;i>=0;i=parent(i)){if(page().nodes[i].routeAction!=RouteAction::None&&visible(page(),state(),i,true))return i;if(i==boundary)break;}
    return -1;
  }
  const FocusDesc *focusDesc(int i) const {
    if(!input||i<0)return nullptr;
    if(focusIndexConfig!=input) {
      focusIndex.resize(input->focusCount);for(size_t n=0;n<input->focusCount;++n)focusIndex[n]=n;
      std::sort(focusIndex.begin(),focusIndex.end(),[&](size_t a,size_t b){return input->focus[a].id!=input->focus[b].id?input->focus[a].id<input->focus[b].id:a<b;});focusIndexConfig=input;
    }
    const Id id=page().nodes[i].id;size_t first=0,last=focusIndex.size();
    while(first<last){const size_t middle=first+(last-first)/2;if(input->focus[focusIndex[middle]].id<id)first=middle+1;else last=middle;}
    if(first<focusIndex.size()&&input->focus[focusIndex[first]].id==id)return &input->focus[focusIndex[first]];
    return nullptr;
  }
  int scopeFor(int i) const { for(int child=i,a=parent(i);a>=0;child=a,a=parent(a)){if(viewportOverlayRoot(page(),child))break;if(scrolling(page().nodes[a].kind)) { const auto *f=focusDesc(a);if(page().nodes[a].kind==Kind::Scroll&&f&&!f->enabled)continue;return a; }}return -1; }
  int32_t scrollReadingStep(int i,bool horizontal) const {
    const Style *style=&styleFor(page(),state(),i);
    // A reader normally has one Text child. Bound fallback sampling so a key
    // never scans an arbitrarily long menu just to choose its line height.
    for(int next=i+1,end=std::min<int>(int(page().count),i+17);next<end;++next){
      if(!descendant(next,i))break;const auto kind=page().nodes[next].kind;
      if((kind==Kind::Text||kind==Kind::Label)&&visible(page(),state(),next)){style=&styleFor(page(),state(),next);break;}
    }
    return style->font?std::max<int32_t>(1,(horizontal?style->font->width:style->font->height)*style->fontSize):(horizontal?8:16);
  }
  bool focusable(int i,bool inScope=true) const {
    if(i<0||!inInputLayer(i)||!visible(page(),state(),i,true)||opacity(page(),state(),i)==0)return false;
    const auto *f=focusDesc(i);const auto &n=page().nodes[i];
    if(state()[i].focusOverride==0)return false;
    if(state()[i].focusOverride<0&&(f?!f->enabled:!(control(n.kind)||scrolling(n.kind)||n.routeAction!=RouteAction::None||focusStyleOwner(n.id))))return false;
    const Rect own=baseRect(page(),state(),i);if(own.width<=0||own.height<=0)return false;
    Rect revealCandidate=behaviorFocusRect(i);
    for(int child=i,a=parent(i);a>=0;child=a,a=parent(a)) {
      if(viewportOverlayRoot(page(),child))break;
      if(page().nodes[a].kind==Kind::Pager) {
        int index=0;for(int k=a+1;k<child;++k)if(page().nodes[k].parent==a)++index;
        if(index!=state()[a].value)return false;
      }
      if(scrolling(page().nodes[a].kind))revealCandidate=behaviorFocusRect(a);
      else if(state()[a].flags&Clip) { const Rect r=intersect(revealCandidate,contentBox(behaviorFocusRect(a),styleFor(page(),state(),a)));if(r.width<=0||r.height<=0)return false; }
    }
    return !inScope||scopeFor(i)==focusScope;
  }
  const InputPage *inputPage() const { if(input)for(size_t i=0;i<input->pageCount;++i)if(!strcmp(input->pages[i].path,page().path))return &input->pages[i];return nullptr; }
  void reveal(int i) {
    if(revealMenuFocus(i))return;
    for(int child=i,a=parent(i);a>=0;child=a,a=parent(a)){if(viewportOverlayRoot(page(),child))break;if(page().nodes[a].kind==Kind::Scroll) {
      const Rect r=rect(i),box=contentBox(rect(a),styleFor(page(),state(),a));
      const int32_t dx=r.x<box.x?r.x-box.x:r.x+r.width>box.x+box.width?r.x+r.width-box.x-box.width:0;
      const int32_t dy=r.y<box.y?r.y-box.y:r.y+r.height>box.y+box.height?r.y+r.height-box.y-box.height:0;
      scroll(a,state()[a].scrollX+dx,state()[a].scrollY+dy);
    }}
  }
  bool setFocus(int i,bool revealTarget=true) {
    if(!focusable(i,false))return false;
    if(editNode>=0&&editNode!=i) { const int old=editNode;editNode=-1;if(!behaviorKeepDraft(old)){setValue(old,editValue);dispatch(old,EventType::Input);}if(!focusable(i,false))return false; }
    const int old=focusNode;focusScope=scopeFor(i);focusNode=i;
    if(pageFocus.size()==project->count)pageFocus[history.back()]=page().nodes[i].id;
    if(revealTarget)reveal(i);aimMenuIndicator();invalidate(old);invalidate(i);
    if(old!=i&&focusStyleCount)refresh->invalidateAll();
    if(focusNode!=i||!focusable(i,false))return false;
    if(old!=i) {
      if(old>=0)dispatch(old,EventType::Blur);
      // Blur callbacks can close an expanded panel, restoring its opener, or
      // hide/disable the requested target. Do not announce stale focus.
      if(focusNode!=i||!focusable(i,false))return false;
      dispatch(i,EventType::Focus);
    }
    return focusNode==i&&focusable(i,false);
  }
  void setKeyInput(bool enabled) {
    if(keyInput==enabled)return;
    keyInput=enabled;
    if(project&&menuMotionEnabled()){if(enabled)aimMenuIndicator(true);else resetMenuMotion();}
    if(project)invalidate(focusNode);
    invalidateKeyboardKey(softInput.keyFocus);
  }
  const std::vector<int> &focusTargets() const {
    focusItems.clear();for(uint16_t i=0;i<page().count;++i)if(focusable(i))focusItems.push_back(i);
    std::sort(focusItems.begin(),focusItems.end(),[&](int a,int b){const auto *fa=focusDesc(a),*fb=focusDesc(b);const int ao=fa?fa->order:0,bo=fb?fb->order:0;return ao!=bo?ao<bo:a<b;});return focusItems;
  }
  void restoreFocus() {
    focusNode=focusScope=editNode=-1;
    applyPageMenuMotion();
    resetMenuMotion();
    compactMenuRows();
    if(!input)return;
    const auto *p=inputPage();const Id id=pageFocus.size()==project->count&&pageFocus[history.back()]?pageFocus[history.back()]:p?p->initial:0;
    const int i=find(id);if(i>=0&&focusable(i,false))setFocus(i);
  }
  bool moveFocus(InputAction action) {
    while(focusScope>=0&&!focusable(focusScope,false))focusScope=scopeFor(focusScope);
    const auto &items=focusTargets();if(items.empty())return false;
    if(!focusable(focusNode))return setFocus(items.front());
    const bool reverse=action==InputAction::Previous||action==InputAction::Up||action==InputAction::Left;
    const auto *f=focusDesc(focusNode);
    Id explicitId=f?(action==InputAction::Up?f->up:action==InputAction::Down?f->down:action==InputAction::Left?f->left:action==InputAction::Right?f->right:0):0;
    if(explicitId) { int i=find(explicitId);return focusable(i,false)&&setFocus(i); }
    const auto *p=inputPage();
    if(p&&p->spatial&&action!=InputAction::Next&&action!=InputAction::Previous) {
      const Rect r=rect(focusNode);int best=-1;int64_t score=INT64_MAX;
      for(int i:items)if(i!=focusNode) { Rect t=rect(i);int64_t dx=2LL*t.x+t.width-2LL*r.x-r.width,dy=2LL*t.y+t.height-2LL*r.y-r.height;
        const bool horizontal=action==InputAction::Left||action==InputAction::Right;const int64_t primary=horizontal?dx:dy,secondary=horizontal?dy:dx;
        if((reverse?primary<0:primary>0)) { int64_t s=primary*primary+4*secondary*secondary;if(s<score){score=s;best=i;} }
      }
      return best>=0&&setFocus(best);
    }
    const auto at=std::find(items.begin(),items.end(),focusNode);int index=at==items.end()?0:int(at-items.begin());
    if(p&&!p->wrapNavigation&&((reverse&&index==0)||(!reverse&&index==int(items.size())-1)))return false;
    if(focusScope>=0&&((reverse&&index==0)||(!reverse&&index==int(items.size())-1)))return setFocus(focusScope);
    index=(index+(reverse?-1:1)+int(items.size()))%int(items.size());return setFocus(items[index]);
  }
  const KeyBinding *keyBinding(Id key,KeyGesture gesture) const {
    if(!input||!project)return nullptr;const KeyBinding *global=nullptr;
    if(const auto *local=inputPage())if(local->bindings)for(size_t i=0;i<local->bindingCount;++i){const auto &binding=local->bindings[i];if(binding.key==key&&binding.gesture==gesture)return &binding;}
    for(size_t i=0;i<input->count;++i) { const auto &b=input->bindings[i];if(b.key!=key||b.gesture!=gesture)continue;if(b.page&&!strcmp(b.page,page().path))return &b;if(!b.page)global=&b; }
    return global;
  }
  bool keyAction(InputAction action,Id targetId,const char *route,uint32_t now) {
    if(action!=InputAction::None&&dismissInputToasts())return true;
    if(behaviorInputBlocked())return true;
    if(!softInput.visible&&focusNode>=0&&focusScope==focusNode&&page().nodes[focusNode].kind==Kind::Scroll&&focusable(focusNode,false)&&(!targetId||targetId==page().nodes[focusNode].id)){
      if(action==InputAction::Back||action==InputAction::Confirm||action==InputAction::Activate){focusScope=scopeFor(focusNode);invalidate(focusNode);return true;}
      const bool previous=action==InputAction::Previous||action==InputAction::Up||action==InputAction::Left;
      const bool next=action==InputAction::Next||action==InputAction::Down||action==InputAction::Right;
      if(previous||next){const bool horizontal=(state()[focusNode].flags&Horizontal)!=0;const int32_t delta=scrollReadingStep(focusNode,horizontal)*(previous?-1:1);return scroll(focusNode,state()[focusNode].scrollX+(horizontal?delta:0),state()[focusNode].scrollY+(horizontal?0:delta));}
    }
    if(action==InputAction::Back&&dismissBehaviorModal())return true;
    if(softInput.visible)return softKeyAction(action);
    const int modal=modalRoot();
    if(modal>=0){
      if(action==InputAction::Back){requestHide(modal);return true;}
      if(action==InputAction::Home||action==InputAction::Push||action==InputAction::Replace||action==InputAction::Callback)return false;
    }
    if(softInput.node>=0&&(action==InputAction::Back||action==InputAction::Home))softCancel();
    if(action==InputAction::None)return true;
    if(action==InputAction::Home)return navigate(nullptr,RouteAction::Home);
    if(action==InputAction::Push||action==InputAction::Replace)return route&&navigate(route,action==InputAction::Push?RouteAction::Push:RouteAction::Replace);
    if(action==InputAction::Callback) { Event e{EventType::KeyAction,EventPhase::Target,targetId,targetId,0,0,0};e.bubbles=false;const auto copy=listeners;invoke(e,targetId,false,EventPhase::Target,*copy);return true; }
    if(action==InputAction::Back) {
      if(editNode>=0) { const int i=editNode;editNode=-1;if(state()[i].value!=editValue){setValue(i,editValue);dispatch(i,EventType::Input);}return true; }
      prepareComponents();
      for(size_t c=componentLinks.size();c>0;--c){const auto &link=componentLinks[c-1];if(link.panel>=0&&visible(page(),state(),link.root,true)&&!(state()[link.panel].flags&Hidden)){requestHide(link.panel);return true;}}
      if(focusScope>=0)return setFocus(focusScope);
      return navigate(nullptr,RouteAction::Back);
    }
    int i=targetId?focusTarget(targetId,action):focusNode;
    if(action==InputAction::Focus)return setFocus(i);
    const bool direction=action==InputAction::Previous||action==InputAction::Next||action==InputAction::Up||action==InputAction::Down||action==InputAction::Left||action==InputAction::Right;
    const bool reverse=action==InputAction::Previous||action==InputAction::Up||action==InputAction::Left||action==InputAction::Decrement||action==InputAction::PagePrevious||action==InputAction::ScrollPrevious;
    if(direction&&editNode<0&&!(i>=0&&page().nodes[i].kind==Kind::Pager&&(action==InputAction::Left||action==InputAction::Right)))return moveFocus(action);
    if(targetId&&(action==InputAction::ScrollPrevious||action==InputAction::ScrollNext)&&i>=0&&page().nodes[i].kind==Kind::Scroll&&visible(page(),state(),i,true)&&inInputLayer(i)){
      const int32_t delta=action==InputAction::ScrollPrevious?-24:24;return scroll(i,state()[i].scrollX+delta,state()[i].scrollY+delta);
    }
    if(!focusable(i,false)) { if(targetId)return false;const auto &items=focusTargets();if(items.empty())return false;setFocus(items.front());i=focusNode; }
    const Kind kind=page().nodes[i].kind;
    if(action==InputAction::Confirm||action==InputAction::Activate) {
      if(kind==Kind::Spinbox)return softBegin(i);
      if(kind==Kind::Range||kind==Kind::Spinbox) { if(editNode==i){editNode=-1;if(state()[i].value!=editValue)dispatch(i,EventType::Change);}else{editNode=i;editValue=state()[i].value;}refresh->invalidateAll();return true; }
      if(scrolling(kind)) { focusScope=i;const auto &items=focusTargets();if(items.empty()){
        if(kind==Kind::Scroll){int32_t mx,my;extent(i,mx,my);if((state()[i].flags&Horizontal)?mx>0:my>0){invalidate(i);return true;}}
        focusScope=scopeFor(i);return false;
      }return setFocus(items.front()); }
      activateNode(i,now,false);return true;
    }
    if((direction&&editNode==i)||action==InputAction::Increment||action==InputAction::Decrement) {
      if(kind!=Kind::Range&&kind!=Kind::Spinbox)return false;const auto &n=page().nodes[i];int32_t old=state()[i].value;
      setValue(i,int32_t(std::max<int64_t>(n.minimum,std::min<int64_t>(n.maximum,int64_t(old)+(reverse?-int64_t(n.step):n.step)))));
      if(old!=state()[i].value){dispatch(i,EventType::Input);if(editNode!=i)dispatch(i,EventType::Change);}return true;
    }
    if((direction&&kind==Kind::Pager)||action==InputAction::PagePrevious||action==InputAction::PageNext)return setPage(i,state()[i].value+(reverse?-1:1),true);
    if(action==InputAction::ScrollPrevious||action==InputAction::ScrollNext)return scroll(i,state()[i].scrollX+(reverse?-24:24),state()[i].scrollY+(reverse?-24:24));
    return false;
  }
  void invoke(Event &event,Id id,bool capture,EventPhase phase,const std::vector<Listener> &copy,bool defaults=false) {
    event.currentTarget=id; event.phase=phase;
    const Listener key{id,event.type,nullptr,nullptr,capture};
    const auto first=std::lower_bound(copy.begin(),copy.end(),key,listenerBefore);
    for(auto it=first;it!=copy.end()&&it->id==id&&it->type==event.type&&it->capture==capture;++it) {
      if(coordinated()&&event.type==EventType::Click&&it->defaultAction!=defaults)continue;
      if(!it->uiAction&&behaviorEditInput(find(event.target),event.type))continue;
      if((defaults||it->scoped)&&!acceptsEvent(event))continue;
      if(event.immediatePropagationStopped)break;
      ++stats.listenerVisits;it->callback(it->context,event);
    }
  }
  static bool listenerBefore(const Listener &a,const Listener &b) {
    if(a.id!=b.id)return a.id<b.id;
    if(a.type!=b.type)return uint8_t(a.type)<uint8_t(b.type);
    return a.capture<b.capture;
  }
  void addListener(const Listener &listener) {
    if(listeners.use_count()!=1)listeners=std::make_shared<std::vector<Listener>>(*listeners);
    const auto at=std::upper_bound(listeners->begin(),listeners->end(),listener,listenerBefore);
    listeners->insert(at,listener);
  }
  bool dispatch(int index,EventType type,int32_t animationEndValue=0) {
    if(index<0||!project) return true;
    const bool wasDown=down;
    if(eventDepth==eventPaths.size())eventPaths.emplace_back();
    auto &path=eventPaths[eventDepth++];path.clear();
    for(int i=index;i>=0;i=parent(i))path.push_back(i);
    // Nested notifications can grow the outer vector, but moving its vectors
    // preserves these element pointers. Each nesting level owns a separate path.
    const int *chain=path.data();const size_t length=path.size();
    struct DepthGuard {size_t &depth;~DepthGuard(){--depth;}} depthGuard{eventDepth};
    Event event{type,EventPhase::Target,page().nodes[index].id,page().nodes[index].id,x,y,state()[index].value};
    if(type==EventType::AnimationEnd)event.value=animationEndValue;
    if(const auto *link=componentFor(index,true)){event.component=link->desc->id;event.optionIndex=componentOption(*link,index);if(index==link->root&&(link->desc->kind==ComponentKind::Select||link->desc->kind==ComponentKind::Choice)&&(type==EventType::Input||type==EventType::Change))event.optionIndex=state()[index].value;if(type==EventType::Input)updateReadout(index);}
    event.bubbles=type!=EventType::Scroll;
    event.cancelable=type==EventType::Click||type==EventType::PointerDown||type==EventType::PointerMove||type==EventType::PointerUp;
    event.scrollX=state()[index].scrollX;event.scrollY=state()[index].scrollY;
    const auto copy=listeners;
    const bool savedPointerDispatch=pointerDispatch;
    pointerDispatch=type==EventType::PointerDown||type==EventType::PointerMove;
    struct PointerDispatchGuard {bool &value;bool previous;~PointerDispatchGuard(){value=previous;}} pointerGuard{pointerDispatch,savedPointerDispatch};
    for(size_t i=length;i>1&&!event.propagationStopped;--i) invoke(event,page().nodes[chain[i-1]].id,true,EventPhase::Capture,*copy);
    if(!event.propagationStopped) {
      invoke(event,event.target,true,EventPhase::Target,*copy);
      if(!event.immediatePropagationStopped)invoke(event,event.target,false,EventPhase::Target,*copy);
    }
    for(size_t i=1;event.bubbles&&i<length&&!event.propagationStopped;++i) invoke(event,page().nodes[chain[i]].id,false,EventPhase::Bubble,*copy);
    if(coordinated()&&type==EventType::Click&&!event.defaultPrevented&&(!wasDown||down)) {
      // Default actions are queued conceptually: no action runs before all veto
      // listeners. Independent native controls bound the activation chain.
      event.immediatePropagationStopped=false;
      const int boundary=activationBoundary(index);
      for(size_t i=0;i<length&&!event.defaultPrevented;++i){
        invoke(event,page().nodes[chain[i]].id,false,i?EventPhase::Bubble:EventPhase::Target,*copy,true);
        if(chain[i]==boundary)break;
      }
    }
    return !event.defaultPrevented;
  }
  bool routeEvent(EventType type,const char *from,const char *to,RouteAction action) {
    Event e{type,EventPhase::Target,0,0,0,0,0}; e.from=from;e.to=to;e.routeAction=action;
    e.bubbles=false;e.cancelable=type==EventType::BeforeNavigate;
    const auto copy=listeners;
    invoke(e,0,false,EventPhase::Target,*copy);
    return !e.defaultPrevented;
  }
  void cancelTarget() {
    if(target>=0&&!canceled) { canceled=true; invalidate(activeControl); dispatch(target,EventType::PointerCancel); }
  }
  void cancel(bool rebound=false) {
    cancelSystemChromeContact();
    const bool restore=rebound&&groupMotion.active&&groupMotion.follow&&groupMotion.prepared&&groupMotion.offset;
    if(!down) {if(restore)settleMotion(false,groupMotion.updated);else abortGroup();return;}
    const int canceledOwner=owner;const bool notifyOwner=coordinated()&&canceledOwner>=0&&!customCompatibility&&ownerStarted&&!ownerFinished;
    ownerFinished=true;
    down=false; cancelTarget();
    if(notifyOwner)dispatch(canceledOwner,EventType::GestureCancel);
    // A canceled pager drag returns to its last committed page.
    if(drag==Drag::Pager&&owner>=0) { state()[owner].scrollX=state()[owner].value*pagerStride(owner); invalidate(owner); }
    if(interruptedPager>=0) { state()[interruptedPager].scrollX=state()[interruptedPager].value*pagerStride(interruptedPager);invalidate(interruptedPager);interruptedPager=-1; }
    if(drag==Drag::Back) { backOffset=0;backPrepared=false;refresh->invalidateAll();routeEvent(EventType::NavigationCancel,page().path,project->pages[history[history.size()-2]].path,RouteAction::Back); }
    if(restore)settleMotion(false,groupMotion.updated);else abortGroup();gesture=nullptr;navigationDot=-1;navigationFooterContact=false;
    target=activeControl=owner=-1;drag=Drag::None;mediaDragTarget=-1;
  }
  void queueNavigation(const char *path,RouteAction action,const TransitionDesc *config=nullptr,bool instant=false) {
    pendingPath=path?path:"";pendingAction=action;pendingNavigation=true;
    pendingExplicitTransition=config!=nullptr;if(config)pendingTransition=*config;pendingInstant=instant;
  }
  void endProcessing() {
    processing=false;
    if(pendingNavigation&&(!transition.active||sharedComponentDeparture())&&!groupMotion.active) {
      const std::string path=pendingPath;const RouteAction action=pendingAction;const TransitionDesc config=pendingTransition;
      const bool explicitTransition=pendingExplicitTransition,instant=pendingInstant;pendingNavigation=false;
      navigate(path.c_str(),action,explicitTransition?&config:nullptr,instant);
    }
  }
  void commitRoute(int destination,RouteAction action,const char *from,bool playEnter=true,bool resumeLoops=false) {
    invalidateSystemModalBaseLive();
    const char *to=project->pages[destination].path;
    if(destination!=history.back())prepareBehaviorDeparture();
    captureSharedMedia(uint16_t(destination));
    saveSystemPanelState(history.back());
    if(action==RouteAction::Home) history.assign(1,uint16_t(destination));
    else if(action==RouteAction::Back) history.pop_back();
    else if(action==RouteAction::Replace) history.back()=uint16_t(destination);
    else history.push_back(uint16_t(destination));
    initializeActive();restoreSystemPanelState(history.back());++inputEpoch;restoreFocus();enterAnimations(playEnter);enterSharedMedia();refresh->invalidateAll();
    // Preserve loop playback, while finite entry effects stay revealed. The
    // route callback runs last so the application can still pause these loops.
    if(resumeLoops)for(uint16_t i=0;i<page().count;++i){const auto &a=page().nodes[i].animation;
      if(a.trigger==AnimationTrigger::Enter&&!a.repeat&&a.duration)startAnimation(i,0,true);
    }
    routeEvent(EventType::RouteChange,from,to,action);
  }
  TransitionSample pageSample(uint32_t elapsed) const {
    const Rect v=transitionViewport();
    if(transition.phase==TransitionPhase::Paired)return sampleTransition(transition.config,elapsed,v.width,v.height,transition.reverse);
    const TransitionPhaseSample s=sampleTransitionPhase(transition.config,elapsed,v.width,v.height,transition.phase==TransitionPhase::Enter,transition.reverse);
    return transition.phase==TransitionPhase::Exit?TransitionSample{s.x,s.y,0,0,s.opacity,s.active}:TransitionSample{0,0,s.x,s.y,s.opacity,s.active};
  }
  void nextTransitionEpoch() {invalidateSystemModalBaseLive();if(!++transitionEpoch)++transitionEpoch;completedTransitionEpoch=0;}
  void pagePhase(TransitionPhase phase,const TransitionDesc &config) {
    nextTransitionEpoch();
    transition.phase=phase;transition.config=config;transition.active=transition.pending=true;
    transition.sample=pageSample(0);
    refresh->invalidateAll();
  }
  void commitDeparture() {
    const char *from=transition.from->path;
    const int destination=transition.destination;const RouteAction action=transition.action;
    // Shared navigation remains touchable during departure. A later contact
    // can still be held when its terminal frame commits, so retire its old
    // page indices before changing history. Cancel listeners may queue another
    // navigation, but must not reenter or retarget this accepted commit.
    if(down){const bool nested=processing;processing=true;cancel();processing=nested;}
    if(transition.sequential) {
      const TransitionDesc incoming=*transition.incoming;
      transition.outgoing.clear();transition.from=nullptr;
      if(incoming.effect!=TransitionEffect::None)pagePhase(TransitionPhase::Enter,incoming);
      else {transition.active=transition.pending=false;transition.phase=TransitionPhase::Enter;}
    } else if(!transition.sharedComponents&&project->transition.effect!=TransitionEffect::None&&destination!=history.back()) {
      // The paired transition sees the final component-departure pose.
      transition.outgoing=state();transition.outgoingMotions=pageMotions[history.back()];pagePhase(TransitionPhase::Paired,project->transition);
    } else {
      transition.active=transition.pending=false;transition.from=nullptr;transition.outgoing.clear();
    }
    processing=true;commitRoute(destination,action,from);processing=false;
  }
  void finishComponents() {
    if(transition.sequential) {
      const TransitionDesc &out=transition.from->exitTransition?*transition.from->exitTransition:project->transition;
      if(out.effect!=TransitionEffect::None){pagePhase(TransitionPhase::Exit,out);return;}
    }
    commitDeparture();
  }
  bool sharedComponentDeparture() const {return transition.active&&transition.sharedComponents&&transition.phase==TransitionPhase::Components;}
  int sharedNavigationHit(int32_t px,int32_t py) const {
    if(!sharedComponentDeparture()||contains(transition.viewport,px,py))return -1;
    const int index=hit(px,py),nav=nearestNavigation(index);
    if(nav<0)return -1;
    const NodeDesc &n=page().nodes[nav];const int destination=routeIndex(n.route);
    return destination>=0&&(destination==history.back()||sharesMediaWith(uint16_t(destination)))?index:-1;
  }
  void beginDeparture(int destination,RouteAction action,bool sequential,bool sharedComponents=false) {
    nextTransitionEpoch();
    transition.from=&page();transition.outgoing=state();transition.outgoingMotions=pageMotions[history.back()];transition.destination=destination;transition.action=action;
    transition.viewport=navigationViewport(page().path,project->pages[destination].path);
    transition.sequential=sequential;transition.sharedComponents=sharedComponents;transition.reverse=action==RouteAction::Back||action==RouteAction::Home;
    const PageDesc &incoming=project->pages[destination];
    transition.incoming=incoming.enterTransition?incoming.enterTransition:&project->transition;
    transition.background=incoming.background;
    transition.phase=TransitionPhase::Components;transition.active=transition.pending=true;
    transition.sample={0,0,0,0,255,true};
    bool any=false;
    for(uint16_t i=0;i<page().count;++i) {
      const AnimationDesc *a=page().nodes[i].exitAnimation;
      resetMotion(i,false);
      state()[i].animating=a&&a->effect!=AnimationEffect::None&&visible(page(),state(),i);
      any=any||state()[i].animating;
    }
    if(!any)finishComponents();
  }
  void advanceDeparture(uint32_t now) {
    ProfileTimer animationTimer(*this,&RuntimeStats::animationMicros);
    if(transition.pending){transition.started=now;transition.pending=false;}
    const uint32_t elapsed=uint32_t(now-transition.started);bool any=false;
    for(uint16_t i=0;i<page().count;++i) {
      State &v=state()[i];if(!v.animating)continue;
      if(!visible(page(),state(),i)){v.animating=false;continue;}
      const AnimationDesc &a=*page().nodes[i].exitAnimation;
      const AnimationSample sample=sampleAnimation(a,elapsed);const State &base=transition.outgoing[i];
      const int32_t x=base.animationX+sample.x,y=base.animationY+sample.y;
      const uint8_t alpha=uint8_t(uint32_t(base.opacity)*sample.opacity/255);
      const bool changed=v.animationX!=x||v.animationY!=y||v.opacity!=alpha;
      if(changed)invalidateSubtree(i);
      v.animationX=x;v.animationY=y;v.opacity=alpha;v.animating=sample.active;
      if(changed)invalidateSubtree(i);any=any||sample.active;
    }
    // Unchanged zero-distance exits need no display work when already drained.
    // If an older frame is in flight, publish a minimal in-panel witness instead
    // of transferring a whole screen just to establish the terminal fence.
    if(transition.sample.active&&!any&&!refresh->dirty()) {
      if(!refresh->busy())completedTransitionEpoch=transitionEpoch;
      else refresh->invalidate(DirtyRect(page().width/2,page().height/2,1,1));
    }
    transition.sample.active=any;
  }
  bool cancelDeparture() {
    if(groupMotion.active&&groupMotion.follow){processing=true;cancel(true);endProcessing();return true;}
    if(!transition.active||(transition.phase!=TransitionPhase::Components&&transition.phase!=TransitionPhase::Exit))return false;
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
    if(cancelNativePipelineWork()!=Status::Ok)return false;
#else
    if(refresh->cancel()!=Status::Ok)return false;
#endif
    const char *from=transition.from->path,*to=project->pages[transition.destination].path;const RouteAction action=transition.action;
    for(uint16_t i=0;i<page().count;++i) {
      State &v=state()[i];const State &base=transition.outgoing[i];
      v.animationX=base.animationX;v.animationY=base.animationY;v.opacity=base.opacity;
      v.animating=base.animating;v.animationPending=base.animationPending;v.animationAt=base.animationAt;
      v.motionRevision=base.motionRevision;
      if(v.motionSlot!=NoParent&&v.motionSlot<transition.outgoingMotions.size())pageMotions[history.back()][v.motionSlot]=transition.outgoingMotions[v.motionSlot];
      if(!visible(page(),state(),i)){auto *m=liveMotion(i);if(m&&m->hiding)v.animating=v.animationPending=false;resetMotion(i,false);}
    }
    refreshMotionPending();
    transition.active=transition.pending=false;transition.from=nullptr;transition.outgoing.clear();
    pendingNavigation=false;releaseSnapshot();refresh->invalidateAll();
    processing=true;routeEvent(EventType::NavigationCancel,from,to,action);processing=false;return true;
  }
  const char *backFallbackRoute() const {
    for(uint16_t i=0;i<page().count;++i) {
      const NodeDesc &n=page().nodes[i];
      if(n.routeAction!=RouteAction::Back||!n.route||!*n.route||!visible(page(),state(),i,true)||!opacity(page(),state(),i))continue;
      const Rect shown=intersect(rect(i),clip(page(),state(),i));
      if(shown.width>0&&shown.height>0)return n.route;
    }
    return nullptr;
  }
  bool navigate(const char *path,RouteAction action,const TransitionDesc *config=nullptr,bool instant=false) {
    instant=instant||displayPolicy.slowRefresh||menuMotionConfig.preset==MenuPreset::Reduced;
    if(!project||guarding||backAnimation.active||groupMotion.active) return false;
    if(processing) {
      // InputAction::Back is deferred inside its event scope; retain the physical
      // request's configured fallback before nullptr becomes the pending string.
      if(action==RouteAction::Back&&!path&&history.size()<2)path=backFallbackRoute();
      queueNavigation(path,action,config,instant);return true;
    }
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
    if(cancelNativePipelineWork(false)!=Status::Ok)return false;
#endif
    if(transition.active&&!sharedComponentDeparture())return false;
    if(action==RouteAction::Home&&history.size()==1&&history.back()==project->initial) { processing=true;cancel();softCancel();if(instant){enterAnimations(false);refresh->invalidateAll();}endProcessing();return true; }
    if(action==RouteAction::Back&&history.size()<2) {
      // A declared route is an opt-in fallback for direct/Replace entry. Physical
      // Back uses the first Back control currently visible and enabled on screen.
      if(!path)path=backFallbackRoute();
      if(!path||!*path){navigationFeedback("Already at root");return false;}
      action=RouteAction::Replace;
    }
    if(action==RouteAction::Push&&history.size()>=navigationLimit){navigationFeedback("Navigation limit");return false;}
    int destination=-1;
    if(action==RouteAction::Home) destination=project->initial;
    else if(action==RouteAction::Back) destination=history[history.size()-2];
    else if(path) for(uint16_t i=0;i<project->count;++i) if(strcmp(project->pages[i].path,path)==0) { destination=i;break; }
    if(destination<0){navigationFeedback("Page unavailable");return false;}
    if(!sharedComponentDeparture()&&destination==history.back()&&
       (action==RouteAction::Push||action==RouteAction::Replace)&&navigationFeedback("Already on page"))return false;
    if(sharedComponentDeparture()) {
      if(destination==history.back())return cancelDeparture();
      if(!sharesMediaWith(uint16_t(destination)))return false;
      const char *from=page().path,*to=project->pages[destination].path;
      processing=true;bool allowed=routeEvent(EventType::BeforeNavigate,from,to,action);
      if(allowed&&guard){guarding=true;allowed=guard(guardContext,from,to,action);guarding=false;}
      if(allowed){transition.destination=destination;transition.action=action;transition.background=project->pages[destination].background;}
      else routeEvent(EventType::NavigationCancel,from,to,action);
      processing=false;return allowed;
    }
    const char *from=page().path,*to=project->pages[destination].path;
    processing=true;
    bool allowed=action==RouteAction::Back&&backPrepared;
    if(!allowed) { allowed=routeEvent(EventType::BeforeNavigate,from,to,action);
      if(allowed&&guard) { guarding=true;allowed=guard(guardContext,from,to,action);guarding=false; } }
    if(!allowed) { routeEvent(EventType::NavigationCancel,from,to,action);processing=false;pendingNavigation=false;return false; }
    const bool interactive=action==RouteAction::Back&&backPrepared;
    const bool same=destination==history.back();
    if(!same)prepareBehaviorDeparture();
    // A shared backdrop stays in the same coordinate space while content is
    // replaced. Whole-page snapshots would move it twice and gate rapid tabs.
    const bool sharedContinuous=!same&&sharesMediaWith(uint16_t(destination));
    const bool sequential=!sharedContinuous&&!instant&&!config&&!interactive&&!same&&(page().exitTransition||project->pages[destination].enterTransition);
    bool exits=false;
    if(!instant&&!config&&!interactive&&!same)for(uint16_t i=0;i<page().count;++i)if(page().nodes[i].exitAnimation&&page().nodes[i].exitAnimation->effect!=AnimationEffect::None&&visible(page(),state(),i)){exits=true;break;}
    const TransitionDesc paired=config?*config:project->transition;
    const bool animated=!sharedContinuous&&!instant&&paired.effect!=TransitionEffect::None&&!same&&!interactive;
    cancel();
    softCancel();
    if(editNode>=0){int i=editNode;editNode=-1;if(state()[i].value!=editValue){setValue(i,editValue);dispatch(i,EventType::Input);}}
    backPrepared=false;backOffset=0;
    inertia.active=false;
    if(animation.active) { state()[animation.node].scrollX=state()[animation.node].value*pagerStride(animation.node);animation.active=false; }
    if(sequential||exits) {
      beginDeparture(destination,action,sequential,sharedContinuous);processing=false;return true;
    }
    if(animated) {
      nextTransitionEpoch();
      transition.from=&page();transition.outgoing=state();transition.outgoingMotions=pageMotions[history.back()];transition.active=transition.pending=true;
      transition.viewport=navigationViewport(from,to);
      transition.phase=TransitionPhase::Paired;transition.config=paired;
      transition.reverse=!config&&(action==RouteAction::Back||action==RouteAction::Home);
      transition.sample=pageSample(0);
    }
    commitRoute(destination,action,from,!instant&&!same&&!config,same&&!instant);
    // Navigation callbacks may schedule the next transition; tick consumes it.
    processing=false;
    return true;
  }
  int32_t normalized(int i,int32_t v) const {
    const NodeDesc &n=page().nodes[i];
    if(n.kind==Kind::Checkbox||n.kind==Kind::Switch) return v?1:0;
    if(n.kind!=Kind::Range&&n.kind!=Kind::Progress&&!instrumentKind(n.kind)) return v;
    v=clamp32(v,n.minimum,n.maximum);
    const int64_t step=(n.kind==Kind::Range||n.kind==Kind::Progress||n.kind==Kind::Spinbox)&&n.step>0?n.step:1;
    const int64_t snapped=int64_t(n.minimum)+((int64_t(v)-n.minimum+step/2)/step)*step;
    return int32_t(std::min<int64_t>(int64_t(n.maximum),snapped));
  }
  bool setValue(int i,int32_t v,bool animated=true) {
    animated=animated&&!displayPolicy.slowRefresh;
    if(i<0) return false;
    if(page().nodes[i].kind==Kind::Pager) return setPage(i,v,true);
    if(page().nodes[i].kind==Kind::InputField){
      const auto *d=inputDescriptor(i);if(!d||d->type!=InputFieldType::Number)return false;
      const std::string formatted=inputNumberText(v,d->decimals);return setInputText(i,formatted.c_str());
    }
    v=normalized(i,v);if(state()[i].value==v){if(!animated)snapSwitch(i);updateReadout(i);return true;}
    state()[i].value=v;if(page().nodes[i].instrument){
      auto &a=instrument(i);
      if(animated)a.animate(page().nodes[i],v);else {a.visual=v;a.active=false;}
    }
    if(animated)animateSwitch(i);else snapSwitch(i);invalidate(i);updateReadout(i);return true;
  }
  void snapSwitch(int i) {
    if(i<0||page().nodes[i].kind!=Kind::Switch)return;
    State &v=state()[i];const uint16_t to=v.value?1000:0;
    if(v.switchPosition!=to)invalidate(i);
    v.switchPosition=v.switchFrom=v.switchTo=to;v.switchPending=false;
  }
  void animateSwitch(int i) {
    if(i<0||page().nodes[i].kind!=Kind::Switch)return;
    State &v=state()[i];const uint16_t to=v.value?1000:0;
    if(v.switchTo==to)return;
    v.switchFrom=v.switchPosition;v.switchTo=to;v.switchPending=true;
    invalidate(i);
  }
  void extent(const PageDesc &p,const RetainedVector<State> &values,int i,int32_t &maxX,int32_t &maxY) const {
    const Rect b=contentBox(layoutRect(p,values,i),styleFor(p,values,i));maxX=maxY=0;
    if(p.nodes[i].kind==Kind::Pager) {
      int32_t count=0;for(uint16_t j=uint16_t(i+1);j<p.count;++j)if(p.nodes[j].parent==i)++count;
      maxX=std::max<int32_t>(0,count-1)*b.width;return;
    }
    const auto parentOf=[&](int n){return p.nodes[n].parent==NoParent?-1:int(p.nodes[n].parent);};
    for(uint16_t j=uint16_t(i+1);j<p.count;++j) {
      int a=j;while(a>=0&&a!=i){if(viewportOverlayRoot(p,a)){a=-1;break;}a=parentOf(a);}
      if(a==i&&visible(p,values,j)) {
        Rect r=layoutRect(p,values,j);
        for(int ancestor=parentOf(j);ancestor>=0&&ancestor!=i;ancestor=parentOf(ancestor))
          if((values[ancestor].flags&Clip)||scrolling(p.nodes[ancestor].kind))r=intersect(r,contentBox(layoutRect(p,values,ancestor),styleFor(p,values,ancestor)));
        maxX=std::max<int32_t>(maxX,r.x+r.width-b.x-b.width);maxY=std::max<int32_t>(maxY,r.y+r.height-b.y-b.height);
      }
    }
  }
  void extent(int i,int32_t &maxX,int32_t &maxY) const {extent(page(),state(),i,maxX,maxY);}
  bool scroll(int i,int32_t sx,int32_t sy,bool emit=true) {
    if(i<0||!scrolling(page().nodes[i].kind)) return false;
    cancelMenuScroll(i);
    int32_t maxX,maxY;extent(i,maxX,maxY);
    const bool horizontal=page().nodes[i].kind==Kind::Pager||(state()[i].flags&Horizontal);
    sx=horizontal?clamp32(sx,0,maxX):0;sy=horizontal?0:clamp32(sy,0,maxY);
    if(state()[i].scrollX==sx&&state()[i].scrollY==sy) return true;
    state()[i].scrollX=sx;state()[i].scrollY=sy;invalidate(i);
    if(emit) dispatch(i,EventType::Scroll);
    return true;
  }
  bool setPage(int i,int32_t index,bool emit) {
    if(i<0||page().nodes[i].kind!=Kind::Pager) return false;
    int32_t mx,my;extent(i,mx,my);
    const int32_t width=pagerStride(i);
    if(width<=0) return false;
    index=clamp32(index,0,(mx+width-1)/width);
    const bool change=state()[i].value!=index;state()[i].value=index;
    // A drag/programmatic scroll can reach this offset before committing its
    // page index. The indicator still changes even when scroll() is a no-op.
    if(change&&state()[i].scrollX==index*width&&state()[i].scrollY==0)invalidate(i);
    scroll(i,index*width,0,emit);
    if(change)if(const auto *link=componentFor(i))if(link->desc->kind==ComponentKind::Tabs&&link->pager==i){
      for(uint8_t option=0;option<link->desc->optionCount;++option)invalidate(find(link->desc->options[option].id));
      revealComponentSelection(page(),state(),*link->desc);}
    if(change&&emit) dispatch(i,EventType::PageChange);
    return true;
  }
  void range(int i) {
    const NodeDesc &n=page().nodes[i];const Style &style=styleFor(page(),state(),i);Rect r=contentBox(rect(i),style);
    const auto geometry=detail::rangeGeometry(r,style,(n.flags&VerticalRange)!=0,true,state()[i].value,n.minimum,n.maximum);
    const int32_t length=geometry.length,px=geometry.coordinate(r,x,y,rangeGrabOffset);
    const int64_t span=int64_t(n.maximum)-n.minimum;
    const int32_t delta=geometry.vertical?startY-y:x-startX;
    const int64_t raw=rangeThumbContact?int64_t(startValue)+span*delta/length:int64_t(n.minimum)+span*px/length;
    int32_t next=normalized(i,int32_t(std::max<int64_t>(n.minimum,std::min<int64_t>(n.maximum,raw))));
    if(next!=state()[i].value) { state()[i].value=next;changed=true;invalidate(i);dispatch(i,EventType::Input); }
  }
  #include "detail/InteractionRuntime.inc.h"
  void pointerDown(int32_t px,int32_t py,uint32_t now) {
    if(!project) return;processing=true;cancel();systemChromeSavedFocus=focusNode>=0?page().nodes[focusNode].id:0;if(editNode>=0){int i=editNode;editNode=-1;if(!behaviorKeepDraft(i)){setValue(i,editValue);dispatch(i,EventType::Input);}}++inputEpoch;px=clamp32(px,-32768,32767);py=clamp32(py,-32768,32767);x=startX=px;y=startY=py;downAt=now;spinRepeatAt=now+500;down=true;
    const bool interruptedInertia=inertia.active&&contains(rect(inertia.node),px,py)&&contains(clip(page(),state(),inertia.node),px,py);inertia.active=false;
    interruptedPager=animation.active?animation.node:-1;animation.active=false;
    lastX=px;lastY=py;lastAt=now;velocityX=velocityY=0;
    canceled=changed=longPressed=false;moved=interruptedPager>=0||interruptedInertia;allowX=allowY=true;drag=Drag::None;owner=-1;mediaDragTarget=-1;
    pointerClaim=-1;pointerClaimAxis=GestureAxis::None;defaultsBlocked=customCompatibility=ownerStarted=ownerFinished=false;contactModal=modalRoot();
    target=hit(x,y);const int bar=systemChromeBarTarget(x,y);if(bar>=0)target=bar;activeControl=nearestControl(target);
    gesture=nullptr;navigationDot=-1;navigationFooterContact=false;
    if(contactModal<0)if(const auto *g=navigationGroup(page().path))if(g->indicatorHeight&&contains(navigationFooter(*g),x,y)){
      navigationFooterContact=true;navigationDot=navigationDotAt(*g,x,y);target=activeControl=-1;
    }
    if(input||focusStyleCount) { int candidate=activeControl>=0?activeControl:target;for(;candidate>=0&&!focusable(candidate,false);candidate=parent(candidate)){}if(candidate>=0)setFocus(candidate,false); }
    rangeGrabOffset=0;rangeThumbContact=false;
    if(activeControl>=0) startValue=state()[activeControl].value;
    if(activeControl>=0&&page().nodes[activeControl].kind==Kind::Range) {
      const NodeDesc &n=page().nodes[activeControl];const Style &style=styleFor(page(),state(),activeControl);const Rect r=contentBox(rect(activeControl),style);
      const auto geometry=detail::rangeGeometry(r,style,(n.flags&VerticalRange)!=0,true,startValue,n.minimum,n.maximum);
      rangeThumbContact=contains(geometry.thumb,x,y);
      if(rangeThumbContact)rangeGrabOffset=geometry.vertical?y-(geometry.thumb.y+geometry.thumb.height/2):x-(geometry.thumb.x+geometry.thumb.width/2);
    }
    for(int a=target;a>=0;a=gestureParent(a)) {
      const uint16_t f=state()[a].flags;
      if(f&TouchNone) allowX=allowY=false;
      if(f&TouchPanX)allowY=false;if(f&TouchPanY)allowX=false;
      if(scrolling(page().nodes[a].kind))break;
    }
    invalidate(activeControl);
    const bool accepted=dispatch(target,EventType::PointerDown);
    if(coordinated())defaultsBlocked=!accepted;
    if(accepted&&down)for(const auto &pair:systemChromePairs)if(pair.page==history.back()){
      if(descendant(target,pair.bar)){systemChromePanel=pair.panel;systemChromeOpening=true;systemChromeHandle=false;break;}
      if(target==pair.handle||systemPanelBlank(target,pair.panel)){systemChromePanel=pair.panel;systemChromeOpening=false;systemChromeHandle=target==pair.handle;break;}
    }
    if(systemChromePanel>=0){systemChromePage=history.back();auto *b=behavior(page().nodes[systemChromePanel].id);if(b&&b->open)beginSystemChromeDrag();}
    endProcessing();
  }
  void pointerMove(int32_t px,int32_t py,uint32_t now) {
    if(!project||!down) return;
    // Motion samples still dispatch every event, but an unchanged button
    // contact does not change pixels. Remember the previous press silhouette
    // before callbacks or gesture arbitration can cancel/change the target.
    const int oldControl=activeControl;
    const bool wasPressed=!canceled&&oldControl>=0&&nodeContains(oldControl,x,y);
    processing=true;x=clamp32(px,-32768,32767);y=clamp32(py,-32768,32767);
    const int32_t dx=x-startX,dy=y-startY;
    if(systemChromePanel>=0)systemChromeVelocityFresh=now!=lastAt&&uint32_t(now-lastAt)<150;
    if(now!=lastAt&&uint32_t(now-lastAt)<150) {
      velocityX=clamp32(int32_t(int64_t(x-lastX)*1000/uint32_t(now-lastAt)),-4000,4000);
      velocityY=clamp32(int32_t(int64_t(y-lastY)*1000/uint32_t(now-lastAt)),-4000,4000);
    }
    lastX=x;lastY=y;lastAt=now;
    const bool defaultMove=canceled||dispatch(target,EventType::PointerMove);
    // A listener may cancel, hide or disable the target. Do not acquire a
    // gesture owner (especially an edge-back preview) after that cancellation.
    if(!down) { endProcessing();return; }
    // A custom map/canvas handler can consume a move while retaining this
    // pointer's target. Do not turn it into a built-in drag and PointerCancel;
    // later samples/up still belong to that handler, and a drag cannot click.
    if(!defaultMove||abs(dx)>=(coordinated()?8:7)||abs(dy)>=(coordinated()?8:7))moved=true;
    if(systemChromePanel>=0){
      if(!defaultMove)cancelSystemChromeContact();
      else {
        if(systemChromeDragging&&!systemChromeDirect&&dy){systemChromeDirect=true;moved=true;drag=Drag::Consumed;cancelTarget();}
        if(!systemChromeDragging&&!systemChromeDirect&&!systemChromeRejected){
          const int32_t distance=systemChromeDragging?abs(dy):(systemChromeOpening?dy:-dy);
          if(distance>=1&&int64_t(distance)*100>int64_t(abs(dx))*125){
            if(systemChromeDragging||beginSystemChromeDrag()){systemChromeDirect=true;moved=true;drag=Drag::Consumed;cancelTarget();}
          }else if(abs(dx)>=(coordinated()?8:7)&&int64_t(abs(dy))*100<=int64_t(abs(dx))*125)systemChromeRejected=true;
        }
        if(!down){endProcessing();return;}
        if(systemChromeDragging&&(systemChromeDirect||dy)&&!updateSystemChromeDrag(dy))cancelSystemChromeContact();
        if(moved)drag=Drag::Consumed;
        if(wasPressed!=(!canceled&&oldControl>=0&&nodeContains(oldControl,x,y)))invalidate(oldControl);
        endProcessing();return;
      }
    }
    if(coordinated()&&!defaultMove&&drag==Drag::None&&!defaultsBlocked&&pointerClaim<0){
      // Raw custom handlers consume the pointer sequence. Once consumed, later
      // samples cannot accidentally turn this sequence into page navigation.
      drag=Drag::Custom;owner=target;customCompatibility=true;
    }
    if(coordinated()&&defaultMove&&!defaultsBlocked&&drag==Drag::None)recognizeCoordinated(dx,dy,now);
    if(!down){endProcessing();return;}
    if(!coordinated()&&defaultMove&&drag==Drag::None&&(abs(dx)>6||abs(dy)>6)) {
      const bool horizontal=abs(dx)>=abs(dy);
      const SwipeDirection direction=horizontal?(dx<0?SwipeDirection::Left:SwipeDirection::Right):(dy<0?SwipeDirection::Up:SwipeDirection::Down);
      const bool axisAllowed=horizontal?allowX:allowY;
      const auto *edgeGesture=axisAllowed&&contactModal<0?swipeBinding(direction,true):nullptr;
      if(edgeGesture) claimGesture(edgeGesture,now);
      else if(contactModal<0&&!navigation&&swipeHome&&startY>=int32_t(page().height)-24&&startY<page().height&&startX>=0&&startX<page().width&&dy<-6&&!horizontal&&allowY) {
        drag=Drag::Home;cancelTarget();
      }
      else if(contactModal<0&&!navigation&&edgeBack&&history.size()>1&&startX<24&&dx>6&&horizontal&&allowX&&activeControl<0) {
        const char *from=page().path,*to=project->pages[history[history.size()-2]].path;
        bool prepared=routeEvent(EventType::BeforeNavigate,from,to,RouteAction::Back);
        if(prepared&&down&&guard) { guarding=true;prepared=guard(guardContext,from,to,RouteAction::Back);guarding=false; }
        if(!down) { routeEvent(EventType::NavigationCancel,from,to,RouteAction::Back);endProcessing();return; }
        backPrepared=prepared;
        if(backPrepared) { drag=Drag::Back;cancelTarget(); }
        else { routeEvent(EventType::NavigationCancel,from,to,RouteAction::Back);allowX=false; }
      }
      else if(horizontal&&allowX&&[&](){for(int a=target;a>=0;a=gestureParent(a))if(mediaScrubTarget(a)>=0){acquireMediaScrub(a);return true;}return false;}()) {}
      else if(activeControl>=0&&page().nodes[activeControl].kind==Kind::Range&&horizontal==!(page().nodes[activeControl].flags&VerticalRange)&&!canceled) { drag=Drag::Range;owner=activeControl; }
      else if((horizontal&&allowX)||(!horizontal&&allowY)) {
        bool contained=false;
        for(int a=target;a>=0;a=gestureParent(a)) {
          if(!scrolling(page().nodes[a].kind)) continue;
          const bool axis=page().nodes[a].kind==Kind::Pager||(state()[a].flags&Horizontal);
          int32_t mx,my;extent(a,mx,my);
          const int32_t pos=axis?state()[a].scrollX:state()[a].scrollY,maxPos=axis?mx:my,delta=axis?dx:dy;
          if(axis==horizontal&&maxPos>0&&((delta<0&&pos<maxPos)||(delta>0&&pos>0)||(state()[a].flags&OverscrollContain))) {
            owner=a;drag=page().nodes[a].kind==Kind::Pager?Drag::Pager:Drag::Scroll;
            startScrollX=state()[a].scrollX;startScrollY=state()[a].scrollY;cancelTarget();break;
          }
          if(axis==horizontal&&(state()[a].flags&OverscrollContain))contained=true;
          if(state()[a].flags&OverscrollContain) break;
        }
        if(drag==Drag::None&&!contained&&down&&contactModal<0) {
          if(const auto *binding=swipeBinding(direction,false))claimGesture(binding,now);
          else if(horizontal){int index=-1;const auto *g=navigationGroup(page().path,&index);
            if(g&&g->swipe&&contains(navigationViewport(page().path),startX,startY)){const int next=index+(dx<0?1:-1);
              if(next>=0&&next<g->count&&prepareGroup(routeIndex(g->paths[next]),now)){drag=Drag::Group;cancelTarget();}
            }
          }
          if(!down){endProcessing();return;}
          if(drag==Drag::None&&navigation&&swipeHome&&startY>=int32_t(page().height)-24&&startY<page().height&&startX>=0&&startX<page().width&&dy<-6&&!horizontal){drag=Drag::Home;cancelTarget();}
          else if(drag==Drag::None&&navigation&&edgeBack&&history.size()>1&&startX<24&&dx>6&&horizontal&&activeControl<0){
            const char *from=page().path,*to=project->pages[history[history.size()-2]].path;
            bool prepared=routeEvent(EventType::BeforeNavigate,from,to,RouteAction::Back);
            if(prepared&&down&&guard){guarding=true;prepared=guard(guardContext,from,to,RouteAction::Back);guarding=false;}
            if(down&&prepared){backPrepared=true;drag=Drag::Back;cancelTarget();}
            else if(down)routeEvent(EventType::NavigationCancel,from,to,RouteAction::Back);
          }
        }
      }
      // PointerCancel listeners can cancel the entire sequence while a
      // scroller or navigation gesture takes ownership.
      if(!down) { endProcessing();return; }
      // Lock the first recognized axis even at a boundary or on empty space.
      // A later reversal cannot turn this sequence into a tap or another drag.
      if(drag==Drag::None) { drag=Drag::Consumed;cancelTarget(); }
    }
    if(coordinated()&&owner>=0&&!customCompatibility&&defaultMove&&!defaultsBlocked)dispatch(owner,EventType::GestureUpdate);
    if(!down){endProcessing();return;}
    if(coordinated()&&(!defaultMove||defaultsBlocked)){endProcessing();return;}
    if(drag==Drag::Media)updateMediaScrub(dx);
    else if(drag==Drag::Range) range(owner);
    else if(drag==Drag::Scroll||drag==Drag::Pager) scroll(owner,startScrollX-dx,startScrollY-dy);
    else if(drag==Drag::Back) { backOffset=clamp32(dx,0,page().width);refresh->invalidateAll(); }
    else if(groupMotion.active&&(drag==Drag::Group||drag==Drag::Gesture)) {
      const int32_t delta=groupMotion.vertical?dy:dx,distance=motionDistance();
      groupMotion.offset=groupMotion.sign<0?clamp32(delta,-distance,0):clamp32(delta,0,distance);
      groupMotion.updated=now;
      if(!groupMotion.prepared)groupMotion.offset/=3;invalidateMotion(navigationViewport(page().path,project->pages[groupMotion.destination].path));
    }
    else if(activeControl>=0) {
      const bool isPressed=!canceled&&nodeContains(activeControl,x,y);
      if(oldControl!=activeControl||wasPressed!=isPressed)invalidate(activeControl);
    }
    endProcessing();
  }
  void pointerUp(int32_t px,int32_t py,uint32_t now) {
    // Some drivers only report down/up, or the final movement with release.
    // Arbitrate that sample before dispatching up or deciding to activate.
    if(project&&down&&(px!=x||py!=y)) pointerMove(px,py,now);
    if(!project||!down) return;processing=true;x=clamp32(px,-32768,32767);y=clamp32(py,-32768,32767);
    const int oldControl=activeControl;
    if(systemChromePanel>=0&&systemChromeDragging){
      const bool samePage=systemChromePage==history.back();
      auto *b=samePage&&systemChromePanel<page().count?behavior(page().nodes[systemChromePanel].id):nullptr;
      if(!b||!b->open||!b->dragging||b->revision!=systemChromeRevision){
        systemChromePanel=-1;systemChromeDragging=systemChromeDirect=false;
        if(samePage)cancelTarget();down=false;target=activeControl=owner=interruptedPager=-1;drag=Drag::None;
        if(samePage)invalidate(oldControl);endProcessing();return;
      }
    }
    // Keep the sequence cancelable until release callbacks/defaults finish.
    const bool perform=!canceled&&dispatch(target,EventType::PointerUp)&&(!coordinated()||!defaultsBlocked);
    if(!down) { endProcessing();return; }
    if(systemChromePanel>=0){
      if(systemChromeDragging){
        const bool direct=systemChromeDirect;bool open=systemChromeInitialOpen;int32_t speed=0;
        if(direct){
          auto *b=behavior(page().nodes[systemChromePanel].id);const int32_t height=b?std::max<int32_t>(1,b->full.height):1;
          if(systemChromeVelocityFresh&&abs(y-startY)>=12&&uint32_t(now-lastAt)<150)speed=clamp32(int32_t(int64_t(velocityY)*1024/height),-8192,8192);
          const int32_t progress=b?clamp32(b->motionAxis.pixels(),0,1024):0;open=int64_t(progress)+int64_t(speed)*120/1000>=512;
        }else if(perform&&!moved&&!longPressed&&(systemChromeOpening||systemChromeHandle))open=systemChromeOpening;
        const int panel=systemChromePanel;systemChromeDragging=systemChromeDirect=false;
        settleSystemPanelDrag(panel,open,speed);
        if(direct){systemChromePanel=-1;down=false;target=activeControl=owner=interruptedPager=-1;drag=Drag::None;invalidate(oldControl);endProcessing();return;}
      }
    }
    if(coordinated()&&owner>=0&&!customCompatibility&&ownerStarted){ownerFinished=true;dispatch(owner,EventType::GestureEnd);if(!down){endProcessing();return;}}
    if(drag==Drag::Gesture&&gesture) {
      const bool horizontal=!verticalSwipe(gesture->direction);
      const int32_t distance=gesture->direction==SwipeDirection::Left?startX-x:gesture->direction==SwipeDirection::Right?x-startX:gesture->direction==SwipeDirection::Up?startY-y:y-startY;
      const int32_t cross=horizontal?abs(y-startY):abs(x-startX);
      const bool commit=distance>=gesture->threshold&&distance>cross;
      if(gesture->interaction==GestureInteraction::Follow){
        if(groupMotion.active)settleMotion(commit,now);
        else if(commit&&gestureDestination(*gesture)==history.back())queueNavigation(gesture->route,gesture->action,nullptr,true);
      }else if(commit)queueNavigation(gesture->route,gesture->action,gesture->interaction==GestureInteraction::Instant?nullptr:gesture->transition,gesture->interaction==GestureInteraction::Instant);
    } else if(drag==Drag::Group&&groupMotion.active) {
      const int32_t distance=abs(groupMotion.offset),cross=abs(y-startY);
      const bool flick=uint32_t(now-lastAt)<=150&&abs(velocityX)>400&&((groupMotion.offset<0&&velocityX<0)||(groupMotion.offset>0&&velocityX>0));
      settleMotion(distance>cross&&(distance>=std::max<int32_t>(32,page().width/4)||flick),now);
    } else if(perform&&drag==Drag::None&&!moved&&!longPressed&&navigationFooterContact) {
      int index=-1;const auto *g=navigationGroup(page().path,&index);
      if(g&&navigationDot>=0&&navigationDot!=index&&navigationDotAt(*g,x,y)==navigationDot&&prepareGroup(routeIndex(g->paths[navigationDot]),now)){
        settleMotion(true,now);
      }
    } else if(drag==Drag::Home) {
      if(startY-y>=std::max<int32_t>(32,int32_t(page().height)/4)&&abs(x-startX)<page().width/3) {
        queueNavigation(nullptr,RouteAction::Home);
      }
    } else if(drag==Drag::Back) {
      const bool commit=x-startX>=std::max<int32_t>(32,int32_t(page().width)/4)&&abs(y-startY)<page().height/3;
      if(displayPolicy.slowRefresh){backPrepared=false;backOffset=0;if(commit)queueNavigation(nullptr,RouteAction::Back,nullptr,true);else refresh->invalidateAll();}
      else backAnimation={backOffset,commit?int32_t(page().width):0,now,true,commit};
    } else if(drag==Drag::Pager||(drag==Drag::None&&interruptedPager>=0)) {
      if(owner<0)owner=interruptedPager;
      const int32_t w=pagerStride(owner);
      int32_t index=(state()[owner].scrollX+w/2)/w;
      if(uint32_t(now-lastAt)<=150&&abs(velocityX)>400)index=state()[owner].value+(velocityX<0?1:-1);
      int32_t mx,my;extent(owner,mx,my);index=clamp32(index,0,(mx+w-1)/w);
      if(displayPolicy.slowRefresh)setPage(owner,index,true);
      else animation={owner,state()[owner].scrollX,index*w,index,now,true};
    } else if(drag==Drag::Scroll) {
      int32_t speed=(state()[owner].flags&Horizontal)?velocityX:velocityY;
      if(!displayPolicy.slowRefresh&&uint32_t(now-lastAt)<=150&&abs(speed)>80)inertia={owner,-speed,0,now,true};
    } else if(perform&&activeControl>=0&&page().nodes[activeControl].kind==Kind::Range&&(drag==Drag::Range||!moved)) {
      const int rangeControl=activeControl;if(!coordinated()||drag!=Drag::Range)range(rangeControl);
      if(down&&state()[rangeControl].value!=startValue) dispatch(rangeControl,EventType::Change);
    } else if(perform&&drag==Drag::None&&!moved&&!longPressed&&target>=0&&nodeContains(target,x,y)) {
      activateNode(target,now,true);
    }
    systemChromePanel=-1;down=false;target=activeControl=owner=interruptedPager=-1;drag=Drag::None;mediaDragTarget=-1;gesture=nullptr;navigationDot=-1;navigationFooterContact=false;invalidate(oldControl);endProcessing();
  }
  void spinbox(int i,int32_t px,bool pointer) {
    const auto &n=page().nodes[i];const Rect r=contentBox(rect(i),styleFor(page(),state(),i));const auto g=spinboxGeometry(r,*n.instrument);
    const int direction=px>=g.minusX&&px<g.minusX+g.button?-1:px>=g.plusX&&px<g.plusX+g.button?1:0;if(!direction)return;
    const int32_t before=state()[i].value;setValue(i,int32_t(std::max<int64_t>(n.minimum,std::min<int64_t>(n.maximum,int64_t(before)+int64_t(direction)*n.step))));
    if(before!=state()[i].value){dispatch(i,EventType::Input);if((!pointer||down)&&visible(page(),state(),i,true))dispatch(i,EventType::Change);}
  }
  void activateNode(int clickedIndex,uint32_t now,bool pointer) {
    const auto *component=componentFor(clickedIndex,true);
    if(component&&(component->desc->kind==ComponentKind::Select||component->desc->kind==ComponentKind::Choice)) {
      const int option=componentOption(*component,clickedIndex);
      const bool opener=component->desc->kind==ComponentKind::Select&&clickedIndex==component->text;
      if(option>=0||opener) {
        if(!dispatch(clickedIndex,EventType::Click)||(pointer&&!down)||!visible(page(),state(),clickedIndex,true))return;
        for(int a=clickedIndex;a>=0;a=parent(a))if(page().nodes[a].animation.trigger==AnimationTrigger::Click)startAnimation(a,now);
        if(opener){const int panel=component->panel;if(panel>=0){const auto *motion=liveMotion(panel);if((state()[panel].flags&Hidden)||(motion&&motion->hiding))requestShow(panel,false);else requestHide(panel,false);}return;}
        const int root=component->root;const bool changed=state()[root].value!=option;
        selectOption(*component,option);
        // The accepted option activation owns its closing menu. Let its Input
        // and Change finish before pointerUp ends that sequence; public hide
        // and visibility setters still cancel pointers invalidated by callers.
        if(component->panel>=0&&!(state()[component->panel].flags&Hidden))requestHide(component->panel,false);
        if(changed){dispatch(root,EventType::Input);if((!pointer||down)&&visible(page(),state(),root,true))dispatch(root,EventType::Change);}
        return;
      }
    }
      const int activation=nearestControl(clickedIndex);
      if(activation>=0&&page().nodes[activation].kind==Kind::InputField){if(dispatch(clickedIndex,EventType::Click)&&(!pointer||down))softBegin(activation);return;}
      if(activation>=0&&page().nodes[activation].kind==Kind::Spinbox){if(dispatch(clickedIndex,EventType::Click)&&(!pointer||down)&&visible(page(),state(),clickedIndex,true)&&visible(page(),state(),activation,true)){const Rect r=contentBox(rect(activation),styleFor(page(),state(),activation));const auto g=spinboxGeometry(r,*page().nodes[activation].instrument);if(!pointer||(x>=g.textX&&x<g.textX+g.textWidth))softBegin(activation);else spinbox(activation,x,pointer);}return;}
      const bool toggle=activation>=0&&(page().nodes[activation].kind==Kind::Checkbox||page().nodes[activation].kind==Kind::Switch);
      const int32_t previous=toggle?state()[activation].value:0;
      if(toggle)state()[activation].value=!previous; // HTML checkbox pre-activation precedes click listeners.
      const bool allowed=dispatch(clickedIndex,EventType::Click)&&(!pointer||down)&&
        visible(page(),state(),clickedIndex,true)&&(activation<0||visible(page(),state(),activation,true));
      if(toggle&&!allowed)state()[activation].value=previous;
      // Physical/keyboard activation has no pointer-release repaint. Switches
      // invalidate their motion below; a changed checkbox needs its own dirty.
      if(toggle&&page().nodes[activation].kind==Kind::Checkbox&&state()[activation].value!=previous)invalidate(activation);
      if(toggle)animateSwitch(activation);
      if(allowed) {
        for(int a=clickedIndex;a>=0;a=parent(a))if(page().nodes[a].animation.trigger==AnimationTrigger::Click)startAnimation(a,now);
        if(toggle) { dispatch(activation,EventType::Input);if((!pointer||down)&&visible(page(),state(),activation,true))dispatch(activation,EventType::Change); }
        if((pointer&&!down)||!visible(page(),state(),clickedIndex,true))return;
        const NodeDesc &clicked=page().nodes[clickedIndex];
        if(pointer&&clicked.kind==Kind::Pager&&clicked.pager.indicatorHeight) {
          const Rect r=contentBox(rect(clickedIndex),styleFor(page(),state(),clickedIndex));int count=0;
          for(uint16_t i=0;i<page().count;++i)if(page().nodes[i].parent==clickedIndex)++count;
          const int step=clicked.pager.dotSize+clicked.pager.dotGap;
          const int left=r.x+(r.width-(count*step-clicked.pager.dotGap))/2;
          if(y>=r.y+r.height-clicked.pager.indicatorHeight&&x>=left-clicked.pager.dotGap/2&&x<left+count*step-clicked.pager.dotGap/2) {
            const int index=clamp32((x-left+clicked.pager.dotGap/2)/step,0,count-1);
            if(displayPolicy.slowRefresh)setPage(clickedIndex,index,true);
            else animation={clickedIndex,state()[clickedIndex].scrollX,index*pagerStride(clickedIndex),index,now,true};
          }
        }
        int navigation=nearestNavigation(clickedIndex);
        // Labels keep their associated control's default navigation if neither
        // the label nor an ancestor declares its own navigation action.
        if(navigation<0&&activation>=0&&visible(page(),state(),activation,true)&&page().nodes[activation].routeAction!=RouteAction::None) navigation=activation;
        if(navigation>=0) { const NodeDesc &n=page().nodes[navigation];queueNavigation(n.route,n.routeAction); }
      }
  }
  NICE_GFX_UI_GEOMETRY_IRAM_ATTR Rect backgroundCover(int index,Rect clipped) const {
#if NICE_GFX_UI_BACKGROUND_COVER_PROFILE
    if(profilingClock)++stats.backgroundCoverCalls;
#endif
    Rect best{0,0,0,0};int64_t bestArea=0;
    if(behaviorVisualMoving())return best;
    // Tiny backgrounds cost less to paint than scanning later occluders.
    // Returning no cover only restores normal paint order; never skip pixels.
    if(clipped.width<=0||clipped.height<=0||uint64_t(clipped.width)*clipped.height<=64)return best;
    if(index<0||paintCache->nodes[index].alpha!=255)return best;
    // Reuse the frame's vertical candidate bands. A later node whose clipped
    // bounds do not intersect this tile cannot cover any of its background.
    // Visit set bits in source order so equal-area covers retain the same tie.
    const FrameCache &cache=*paintCache;const size_t start=size_t(index)+1;
    const int firstBand=clamp32((clipped.y-paintOffsetY)/cache.bandHeight,0,31),lastBand=clamp32((clipped.y+clipped.height-1-paintOffsetY)/cache.bandHeight,0,31);
    for(size_t word=start/32;word<cache.words;++word){uint32_t candidates=0;
      for(int band=firstBand;band<=lastBand;++band)candidates|=cache.bands[size_t(band)*cache.words+word];
      if(word==start/32)candidates&=uint32_t(0xffffffffu)<<(start%32);
      while(candidates){int bit=0;
#if defined(__GNUC__) || defined(__clang__)
        bit=__builtin_ctz(candidates);
#else
        uint32_t bits=candidates;
        if(!(bits&0xffffu)){bits>>=16;bit+=16;}if(!(bits&0xffu)){bits>>=8;bit+=8;}
        if(!(bits&0xfu)){bits>>=4;bit+=4;}if(!(bits&3u)){bits>>=2;bit+=2;}if(!(bits&1u))++bit;
#endif
        candidates&=candidates-1;const uint16_t i=uint16_t(word*32+size_t(bit));
#if NICE_GFX_UI_ROI_PREFIX_BOOTSTRAP
        // A retained prefix cannot borrow a hole filled by a later painter.
        if(nativePrefixLimit>=0&&size_t(i)>=size_t(nativePrefixLimit))continue;
#endif
#if NICE_GFX_UI_BACKGROUND_COVER_PROFILE
        if(profilingClock)++stats.backgroundCoverCandidates;
#endif
        if(viewportOverlayLevels&&viewportOverlayDepth(*rasterFrame.current.page,rasterFrame.current.values,i)<viewportOverlayDepth(*rasterFrame.current.page,rasterFrame.current.values,index))continue;
        const Geometry &g=paintCache->nodes[i];if(!g.visible||g.alpha!=255)continue;
        if(rasterFrame.current.page->nodes[i].motion){const auto &m=motionPose(*rasterFrame.current.page,rasterFrame.current.values,i);if(m.scale!=1024||m.rotation%360!=0)continue;}
        const State &v=rasterFrame.current.values[i];
        const bool disabled=(v.flags&Disabled)!=0,pressed=i==snapshotPressed;
        if(disabled?(v.flags&DisabledTransparent)!=0:pressed?(v.flags&ActiveTransparent)!=0:(v.flags&Transparent)!=0)continue;
        Rect r=g.rect;r.x+=paintOffsetX;r.y+=paintOffsetY;
        Rect clip=g.clip;clip.x+=paintOffsetX;clip.y+=paintOffsetY;
        const Rect visible=intersect(clipped,intersect(r,clip));if(visible.width<=0||visible.height<=0)continue;
        const Style &style=styleFor(*rasterFrame.current.page,rasterFrame.current.values,i);
        const int32_t radius=std::min<int32_t>(style.radius,std::min<int32_t>(r.width,r.height)/2);
        const Rect cover=raster_occlusion::interior(r,radius,visible);
        const int64_t area=int64_t(cover.width)*cover.height;
        if(area>bestArea){best=cover;bestArea=area;}
        // This contained cover is the largest possible. A later equal-area
        // rectangle cannot replace it under the original strict-greater rule.
        if(bestArea==int64_t(clipped.width)*clipped.height)return best;
      }
    }
    return best;
  }
  void rounded(Rect r,uint16_t color,uint16_t border,const Style &s,bool gradient=false,int index=-1,Rect clipped={0,0,0,0}) {
    if(r.width<=0||r.height<=0)return;
    const int radius=std::min<int32_t>(s.radius,std::min<int32_t>(r.width,r.height)/2);
    Rect cover{0,0,0,0};bool thin=false;
#if NICE_GFX_UI_THIN_BACKGROUND_COVER
    // Only restore background writes that the old occlusion optimization may
    // omit. Keep the established painter, clip, blocking handler and ordering.
    // A later proven opaque cover still replaces those restored pixels.
    thin=index>=0&&paintCache&&size_t(index)<paintCache->nodes.size()&&
      rasterFrame.current.page->nodes[index].kind==Kind::Container&&paintCache->nodes[index].alpha==255&&
      !s.radius&&!s.borderWidth&&!s.gradient&&(r.width<=2||r.height<=2)&&uint64_t(r.width)*r.height<=4096&&
      clipped.width>0&&clipped.height>0&&uint64_t(clipped.width)*clipped.height>64&&
      !displayPolicy.monochrome&&!buildingSurface&&!snapshotTransition&&!rasterFrame.preview.page&&
      !snapshotSurface&&!snapshotAnimationSurface&&!foregroundCapture&&!renderingViewport&&!viewportOverlayLevels&&
      !snapshotKeyboard.visible&&!snapshotInput&&!snapshotMenuEnabled&&!styledFocus()&&!groupMotion.active&&!groupMotion.prepared&&
      !stagingNativeTarget&&nativePrefixLimit<0;
#if NICE_GFX_UI_NATIVE_CACHE_REPAIR
    thin=thin&&!nativeCacheRepairing;
#endif
#if NICE_GFX_UI_BACKGROUND_COVER_PROFILE
    if(thin&&profilingClock)++stats.thinBackgroundCoverSkips;
#endif
#endif
    if(!thin)cover=backgroundCover(index,clipped);
    Rect visible[4];const int count=index>=0?raster_occlusion::strips(clipped,cover,visible):1;
    for(int part=0;part<count;++part) {
      if(index>=0&&cover.width>0)canvas.setViewport(visible[part].x,visible[part].y,visible[part].width,visible[part].height,false);
      if(gradient&&s.gradient) {
        const GradientDesc &g=*s.gradient;
        canvas.fillLinearGradientRoundRect(r.x,r.y,r.width,r.height,uint8_t(radius),g.colors,g.origin,g.stepX,g.stepY,g.dither!=0);
      } else canvas.fillRoundRect(r.x,r.y,r.width,r.height,radius,color);
    }
    if(index>=0&&cover.width>0)canvas.setViewport(clipped.x,clipped.y,clipped.width,clipped.height,false);
    for(int i=0;i<s.borderWidth&&2*i<std::min<int32_t>(r.width,r.height);++i)
      canvas.drawRoundRect(r.x+i,r.y+i,r.width-2*i,r.height-2*i,std::max<int32_t>(0,radius-i),border);
  }
  uint16_t nextCode(const char *s,size_t &i,size_t end) {
    uint16_t n=0;const uint16_t code=canvas.decodeUTF8(reinterpret_cast<uint8_t *>(const_cast<char *>(s+i)),&n,uint16_t(std::min<size_t>(end-i,4)));i+=n;return code;
  }
  Rect textBox(Rect r,const NodeDesc &n,const Style &style) const {
    r=contentBox(r,style);if(n.kind==Kind::Checkbox){const int32_t w=std::min<int32_t>(r.width,int32_t(style.thumbWidth)+8);r.x+=w;r.width-=w;}return r;
  }
  template<class Cache> void prepareText(Cache &cache,Geometry &g,const NodeDesc &n,const State &v,const Style &style) {
    g.lineFirst=uint32_t(cache.lines.size());g.lineCount=0;g.textY=0;g.ellipsisCount=0;
    if(v.kind!=Kind::Text&&v.kind!=Kind::Label&&v.kind!=Kind::Button&&v.kind!=Kind::Checkbox)return;
    const char *s=v.text.c_str();Rect r=textBox(g.rect,n,style);if(r.width<=0||r.height<=0||!*s)return;
    ++stats.textLayouts;const int32_t h=style.font->height*style.fontSize;
    if(n.id==snapshotMarqueeId&&snapshotMarqueeId){
      int32_t width=0;const size_t length=strlen(s);for(size_t at=0;at<length;){const auto cp=nextCode(s,at,length);if(cp=='\n'||cp=='\r')break;width+=nice_gfx::detail::monoAdvance(*style.font,cp)*style.fontSize;}
      cache.lines.push_back({0,length,width});g.lineCount=1;g.textY=std::max<int32_t>(0,(r.height-h)/2);return;
    }
    if(menuMotionEnabled()&&menuMotionConfig.ellipsis&&(v.kind==Kind::Button||v.kind==Kind::Checkbox||r.height<2*h)){
      int32_t full=0;const size_t length=strlen(s);for(size_t at=0;at<length;){const auto cp=nextCode(s,at,length);if(cp=='\n'||cp=='\r')break;full+=nice_gfx::detail::monoAdvance(*style.font,cp)*style.fontSize;}
      if(full>r.width){const int32_t dot=std::max<int32_t>(1,nice_gfx::detail::monoAdvance(*style.font,'.')*style.fontSize);g.ellipsisCount=uint8_t(std::min<int32_t>(3,r.width/dot));const int32_t budget=r.width-g.ellipsisCount*dot;size_t end=0;int32_t width=0;
        for(size_t at=0;at<length;){const size_t before=at;const auto cp=nextCode(s,at,length);const int32_t advance=nice_gfx::detail::monoAdvance(*style.font,cp)*style.fontSize;if(cp=='\n'||cp=='\r'||width+advance>budget){end=before;break;}width+=advance;end=at;}
        cache.lines.push_back({0,end,width+g.ellipsisCount*dot});g.lineCount=1;g.textY=std::max<int32_t>(0,(r.height-h)/2);return;}
    }
    if(v.kind==Kind::Button||v.kind==Kind::Checkbox) {
      int32_t available=r.width;bool fits=true;const size_t length=strlen(s);
      for(size_t pos=0;pos<length;) {const uint16_t cp=nextCode(s,pos,length);
        if(cp=='\n'||cp=='\r'){fits=false;break;}
        const int32_t advance=nice_gfx::detail::monoAdvance(*style.font,cp)*style.fontSize;
        if(advance>available){fits=false;break;}available-=advance;
      }
      if(fits)g.textY=std::max<int32_t>(0,(r.height-h)/2);
    }
    const size_t end=strlen(s);size_t begin=0,i=0;int32_t width=0,y=g.textY;
    auto line=[&](size_t last){cache.lines.push_back({begin,last,width});++g.lineCount;};
    // Keep the visible portion of the final line; the viewport clips glyphs.
    while(i<end&&y<r.height){if(s[i]=='\r'||s[i]=='\n'){line(i);const char separator=s[i++];if(separator=='\r'&&i<end&&s[i]=='\n')++i;begin=i;width=0;y+=h;continue;}
      size_t next=i;const uint16_t cp=nextCode(s,next,end);const int32_t w=nice_gfx::detail::monoAdvance(*style.font,cp)*style.fontSize;
      if(width>0&&width+w>r.width){line(i);begin=i;width=0;y+=h;continue;}width+=w;i=next;
    }
    if(i>begin&&y<r.height)line(i);
  }
  void prepareText(const PageDesc &p,FrameCache &cache,Geometry &g,const NodeDesc &n,const State &v,const Style &style) {
#if NICE_GFX_UI_TEXT_LAYOUT_MEMO
    if(v.kind==Kind::Text||v.kind==Kind::Label||v.kind==Kind::Button||v.kind==Kind::Checkbox){
      const char *s=v.text.c_str();const Rect r=textBox(g.rect,n,style);
      if(r.width>0&&r.height>0&&*s){
        TextLayoutMemoGuard memo(*this,p,cache,g,n,v,style,r,s);if(memo.reuse())return;
        prepareText(cache,g,n,v,style);return;
      }
    }
#else
    (void)p;
#endif
    prepareText(cache,g,n,v,style);
  }
#if NICE_GFX_UI_INCREMENTAL_FRAME_GEOMETRY
  NICE_GFX_UI_GEOMETRY_IRAM_ATTR void prepareCacheGeometryNode(const PageDesc &p,const RetainedVector<State> &s,FrameCache &cache,uint16_t i,bool countChildren) {
    auto &g=cache.nodes[i];const auto &n=p.nodes[i];const auto &v=s[i];
      if(countChildren&&v.kind==Kind::Media){if(cache.mediaFirst==NoParent)cache.mediaFirst=i;cache.mediaEnd=uint16_t(i+1);}
      g.rect=baseRect(p,s,i);g.rect.x+=v.animationX;g.rect.y+=v.animationY;g.clip={0,0,p.width,p.height};g.visible=!(v.flags&Hidden);g.inputVisible=g.visible&&!(v.flags&Disabled);
      if(v.parent!=NoParent){auto &a=cache.nodes[v.parent];if(countChildren)++a.children;const Rect b=p.nodes[v.parent].rect;
        if(!viewportOverlayRoot(p,i)){
          g.rect.x+=a.rect.x-b.x-s[v.parent].scrollX;g.rect.y+=a.rect.y-b.y-s[v.parent].scrollY;
          g.clip=a.clip;if((s[v.parent].flags&Clip)||scrolling(s[v.parent].kind))g.clip=intersect(g.clip,childBox(p,s,v.parent,a.rect));
        }
        g.visible=g.visible&&a.visible;g.inputVisible=g.inputVisible&&a.inputVisible;
      }
      if(n.motion){const auto &m=motionPose(p,s,i);
        if(m.reveal<1024&&m.scale==1024&&m.rotation%360==0)g.clip=intersect(g.clip,motion_geometry::reveal(g.rect,m.reveal,n.motion->flags));
        g.rect=motion_geometry::bounds(g.rect,m.scale,m.rotation,n.motion->pivotX,n.motion->pivotY,v.kind==Kind::Shape);
      }
      // Retain the actual paint bound with this immutable geometry/pose. It
      // cannot be reconstructed later from a surface key's live motion slot.
#if NICE_GFX_UI_SHAPE_INK_DAMAGE
      g.paintRect=shapePaintBounds(p,s,i,g.rect);
#endif
      // Preserve the original descendant-to-root integer alpha rounding.
      g.alpha=v.opacity==255&&(v.parent==NoParent||cache.nodes[v.parent].alpha==255)?255:opacity(p,s,i);g.lineCount=0;
  }
#endif
  NICE_GFX_UI_GEOMETRY_IRAM_ATTR void prepareCache(const PageDesc &p,const RetainedVector<State> &s,FrameCache &cache,int32_t visibleHeight=-1) {
    GeometryTimer geometryTimer(*this);
#if NICE_GFX_UI_FRAME_GEOMETRY_IN_PLACE
    invalidateBorrowedFrameGeometry(cache);
#endif
    cache.visibleHeight=visibleHeight;
    cache.nodes.resize(p.count);cache.words=(size_t(p.count)+31)/32;cache.bandHeight=(int32_t(p.height)+31)/32;
    cache.mediaFirst=NoParent;cache.mediaEnd=0;
    cache.bands.assign(cache.words*32,0);
    cache.lines.clear();
    for(uint16_t i=0;i<p.count;++i)cache.nodes[i].children=0;
    for(uint16_t i=0;i<p.count;++i){++stats.geometryNodes;auto &g=cache.nodes[i];const auto &n=p.nodes[i];const auto &v=s[i];
#if NICE_GFX_UI_INCREMENTAL_FRAME_GEOMETRY
      prepareCacheGeometryNode(p,s,cache,i,true);
#else
      if(v.kind==Kind::Media){if(cache.mediaFirst==NoParent)cache.mediaFirst=i;cache.mediaEnd=uint16_t(i+1);}
      g.rect=baseRect(p,s,i);g.rect.x+=v.animationX;g.rect.y+=v.animationY;g.clip={0,0,p.width,p.height};g.visible=!(v.flags&Hidden);g.inputVisible=g.visible&&!(v.flags&Disabled);
      if(v.parent!=NoParent){auto &a=cache.nodes[v.parent];++a.children;const Rect b=p.nodes[v.parent].rect;
        if(!viewportOverlayRoot(p,i)){
          g.rect.x+=a.rect.x-b.x-s[v.parent].scrollX;g.rect.y+=a.rect.y-b.y-s[v.parent].scrollY;
          g.clip=a.clip;if((s[v.parent].flags&Clip)||scrolling(s[v.parent].kind))g.clip=intersect(g.clip,childBox(p,s,v.parent,a.rect));
        }
        g.visible=g.visible&&a.visible;g.inputVisible=g.inputVisible&&a.inputVisible;
      }
      if(n.motion){const auto &m=motionPose(p,s,i);
        if(m.reveal<1024&&m.scale==1024&&m.rotation%360==0)g.clip=intersect(g.clip,motion_geometry::reveal(g.rect,m.reveal,n.motion->flags));
        g.rect=motion_geometry::bounds(g.rect,m.scale,m.rotation,n.motion->pivotX,n.motion->pivotY,v.kind==Kind::Shape);
      }
      // Retain the actual paint bound with this immutable geometry/pose. It
      // cannot be reconstructed later from a surface key's live motion slot.
#if NICE_GFX_UI_SHAPE_INK_DAMAGE
      g.paintRect=shapePaintBounds(p,s,i,g.rect);
#endif
      // Preserve the original descendant-to-root integer alpha rounding.
      g.alpha=v.opacity==255&&(v.parent==NoParent||cache.nodes[v.parent].alpha==255)?255:opacity(p,s,i);g.lineCount=0;
#endif
      Rect c=intersect(g.rect,g.clip);if(visibleHeight>=0)c=intersect(c,{0,0,p.width,visibleHeight});if(!g.visible||!g.alpha||c.width<=0||c.height<=0)continue;
      prepareText(p,cache,g,n,v,styleFor(p,s,i));
      const int first=clamp32(c.y/cache.bandHeight,0,31),last=clamp32((c.y+c.height-1)/cache.bandHeight,0,31);
      for(int band=first;band<=last;++band)cache.bands[size_t(band)*cache.words+i/32]|=uint32_t(1)<<(i%32);
    }
  }
  // Pixel-only surface repairs retain exact geometry, bands and unchanged text
  // layouts. Compact copied line ranges each time so changing line counts never
  // grow a history of obsolete layouts in the retained surface.
  template<class Previous> NICE_GFX_UI_GEOMETRY_IRAM_ATTR void prepareVisualCache(const PageDesc &p,const RetainedVector<State> &s,const Previous &previous,
                          const FrameCache &source,FrameCache &cache) {
    GeometryTimer geometryTimer(*this);
#if NICE_GFX_UI_FRAME_GEOMETRY_IN_PLACE
    invalidateBorrowedFrameGeometry(cache);
#endif
    cache.nodes=source.nodes;cache.bands=source.bands;cache.words=source.words;cache.bandHeight=source.bandHeight;cache.visibleHeight=source.visibleHeight;cache.mediaFirst=source.mediaFirst;cache.mediaEnd=source.mediaEnd;cache.lines.clear();
    stats.geometryReuses+=p.count;
    for(uint16_t i=0;i<p.count;++i){auto &g=cache.nodes[i];const auto &old=source.nodes[i];
      if(previous[i].text.revision!=s[i].text.revision||previous[i].text.compiled!=s[i].text.compiled){
        Rect c=intersect(g.rect,g.clip);if(cache.visibleHeight>=0)c=intersect(c,{0,0,p.width,cache.visibleHeight});
        if(g.visible&&g.alpha&&c.width>0&&c.height>0)prepareText(p,cache,g,p.nodes[i],s[i],styleFor(p,s,i));
        else {g.lineFirst=uint32_t(cache.lines.size());g.lineCount=0;g.textY=0;}
      }else{
        g.lineFirst=uint32_t(cache.lines.size());
        if(old.lineCount){cache.lines.insert(cache.lines.end(),source.lines.begin()+old.lineFirst,source.lines.begin()+old.lineFirst+old.lineCount);++stats.textLayoutReuses;}
      }
    }
  }
  void text(int i,const NodeDesc &n,const State &v,const Style &style,Rect clipped) {
    const auto &g=paintCache->nodes[i];Rect r=textBox(g.rect,n,style);r.x+=paintOffsetX;r.y+=paintOffsetY;
    const Rect visible=intersect(clipped,r);if(!g.lineCount||visible.width<=0||visible.height<=0)return;
    canvas.setViewport(visible.x,visible.y,visible.width,visible.height,false);canvas.setFont(style.font);canvas.setTextSize(style.fontSize);canvas.setTextColor(style.color);
#if NICE_GFX_UI_TEXT_MASK_NATIVE && NICE_GFX_UI_BUTTON_TEXT_MASK
    // The button background/selection/focus decorations were already painted.
    // Composite only its ink, using the exact supplied effective text/style.
    // Any whole-node alpha is applied by the caller after this opaque pass.
    const bool coldStage=NICE_GFX_UI_VIEWPORT_ANIMATION_ATLAS&&buildingColdFadeStage&&stagingNativeTarget;
    const auto *captionTarget=coldStage?stagingNativeTarget:nativeOverlayTarget;
    if(n.kind==Kind::Button&&rasterFrame.current.page->nodes[i].kind==Kind::Button&&!n.instrument&&!v.instrument&&
       !displayPolicy.monochrome&&renderOrder==PixelOrder::Native&&!buildingSurface&&!nativeCacheRepairing&&!foregroundCapture&&nativePrefixLimit<0&&
       !snapshotTransition&&!rasterFrame.preview.page&&!snapshotSurface&&!previewSurface&&!snapshotAnimationSurface&&(!stagingNativeTarget||coldStage)&&
       rasterFrame.current.page==&page()&&captionTarget&&canvas.targetsNativeFramebuffer(*captionTarget)){
      bool ordinary=true;const auto &p=*rasterFrame.current.page;const auto &values=rasterFrame.current.values;
      for(int at=i;at>=0;at=values[at].parent==NoParent?-1:values[at].parent)if(p.nodes[at].motion){const auto &pose=motionPose(p,values,at);if(pose.scale!=1024||pose.rotation%360||pose.reveal!=1024){ordinary=false;break;}}
      if(ordinary&&paintTextMaskInk(i,n,v,style,visible,255,true)==Status::Ok){if(coldStage)++stats.coldFadeButtonMaskBlits;canvas.setViewport(clipped.x,clipped.y,clipped.width,clipped.height,false);return;}
    }
#endif
    {
    ProfileTimer legacyGlyphTimer(*this,&RuntimeStats::legacyGlyphInkMicros);
    const char *s=v.text.c_str();int32_t y=r.y+g.textY;const int32_t h=style.font->height*style.fontSize;
    for(size_t lineIndex=g.lineFirst;lineIndex<size_t(g.lineFirst)+g.lineCount;++lineIndex){const auto &line=paintCache->lines[lineIndex];if(y+h>clipped.y&&y<clipped.y+clipped.height){
      int32_t x=r.x;if(n.id==snapshotMarqueeId)x-=snapshotMarqueeOffset;else if(style.textAlign==1)x+=std::max<int32_t>(0,(r.width-line.width)/2);else if(style.textAlign==2)x+=std::max<int32_t>(0,r.width-line.width);
      for(size_t j=line.begin;j<line.end;){if(s[j]=='\r'){++j;continue;}const uint16_t cp=nextCode(s,j,line.end);if(cp>=32&&cp!=127)x+=canvas.drawChar(cp,x,y);}
      for(uint8_t dot=0;dot<g.ellipsisCount;++dot)x+=canvas.drawChar('.',x,y);
    }y+=h;}
    }
    canvas.setViewport(clipped.x,clipped.y,clipped.width,clipped.height,false);
  }
  Status paintTextMaskInk(int index,const NodeDesc &n,const State &value,const Style &style,Rect clipped,uint8_t alpha,bool button) {
#if NICE_GFX_UI_TEXT_MASK_NATIVE
    const auto *target=stagingNativeTarget?stagingNativeTarget:nativeOverlayTarget;
    if(!target)return Status::Unsupported;
    const auto &geometry=paintCache->nodes[index];
    if(geometry.ellipsisCount||n.id==snapshotMarqueeId)return Status::Unsupported;
    Rect box=textBox(geometry.rect,n,style);box.x+=paintOffsetX;box.y+=paintOffsetY;
    clipped=intersect(clipped,box);if(clipped.width<=0||clipped.height<=0||!geometry.lineCount)return Status::Ok;
    if(geometry.lineFirst>paintCache->lines.size()||geometry.lineCount>paintCache->lines.size()-geometry.lineFirst)return Status::Unsupported;
    const detail::TextMaskLayout layout{style.font,value.text.c_str(),paintCache->lines.data()+geometry.lineFirst,
      geometry.lineCount,box.width,box.height,geometry.textY,style.fontSize,style.textAlign,(value.flags&NoAntialias)==0};
    const uint64_t previousHits=textMaskCache.hits(),previousBuilds=textMaskCache.builds(),previousRejects=textMaskCache.rejected();
    const uint32_t resolveStarted=profilingClock?profilingClock():0;
    detail::TextMaskView mask;const bool resolved=textMaskCache.resolve(layout,mask);
    if(profilingClock)stats.textResolveMicros+=profilingClock()-resolveStarted;
    stats.textMaskHits+=textMaskCache.hits()-previousHits;stats.textMaskBuilds+=textMaskCache.builds()-previousBuilds;
    stats.textMaskRejects+=textMaskCache.rejected()-previousRejects;stats.textMaskBytes=textMaskCache.bytes();
    if(!resolved)return Status::Unsupported;
    stats.textMaskRoiPixels+=uint64_t(clipped.width)*clipped.height;
    if(mask.empty){++stats.textMaskEmptySkips;return Status::Ok;}
#if NICE_GFX_UI_TEXT_MASK_INK_BOUNDS
    Rect ink;if(mask.inkBounds(ink)){
      const Rect previous=clipped;ink.x+=box.x;ink.y+=box.y;clipped=intersect(clipped,ink);
      if(clipped.width!=previous.width||clipped.height!=previous.height)++stats.textMaskBoundsClips;
    }
#endif
    if(clipped.width<=0||clipped.height<=0){++stats.textMaskEmptySkips;return Status::Ok;}
    stats.textMaskCompositePixels+=uint64_t(clipped.width)*clipped.height;
    const uint32_t compositeStarted=profilingClock?profilingClock():0;
    const bool composited=detail::blendTextMaskIntoNative(*target,clipped.x,clipped.y,mask,
      size_t(clipped.x-box.x),size_t(clipped.y-box.y),size_t(clipped.width),size_t(clipped.height),style.color,alpha);
    if(profilingClock)stats.textCompositeMicros+=profilingClock()-compositeStarted;
    if(!composited)return Status::Unsupported;
    if(button)++stats.buttonTextMaskBlits;else ++stats.textNativeBlits;return Status::Ok;
#else
    (void)index;(void)n;(void)value;(void)style;(void)clipped;(void)alpha;(void)button;return Status::Unsupported;
#endif
  }
  Status paintTextMaskNative(int index,Rect clipped,uint8_t alpha) {
#if NICE_GFX_UI_TEXT_MASK_NATIVE
    // Geometry and offsets describe the selected transition layer. Cold fades
    // paint into a Native565 stage, while surface repairs own a scratch target.
    // Reuse the exact ink compositor without writing into the wrong frame.
    const auto *target=stagingNativeTarget?stagingNativeTarget:nativeOverlayTarget;
    if(!alpha||displayPolicy.monochrome||renderOrder!=PixelOrder::Native||buildingSurface||!target||
       focusStyle(index)||snapshotPressed==index)return Status::Unsupported;
    const auto &page=*rasterFrame.current.page;const auto &values=rasterFrame.current.values;
    const auto &n=page.nodes[index];const auto &value=values[index];
    if((n.kind!=Kind::Text&&n.kind!=Kind::Label)||!(value.flags&Transparent))return Status::Unsupported;
    for(int at=index;at>=0;at=values[at].parent==NoParent?-1:values[at].parent){
      if(values[at].flags&Disabled)return Status::Unsupported;
      if(page.nodes[at].motion){
        // This leaf's translation and opacity are already resolved in Geometry.
        // Its untransformed glyph coverage stays reusable across those samples.
        // Ancestor motion and local affine/reveal poses keep the ordered painter.
        if(at!=index)return Status::Unsupported;
        const auto &pose=motionPose(page,values,at);
        if(pose.scale!=1024||pose.rotation%360!=0||pose.reveal!=1024)return Status::Unsupported;
      }
    }
    const auto &style=styleFor(page,values,index);
    if(!style.font||style.borderWidth||style.radius||style.gradient)return Status::Unsupported;
    return paintTextMaskInk(index,n,value,style,clipped,alpha,false);
#else
    (void)index;(void)clipped;(void)alpha;return Status::Unsupported;
#endif
  }
  Status paintTextMaskOverlay(int index,Rect clipped,uint8_t alpha) {
    if(!nativeGpuPaintAllowed())return Status::Unsupported;
#if NICE_GFX_UI_TEXT_MASK_PPA
    if(!alpha||!ppaBlendEnabled||!rgb565_ppa::Context::available()||displayPolicy.monochrome||
       renderOrder!=PixelOrder::Native||buildingSurface||snapshotTransition||rasterFrame.preview.page||
       !nativeOverlayTarget||stagingNativeTarget||focusStyle(index)||snapshotPressed==index)return Status::Unsupported;
    const auto &page=*rasterFrame.current.page;const auto &values=rasterFrame.current.values;
    const auto &n=page.nodes[index];const auto &value=values[index];
    if((n.kind!=Kind::Text&&n.kind!=Kind::Label)||!(value.flags&Transparent))return Status::Unsupported;
    // Translation and inherited opacity are already in Geometry. Keep other
    // motion/focus/disabled semantics on the original ordered node painter.
    for(int at=index;at>=0;at=values[at].parent==NoParent?-1:values[at].parent)
      if((values[at].flags&Disabled)||page.nodes[at].motion)return Status::Unsupported;
    const auto &style=styleFor(page,values,index);
    if(!style.font||style.borderWidth||style.radius||style.gradient)return Status::Unsupported;
    const auto &geometry=paintCache->nodes[index];
    Rect box=textBox(geometry.rect,n,style);box.x+=paintOffsetX;box.y+=paintOffsetY;
    clipped=intersect(clipped,box);if(clipped.width<=0||clipped.height<=0||!geometry.lineCount)return Status::Ok;
    if(geometry.lineFirst>paintCache->lines.size()||geometry.lineCount>paintCache->lines.size()-geometry.lineFirst)return Status::Unsupported;
    const detail::TextMaskLayout layout{style.font,value.text.c_str(),paintCache->lines.data()+geometry.lineFirst,
      geometry.lineCount,box.width,box.height,geometry.textY,style.fontSize,style.textAlign,(value.flags&NoAntialias)==0};
    detail::TextMaskView mask;if(!textMaskCache.resolve(layout,mask))return Status::Unsupported;
    stats.textMaskRoiPixels+=uint64_t(clipped.width)*clipped.height;
    if(mask.empty){++stats.textMaskEmptySkips;return Status::Ok;}
#if NICE_GFX_UI_TEXT_MASK_INK_BOUNDS
    Rect ink;if(mask.inkBounds(ink)){
      const Rect previous=clipped;ink.x+=box.x;ink.y+=box.y;clipped=intersect(clipped,ink);
      if(clipped.width!=previous.width||clipped.height!=previous.height)++stats.textMaskBoundsClips;
    }
#endif
    if(clipped.width<=0||clipped.height<=0){++stats.textMaskEmptySkips;return Status::Ok;}
    stats.textMaskCompositePixels+=uint64_t(clipped.width)*clipped.height;
    platform::OverlayView overlay;overlay.data=mask.data;overlay.width=mask.width;overlay.height=mask.height;
    overlay.stride=mask.stride;overlay.bytes=mask.bytes;overlay.format=platform::OverlayFormat::Alpha8;
    overlay.alpha=alpha;overlay.color=style.color;
    const size_t x=size_t(clipped.x-box.x),y=size_t(clipped.y-box.y);
    const uint32_t started=profilingClock?profilingClock():0;
    const Status result=ppaFade.blendOverlayIntoNative(*nativeOverlayTarget,clipped.x,clipped.y,overlay,x,y,clipped.width,clipped.height);
    if(result==Status::Ok){++stats.textGpuBlits;++stats.ppaBlendCalls;stats.ppaBlendPixels+=uint64_t(clipped.width)*clipped.height;
      if(profilingClock)stats.ppaBlendMicros+=profilingClock()-started;}
    return result;
#else
    (void)index;(void)clipped;(void)alpha;return Status::Unsupported;
#endif
  }
  using ComponentPaint=detail::ComponentPaint;
  detail::ComponentPaintIndex &prepareComponentPaint(const PageDesc &p) const {
    size_t slot=0;
    if(componentPaintIndices[0].matches(p,components,componentCount))slot=0;
    else if(componentPaintIndices[1].matches(p,components,componentCount))slot=1;
    else {
      slot=componentPaintUsed[0]<=componentPaintUsed[1]?0:1;
      componentPaintIndices[slot].prepare(p,components,componentCount);++stats.componentIndexBuilds;
    }
    componentPaintUsed[slot]=++componentPaintClock;return componentPaintIndices[slot];
  }
  ComponentPaint componentPaint(const PageDesc &p,const RetainedVector<State> &values,Id id) const {
    return componentCount?prepareComponentPaint(p).paint(id,values):ComponentPaint{};
  }
  void paintRoundedMedia(const MediaDesc &m,const Image::Impl *image,const uint8_t *frame,size_t stride,
                         Rect r,Rect clipped,const MediaRoundClip &clip) {
    const Rect area=intersect(r,clipped);if(area.width<=0||area.height<=0)return;
    mediaRow.resize(size_t(area.width));uint8_t *bytes=reinterpret_cast<uint8_t *>(mediaRow.data());
    const int32_t xStep=m.width/r.width,xRemainder=m.width%r.width;
    const int64_t yNumerator=int64_t(area.y-r.y)*m.height;
    int32_t sourceY=int32_t(yNumerator/r.height),remainderY=int32_t(yNumerator%r.height);
    const int32_t yStep=m.height/r.height,yRemainder=m.height%r.height;
    for(int32_t y=area.y;y<area.y+area.height;++y) {
      int32_t first=area.x,end=area.x+area.width;clip.span(y,first,end);
      if(first<end) {
        const uint8_t *row=frame+size_t(sourceY)*stride;
        const int64_t xNumerator=int64_t(first-r.x)*m.width;
        int32_t sourceX=int32_t(xNumerator/r.width),remainderX=int32_t(xNumerator%r.width);
        for(int32_t col=0;col<end-first;++col) {
          const uint8_t *p=row+size_t(sourceX)*m.bytesPerPixel;
          if(m.bytesPerPixel==2) {
            if(image&&!image->flash&&image->source.order==PixelOrder::Native){uint16_t color;memcpy(&color,p,2);bytes[2*col]=uint8_t(color>>8);bytes[2*col+1]=uint8_t(color);}
            else if(image&&!image->flash){bytes[2*col]=p[0];bytes[2*col+1]=p[1];}
            else {bytes[2*col]=pgm_read_byte(p);bytes[2*col+1]=pgm_read_byte(p+1);}
          } else {
            const uint8_t value=image&&!image->flash?*p:pgm_read_byte(p);
            const uint16_t red=((value>>5)*31+3)/7,green=((value>>2)&7)*9,blue=((value&3)*31+1)/3,color=(red<<11)|(green<<5)|blue;
            bytes[2*col]=uint8_t(color>>8);bytes[2*col+1]=uint8_t(color);
          }
          sourceX+=xStep;remainderX+=xRemainder;if(remainderX>=r.width){remainderX-=r.width;++sourceX;}
        }
        canvas.blit(first,y,PixelView(mediaRow.data(),size_t(end-first),1,0,PixelOrder::Wire));
      }
      sourceY+=yStep;remainderY+=yRemainder;if(remainderY>=r.height){remainderY-=r.height;++sourceY;}
    }
  }
  Status paintShapeWire(int index,Rect clipped,const DirtyRect &area,uint16_t *pixels,size_t stride,uint8_t alpha) {
#if NICE_GFX_UI_SHAPE_WIRE_SPANS
    if(!alpha||(!buildingSurface&&renderOrder!=PixelOrder::Wire)||displayPolicy.monochrome||focusStyle(index)||snapshotPressed==index)return Status::Unsupported;
    const auto &n=rasterFrame.current.page->nodes[index];const auto &v=rasterFrame.current.values[index];
    if(n.kind!=Kind::Shape||!n.shape||n.motion||!(v.flags&Transparent)||(v.flags&Disabled))return Status::Unsupported;
    for(uint16_t parent=v.parent;parent!=NoParent;parent=rasterFrame.current.values[parent].parent)if(rasterFrame.current.values[parent].flags&Disabled)return Status::Unsupported;
    const auto &style=styleFor(*rasterFrame.current.page,rasterFrame.current.values,index);
    if(style.radius||style.borderWidth||style.gradient||style.paddingTop||style.paddingRight||style.paddingBottom||style.paddingLeft)return Status::Unsupported;
    Rect box=paintCache->nodes[index].rect;box.x+=paintOffsetX;box.y+=paintOffsetY;
    clipped=intersect(box,clipped);if(clipped.width<=0||clipped.height<=0)return Status::Ok;
    detail::ShapeOverlayCache::NativeTexture texture;if(!shapeOverlayCache.resolveNative(*n.shape,box,0,texture))return Status::Unsupported;
    NativeFramebuffer frame;frame.pixels=pixels;frame.width=area.width;frame.height=area.height;frame.stride=stride;frame.bytes=(size_t(area.height-1)*stride+area.width)*2;
    box.x-=area.x;box.y-=area.y;clipped.x-=area.x;clipped.y-=area.y;
    const Status result=detail::paintShapeNativeSpans<true>(frame,texture,box,clipped,alpha);if(result==Status::Ok){++stats.shapeNativeBlits;markShapePaintPath(ShapePaintPath::WireSpanCache);}return result;
#else
    (void)index;(void)clipped;(void)area;(void)pixels;(void)stride;(void)alpha;return Status::Unsupported;
#endif
  }
#if NICE_GFX_UI_STAGE_SHAPE_NATIVE_SPANS
  Status paintStageShapeNative(int index,Rect clipped,const DirtyRect &area,uint16_t *pixels,size_t stride,uint8_t alpha) {
#if NICE_GFX_UI_NATIVE_STAGE && NICE_GFX_UI_RETAINED_CHART_CPU_CONTEXT && NICE_GFX_UI_SHAPE_NATIVE_SPANS
    if(!alpha||!nativeStagePainting||renderOrder!=PixelOrder::Native||buildingSurface||snapshotTransition||
       rasterFrame.preview.page||nativeOverlayTarget||stagingNativeTarget||focusStyle(index)||snapshotPressed==index)
      return Status::Unsupported;
    const auto &n=rasterFrame.current.page->nodes[index];const auto &v=rasterFrame.current.values[index];
    if(n.kind!=Kind::Shape||!n.shape||!(v.flags&Transparent)||(v.flags&Disabled))return Status::Unsupported;
    if(n.motion){
#if defined(CONFIG_IDF_TARGET_ESP32P4)
      const auto &pose=motionPose(*rasterFrame.current.page,rasterFrame.current.values,index);
      if(pose.scale!=1024||pose.rotation%360)return Status::Unsupported;
#else
      return Status::Unsupported;
#endif
    }
    for(uint16_t parent=v.parent;parent!=NoParent;parent=rasterFrame.current.values[parent].parent)
      if(rasterFrame.current.values[parent].flags&Disabled)return Status::Unsupported;
    const auto &style=styleFor(*rasterFrame.current.page,rasterFrame.current.values,index);
    if(style.radius||style.borderWidth||style.gradient||style.paddingTop||style.paddingRight||style.paddingBottom||style.paddingLeft)
      return Status::Unsupported;
    if(nativeStageFatal||!nativeStageStorage||!nativeStageSourceOwner||!nativeStageFrozenSnapshotMatches()||
       area.x<0||area.y<0||area.width<=0||area.height<=0||stride<2||
       int64_t(area.x)+area.width>INT32_MAX||int64_t(area.y)+area.height>INT32_MAX)
      return Status::IoError;
    const size_t width=size_t(area.width),height=size_t(area.height);
    const size_t stageWords=NativeStageBytes/sizeof(uint16_t),payloadWords=stageWords-2*NativeStageGuardWords;
    if(stride>payloadWords||(stride&31u)||width>stride-2)return Status::IoError;
    const size_t pictureRows=payloadWords/stride;
    if(pictureRows<=2||height>std::min<size_t>(75,pictureRows-2))return Status::IoError;
    const size_t offsetWords=NativeStageGuardWords+stride+1;
    if(height-1>(SIZE_MAX-width)/stride)return Status::IoError;
    const size_t envelopeWords=(height-1)*stride+width;
    if(offsetWords>stageWords-NativeStageGuardWords||
       envelopeWords>stageWords-NativeStageGuardWords-offsetWords||envelopeWords>SIZE_MAX/sizeof(uint16_t))
      return Status::IoError;
    auto *memory=nativeStageStorage.get();const uintptr_t allocation=reinterpret_cast<uintptr_t>(memory);
    if((allocation&63u)||allocation>UINTPTR_MAX-NativeStageBytes||pixels!=memory+offsetWords)return Status::IoError;

    Rect box=paintCache->nodes[index].rect;
    const int64_t globalX=int64_t(box.x)+paintOffsetX,globalY=int64_t(box.y)+paintOffsetY;
    if(box.width<=0||box.height<=0||globalX<INT32_MIN||globalX>INT32_MAX||globalY<INT32_MIN||globalY>INT32_MAX||
       globalX+box.width>INT32_MAX||globalY+box.height>INT32_MAX)return Status::Unsupported;
    box.x=int32_t(globalX);box.y=int32_t(globalY);
    if(clipped.width<=0||clipped.height<=0)return Status::Ok;
    const int64_t left=std::max(globalX,std::max(int64_t(clipped.x),int64_t(area.x)));
    const int64_t top=std::max(globalY,std::max(int64_t(clipped.y),int64_t(area.y)));
    const int64_t right=std::min(globalX+box.width,std::min(int64_t(clipped.x)+clipped.width,int64_t(area.x)+area.width));
    const int64_t bottom=std::min(globalY+box.height,std::min(int64_t(clipped.y)+clipped.height,int64_t(area.y)+area.height));
    if(right<=left||bottom<=top)return Status::Ok;
    clipped={int32_t(left),int32_t(top),int32_t(right-left),int32_t(bottom-top)};
    detail::ShapeOverlayCache::NativeTexture texture;
    const uint32_t resolveStarted=profilingClock?profilingClock():0;
    // The original global box retains the cache's translation qualification.
    const bool resolved=shapeOverlayCache.resolveNative(*n.shape,box,style.radius,texture);
    if(profilingClock)stats.shapeResolveMicros+=profilingClock()-resolveStarted;
    if(!resolved)return Status::Unsupported;
    if(nativeMemoryOverlap(allocation,NativeStageBytes,texture.spans,texture.bytes)||
       nativeMemoryOverlap(allocation,NativeStageBytes,texture.argb,texture.argbBytes))return Status::IoError;
    const int64_t localX=int64_t(box.x)-area.x,localY=int64_t(box.y)-area.y;
    if(localX<INT32_MIN||localX>INT32_MAX||localY<INT32_MIN||localY>INT32_MAX||
       localX+box.width>INT32_MAX||localY+box.height>INT32_MAX)return Status::Unsupported;
    box.x=int32_t(localX);box.y=int32_t(localY);
    clipped.x-=area.x;clipped.y-=area.y;
    NativeFramebuffer frame;frame.pixels=pixels;frame.width=width;frame.height=height;
    frame.stride=stride;frame.bytes=envelopeWords*sizeof(uint16_t);
    const uint32_t compositeStarted=profilingClock?profilingClock():0;
    if(!nativeStageFrozenSnapshotMatches())return Status::IoError;
    const Status status=detail::paintShapeNativeSpans<false>(frame,texture,box,clipped,alpha);
    if(profilingClock)stats.shapeCompositeMicros+=profilingClock()-compositeStarted;
    if(status==Status::Ok){++stats.shapeNativeBlits;markShapePaintPath(alpha==255?ShapePaintPath::NativeSpanOpaque:ShapePaintPath::NativeSpanAlpha);}
    return status;
#else
    (void)index;(void)clipped;(void)area;(void)pixels;(void)stride;(void)alpha;return Status::Unsupported;
#endif
  }
#endif
  Status paintShapeNative(int index,Rect clipped,uint8_t alpha) {
#if NICE_GFX_UI_SHAPE_NATIVE_SPANS
    if(!alpha||renderOrder!=PixelOrder::Native||buildingSurface||snapshotTransition||rasterFrame.preview.page||
       !nativeOverlayTarget||stagingNativeTarget||focusStyle(index)||snapshotPressed==index)return Status::Unsupported;
    const auto &n=rasterFrame.current.page->nodes[index];const auto &v=rasterFrame.current.values[index];
    if(n.kind!=Kind::Shape||!n.shape||!(v.flags&Transparent)||(v.flags&Disabled))return Status::Unsupported;
    if(n.motion){
#if defined(CONFIG_IDF_TARGET_ESP32P4)
      const auto &pose=motionPose(*rasterFrame.current.page,rasterFrame.current.values,index);if(pose.scale!=1024||pose.rotation%360)return Status::Unsupported;
#else
      return Status::Unsupported;
#endif
    }
    for(uint16_t parent=v.parent;parent!=NoParent;parent=rasterFrame.current.values[parent].parent)
      if(rasterFrame.current.values[parent].flags&Disabled)return Status::Unsupported;
    const auto &style=styleFor(*rasterFrame.current.page,rasterFrame.current.values,index);
    if(style.radius||style.borderWidth||style.gradient||style.paddingTop||style.paddingRight||style.paddingBottom||style.paddingLeft)return Status::Unsupported;
    Rect box=paintCache->nodes[index].rect;box.x+=paintOffsetX;box.y+=paintOffsetY;
    clipped=intersect(box,clipped);if(clipped.width<=0||clipped.height<=0)return Status::Ok;
#if NICE_GFX_UI_SHAPE_ALPHA_PPA
    // Large alpha spans are slower than PPA on the measured P4. Keep opaque
    // fills and small shapes on CPU, and retain the cached-span fallback when
    // the GPU cannot accept this exact source/target/window.
    // Choose by whole geometry so small dirty clips use the same rounding as
    // a full redraw of this shape.
    if(ppaBlendEnabled&&alpha<255&&uint64_t(box.width)*box.height>=NICE_GFX_UI_SHAPE_ALPHA_PPA_MIN_PIXELS){
      const Status accelerated=paintShapeOverlay(index,clipped,alpha);
      if(accelerated!=Status::Unsupported)return accelerated;
    }
#endif
    detail::ShapeOverlayCache::NativeTexture texture;
    const uint32_t resolveStarted=profilingClock?profilingClock():0;
    const bool resolved=shapeOverlayCache.resolveNative(*n.shape,box,style.radius,texture);
    if(profilingClock)stats.shapeResolveMicros+=profilingClock()-resolveStarted;
    if(!resolved)return Status::Unsupported;
    const uint32_t compositeStarted=profilingClock?profilingClock():0;
    const Status status=detail::paintShapeNativeSpans(*nativeOverlayTarget,texture,box,clipped,alpha);
    if(profilingClock)stats.shapeCompositeMicros+=profilingClock()-compositeStarted;
    if(status==Status::Ok){++stats.shapeNativeBlits;markShapePaintPath(alpha==255?ShapePaintPath::NativeSpanOpaque:ShapePaintPath::NativeSpanAlpha);}
    return status;
#else
    (void)index;(void)clipped;(void)alpha;return Status::Unsupported;
#endif
  }
  Status paintShapeOverlay(int index,Rect clipped,uint8_t alpha) {
    if(!nativeGpuPaintAllowed())return Status::Unsupported;
#if NICE_GFX_UI_SHAPE_OVERLAY_PPA
#if defined(CONFIG_IDF_TARGET_ESP32P4)
    if(!ppaBlendEnabled)return Status::Unsupported;
#endif
    if(!alpha||renderOrder!=PixelOrder::Native||buildingSurface||snapshotTransition||rasterFrame.preview.page||
       !nativeOverlayTarget||stagingNativeTarget||focusStyle(index)||snapshotPressed==index)return Status::Unsupported;
    const auto &n=rasterFrame.current.page->nodes[index];const auto &v=rasterFrame.current.values[index];
    if(n.kind!=Kind::Shape||!n.shape||!(v.flags&Transparent)||(v.flags&Disabled))return Status::Unsupported;
    if(n.motion){
#if defined(CONFIG_IDF_TARGET_ESP32P4)
      const auto &pose=motionPose(*rasterFrame.current.page,rasterFrame.current.values,index);if(pose.scale!=1024||pose.rotation%360)return Status::Unsupported;
#else
      return Status::Unsupported;
#endif
    }
    for(uint16_t parent=v.parent;parent!=NoParent;parent=rasterFrame.current.values[parent].parent)
      if(rasterFrame.current.values[parent].flags&Disabled)return Status::Unsupported;
    const auto &style=styleFor(*rasterFrame.current.page,rasterFrame.current.values,index);
    if(style.radius||style.borderWidth||style.gradient||style.paddingTop||style.paddingRight||style.paddingBottom||style.paddingLeft)return Status::Unsupported;
    Rect box=paintCache->nodes[index].rect;box.x+=paintOffsetX;box.y+=paintOffsetY;
    clipped=intersect(box,clipped);if(clipped.width<=0||clipped.height<=0)return Status::Ok;
    ShapeDesc shape=*n.shape;detail::ShapeOverlayCache::Texture texture;
    if(!shapeOverlayCache.resolve(shape,box,style.radius,texture))return Status::Unsupported;
    platform::OverlayView overlay;overlay.data=reinterpret_cast<const uint8_t *>(texture.pixels);
    overlay.width=texture.width;overlay.height=texture.height;overlay.stride=texture.stride;overlay.bytes=texture.bytes;
    overlay.format=platform::OverlayFormat::Argb8888;overlay.alpha=alpha;
    const size_t x=size_t(clipped.x-box.x),y=size_t(clipped.y-box.y);
    const uint32_t started=profilingClock?profilingClock():0;
    const Status status=ppaFade.blendOverlayIntoNative(*nativeOverlayTarget,clipped.x,clipped.y,overlay,x,y,clipped.width,clipped.height);
    if(status==Status::Ok){markShapePaintPath(ShapePaintPath::GpuOverlay);++stats.shapeGpuBlits;++stats.ppaBlendCalls;stats.ppaBlendPixels+=uint64_t(clipped.width)*clipped.height;
      if(profilingClock)stats.ppaBlendMicros+=profilingClock()-started;}
    return status;
#else
    (void)index;(void)clipped;(void)alpha;return Status::Unsupported;
#endif
  }
  Status paintRgb565MediaOverlay(int index,Rect clipped,uint8_t alpha) {
    if(!nativeGpuPaintAllowed())return Status::Unsupported;
#if NICE_GFX_UI_NATIVE_CACHE_REPAIR && defined(CONFIG_IDF_TARGET_ESP32P4)
    if(!alpha||renderOrder!=PixelOrder::Native||buildingSurface||snapshotTransition||rasterFrame.preview.page||!nativeOverlayTarget||stagingNativeTarget||focusStyle(index)||snapshotPressed==index)return Status::Unsupported;
    const auto &n=rasterFrame.current.page->nodes[index];const auto &v=rasterFrame.current.values[index];
    if(n.kind!=Kind::Media||v.imageOverride||(v.flags&Disabled)||!n.media||n.media->bytesPerPixel!=2||!n.media->width||!n.media->height||v.mediaFrame>=n.media->frames||!esp_ptr_in_drom(n.media->pixels))return Status::Unsupported;
    if(n.motion){const auto &pose=motionPose(*rasterFrame.current.page,rasterFrame.current.values,index);if(pose.scale!=1024||pose.rotation%360)return Status::Unsupported;}
    const size_t frameBytes=mediaFrameBytes(*n.media);const uintptr_t address=reinterpret_cast<uintptr_t>(n.media->pixels);
    if(!frameBytes||n.media->frames>(UINTPTR_MAX-address)/frameBytes||!esp_ptr_in_drom(reinterpret_cast<const void*>(address+frameBytes*n.media->frames-1)))return Status::Unsupported;
    for(uint16_t parent=v.parent;parent!=NoParent;parent=rasterFrame.current.values[parent].parent)if(rasterFrame.current.values[parent].flags&Disabled)return Status::Unsupported;
    const auto &style=styleFor(*rasterFrame.current.page,rasterFrame.current.values,index);
    if(style.radius||style.borderWidth||style.gradient||style.paddingTop||style.paddingRight||style.paddingBottom||style.paddingLeft)return Status::Unsupported;
    Rect box=paintCache->nodes[index].rect;box.x+=paintOffsetX;box.y+=paintOffsetY;
    clipped=intersect(box,clipped);if(clipped.width<=0||clipped.height<=0)return Status::Ok;
    platform::OverlayView source;if(!rgb565MediaCache.resolveFrame(*n.media,v.mediaFrame,box.width,box.height,source))return Status::Unsupported;source.alpha=alpha;
    if(alpha==255){
      const size_t sx=size_t(clipped.x-box.x),sy=size_t(clipped.y-box.y);
      const PixelView pixels(source.data,source.width,source.height,source.stride,PixelOrder::Native);
      Status copied=Status::Unsupported;
      if(!clipped.x&&size_t(clipped.width)==nativeOverlayTarget->width&&source.stride==size_t(clipped.width)){
        if(auto *panel=nice_gfx::detail::DisplayAccess::panel(display)){
          const PixelView rows(source.data+(sy*source.stride+sx)*2,clipped.width,clipped.height,source.stride,PixelOrder::Native);
          if(nativeCacheRepairing){
#if NICE_GFX_UI_OFFSCREEN_GDMA
            copied=panel->copySurfaceRows(*nativeOverlayTarget,clipped.y,rows);
            if(copied==Status::Ok){++stats.offscreenGdmaCopies;stats.offscreenGdmaPixels+=uint64_t(clipped.width)*clipped.height;}
#endif
          }else if(panel->canFrameBlit(rows)){
            copied=panel->flush(0,clipped.y,rows,false);
            if(copied==Status::Ok&&panel->busy())copied=panel->finish();
            if(copied!=Status::Ok&&copied!=Status::Unsupported)panel->finish();
          }
        }
      }
      if(copied==Status::Unsupported)copied=ppaFade.copyIntoNative(*nativeOverlayTarget,clipped.x,clipped.y,pixels,source.bytes,sx,sy,clipped.width,clipped.height);
      if(copied==Status::Ok){++stats.rgb565NativeCopies;return copied;}
      if(copied!=Status::Unsupported)return copied;
    }
    const Status result=ppaFade.blendOverlayIntoNative(*nativeOverlayTarget,clipped.x,clipped.y,source,clipped.x-box.x,clipped.y-box.y,clipped.width,clipped.height);
    if(result==Status::Ok){++stats.ppaBlendCalls;stats.ppaBlendPixels+=uint64_t(clipped.width)*clipped.height;}return result;
#else
    (void)index;(void)clipped;(void)alpha;return Status::Unsupported;
#endif
  }
  Status paintKeyedMediaOverlay(int index,Rect clipped,uint8_t alpha) {
    if(!nativeGpuPaintAllowed())return Status::Unsupported;
#if NICE_GFX_UI_KEYED_MEDIA_PPA && (defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32P4))
    if(!alpha||!ppaBlendEnabled||renderOrder!=PixelOrder::Native||buildingSurface||snapshotTransition||rasterFrame.preview.page||
       !nativeOverlayTarget||stagingNativeTarget||focusStyle(index)||snapshotPressed==index)return Status::Unsupported;
#if !defined(CONFIG_IDF_TARGET_ESP32P4)
    if(alpha!=255)return Status::Unsupported;
#endif
    const auto &n=rasterFrame.current.page->nodes[index];const auto &v=rasterFrame.current.values[index];
    if(n.kind!=Kind::Media||v.imageOverride||!(v.flags&Transparent)||(v.flags&Disabled)||!n.media||
       !mediaColorKeyed(*n.media)||v.mediaFrame>=n.media->frames)return Status::Unsupported;
#if defined(CONFIG_IDF_TARGET_ESP32P4)
    if(n.motion){const auto &pose=motionPose(*rasterFrame.current.page,rasterFrame.current.values,index);if(pose.scale!=1024||pose.rotation%360)return Status::Unsupported;}
#else
    if(n.motion||n.media->frames!=1||v.mediaFrame)return Status::Unsupported;
#endif
    for(uint16_t parent=v.parent;parent!=NoParent;parent=rasterFrame.current.values[parent].parent)
      if(rasterFrame.current.values[parent].flags&Disabled)return Status::Unsupported;
    const auto &style=styleFor(*rasterFrame.current.page,rasterFrame.current.values,index);
    if(style.radius||style.borderWidth||style.gradient||style.paddingTop||style.paddingRight||style.paddingBottom||style.paddingLeft)return Status::Unsupported;
    const auto &m=*n.media;Rect box=paintCache->nodes[index].rect;box.x+=paintOffsetX;box.y+=paintOffsetY;
#if !defined(CONFIG_IDF_TARGET_ESP32P4)
    if(box.width!=m.width||box.height!=m.height)return Status::Unsupported;
#endif
    if(box.width<=0||box.height<=0||box.width>=0x4000||box.height>=0x4000)return Status::Unsupported;
    clipped=intersect(box,clipped);if(clipped.width<=0||clipped.height<=0)return Status::Ok;
    const size_t stride=mediaStrideBytes(m),bytes=mediaFrameBytes(m,stride);const uint8_t *frame=mediaPixels(m);
    const uintptr_t address=reinterpret_cast<uintptr_t>(frame);
    if(!bytes||m.frames>(UINTPTR_MAX-address)/bytes||!esp_ptr_in_drom(m.pixels)||!esp_ptr_in_drom(frame)||
       !esp_ptr_in_drom(reinterpret_cast<const void *>(address+bytes*m.frames-1)))return Status::Unsupported;
    platform::OverlayView overlay;
#if defined(CONFIG_IDF_TARGET_ESP32P4)
    if(!keyedMediaPpaCache.resolveFrame(m,v.mediaFrame,uint16_t(box.width),uint16_t(box.height),overlay))return Status::Unsupported;
#else
    if(!keyedMediaPpaCache.resolve(m,frame,stride,overlay))return Status::Unsupported;
#endif
    overlay.alpha=alpha;
    const size_t x=size_t(clipped.x-box.x),y=size_t(clipped.y-box.y);
    const uint32_t started=profilingClock?profilingClock():0;
    const Status status=ppaFade.blendOverlayIntoNative(*nativeOverlayTarget,clipped.x,clipped.y,overlay,x,y,clipped.width,clipped.height);
    if(status==Status::Ok){++stats.keyedGpuBlits;++stats.ppaBlendCalls;stats.ppaBlendPixels+=uint64_t(clipped.width)*clipped.height;
      if(profilingClock)stats.ppaBlendMicros+=profilingClock()-started;}
    return status;
#else
    (void)index;(void)clipped;(void)alpha;return Status::Unsupported;
#endif
  }
  void paintKeyedMedia(const MediaDesc &m,const uint8_t *frame,size_t stride,Rect r,Rect clipped,
                       const MediaRoundClip *roundClip=nullptr) {
    const Rect area=intersect(r,clipped);if(area.width<=0||area.height<=0)return;
    const uint16_t key=uint16_t(pgm_read_byte(m.pixels))<<8|pgm_read_byte(m.pixels+1);
    // Native-size color-key pixels already form immutable Wire565 runs. Borrow
    // those runs directly instead of copying every opaque byte through the
    // PSRAM media row before blitting it into the framebuffer. ESP8266 keeps
    // explicit byte reads because its flash cannot use ordinary row copies.
#if !defined(ARDUINO_ARCH_ESP8266)
    if(!roundClip&&r.width==m.width&&r.height==m.height&&
       !(reinterpret_cast<uintptr_t>(frame)&1)&&!(stride&1)){
      const uint16_t storedKey=rgb565_blend::toWire(key);
      for(int32_t y=area.y;y<area.y+area.height;++y){
        const uint8_t *row=frame+size_t(y-r.y)*stride+size_t(area.x-r.x)*2;
        const uint8_t *cursor=row,*end=row+size_t(area.width)*2;
        while(cursor<end){
          while(cursor<end&&pgm_read_word(cursor)==storedKey)cursor+=2;
          const uint8_t *first=cursor;
          while(cursor<end&&pgm_read_word(cursor)!=storedKey)cursor+=2;
          if(first<cursor)canvas.blit(area.x+int32_t((first-row)/2),y,
            PixelView(first,size_t(cursor-first)/2,1,0,PixelOrder::Wire));
        }
      }
      return;
    }
#endif
    mediaRow.resize(size_t(area.width));uint8_t *bytes=reinterpret_cast<uint8_t *>(mediaRow.data());
    const int32_t xStep=m.width/r.width,xRemainder=m.width%r.width;
    for(int32_t y=area.y;y<area.y+area.height;++y){
      int32_t first=area.x,end=area.x+area.width;if(roundClip)roundClip->span(y,first,end);
      if(first>=end)continue;
      const size_t sourceY=size_t(int64_t(y-r.y)*m.height/r.height);
      const int64_t initialX=int64_t(first-r.x)*m.width;
      int32_t sourceX=int32_t(initialX/r.width),remainderX=int32_t(initialX%r.width);
      int32_t run=-1;
      for(int32_t x=first;x<=end;++x){
        uint16_t color=key;
        if(x<end){const uint8_t *p=frame+sourceY*stride+size_t(sourceX)*2;
          color=uint16_t(pgm_read_byte(p))<<8|pgm_read_byte(p+1);
          sourceX+=xStep;remainderX+=xRemainder;if(remainderX>=r.width){remainderX-=r.width;++sourceX;}}
        if(color!=key){if(run<0)run=x;bytes[2*(x-first)]=uint8_t(color>>8);bytes[2*(x-first)+1]=uint8_t(color);}
        else if(run>=0){canvas.blit(run,y,PixelView(bytes+2*(run-first),size_t(x-run),1,0,PixelOrder::Wire));run=-1;}
      }
    }
  }
  void paintPackedMedia(const MediaDesc &m,const Image::Impl *image,const uint8_t *frame,size_t stride,
                        Rect r,Rect clipped,Rect borderBox,const Style &style) {
    const Rect area=intersect(r,clipped);if(area.width<=0||area.height<=0)return;
    const bool ram=image&&!image->flash;
    const MediaRoundClip roundClip(borderBox,style);
    mediaRow.resize(size_t(area.width));
    auto *bytes=reinterpret_cast<uint8_t *>(mediaRow.data());
    const int32_t xStep=m.width/r.width,xRemainder=m.width%r.width;
    const int64_t yNumerator=int64_t(area.y-r.y)*m.height;
    int32_t sourceY=int32_t(yNumerator/r.height),remainderY=int32_t(yNumerator%r.height);
    const int32_t yStep=m.height/r.height,yRemainder=m.height%r.height;
    int32_t previousY=-1;
    for(int32_t y=area.y;y<area.y+area.height;++y){
      int32_t first=area.x,end=area.x+area.width;
      if(style.radius&&roundClip.rounded())roundClip.span(y,first,end);
      if(!mediaMasked(m)){
        // Integer upscaling often repeats the same source scanline. Expand its
        // bits once, then copy the row (including rounded/clipped subspans).
        if(sourceY!=previousY){
          const int64_t numerator=int64_t(area.x-r.x)*m.width;
          int32_t sourceX=int32_t(numerator/r.width),remainderX=int32_t(numerator%r.width);
          for(int32_t col=0;col<area.width;++col){
            const uint8_t b=detail::mediaByte(frame+size_t(sourceY)*stride+size_t(sourceX)/8,ram);
            const uint8_t value=(b&(0x80U>>(sourceX&7)))?0xff:0;
            bytes[size_t(col)*2]=value;bytes[size_t(col)*2+1]=value;
            sourceX+=xStep;remainderX+=xRemainder;if(remainderX>=r.width){remainderX-=r.width;++sourceX;}
          }
          previousY=sourceY;
        }
        if(first<end)canvas.blit(first,y,PixelView(bytes+size_t(first-area.x)*2,size_t(end-first),1,0,PixelOrder::Wire));
        sourceY+=yStep;remainderY+=yRemainder;if(remainderY>=r.height){remainderY-=r.height;++sourceY;}
        continue;
      }
      const int64_t xNumerator=int64_t(first-r.x)*m.width;
      int32_t sourceX=int32_t(xNumerator/r.width),remainderX=int32_t(xNumerator%r.width),run=-1;
      for(int32_t x=first;x<=end;++x){
        uint16_t color=0;
        const bool opaque=x<end&&detail::sampleMediaPixel(m,frame,stride,size_t(sourceX),size_t(sourceY),ram,PixelOrder::Wire,color);
        if(opaque){if(run<0)run=x;const size_t at=size_t(x-area.x)*2;bytes[at]=uint8_t(color>>8);bytes[at+1]=uint8_t(color);}
        else if(run>=0){canvas.blit(run,y,PixelView(bytes+size_t(run-area.x)*2,size_t(x-run),1,0,PixelOrder::Wire));run=-1;}
        sourceX+=xStep;remainderX+=xRemainder;if(remainderX>=r.width){remainderX-=r.width;++sourceX;}
      }
      sourceY+=yStep;remainderY+=yRemainder;if(remainderY>=r.height){remainderY-=r.height;++sourceY;}
    }
  }
  // Raw format is chosen once per image, outside its exact nearest-neighbor
  // DDA. Return a Wire565 storage word so each gathered texel has one store.
  template<unsigned Format>
#if defined(__GNUC__)
  __attribute__((always_inline))
#endif
  static inline uint16_t rawMediaStored(const uint8_t *p) {
    if constexpr(Format==0) {
      uint16_t stored;auto *bytes=reinterpret_cast<uint8_t *>(&stored);
      bytes[0]=pgm_read_byte(p);bytes[1]=pgm_read_byte(p+1);return stored;
    } else if constexpr(Format==1) {
      uint16_t stored;memcpy(&stored,p,2);return stored;
    } else if constexpr(Format==2) {
      uint16_t color;memcpy(&color,p,2);return rgb565_blend::toWire(color);
    } else {
      const uint8_t value=Format==3?pgm_read_byte(p):*p;
      const uint16_t color=uint16_t((((value>>5)*31+3)/7)<<11|(((value>>2)&7)*9)<<5|((value&3)*31+1)/3);
      return rgb565_blend::toWire(color);
    }
  }
  template<unsigned Format>
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("O3")))
#elif defined(__GNUC__)
  __attribute__((noinline))
#endif
  void paintRawRescaledMedia(const uint8_t *frame,size_t stride,uint16_t sourceWidth,uint16_t sourceHeight,Rect r,Rect area) {
    mediaRow.resize(size_t(area.width));uint16_t *output=mediaRow.data();
    const int64_t xn=int64_t(area.x-r.x)*sourceWidth,yn=int64_t(area.y-r.y)*sourceHeight;
    const int32_t firstX=int32_t(xn/r.width),firstRemainder=int32_t(xn%r.width);
    const int32_t xStep=sourceWidth/r.width,xRemainder=sourceWidth%r.width;
    const int32_t yStep=sourceHeight/r.height,yRemainder=sourceHeight%r.height;
    int32_t sourceY=int32_t(yn/r.height),remainderY=int32_t(yn%r.height),previousY=-1;
    for(int32_t y=area.y;y<area.y+area.height;++y) {
      if(sourceY!=previousY) {
        const uint8_t *row=frame+size_t(sourceY)*stride;
        int32_t sourceX=firstX,remainderX=firstRemainder;
        for(int32_t col=0;col<area.width;++col) {
          output[col]=rawMediaStored<Format>(row+size_t(sourceX)*(Format<3?2:1));
          sourceX+=xStep;remainderX+=xRemainder;if(remainderX>=r.width){remainderX-=r.width;++sourceX;}
        }
        previousY=sourceY;
      }
      canvas.blit(area.x,y,PixelView(output,size_t(area.width),1,0,PixelOrder::Wire));
      sourceY+=yStep;remainderY+=yRemainder;if(remainderY>=r.height){remainderY-=r.height;++sourceY;}
    }
  }

  struct RawMediaAlphaTile {
    const uint8_t *frame;uint16_t *pixels;size_t sourceStride,targetStride;
    int32_t width,height,destinationWidth,destinationHeight,firstX,firstRemainder;
    int32_t sourceY,remainderY,xStep,xRemainder,yStep,yRemainder;
    uint8_t alpha;
  };
  template<unsigned Format>
#if defined(__GNUC__)
  __attribute__((always_inline))
#endif
  static inline uint16_t rawMediaNativeColor(const uint8_t *source) {
    if constexpr(Format==2){uint16_t color;memcpy(&color,source,2);return color;}
    else return rgb565_blend::fromWire(rawMediaStored<Format>(source));
  }
  // Gather exact floor-DDA texels and blend against the real destination in
  // one pass. Node opacity already includes ancestors; apply it exactly once.
  template<unsigned Format,bool NativeTarget,bool UseSolidLut>
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("O3")))
#elif defined(__GNUC__)
  __attribute__((noinline))
#endif
  static void blendRawMediaAlphaKernel(const RawMediaAlphaTile &tile,const rgb565_blend::SolidBackgroundLut *lut,uint16_t background) {
    const int32_t width=tile.width,height=tile.height,destinationWidth=tile.destinationWidth;
    const int32_t xStep=tile.xStep,xRemainder=tile.xRemainder;const uint8_t alpha=tile.alpha;
    int32_t sourceY=tile.sourceY,remainderY=tile.remainderY;
    for(int32_t y=0;y<height;++y){
      const uint8_t *row=tile.frame+size_t(sourceY)*tile.sourceStride;
      uint16_t *target=tile.pixels+size_t(y)*tile.targetStride;
      int32_t x=0,sourceX=tile.firstX,remainderX=tile.firstRemainder;
      const auto next=[&](){const uint16_t color=rawMediaNativeColor<Format>(row+size_t(sourceX)*2);
        sourceX+=xStep;remainderX+=xRemainder;if(remainderX>=destinationWidth){remainderX-=destinationWidth;++sourceX;}return color;};
      const auto scalar=[&](){const uint16_t front=next(),back=NativeTarget?*target:rgb565_blend::fromWire(*target);
        if(front!=back){uint16_t color;
          if constexpr(UseSolidLut)color=back==background?lut->lookupNative(front):rgb565_blend::color(front,back,alpha);
          else color=rgb565_blend::color(front,back,alpha);
          *target=NativeTarget?color:rgb565_blend::toWire(color);}++target;++x;};
#if defined(__GNUC__) && defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
      if((reinterpret_cast<uintptr_t>(target)&3)&&x<width)scalar();
      typedef uint32_t RawAlphaWord __attribute__((__may_alias__));
      for(;x+1<width;x+=2,target+=2){
        const uint32_t first=next(),second=next(),front=first|(second<<16);
        auto *pair=reinterpret_cast<RawAlphaWord *>(target);
        const uint32_t back=NativeTarget?*pair:rgb565_blend::swapPairBytes(*pair);
        if(front!=back){uint32_t color;
          if constexpr(UseSolidLut){
            const uint32_t solid=uint32_t(background)|(uint32_t(background)<<16);
            color=back==solid?uint32_t(lut->lookupNative(uint16_t(front)))|(uint32_t(lut->lookupNative(uint16_t(front>>16)))<<16):rgb565_blend::colorPair(front,back,alpha);
          }else color=rgb565_blend::colorPair(front,back,alpha);
          *pair=NativeTarget?color:rgb565_blend::swapPairBytes(color);}
      }
#endif
      for(;x<width;)scalar();
      sourceY+=tile.yStep;remainderY+=tile.yRemainder;
      if(remainderY>=tile.destinationHeight){remainderY-=tile.destinationHeight;++sourceY;}
    }
  }
  // A cheap 4x4 vote chooses only a performance strategy. Every actual
  // destination pixel/pair still checks its real background before using LUT.
  template<unsigned Format,bool NativeTarget>
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("O3")))
#elif defined(__GNUC__)
  __attribute__((noinline))
#endif
  static void blendRawMediaAlpha(const RawMediaAlphaTile &tile) {
    const uint16_t background=NativeTarget?tile.pixels[0]:rgb565_blend::fromWire(tile.pixels[0]);
    unsigned matches=0;
    for(unsigned y=0;y<4;++y){const size_t row=size_t((int64_t(tile.height-1)*y)/3)*tile.targetStride;
      for(unsigned x=0;x<4;++x){const size_t col=size_t((int64_t(tile.width-1)*x)/3);const uint16_t stored=tile.pixels[row+col];
        if((NativeTarget?stored:rgb565_blend::fromWire(stored))==background)++matches;}}
    if(matches>=12){const rgb565_blend::SolidBackgroundLut lut(background,tile.alpha);
      blendRawMediaAlphaKernel<Format,NativeTarget,true>(tile,&lut,background);
    }else blendRawMediaAlphaKernel<Format,NativeTarget,false>(tile,nullptr,0);
  }
  bool paintRawMediaAlpha(int index,Rect clipped,const DirtyRect &area,uint16_t *pixels,size_t stride,uint8_t alpha) {
    const NodeDesc &node=rasterFrame.current.page->nodes[index];
    if(focusStyle(index)||node.kind!=Kind::Media||!alpha||alpha==255||displayPolicy.monochrome)return false;
    const State &value=rasterFrame.current.values[index];const Style &style=styleFor(*rasterFrame.current.page,rasterFrame.current.values,index);
    if(style.radius||style.borderWidth||style.gradient||style.paddingTop||style.paddingRight||style.paddingBottom||style.paddingLeft)return false;
    const MediaDesc *source=media(node,value);
    if(!source||source->bytesPerPixel!=2)return false; // keyed/packed/masked/other formats keep the ordinary painter.
    Rect rect=paintCache->nodes[index].rect;rect.x+=paintOffsetX;rect.y+=paintOffsetY;
    const int64_t dx=int64_t(clipped.x)-rect.x,dy=int64_t(clipped.y)-rect.y;
    if(rect.width<=0||rect.height<=0||clipped.width<=0||clipped.height<=0||dx<0||dy<0||dx+clipped.width>rect.width||dy+clipped.height>rect.height)return false;
    const Image::Impl *image=value.imageOverride?value.image.impl_:nullptr;
    const size_t sourceStride=image?image->source.strideBytes:mediaStrideBytes(*source),rowBytes=size_t(source->width)*2;
    if(!source->height||sourceStride<rowBytes||size_t(source->height-1)>(SIZE_MAX-rowBytes)/sourceStride)return false;
    const size_t sourceBytes=size_t(source->height-1)*sourceStride+rowBytes;
    const size_t frameBytes=mediaFrameBytes(*source,sourceStride);
    if(value.mediaFrame&&frameBytes>SIZE_MAX/value.mediaFrame)return false;
    const size_t frameOffset=size_t(value.mediaFrame)*frameBytes;
    const uintptr_t sourceBase=reinterpret_cast<uintptr_t>(mediaPixels(*source));
    if(frameOffset>UINTPTR_MAX-sourceBase)return false;const uintptr_t address=sourceBase+frameOffset;
    if(sourceBytes>UINTPTR_MAX-address||area.width<=0||area.height<=0||stride<size_t(area.width)||
       size_t(area.height-1)>(SIZE_MAX-size_t(area.width))/stride)return false;
    const size_t targetWords=size_t(area.height-1)*stride+size_t(area.width);
    if(targetWords>SIZE_MAX/2)return false;const size_t targetBytes=targetWords*2;
    const uintptr_t destination=reinterpret_cast<uintptr_t>(pixels);
    if(targetBytes>UINTPTR_MAX-destination||(address<destination+targetBytes&&destination<address+sourceBytes))return false;
    if(clipped.x<area.x||clipped.y<area.y||int64_t(clipped.x)+clipped.width>int64_t(area.x)+area.width||int64_t(clipped.y)+clipped.height>int64_t(area.y)+area.height)return false;
    const int64_t xn=dx*source->width,yn=dy*source->height;
    const RawMediaAlphaTile tile{reinterpret_cast<const uint8_t *>(address),pixels+size_t(clipped.y-area.y)*stride+size_t(clipped.x-area.x),sourceStride,stride,
      clipped.width,clipped.height,rect.width,rect.height,int32_t(xn/rect.width),int32_t(xn%rect.width),
      int32_t(yn/rect.height),int32_t(yn%rect.height),source->width/rect.width,source->width%rect.width,source->height/rect.height,source->height%rect.height,alpha};
    const bool native=renderOrder==PixelOrder::Native;
    if(image&&!image->flash){
      if(image->source.order==PixelOrder::Native){if(native)blendRawMediaAlpha<2,true>(tile);else blendRawMediaAlpha<2,false>(tile);}
      else {if(native)blendRawMediaAlpha<1,true>(tile);else blendRawMediaAlpha<1,false>(tile);}
    }else {if(native)blendRawMediaAlpha<0,true>(tile);else blendRawMediaAlpha<0,false>(tile);}
    return true;
  }

#ifndef NICE_GFX_UI_SKIP_EMPTY_CONTAINERS
#define NICE_GFX_UI_SKIP_EMPTY_CONTAINERS 1
#endif
#include "detail/RetainedChartInk.inc.h"
#include "detail/ChartBackdropGrid.inc.h"
  bool emptyContainerPaint(int i) const {
    const auto &page=*rasterFrame.current.page;const auto &values=rasterFrame.current.values;
    const auto &node=page.nodes[i];const auto &value=values[i];
    if(node.kind!=Kind::Container||value.kind!=Kind::Container||node.motion||node.instrument)return false;
    Style style=styleFor(page,values,i);uint16_t flags=value.flags;
    applyFocusStyle(focusStyle(i),style,flags);
    if(style.borderWidth)return false;
    bool disabled=(flags&Disabled)!=0;
    for(uint16_t parent=value.parent;parent!=NoParent&&!disabled;parent=values[parent].parent)disabled=(values[parent].flags&Disabled)!=0;
    // Container has no component selection, text, media or other content of
    // its own. Effective transparency plus no border makes this paint empty;
    // geometry, inherited alpha and child paints remain independent.
    return (flags&(disabled?DisabledTransparent:i==snapshotPressed?ActiveTransparent:Transparent))!=0;
  }
  void paintNode(int i,Rect clipped,Status *nativeBackgroundStatus=nullptr) {
    (void)nativeBackgroundStatus;
    const NodeDesc &n=rasterFrame.current.page->nodes[i];const State &v=rasterFrame.current.values[i];Style s=styleFor(*rasterFrame.current.page,rasterFrame.current.values,i);
    uint16_t paintFlags=v.flags;const auto *focusedStyle=focusStyle(i);applyFocusStyle(focusedStyle,s,paintFlags);
    canvas.setAntialiasing(!displayPolicy.monochrome&&!(v.flags&NoAntialias));
    Rect r=behaviorPaintRect(i,paintCache->nodes[i].rect);r.x+=paintOffsetX;r.y+=paintOffsetY;
    // Opaque dynamic RGB565 completely replaces its own box background. Keep
    // clipped row copies, but omit the fill/occlusion scan it would overwrite.
    if(v.kind==Kind::Media&&paintCache->nodes[i].alpha==255&&v.imageOverride&&v.image.impl_&&
       !v.image.impl_->flash&&!s.borderWidth&&!s.radius&&!s.paddingTop&&!s.paddingRight&&
       !s.paddingBottom&&!s.paddingLeft){
      const auto &image=*v.image.impl_;const auto &m=image.media;
      if(m.bytesPerPixel==2&&r.width==m.width&&r.height==m.height&&!(image.source.strideBytes&1)){
        canvas.blit(r.x,r.y,PixelView(m.pixels+size_t(v.mediaFrame)*image.source.strideBytes*m.height,
          m.width,m.height,image.source.strideBytes/2,image.source.order));return;
      }
    }
    bool disabled=(v.flags&Disabled)!=0;
    for(uint16_t ancestor=v.parent;ancestor!=NoParent&&!disabled;ancestor=rasterFrame.current.values[ancestor].parent)disabled=(rasterFrame.current.values[ancestor].flags&Disabled)!=0;
    const bool pressed=i==snapshotPressed;
    const ComponentPaint component=v.kind==Kind::Button?componentPaint(*rasterFrame.current.page,rasterFrame.current.values,n.id):ComponentPaint{};
    const bool selected=component.selected&&!disabled;
    const bool focusBackground=!disabled&&focusedStyle&&(pressed||(focusedStyle->flags&FocusBackground));
    uint16_t bg=focusBackground?(pressed?s.pressedColor:s.background):selected?s.thumbColor:disabled?s.disabledColor:(pressed?s.pressedColor:s.background);
    // RGB styles have one foreground for all states. A 1-bit selected/pressed
    // background must choose its opposite foreground to retain readable menus.
    Style monoStyle;
    const Style *contentStyle=&s;
    const bool transparent=(!selected||focusBackground)&&(disabled?(paintFlags&DisabledTransparent)!=0:
      pressed?(paintFlags&ActiveTransparent)!=0:(paintFlags&Transparent)!=0);
    uint16_t monoBackground=bg;
    if(displayPolicy.monochrome){
      bool inheritedState=false;
      if(transparent){monoBackground=rasterFrame.current.page->background;
        // Resolve state backplates for transparent captions/icons without a cache or allocation.
        for(uint16_t ancestor=v.parent;ancestor!=NoParent;ancestor=rasterFrame.current.values[ancestor].parent){
          const auto &av=rasterFrame.current.values[ancestor];Style as=styleFor(*rasterFrame.current.page,rasterFrame.current.values,ancestor);uint16_t af=av.flags;
          const auto *focus=focusStyle(ancestor);applyFocusStyle(focus,as,af);
          bool ad=(af&Disabled)!=0;for(uint16_t p=av.parent;p!=NoParent&&!ad;p=rasterFrame.current.values[p].parent)ad=(rasterFrame.current.values[p].flags&Disabled)!=0;
          const bool ap=ancestor==snapshotPressed,selectedParent=av.kind==Kind::Button&&componentPaint(*rasterFrame.current.page,rasterFrame.current.values,rasterFrame.current.page->nodes[ancestor].id).selected&&!ad;
          inheritedState|=ap||ad||selectedParent;
          const bool fb=!ad&&focus&&(ap||(focus->flags&FocusBackground));
          const bool at=(!selectedParent||fb)&&(ad?(af&DisabledTransparent)!=0:ap?(af&ActiveTransparent)!=0:(af&Transparent)!=0);
          if(!at){monoBackground=fb?(ap?as.pressedColor:as.background):selectedParent?as.thumbColor:ad?as.disabledColor:ap?as.pressedColor:as.background;break;}
          if(viewportOverlayRoot(*rasterFrame.current.page,ancestor))break;
        }
      }
      monoStyle=s;monoStyle.background=monoBackground;
      if(selected||disabled||pressed||inheritedState)monoStyle.color=DisplayPolicy::foregroundOn(monoBackground);
      contentStyle=&monoStyle;
    }
    const bool gradient=!(n.flags&RangeFillGradient)&&(!selected||focusBackground)&&(disabled?(paintFlags&GradientDisabled)!=0:pressed?(paintFlags&GradientActive)!=0:!(focusedStyle&&(focusedStyle->flags&FocusBackground)));
#if NICE_GFX_UI_CHART_NODE_PROFILE
    const bool chartNodeProfile=n.kind==Kind::LineChart||n.kind==Kind::BarChart;
    ProfileTimer chartBackdropTimer(*this,&RuntimeStats::chartBackdropMicros,chartNodeProfile);
    if(chartNodeProfile&&profilingClock)++stats.chartBackdropCalls;
#endif
    bool backgroundPainted=false;
#if NICE_GFX_UI_CHART_FUSED_BACKDROP_GRID
    bool chartGridPrepared=false;
#endif
#if defined(CONFIG_IDF_TARGET_ESP32S31) && NICE_GFX_S31_PPA_BLEND_AVAILABLE && NICE_GFX_UI_SYSTEM_MODAL_BASE_LEASE && NICE_GFX_UI_SYSTEM_MODAL_PANEL_CACHE && NICE_GFX_UI_SYSTEM_PANEL_PAGE_SOURCE
    // Only the ordinary opaque render call can request this writer. The
    // shared source excludes its generated final close handle; its retained
    // body copy has already joined before this complete background is painted.
    const auto &lease=systemModalPanelFrame;const auto *target=nativeOverlayTarget;
    if(nativeBackgroundStatus&&!xPortInIsrContext()&&xPortCanYield()&&
       !transparent&&!displayPolicy.monochrome&&!circularKeyboard&&
       n.kind==Kind::Button&&v.kind==Kind::Button&&!n.instrument&&!v.instrument&&
       paintCache->nodes[i].alpha==255&&!s.borderWidth&&!s.radius&&!s.gradient&&
       !buildingSurface&&!foregroundCapture&&!nativeCacheRepairing&&!systemModalPanelBuilding&&
       !snapshotTransition&&!rasterFrame.preview.page&&!renderingViewport&&!paintOffsetX&&!paintOffsetY&&
       renderOrder==PixelOrder::Native&&target&&!stagingNativeTarget&&ppaBlendEnabled&&
       canvas.targetsNativeFramebuffer(*target)&&!systemModalPanelOverlap.active&&
       lease.sharedSource&&lease.owner==frameSourceOwner&&!history.empty()&&lease.hostPage==history.back()&&
       lease.root==systemModalBaseFrame.root&&lease.root<paintCache->nodes.size()&&
       i==lease.handle&&v.parent==lease.root&&size_t(i)+1==rasterFrame.current.page->count&&
       modalBaseSameRect(lease.target,paintCache->nodes[lease.root].rect)&&
       modalBaseSameRect(r,paintCache->nodes[i].rect)&&modalBaseSameRect(r,clipped)&&
       !r.x&&r.y>=0&&r.width>0&&r.height>0&&size_t(r.width)==target->width&&target->stride==target->width&&
       uint64_t(r.width)*uint64_t(r.height)>=4096){
      const auto *source=systemModalPanelSurface(lease);
      const size_t line=nice_gfx::platform::rgb565_ppa::cacheLineSize(target->pixels);
      // Use the existing native fill threshold and actual SDK cache line.
      // Whole cache rows exclude the neighboring bar and CPU host painters.
      if(source&&line&&!((target->stride*2)%line)){
        const uint32_t started=profilingClock?profilingClock():0;
        const Status filled=ppaFade.fillIntoNative(*target,DirtyRect(r.x,r.y,r.width,r.height),bg);
        if(filled==Status::Ok){
          backgroundPainted=true;++stats.ppaFillCalls;stats.ppaFillPixels+=uint64_t(r.width)*r.height;
          if(profilingClock)stats.ppaFillMicros+=profilingClock()-started;
        }else if(filled!=Status::Unsupported){
          ++stats.ppaFillFailures;*nativeBackgroundStatus=filled;return;
        }
      }
    }
#endif
    if(!transparent){
#if NICE_GFX_UI_CHART_FUSED_BACKDROP_GRID
      if(!backgroundPainted){chartGridPrepared=paintChartBackdropGrid(i,clipped,r,bg,s);backgroundPainted=chartGridPrepared;}
#endif
      if(!backgroundPainted)rounded(r,bg,s.borderColor,s,gradient,i,clipped);
    }
    else for(int b=0;b<s.borderWidth&&2*b<std::min<int32_t>(r.width,r.height);++b)
      canvas.drawRoundRect(r.x+b,r.y+b,r.width-2*b,r.height-2*b,std::max<int32_t>(0,std::min<int32_t>(s.radius,std::min<int32_t>(r.width,r.height)/2)-b),s.borderColor);
#if NICE_GFX_UI_CHART_NODE_PROFILE
    chartBackdropTimer.stop();
#endif
    const auto *monoComponent=displayPolicy.monochrome&&v.kind==Kind::Container?componentFor(i):nullptr;
    const bool monoControl=displayPolicy.monochrome&&(v.kind==Kind::Button||v.kind==Kind::InputField||v.kind==Kind::Checkbox||v.kind==Kind::Switch||v.kind==Kind::Range||v.kind==Kind::Progress||n.instrument||(monoComponent&&monoComponent->root==i&&monoComponent->desc->kind==ComponentKind::ImageButton));
    detail::MonoControlState<nice_gfx::TileCanvas> monoState{canvas,r,clipped,monoBackground,monoControl&&disabled,monoControl&&pressed};
    if(v.kind==Kind::Text||v.kind==Kind::Label||v.kind==Kind::Button) {
      if((selected||(displayPolicy.monochrome&&disabled&&component.selected))&&r.width>0&&r.height>0){const int32_t inset=std::min<int32_t>(12,r.width/4);
        const bool disabledSelection=displayPolicy.monochrome&&disabled;canvas.fillRoundRect(r.x+inset,r.y+std::max<int32_t>(0,r.height-(disabledSelection?2:4)),r.width-2*inset,disabledSelection?1:std::min<int32_t>(2,r.height),disabledSelection?0:1,disabledSelection?DisplayPolicy::foregroundOn(monoBackground):s.fillColor);}
      text(i,n,v,*contentStyle,clipped);
      if(component.select&&r.width>0&&r.height>0){const float cx=r.x+r.width-18,cy=r.y+r.height/2;
        const float direction=component.expanded?-1.0f:1.0f;
        canvas.drawWideLine(cx-4,cy-2*direction,cx,cy+2*direction,2,contentStyle->color);
        canvas.drawWideLine(cx,cy+2*direction,cx+4,cy-2*direction,2,contentStyle->color);}
      if(component.list&&r.width>0&&r.height>0){const float cx=r.x+r.width-18,cy=r.y+r.height/2;
        canvas.drawWideLine(cx-2,cy-4,cx+2,cy,2,contentStyle->color);
        canvas.drawWideLine(cx+2,cy,cx-2,cy+4,2,contentStyle->color);}
      return;
    }
    const Rect borderBox=r;
    r=contentBox(r,s);
    if(v.kind==Kind::InputField){paintInputField(n,v,*contentStyle,r);return;}
    if(v.kind==Kind::Shape&&n.shape&&r.width>0&&r.height>0){markShapePaintPath(ShapePaintPath::PlainRenderShape);ShapeDesc shape=*n.shape;applyFocusShape(focusedStyle,shape);renderShape(canvas,shape,r,clipped,s.radius);return;}
    if(n.instrument&&r.width>0&&r.height>0){NodeDesc styled=n;styled.style=*contentStyle;styled.flags=v.flags|(displayPolicy.monochrome?NoAntialias:0);const uint64_t columnBatches=canvas.chartColumnBatches(),columnPixels=canvas.chartColumnPixels(),gridBatches=canvas.chartGridBatches(),gridPixels=canvas.chartGridPixels();
#if NICE_GFX_UI_CHART_NODE_PROFILE
      ProfileTimer chartInstrumentTimer(*this,&RuntimeStats::chartInstrumentMicros,chartNodeProfile);
      if(chartNodeProfile&&profilingClock)++stats.chartInstrumentCalls;
#endif
#if NICE_GFX_UI_CHART_FUSED_BACKDROP_GRID
      retainedChartInstrumentBegin(i,chartGridPrepared);
#else
      retainedChartInstrumentBegin(i,false);
#endif
      renderInstrument(canvas,styled,*v.instrument,v.value,r,displayPolicy.monochrome
#if NICE_GFX_UI_CHART_FUSED_BACKDROP_GRID
      ,chartGridPrepared
#endif
      );
#if NICE_GFX_UI_CHART_NODE_PROFILE
      chartInstrumentTimer.stop();
#endif
      retainedChartInstrumentEnd();
      stats.chartColumnBatches+=canvas.chartColumnBatches()-columnBatches;stats.chartColumnPixels+=canvas.chartColumnPixels()-columnPixels;stats.chartGridBatches+=canvas.chartGridBatches()-gridBatches;stats.chartGridPixels+=canvas.chartGridPixels()-gridPixels;return;}
    const MediaDesc *source=v.kind==Kind::Media?media(n,v):nullptr;
    if(source&&r.width>0&&r.height>0) {
      MediaDesc decodedMedia{};Image::Impl decodedImage(ImageView(),nullptr,nullptr,0);
      const Image::Impl *image=v.imageOverride?v.image.impl_:nullptr;bool jpeg=false;
      if(mediaJpeg(*source)){
        JpegDecodeTarget target;if(!decodeJpeg(*source,v.mediaFrame,target,n.id))return;
        decodedMedia={reinterpret_cast<const uint8_t *>(target.pixels),target.width,target.height,1,1,2};
        decodedImage.source=ImageView(target.pixels,target.width,target.height,target.capacityBytes,ImageFormat::RGB565,PixelOrder::Native,target.strideBytes);
        decodedImage.media=decodedMedia;source=&decodedMedia;image=&decodedImage;jpeg=true;
      }
      const MediaDesc &m=*source;
      const size_t stride=image?image->source.strideBytes:mediaStrideBytes(m);
      // Public viewport getters describe the coordinate datum, which stays
      // full-screen for setViewport(..., false). Use the renderer's real tile
      // intersection so a 16-row refresh never samples the entire image.
      const Rect area=intersect(r,clipped);
      if(area.width<=0||area.height<=0)return;
      const uint8_t *frame=mediaPixels(m)+(jpeg?0:size_t(v.mediaFrame)*mediaFrameBytes(m,stride));
      if(mediaPacked(m)){paintPackedMedia(m,image,frame,stride,r,clipped,borderBox,s);return;}
      if(mediaColorKeyed(m)){
        const MediaRoundClip clip(borderBox,s);paintKeyedMedia(m,frame,stride,r,clipped,s.radius&&clip.rounded()?&clip:nullptr);return;
      }
      if(s.radius){const MediaRoundClip clip(borderBox,s);
        if(clip.rounded()){paintRoundedMedia(m,image,frame,stride,r,clipped,clip);return;}}
      // Native-size RGB565 needs no nearest-neighbor sampling. RAM sources
      // can be cropped and copied directly, including padded even-byte rows
      // and either byte order. Keep flash accesses explicit on ESP8266.
      if(m.bytesPerPixel==2&&r.width==m.width&&r.height==m.height){
        if(image&&!image->flash&&!(stride&1)){
          canvas.blit(r.x,r.y,PixelView(frame,m.width,m.height,stride/2,image->source.order));return;
        }
        if(!image||image->flash||image->source.order==PixelOrder::Wire){
          mediaRow.resize(size_t(area.width));
          for(int32_t y=area.y;y<area.y+area.height;++y){
            const uint8_t *row=frame+size_t(y-r.y)*stride+size_t(area.x-r.x)*2;
#if !NICE_GFX_NATIVE_IDF && (defined(ARDUINO_ARCH_ESP8266) || defined(ESP32))
            if(!image||image->flash)memcpy_P(mediaRow.data(),row,size_t(area.width)*2);
            else memcpy(mediaRow.data(),row,size_t(area.width)*2);
#else
            memcpy(mediaRow.data(),row,size_t(area.width)*2);
#endif
            canvas.blit(area.x,y,PixelView(mediaRow.data(),size_t(area.width),1,0,PixelOrder::Wire));
          }
          return;
        }
      }
      // Packed, keyed and rounded sources have already taken their own paths.
      // Keep opacity in the existing outer node scope; gather these raw pixels
      // without per-pixel format, storage-location or byte-order branches.
      if(m.bytesPerPixel==2){
        if(image&&!image->flash){
          if(image->source.order==PixelOrder::Native)paintRawRescaledMedia<2>(frame,stride,m.width,m.height,r,area);
          else paintRawRescaledMedia<1>(frame,stride,m.width,m.height,r,area);
        }else paintRawRescaledMedia<0>(frame,stride,m.width,m.height,r,area);
        return;
      }
      if(m.bytesPerPixel==1){
        if(image&&!image->flash)paintRawRescaledMedia<4>(frame,stride,m.width,m.height,r,area);
        else paintRawRescaledMedia<3>(frame,stride,m.width,m.height,r,area);
        return;
      }
      // Preserve the general fallback for future raw resource formats.
      mediaRow.resize(size_t(area.width));uint8_t *bytes=reinterpret_cast<uint8_t *>(mediaRow.data());
      // Exact floor(position * source / destination) nearest-neighbor sampling.
      // Divide once at the clipped origin, then carry remainders. ESP32's
      // software 64-bit division and virtual pixel clipping leave the hot loop.
      const int64_t xNumerator=int64_t(area.x-r.x)*m.width,yNumerator=int64_t(area.y-r.y)*m.height;
      const int32_t firstX=int32_t(xNumerator/r.width),firstRemainder=int32_t(xNumerator%r.width);
      const int32_t xStep=m.width/r.width,xRemainder=m.width%r.width,yStep=m.height/r.height,yRemainder=m.height%r.height;
      int32_t sourceY=int32_t(yNumerator/r.height),remainderY=int32_t(yNumerator%r.height),previousY=-1;
      for(int32_t y=area.y;y<area.y+area.height;++y) {
        if(sourceY!=previousY) {
          const uint8_t *row=frame+size_t(sourceY)*stride;
          int32_t sourceX=firstX,remainderX=firstRemainder;
          for(int32_t col=0;col<area.width;++col) {
            const uint8_t *p=row+size_t(sourceX)*m.bytesPerPixel;
            if(m.bytesPerPixel==2){
              if(image&&!image->flash&&image->source.order==PixelOrder::Native){uint16_t color;memcpy(&color,p,2);bytes[2*col]=uint8_t(color>>8);bytes[2*col+1]=uint8_t(color);}
              else if(image&&!image->flash){bytes[2*col]=p[0];bytes[2*col+1]=p[1];}
              else {bytes[2*col]=pgm_read_byte(p);bytes[2*col+1]=pgm_read_byte(p+1);}
            }
            else {const uint8_t value=image&&!image->flash?*p:pgm_read_byte(p);const uint16_t red=((value>>5)*31+3)/7,green=((value>>2)&7)*9,blue=((value&3)*31+1)/3,color=(red<<11)|(green<<5)|blue;bytes[2*col]=uint8_t(color>>8);bytes[2*col+1]=uint8_t(color);}
            sourceX+=xStep;remainderX+=xRemainder;if(remainderX>=r.width){remainderX-=r.width;++sourceX;}
          }
          previousY=sourceY;
        }
        canvas.blit(area.x,y,PixelView(mediaRow.data(),size_t(area.width),1,0,PixelOrder::Wire));
        sourceY+=yStep;remainderY+=yRemainder;if(remainderY>=r.height){remainderY-=r.height;++sourceY;}
      }
    } else if(v.kind==Kind::Icon&&n.icon) {
      canvas.drawIcon(r.x,r.y,*n.icon,contentStyle->color);
    } else if(v.kind==Kind::Pager&&n.pager.indicatorHeight) {
      const int count=paintCache->nodes[i].children;
      const int size=n.pager.dotSize,step=size+n.pager.dotGap;
      const int left=r.x+(r.width-(count*step-n.pager.dotGap))/2,top=r.y+r.height-(n.pager.indicatorHeight+size)/2;
      for(int j=0;j<count;++j){
        if(displayPolicy.monochrome&&j!=v.value)canvas.drawRoundRect(left+j*step,top,size,size,size/2,DisplayPolicy::foregroundOn(s.background));
        else canvas.fillRoundRect(left+j*step,top,size,size,size/2,displayPolicy.monochrome?DisplayPolicy::foregroundOn(s.background):(j==v.value?s.fillColor:s.trackColor));
      }
    } else if(v.kind==Kind::Range||v.kind==Kind::Progress) {
      const auto g=detail::rangeGeometry(r,s,(n.flags&VerticalRange)!=0,v.kind==Kind::Range,v.value,n.minimum,n.maximum);
      if(displayPolicy.monochrome){detail::monoRange(canvas,g,s,v.kind==Kind::Range);return;}
      const int32_t track=g.vertical?g.track.width:g.track.height;
      const int32_t radius=s.trackRadius==65535?track/2:std::min<int32_t>(s.trackRadius,track/2);
      canvas.fillRoundRect(g.track.x,g.track.y,g.track.width,g.track.height,radius,s.trackColor);
      if(displayPolicy.monochrome)canvas.drawRoundRect(g.track.x,g.track.y,g.track.width,g.track.height,radius,DisplayPolicy::foregroundOn(s.trackColor));
      if(g.position>0){
        if((n.flags&RangeFillGradient)&&s.gradient){const auto &fill=*s.gradient;const Rect origin=rect(i);
          const int32_t projection=int32_t(int64_t(fill.origin)+int64_t(g.fill.x-origin.x)*fill.stepX+int64_t(g.fill.y-origin.y)*fill.stepY);
          canvas.fillLinearGradientRoundRect(g.fill.x,g.fill.y,g.fill.width,g.fill.height,uint8_t(radius),fill.colors,projection,fill.stepX,fill.stepY,false);
        }else canvas.fillRoundRect(g.fill.x,g.fill.y,g.fill.width,g.fill.height,radius,s.fillColor);
      }
      if(v.kind==Kind::Range){const int32_t tw=g.thumb.width,th=g.thumb.height,x=g.thumb.x,y=g.thumb.y,radius=std::min<int32_t>(s.thumbRadius,std::min<int32_t>(tw,th)/2);
        canvas.fillRoundRect(x,y,tw,th,radius,s.trackColor);
        if(tw>2&&th>2)canvas.fillRoundRect(x+1,y+1,tw-2,th-2,std::max<int32_t>(0,radius-1),s.thumbColor);
        if(displayPolicy.monochrome)canvas.drawRoundRect(x,y,tw,th,radius,DisplayPolicy::foregroundOn(s.thumbColor));}
    } else if(v.kind==Kind::Checkbox) {
      const int32_t w=std::min<int32_t>(r.width,s.thumbWidth),h=std::min<int32_t>(r.height,s.thumbHeight);Rect box{r.x,r.y+(r.height-h)/2,w,h};rounded(box,v.value?s.fillColor:s.trackColor,s.borderColor,s);
      if(displayPolicy.monochrome){detail::monoCheckbox(canvas,box,s,v.value,monoBackground);text(i,n,v,*contentStyle,clipped);return;}
      if(v.value&&w>5&&h>5) {const int32_t x0=box.x+w*25/100,y0=box.y+h/2,x1=box.x+w*44/100,y1=box.y+h*70/100,x2=box.x+w*78/100,y2=box.y+h*28/100;
        canvas.drawWideLine(x0,y0,x1,y1,2,s.thumbColor);canvas.drawWideLine(x1,y1,x2,y2,2,s.thumbColor);}
      text(i,n,v,*contentStyle,clipped);
    } else if(v.kind==Kind::Switch) {
      if(displayPolicy.monochrome){detail::monoSwitch(canvas,r,s,v.switchPosition,monoBackground);return;}
      const int32_t th=std::min<int32_t>(r.height,s.thumbHeight),tw=std::min<int32_t>(std::min<int32_t>(r.width,s.thumbWidth),std::max<int32_t>(1,r.width-2));
      const int32_t inset=std::min<int32_t>(1,(r.width-tw)/2),travel=std::max<int32_t>(0,r.width-tw-2*inset);
      const int32_t track=std::min<int32_t>(r.height,s.trackSize);
      canvas.fillRoundRect(r.x,r.y+(r.height-track)/2,r.width,track,std::min<int32_t>(s.radius,track/2),switchColor(s.trackColor,s.fillColor,v.switchPosition));
      canvas.fillRoundRect(r.x+inset+int32_t(int64_t(travel)*v.switchPosition/1000),r.y+(r.height-th)/2,tw,th,std::min<int32_t>(s.thumbRadius,std::min<int32_t>(tw,th)/2),s.thumbColor);
    }
  }
  // A uniform real backdrop needs no saved rectangle. Paint the complete text
  // first, including overlapping glyphs/lines, then apply node opacity once.
#if defined(__GNUC__)
  __attribute__((noinline))
#if !defined(__clang__)
  __attribute__((optimize("O3")))
#endif
#endif
  bool paintFlatTextAlpha(int index,Rect clipped,const DirtyRect &area,uint16_t *pixels,size_t stride,uint8_t alpha) {
    const auto &p=*rasterFrame.current.page;const auto &n=p.nodes[index];const auto &v=rasterFrame.current.values[index];
    if(focusStyle(index)||(n.kind!=Kind::Text&&n.kind!=Kind::Label)||displayPolicy.monochrome)return false;
    const auto &s=styleFor(p,rasterFrame.current.values,index);
    if(!s.font||s.borderWidth||s.radius||s.gradient)return false;
    bool disabled=(v.flags&Disabled)!=0;
    for(uint16_t at=n.parent;at!=NoParent&&!disabled;at=p.nodes[at].parent)disabled=(rasterFrame.current.values[at].flags&Disabled)!=0;
    const bool transparent=disabled?(v.flags&DisabledTransparent)!=0:
      index==snapshotPressed?(v.flags&ActiveTransparent)!=0:(v.flags&Transparent)!=0;
    if(!transparent)return false;
    const uintptr_t output=reinterpret_cast<uintptr_t>(pixels);const size_t outputBytes=(size_t(area.height-1)*stride+area.width)*2;
    const auto aliases=[&](const void *source,size_t bytes){
      if(nativeMemoryOverlap(output,outputBytes,source,bytes))return true;
      for(size_t t=0;t<tileCount;++t)if(tileBuffers[t]&&nativeMemoryOverlap(reinterpret_cast<uintptr_t>(tileBuffers[t]),capacity*2,source,bytes))return true;
      return false;
    };
    // Native's original opacity path borrows an idle tile. Preserve that exact
    // behavior when any font/text source can alias either output or scratch.
    const MonoFont *font=s.font;
    for(unsigned level=0;font&&level<2;++level){
      if(aliases(font,sizeof(*font))||!font->height||!font->count)return false;
      const size_t row=nice_gfx::detail::monoStride(*font);
      if(!row||size_t(font->count)>SIZE_MAX/font->height/row)return false;
      if(aliases(font->bitmap,row*font->height*font->count)||
         (font->codepoints&&aliases(font->codepoints,size_t(font->count)*sizeof(uint16_t))))return false;
      font=font->fallback;
    }
    if(font)return false;
    const char *text=v.text.c_str();if(aliases(text,strlen(text)+1))return false;
    uint16_t *first=pixels+size_t(clipped.y-area.y)*stride+size_t(clipped.x-area.x);
    const uint16_t background=first[0];
    for(int32_t y=0;y<clipped.height;++y){const uint16_t *row=first+size_t(y)*stride;
      for(int32_t x=0;x<clipped.width;++x)if(row[x]!=background)return false;
    }
    const bool wire=buildingSurface||renderOrder==PixelOrder::Wire;
    const uint16_t nativeBackground=wire?rgb565_blend::fromWire(background):background;
    const rgb565_blend::SolidBackgroundLut palette(nativeBackground,alpha);
    canvas.setViewport(clipped.x,clipped.y,clipped.width,clipped.height,false);paintNode(index,clipped);
    for(int32_t y=0;y<clipped.height;++y){uint16_t *row=first+size_t(y)*stride;
      for(int32_t x=0;x<clipped.width;++x){
        const uint16_t sample=row[x];if(sample==background)continue;
        const uint16_t nativeSample=wire?rgb565_blend::fromWire(sample):sample;
        const uint16_t blended=palette.lookupNative(nativeSample);
        row[x]=wire?rgb565_blend::toWire(blended):blended;
      }
    }
    return true;
  }
  // Native frames already own an idle tile buffer. Borrow it per node instead
  // of allocating a full clipped/page-sized opacity underlay. The Native
  // preflight excludes every target/source alias before any framebuffer write.
  Status paintNativeAlphaNodeRows(int index,Rect clipped,const DirtyRect &area,uint16_t *pixels,size_t stride,uint8_t alpha) {
    const int32_t width=int32_t(std::min<size_t>(size_t(clipped.width),capacity));
    const int32_t rows=int32_t(std::min<size_t>(size_t(clipped.height),capacity/size_t(width)));
    uint16_t *saved=tileBuffers[0];
    for(int32_t y=clipped.y;y<clipped.y+clipped.height;y+=rows)for(int32_t x=clipped.x;x<clipped.x+clipped.width;x+=width){
      const Rect chunk{x,y,std::min(width,clipped.x+clipped.width-x),std::min(rows,clipped.y+clipped.height-y)};
      auto *target=pixels+size_t(chunk.y-area.y)*stride+size_t(chunk.x-area.x);
      for(int32_t row=0;row<chunk.height;++row)memcpy(saved+size_t(row)*chunk.width,target+size_t(row)*stride,size_t(chunk.width)*2);
      // A node may temporarily enlarge its viewport. Bind the borrowed tile
      // itself to this chunk so no such helper can touch another chunk early.
      const Status attached=canvas.attach(DirtyRect(chunk.x,chunk.y,chunk.width,chunk.height),target,
        size_t(chunk.height-1)*stride+size_t(chunk.width),stride,PixelOrder::Native);
      if(attached!=Status::Ok)return attached;
      paintNode(index,chunk);
      rgb565_blend::nativeRows(target,stride,saved,size_t(chunk.width),size_t(chunk.width),size_t(chunk.height),alpha);
    }
    return canvas.attach(area,pixels,size_t(area.height-1)*stride+size_t(area.width),stride,PixelOrder::Native);
  }
#include "detail/ContainerPrimitiveTemplate.inc.h"
  Status paintNativeAlphaNode(int index,Rect clipped,const DirtyRect &area,uint16_t *pixels,size_t stride,uint8_t alpha) {
    const Status templated=paintContainerPrimitiveTemplate(index,clipped,area,pixels,stride,alpha);if(templated!=Status::Unsupported)return templated;
#if NICE_GFX_UI_CONTAINER_ALPHA_INTERIOR
    const auto &page=*rasterFrame.current.page;const auto &values=rasterFrame.current.values;const auto &n=page.nodes[index];const auto &value=values[index];
    const auto *target=stagingNativeTarget?stagingNativeTarget:nativeOverlayTarget;
    if(alpha&&alpha<255&&n.kind==Kind::Container&&value.kind==Kind::Container&&!n.instrument&&!value.instrument&&
       !displayPolicy.monochrome&&!buildingSurface&&!foregroundCapture&&renderOrder==PixelOrder::Native&&target&&
       index!=snapshotPressed&&!focusStyle(index)&&!componentFor(index)&&canvas.targetsNativeFramebuffer(*target)){
      bool ordinary=true;for(const auto &behavior:behaviorVisuals)if(behavior.root==index&&behavior.reveal<1024){ordinary=false;break;}
      for(int at=index;ordinary&&at>=0;at=values[at].parent==NoParent?-1:values[at].parent){
        if(values[at].flags&Disabled){ordinary=false;break;}
        if(page.nodes[at].motion){const auto &pose=motionPose(page,values,at);if(pose.scale!=1024||pose.rotation%360||pose.reveal!=1024){ordinary=false;break;}}
      }
      const auto &style=styleFor(page,values,index);
      if(ordinary&&!(value.flags&Transparent)&&!style.gradient){
        Rect rect=paintCache->nodes[index].rect;const Rect actual=behaviorPaintRect(index,rect);
        if(actual.x!=rect.x||actual.y!=rect.y||actual.width!=rect.width||actual.height!=rect.height)
          return paintNativeAlphaNodeRows(index,clipped,area,pixels,stride,alpha);
        rect.x+=paintOffsetX;rect.y+=paintOffsetY;
        const int32_t radius=std::max<int32_t>(0,std::min<int32_t>(style.radius,std::min(rect.width,rect.height)/2));
        // Stay two pixels beyond the border/AA ink. Keep every corner and
        // straight border on the original painter, in disjoint outer strips.
        const int32_t inset=int32_t(style.borderWidth)+2,vertical=radius+inset;
        if(rect.width>2*inset&&rect.height>2*vertical){
          const Rect interior=intersect(clipped,{rect.x+inset,rect.y+vertical,rect.width-2*inset,rect.height-2*vertical});
          if(interior.width>0&&interior.height>0&&uint64_t(interior.width)*interior.height>=128){
            const size_t offset=size_t(interior.y-area.y)*stride+size_t(interior.x-area.x);
            const size_t available=size_t(area.height-1)*stride+size_t(area.width);
            const uint32_t started=profilingClock?profilingClock():0;
            if(detail::blendContainerNativeInterior(pixels+offset,available-offset,stride,interior.width,interior.height,style.background,alpha)){
              Rect strips[4];const int count=raster_occlusion::strips(clipped,interior,strips);
              for(int part=0;part<count;++part){const auto status=paintNativeAlphaNodeRows(index,strips[part],area,pixels,stride,alpha);if(status!=Status::Ok)return status;}
              const auto status=finishOverlayPaintState(index,clipped,area,pixels,stride,alpha);if(status!=Status::Ok)return status;
              ++stats.containerAlphaInteriorBlits;stats.containerAlphaInteriorPixels+=uint64_t(interior.width)*interior.height;
              if(profilingClock)stats.containerAlphaInteriorMicros+=profilingClock()-started;
              return Status::Ok;
            }
          }
        }
      }
    }
#endif
    return paintNativeAlphaNodeRows(index,clipped,area,pixels,stride,alpha);
  }
  Status paintContainerAlphaOverlay(int index,Rect clipped,const DirtyRect &area,uint16_t *pixels,size_t stride,uint8_t alpha){
    if(!nativeGpuPaintAllowed())return Status::Unsupported;
#if NICE_GFX_UI_CONTAINER_ALPHA_PPA
    if(!alpha||alpha==255||!ppaBlendEnabled||renderOrder!=PixelOrder::Native||buildingSurface||snapshotTransition||rasterFrame.preview.page||
       !nativeOverlayTarget||stagingNativeTarget||behaviorVisualMoving()||focusStyle(index)||snapshotPressed==index)return Status::Unsupported;
    const auto &n=rasterFrame.current.page->nodes[index];const auto &value=rasterFrame.current.values[index];
    if(n.kind!=Kind::Container||value.kind!=Kind::Container||n.instrument||value.instrument||(value.flags&(Transparent|Disabled)))return Status::Unsupported;
    for(uint16_t parent=value.parent;parent!=NoParent;parent=rasterFrame.current.values[parent].parent)
      if(rasterFrame.current.values[parent].flags&Disabled)return Status::Unsupported;
    if(n.motion){const auto &pose=motionPose(*rasterFrame.current.page,rasterFrame.current.values,index);if(pose.scale!=1024||pose.rotation%360)return Status::Unsupported;}
    const auto &style=styleFor(*rasterFrame.current.page,rasterFrame.current.values,index);
    if(style.borderWidth||style.gradient)return Status::Unsupported;
    Rect box=paintCache->nodes[index].rect;box.x+=paintOffsetX;box.y+=paintOffsetY;
    if(box.width<=0||box.height<=0||uint64_t(box.width)*box.height<16384)return Status::Unsupported;
    const int32_t radius=std::min<int32_t>(style.radius,std::min(box.width,box.height)/2);
    const Rect center=intersect(clipped,{box.x,box.y+radius,box.width,box.height-2*radius});
    if(center.width<=0||center.height<=0)return Status::Unsupported;
    const uint32_t started=profilingClock?profilingClock():0;
    const Status status=ppaFade.blendColorIntoNative(*nativeOverlayTarget,{center.x,center.y,center.width,center.height},style.background,alpha);
    if(status!=Status::Ok)return status;
    ++stats.containerAlphaGpuBlits;stats.containerAlphaGpuPixels+=uint64_t(center.width)*center.height;
    ++stats.ppaBlendCalls;stats.ppaBlendPixels+=uint64_t(center.width)*center.height;if(profilingClock)stats.ppaBlendMicros+=profilingClock()-started;
    // The corner bands keep the original exact midpoint and CPU blend. The
    // central strip has no coverage/AA boundary and requires no saved underlay.
    const Rect top=intersect(clipped,{box.x,box.y,box.width,radius}),bottom=intersect(clipped,{box.x,box.y+box.height-radius,box.width,radius});
    for(const auto &edge:{top,bottom})if(edge.width>0&&edge.height>0){const Status edgeStatus=paintNativeAlphaNode(index,edge,area,pixels,stride,alpha);if(edgeStatus!=Status::Ok)return edgeStatus;}
    return Status::Ok;
#else
    (void)index;(void)clipped;(void)area;(void)pixels;(void)stride;(void)alpha;return Status::Unsupported;
#endif
  }
  Status finishOverlayPaintState(int index,Rect clipped,const DirtyRect &area,uint16_t *pixels,size_t stride,uint8_t alpha) {
    // GPU overlays bypass paintNode, but later focus/footer painters inherit
    // its AA setting and the ordinary alpha painter's restored canvas bounds.
    if(alpha<255&&renderOrder==PixelOrder::Native){
      const Status status=canvas.attach(area,pixels,size_t(area.height-1)*stride+size_t(area.width),stride,PixelOrder::Native);
      if(status!=Status::Ok)return status;
    }else canvas.setViewport(clipped.x,clipped.y,clipped.width,clipped.height,false);
    canvas.setAntialiasing(!displayPolicy.monochrome&&!(rasterFrame.current.values[index].flags&NoAntialias));
    return Status::Ok;
  }
#include "detail/MotionRender.inc.h"
#include "detail/TextInput.inc.h"
#include "detail/OpaqueCover.inc.h"
#include "detail/SystemModalCoveredBar.inc.h"
#include "detail/FixedChromeRender.inc.h"
#include "detail/NativeResourceWarm.inc.h"
#include "detail/SystemPanelTextPrepare.inc.h"
  // Run once on each finished scanout tile, after raw retained-surface copies.
  // Bayer coordinates are screen anchored, independent of dirty tile boundaries.
  void finishMonochromeTile(const DirtyRect &area,uint16_t *pixels,size_t stride,const Rect *unfusedBackdrop=nullptr) {
    if(buildingSurface||foregroundCapture||nativeCacheCapturing)return;
    paintBehaviorBackdrop({area.x,area.y,area.width,area.height},pixels,stride,unfusedBackdrop);
    paintBehaviorFrames(area);
    if(displayPolicy.monochrome&&!displayPolicy.slowRefresh&&snapshotMonochromeEffects==MonochromeEffects::Bayer4){
      MonoOptions options;options.dither=MonoDither::Bayer4;
      for(int32_t y=0;y<area.height;++y)for(int32_t x=0;x<area.width;++x){
        uint16_t &pixel=pixels[size_t(y)*stride+size_t(x)];
        const uint16_t value=renderOrder==PixelOrder::Wire?rgb565_blend::fromWire(pixel):pixel;
        const uint16_t color=monoWhite(value,options,unsigned(area.x+x),unsigned(area.y+y))?0xffff:0;
        pixel=renderOrder==PixelOrder::Wire?rgb565_blend::toWire(color):color;
      }
    }
    if(displayPolicy.monochrome&&snapshotLightMode){
      for(int32_t y=0;y<area.height;++y)for(int32_t x=0;x<area.width;++x){uint16_t &pixel=pixels[size_t(y)*stride+x];const uint16_t value=renderOrder==PixelOrder::Wire?rgb565_blend::fromWire(pixel):pixel;const uint16_t inverse=DisplayPolicy::foregroundOn(value);pixel=renderOrder==PixelOrder::Wire?rgb565_blend::toWire(inverse):inverse;}
    }
    // Invert the final binary backdrop, including any translucent content.
    if(snapshotInput&&snapshotMenuVisible&&!snapshotTransition&&!rasterFrame.preview.page)
      {paintMenuIndicator(area,pixels,stride);paintAdvancedMenu(area);}
  }
#if defined(CONFIG_IDF_TARGET_ESP32S31) && NICE_GFX_S31_PPA_BLEND_AVAILABLE
  static Status render(void *context,const DirtyRect &requestedArea,uint16_t *pixels,size_t stride) {
    auto &self=*static_cast<Impl*>(context);
    struct FillFailureScope {
      Status &failure;Status saved;
      explicit FillFailureScope(Status &value):failure(value),saved(value){failure=Status::Ok;}
      ~FillFailureScope(){failure=saved;}
    } failureScope(self.nativeOpaqueFillFailure);
    const Status result=renderNativeCanvas(context,requestedArea,pixels,stride);
    return self.nativeOpaqueFillFailure==Status::Ok?result:self.nativeOpaqueFillFailure;
  }
  static Status renderNativeCanvas(void *context,const DirtyRect &requestedArea,uint16_t *pixels,size_t stride) {
#else
  static Status render(void *context,const DirtyRect &requestedArea,uint16_t *pixels,size_t stride) {
#endif
    Impl &self=*static_cast<Impl *>(context);
    DirtyRect area=requestedArea;
    const Rect v=self.snapshotMotionRect;
    if(self.snapshotTransition&&!self.buildingSurface&&!self.renderingViewport&&v.width>0&&
       (v.x||v.y||v.width!=self.rasterFrame.current.page->width||v.height!=self.rasterFrame.current.page->height)){
      // The moving viewport overwrites every content pixel. Paint only the
      // disjoint stationary strips, rather than rasterizing the complete
      // incoming tile and then painting the whole content a second time.
      const Rect content=intersect(v,{area.x,area.y,area.width,area.height});
      Rect fixed[4];const int fixedCount=raster_occlusion::strips({area.x,area.y,area.width,area.height},content,fixed);
      const bool savedTransition=self.snapshotTransition;const PageDesc *savedPreview=self.rasterFrame.preview.page;
      const PageDesc *savedPage=self.rasterFrame.current.page;const Surface *savedSurface=self.snapshotSurface;
      const bool keepSource=self.groupMotion.active&&self.groupMotion.prepared&&savedPreview;
      if(keepSource){self.rasterFrame.current.page=savedPreview;self.rasterFrame.current.values.swap(self.rasterFrame.preview.values);self.rasterFrame.current.motions.swap(self.rasterFrame.preview.motions);std::swap(self.rasterFrame.current.geometry,self.rasterFrame.preview.geometry);self.snapshotSurface=self.previewSurface;}
      self.snapshotTransition=false;self.rasterFrame.preview.page=nullptr;
      Status base=Status::Ok;
      for(int i=0;i<fixedCount&&base==Status::Ok;++i){const Rect &r=fixed[i];
        base=render(context,DirtyRect(r.x,r.y,r.width,r.height),pixels+size_t(r.y-area.y)*stride+size_t(r.x-area.x),stride);}
      if(keepSource){self.rasterFrame.current.values.swap(self.rasterFrame.preview.values);self.rasterFrame.current.motions.swap(self.rasterFrame.preview.motions);std::swap(self.rasterFrame.current.geometry,self.rasterFrame.preview.geometry);self.rasterFrame.current.page=savedPage;self.snapshotSurface=savedSurface;}
      self.snapshotTransition=savedTransition;self.rasterFrame.preview.page=savedPreview;if(base!=Status::Ok)return base;
      if(content.width<=0||content.height<=0)return Status::Ok;
      self.renderingViewport=true;
      const Status result=render(context,DirtyRect(content.x,content.y,content.width,content.height),
        pixels+size_t(content.y-area.y)*stride+size_t(content.x-area.x),stride);
      self.renderingViewport=false;return result;
    }
    RenderTimer timer(self);
    ++self.stats.renderedTiles;self.stats.renderedPixels+=uint64_t(area.width)*area.height;
    const size_t available=(self.buildingSurface||self.renderingFullFrameSurface)?size_t(area.height-1)*stride+size_t(area.width):self.capacity;
    Status result=self.canvas.attach(area,pixels,available,stride,self.buildingSurface?PixelOrder::Wire:self.renderOrder);if(result!=Status::Ok)return result;
    self.canvas.setNativeFillHandler(fillNativeCanvas,&self);
    self.canvas.setChartPhaseClock(self.profilingClock);
    self.retainedCanvasExclusions();
    const bool discardClear=self.repaintDiscardFrame&&!self.buildingSurface&&!self.foregroundCapture;
    // Ordinary write-discard replaces every supplied pixel. The separate
    // retained contract excludes only this exact allocation's certified chart
    // centers; their background is restored from the old-ink mask below.
    // This is the proof behind beginRepaintFrame: before ANY underlay read,
    // establish all pixels of this exact clipped renderer ROI. Alpha painters
    // then read the new page background, never an uninherited old back buffer.
    if(discardClear)self.canvas.fillRect(area.x,area.y,area.width,area.height,self.rasterFrame.current.page->background);
#if (NICE_GFX_P4_NATIVE_PRESENTATION_OPAQUE_FILL && NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE) || (defined(CONFIG_IDF_TARGET_ESP32S31) && NICE_GFX_S31_PPA_BLEND_AVAILABLE)
    if(self.nativeOpaqueFillFailure!=Status::Ok){self.canvas.detach();return self.nativeOpaqueFillFailure;}
#endif
    // A modal keyboard paints an opaque full-width background. Skip every
    // covered page node; a dock leaves only the band above it to rasterize.
    // This also applies to circular output, whose physical clip is imposed by
    // the scheduler on both layers rather than by the keyboard background.
    if(!self.buildingSurface&&!self.snapshotTransition&&!self.rasterFrame.preview.page&&self.snapshotKeyboard.visible&&self.snapshotKeyboard.node>=0){
      area.height=std::max<int32_t>(0,std::min<int32_t>(area.height,self.snapshotKeyboard.rect.y-area.y));
      if(!area.height){self.paintKeyboard({requestedArea.x,requestedArea.y,requestedArea.width,requestedArea.height});self.finishMonochromeTile(requestedArea,pixels,stride);self.canvas.detach();return Status::Ok;}
    }
    // A stationary cached enter/exit fade uses a constant background.
    // Blend immutable pixels directly without materializing an underlay.
    if(self.renderOrder==PixelOrder::Wire&&!self.buildingSurface&&self.transition.config.effect!=TransitionEffect::Dissolve&&self.snapshotTransition&&!self.rasterFrame.preview.page&&self.snapshotSurface&&
       (self.snapshotPhase==TransitionPhase::Enter||self.snapshotPhase==TransitionPhase::Exit)&&
       !self.snapshotKeyboard.visible&&!self.snapshotInput&&
       (self.snapshotGroupIndex<0||!self.snapshotGroup.indicatorHeight)&&
       !v.x&&!v.y&&v.width==self.rasterFrame.current.page->width&&v.height==self.rasterFrame.current.page->height&&
       !self.transitionFrame.outX&&!self.transitionFrame.outY&&!self.transitionFrame.inX&&!self.transitionFrame.inY){
      const size_t pageWidth=self.rasterFrame.current.page->width;
      const size_t offset=size_t(area.y)*pageWidth+size_t(area.x);
      self.blendCachedRows(pixels,stride,self.snapshotSurface->pixels+offset,nullptr,pageWidth,
        size_t(area.width),size_t(area.height),self.transitionFrame.opacity,self.snapshotBackground);
      ++self.surfaceStats.hits;self.finishMonochromeTile(area,pixels,stride);self.canvas.detach();return Status::Ok;
    }
    // Cached paired fades have exact immutable pixels for both pages. Blend
    // them straight into the tile instead of copying the outgoing page,
    // saving its underlay, painting the incoming page, then blending in place.
    if(self.renderOrder==PixelOrder::Wire&&!self.buildingSurface&&self.transition.config.effect!=TransitionEffect::Dissolve&&self.snapshotTransition&&self.snapshotPhase==TransitionPhase::Paired&&
       self.rasterFrame.preview.page&&self.snapshotSurface&&self.previewSurface&&!self.snapshotKeyboard.visible&&
       !self.snapshotInput&&(self.snapshotGroupIndex<0||!self.snapshotGroup.indicatorHeight)&&
       !self.transitionFrame.outX&&!self.transitionFrame.outY&&!self.transitionFrame.inX&&!self.transitionFrame.inY){
      const size_t pageWidth=self.rasterFrame.current.page->width;
      const size_t offset=size_t(area.y)*pageWidth+size_t(area.x);
      self.blendCachedRows(pixels,stride,self.snapshotSurface->pixels+offset,
        self.previewSurface->pixels+offset,pageWidth,size_t(area.width),size_t(area.height),self.transitionFrame.opacity,0);
      self.surfaceStats.hits+=self.transitionFrame.opacity&&self.transitionFrame.opacity<255?2:1;
      self.finishMonochromeTile(area,pixels,stride);self.canvas.detach();return Status::Ok;
    }
    const PageDesc *activePage=self.rasterFrame.current.page;
    const int activePressed=self.snapshotPressed;
    const bool transitionCover=self.renderOrder==PixelOrder::Native&&!self.buildingSurface&&self.opaqueCoverTransition()&&
      self.opaqueCoverAt(self.rasterFrame.current.geometry,area,
        self.snapshotPhase==TransitionPhase::Enter?self.transitionFrame.inX:self.transitionFrame.outX,
        self.snapshotPhase==TransitionPhase::Enter?self.transitionFrame.inY:self.transitionFrame.outY)>=0;
    if(self.snapshotTransition&&!self.rasterFrame.preview.page&&!transitionCover){
      self.canvas.setViewport(area.x,area.y,area.width,area.height,false);
      self.canvas.fillRect(area.x,area.y,area.width,area.height,self.snapshotBackground);
    }
    // An opaque incoming page covers this tile completely at a fade/slide
    // endpoint. Its background also overwrites all outgoing content, so omit
    // that layer without allocating or touching its saved underlay.
    const bool incomingCovers=self.rasterFrame.preview.page&&self.snapshotTransition&&self.transitionFrame.opacity==255&&
      area.x>=self.transitionFrame.inX+(self.renderingViewport?v.x:0)&&area.y>=self.transitionFrame.inY+(self.renderingViewport?v.y:0)&&
      int64_t(area.x)+area.width<=int64_t(self.transitionFrame.inX)+(self.renderingViewport?v.x+v.width:activePage->width)&&
      int64_t(area.y)+area.height<=int64_t(self.transitionFrame.inY)+(self.renderingViewport?v.y+v.height:activePage->height);
    if(self.rasterFrame.preview.page&&!incomingCovers) { self.rasterFrame.current.page=self.rasterFrame.preview.page;self.rasterFrame.current.values.swap(self.rasterFrame.preview.values);self.rasterFrame.current.motions.swap(self.rasterFrame.preview.motions); }
    bool modalBaseBackdropFused=false;Rect modalBaseBackdropRemainder{};
    const int layers=self.rasterFrame.preview.page?2:1;
    for(int layer=incomingCovers?1:0;layer<layers;++layer) {
      self.paintCache=self.rasterFrame.preview.page&&layer==0?&self.rasterFrame.preview.geometry:&self.rasterFrame.current.geometry;
      const bool entering=!self.rasterFrame.preview.page&&self.snapshotPhase==TransitionPhase::Enter;
      self.paintOffsetX=self.snapshotTransition?(entering||layer==1?self.transitionFrame.inX:self.transitionFrame.outX):self.rasterFrame.preview.page&&layer==1?self.snapshotBackOffset:0;
      self.paintOffsetY=self.snapshotTransition?(entering||layer==1?self.transitionFrame.inY:self.transitionFrame.outY):0;
      self.snapshotPressed=self.rasterFrame.preview.page&&layer==0?-1:activePressed;
      const bool fade=self.snapshotTransition&&(layer==1||!self.rasterFrame.preview.page)&&self.transitionFrame.opacity<255;
      // At alpha zero the outgoing tile is already the final image. In
      // particular, do not rasterize a fully invisible incoming page.
      if(fade&&!self.transitionFrame.opacity)continue;
      if(fade) {
        const size_t needed=size_t(area.width)*area.height;
        if(self.transitionUnderlay.size()<needed)self.transitionUnderlay.resize(needed);
        for(int32_t y=0;y<area.height;++y)memcpy(self.transitionUnderlay.data()+size_t(y)*area.width,pixels+size_t(y)*stride,size_t(area.width)*2);
      }
      self.canvas.setViewport(area.x,area.y,area.width,area.height,false);
      bool modalBaseApplied=false;result=self.paintSystemModalBase(area,modalBaseApplied,modalBaseBackdropFused,modalBaseBackdropRemainder);
      if(result!=Status::Ok){self.canvas.detach();return result;}
      const Surface *surface=self.behaviorVisualMoving()?nullptr:self.rasterFrame.preview.page&&layer==0?self.previewSurface:self.snapshotSurface;
      if(!modalBaseApplied&&!self.buildingSurface&&self.snapshotAnimationSurface&&!self.rasterFrame.preview.page){
        result=self.paintAnimationSurface(area);
        if(result!=Status::Ok){self.canvas.detach();return result;}
      }else if(!modalBaseApplied&&!self.buildingSurface&&surface){
        ++self.surfaceStats.hits;
        const Rect source=self.renderingViewport?v:Rect{0,0,self.rasterFrame.current.page->width,self.rasterFrame.current.page->height};
        result=self.canvas.blit(source.x+self.paintOffsetX,source.y+self.paintOffsetY,
          PixelView(surface->pixels+size_t(source.y)*self.rasterFrame.current.page->width+source.x,source.width,source.height,self.rasterFrame.current.page->width,self.cacheOrder));
        if(result!=Status::Ok){self.canvas.detach();return result;}
      } else {
      if(!self.buildingSurface&&!self.surfaces.empty())++self.surfaceStats.misses;
      const Rect source=self.renderingViewport?v:Rect{0,0,self.rasterFrame.current.page->width,self.rasterFrame.current.page->height};
      const int opaqueCover=modalBaseApplied||self.nativePrefixLimit>=0||self.foregroundCapture||self.behaviorVisualMoving()||self.viewportOverlayLevels?-1:self.opaqueCover(area);
      const int containerCover=modalBaseApplied||self.nativePrefixLimit>=0?-1:self.opaqueContainerCover(area);
      int firstPaint=self.foregroundCapture?self.foregroundPrefix+1:std::max(opaqueCover,containerCover);
#if NICE_GFX_UI_NATIVE_CACHE_REPAIR && NICE_GFX_UI_NATIVE_PREFIX_CACHE
      if(self.nativePrefixSource&&self.nativePrefixLimit<0&&self.renderOrder==PixelOrder::Native&&int(self.nativePrefixSource->limit)>firstPaint){
// BEGIN ROI_PREFIX_TIMING copy-total
#if NICE_GFX_UI_ROI_PREFIX_TIMING
        RoiPrefixTimer prefixCopyTimer(self,RoiPrefixTimingPhase::PrefixCopy);
#endif
// END ROI_PREFIX_TIMING copy-total
        const auto &prefix=*self.nativePrefixSource;
        Rect prefixRect{0,0,self.rasterFrame.current.page->width,self.rasterFrame.current.page->height};
#if NICE_GFX_UI_ROI_PREFIX_BOOTSTRAP
        if(prefix.roi.width)prefixRect=prefix.roi;
#endif
        const size_t prefixX=size_t(area.x-prefixRect.x),prefixY=size_t(area.y-prefixRect.y);
        const PixelView source(prefix.storage.pixels,prefixRect.width,prefixRect.height,prefix.pitch,PixelOrder::Native);
        result=Status::Unsupported;
#if defined(CONFIG_IDF_TARGET_ESP32P4)
        // Only the actual mapped display frame may use its transport here.
        // Independent cache repairs keep their own target. A contiguous full
        // row underlay can use the backend's fenced AXI-GDMA path.
        if(self.nativeOverlayTarget&&!self.nativeCacheRepairing&&!self.stagingNativeTarget&&!area.x&&
           size_t(area.width)==source.width&&source.width==self.nativeOverlayTarget->width&&prefix.pitch==source.width){
// BEGIN ROI_PREFIX_TIMING copy-transport
#if NICE_GFX_UI_ROI_PREFIX_TIMING
          RoiPrefixTimer prefixTransportTimer(self,RoiPrefixTimingPhase::TransportCopy);
#endif
// END ROI_PREFIX_TIMING copy-transport
          if(auto *panel=nice_gfx::detail::DisplayAccess::panel(self.display)){
            const PixelView rows(source.data+prefixY*prefix.pitch*2,area.width,area.height,prefix.pitch,PixelOrder::Native);
            if(panel->canFrameBlit(rows)){
              result=panel->flush(0,area.y,rows,false);
              if(result==Status::Ok&&panel->busy())result=panel->finish();
              if(result!=Status::Ok&&result!=Status::Unsupported)panel->finish();
            }
          }
// BEGIN ROI_PREFIX_TIMING copy-transport-result
#if NICE_GFX_UI_ROI_PREFIX_TIMING
          prefixTransportTimer.result(result);
#endif
// END ROI_PREFIX_TIMING copy-transport-result
        }
        // The prefix is immutable PSRAM, and this exact ROI is exclusively
        // owned by the mapped renderer. Let PPA copy large patches without a
        // CPU PSRAM read/write round trip; its blocking coherency contract
        // preserves adjacent CPU painters in the same extended output rows.
        // Cropped ROI prefixes use the established CPU blit below. The
        // pinned SDK blocked indefinitely in this SRM copy during board stress.
        // Full-page legacy surfaces retain their existing transport path.
        if(result==Status::Unsupported&&self.nativeOverlayTarget&&area.pixels()>=4096
#if NICE_GFX_UI_ROI_PREFIX_BOOTSTRAP
           && !prefix.roi.width
#endif
          )
// BEGIN ROI_PREFIX_TIMING copy-ppa-open
#if NICE_GFX_UI_ROI_PREFIX_TIMING
          {RoiPrefixTimer prefixPpaTimer(self,RoiPrefixTimingPhase::PpaCopy);
#endif
// END ROI_PREFIX_TIMING copy-ppa-open
          result=self.ppaFade.copyIntoNative(*self.nativeOverlayTarget,area.x,area.y,source,prefix.storage.bytes,prefixX,prefixY,area.width,area.height);
// BEGIN ROI_PREFIX_TIMING copy-ppa-close
#if NICE_GFX_UI_ROI_PREFIX_TIMING
          prefixPpaTimer.result(result);}
#endif
// END ROI_PREFIX_TIMING copy-ppa-close

#endif
        if(result==Status::Unsupported)
// BEGIN ROI_PREFIX_TIMING copy-canvas-open
#if NICE_GFX_UI_ROI_PREFIX_TIMING
        {RoiPrefixTimer prefixCanvasTimer(self,RoiPrefixTimingPhase::CanvasCopy);
#endif
// END ROI_PREFIX_TIMING copy-canvas-open
        result=self.canvas.blit(prefixRect.x,prefixRect.y,source);
// BEGIN ROI_PREFIX_TIMING copy-canvas-close
#if NICE_GFX_UI_ROI_PREFIX_TIMING
        prefixCanvasTimer.result(result);}
#endif
// END ROI_PREFIX_TIMING copy-canvas-close

        if(result!=Status::Ok){self.canvas.detach();return result;}firstPaint=int(prefix.limit);self.stats.nativePrefixBlitPixels+=area.pixels();
#if NICE_GFX_UI_ROI_PREFIX_BOOTSTRAP
        if(prefix.roi.width)self.stats.roiPrefixBlitPixels+=area.pixels();
#if NICE_GFX_UI_ROI_PREFIX_DIRTY_REGION
        if(prefix.roi.width&&prefix.roiGroup==NoParent){self.stats.roiDirtyPrefixBlitPixels+=area.pixels();if(prefix.roiScopeKind==2)self.stats.roiDirtyPrefixWorldBlitPixels+=area.pixels();}
#endif
#endif
      }
#endif
      if(opaqueCover>=0&&opaqueCover>=containerCover&&firstPaint==opaqueCover&&self.renderOrder==PixelOrder::Native)if(auto *panel=nice_gfx::detail::DisplayAccess::panel(self.display)){
        bool painted=false;result=self.blitOpaqueCover(*panel,opaqueCover,area,painted);
        if(result!=Status::Ok){self.canvas.detach();return result;}
        if(painted)++firstPaint;
      }
      const FrameCache &cache=*self.paintCache;
      const SystemModalCover modalCover=self.systemModalCover();
      const bool topModalCover=self.systemModalTopCover(modalCover);
      if(firstPaint<0&&!self.foregroundCapture&&!modalBaseApplied&&!discardClear){
        // The unchanged opaque root paints every exposed host pixel; the
        // later opaque panel paints the omitted top. Their union covers area.
        if(topModalCover)self.stats.rootClearSkippedPixels+=area.pixels();
        else self.canvas.fillRect(source.x+self.paintOffsetX,source.y+self.paintOffsetY,source.width,source.height,self.rasterFrame.current.page->background);
      }
#if defined(CONFIG_IDF_TARGET_ESP32S31) && NICE_GFX_S31_PPA_BLEND_AVAILABLE
      if(self.nativeOpaqueFillFailure!=Status::Ok){self.canvas.detach();return self.nativeOpaqueFillFailure;}
#endif
#if NICE_GFX_UI_FIXED_CHROME_PPA
      FixedChromeApplied chrome;
      if(!self.foregroundCapture&&opaqueCover>=0&&firstPaint==opaqueCover+1){
        result=self.paintFixedChrome(opaqueCover,area,pixels,available,stride,chrome);
        if(result!=Status::Ok){self.canvas.detach();return result;}
      }
#endif
      const int firstBand=clamp32((area.y-self.paintOffsetY)/cache.bandHeight,0,31),lastBand=clamp32((area.y+area.height-1-self.paintOffsetY)/cache.bandHeight,0,31);
      const bool coveredModalBar=self.systemModalBarCovered(area,modalBaseApplied,modalBaseBackdropFused,firstPaint);
      bool modalPanelApplied=false;
      const size_t firstCandidate=firstPaint>0?size_t(firstPaint):0;
      for(unsigned overlayLevel=0;overlayLevel<=self.viewportOverlayLevels;++overlayLevel)
      for(size_t word=firstCandidate/32;word<cache.words;++word){uint32_t candidates=0;
      for(int band=firstBand;band<=lastBand;++band)candidates|=cache.bands[size_t(band)*cache.words+word];
      if(word==firstCandidate/32)candidates&=uint32_t(0xffffffffu)<<(firstCandidate%32);
      while(candidates){int bit=0;
#if defined(__GNUC__) || defined(__clang__)
      bit=__builtin_ctz(candidates);
#else
      uint32_t bits=candidates;
      if(!(bits&0xffffu)){bits>>=16;bit+=16;}if(!(bits&0xffu)){bits>>=8;bit+=8;}
      if(!(bits&0xfu)){bits>>=4;bit+=4;}if(!(bits&3u)){bits>>=2;bit+=2;}if(!(bits&1u))++bit;
#endif
      candidates&=candidates-1;const int i=int(word*32)+bit;++self.stats.candidateNodes;
      if(self.nativePrefixLimit>=0&&i>=self.nativePrefixLimit)continue;
      if(self.viewportOverlayDepth(*self.rasterFrame.current.page,self.rasterFrame.current.values,i)!=overlayLevel)continue;
      const Geometry &g=cache.nodes[i];Rect clipped=self.behaviorClip(i,intersect(g.clip,g.rect));
      if(self.renderingViewport)clipped=intersect(clipped,v);
      clipped.x+=self.paintOffsetX;clipped.y+=self.paintOffsetY;
      const Rect nodePaintBounds=clipped;
      clipped=intersect(clipped,{area.x,area.y,area.width,area.height});if(clipped.width<=0||clipped.height<=0)continue;
      const uint8_t alpha=g.alpha;if(!alpha)continue;
      if(coveredModalBar&&self.systemModalBaseKind(uint16_t(i),1)){
        result=self.finishOverlayPaintState(i,clipped,area,pixels,stride,alpha);
        if(result!=Status::Ok){self.canvas.detach();return result;}
        ++self.stats.systemModalCoveredNodes;
#ifdef NICE_GFX_UI_SYSTEM_MODAL_COVERED_BAR_TEST_OBSERVER
        NICE_GFX_UI_SYSTEM_MODAL_COVERED_BAR_TEST_OBSERVER(uint16_t(i));
#endif
        continue;
      }
      if(self.systemModalPanelFrameNode(uint16_t(i))){
        if(i==self.systemModalPanelFrame.root){result=self.paintSystemModalPanel(area,modalPanelApplied);if(result!=Status::Ok){self.canvas.detach();return result;}}
        if(modalPanelApplied){result=self.finishOverlayPaintState(i,clipped,area,pixels,stride,alpha);if(result!=Status::Ok){self.canvas.detach();return result;}continue;}
      }
      if(modalBaseApplied&&self.systemModalBaseKind(uint16_t(i),0)){
        result=self.finishOverlayPaintState(i,clipped,area,pixels,stride,alpha);
        if(result!=Status::Ok){self.canvas.detach();return result;}
        ++self.stats.systemModalBaseSkippedNodes;continue;
      }
      if(modalCover.root>=0&&self.systemModalCovers(modalCover,i,uint16_t(overlayLevel),nodePaintBounds)){
        result=self.finishOverlayPaintState(i,clipped,area,pixels,stride,alpha);
        if(result!=Status::Ok){self.canvas.detach();return result;}
        ++self.stats.systemModalCoveredNodes;continue;
      }
      if(topModalCover){
        const Rect before=clipped;
        if(self.clipSystemModalHost(modalCover,i,uint16_t(overlayLevel),clipped)){
          self.stats.systemModalClippedPixels+=uint64_t(before.width)*before.height-uint64_t(clipped.width)*clipped.height;
          if(clipped.height<=0){result=self.finishOverlayPaintState(i,before,area,pixels,stride,alpha);if(result!=Status::Ok){self.canvas.detach();return result;}continue;}
        }
      }
#if NICE_GFX_UI_SKIP_EMPTY_CONTAINERS
      if(self.emptyContainerPaint(i)){
        // Preserve the empty painter's canvas state for later focus/footer
        // drawing, while omitting its identity underlay copy and alpha blend.
        if(alpha<255&&self.renderOrder==PixelOrder::Native){
          result=self.canvas.attach(area,pixels,available,stride,PixelOrder::Native);
          if(result!=Status::Ok){self.canvas.detach();return result;}
        }else self.canvas.setViewport(clipped.x,clipped.y,clipped.width,clipped.height,false);
        self.canvas.setAntialiasing(!self.displayPolicy.monochrome&&!(self.rasterFrame.current.values[i].flags&NoAntialias));
        continue;
      }
#endif
#if NICE_GFX_UI_FIXED_CHROME_PPA
      if(!self.foregroundCapture&&self.fixedChromeContains(chrome,clipped)){
        result=self.finishOverlayPaintState(i,clipped,area,pixels,stride,alpha);
        if(result!=Status::Ok){self.canvas.detach();return result;}++self.stats.fixedChromeNodesSkipped;continue;
      }
#endif
      ++self.stats.paintedNodes;
#if NICE_GFX_UI_NODE_KIND_PROFILE || NICE_GFX_UI_SHAPE_PATH_PROFILE
      NodePaintTimer nodeTimer(self,self.rasterFrame.current.page->nodes[i].kind);
#else
      ProfileTimer nodeTimer(self,paintProfileCounter(self.rasterFrame.current.page->nodes[i].kind));
#endif
      if(self.rasterFrame.current.page->nodes[i].motion){const auto &m=self.motionPose(*self.rasterFrame.current.page,self.rasterFrame.current.values,i);
        if(m.scale!=1024||m.rotation%360!=0){
#if NICE_GFX_UI_AXIS_MEDIA_PPA && NICE_GFX_UI_AXIS_MEDIA_CACHE
          self.axisPpaError=Status::Ok;
#endif
          self.paintTransformedNode(i,clipped,area,pixels,stride,alpha);
#if NICE_GFX_UI_AXIS_MEDIA_PPA && NICE_GFX_UI_AXIS_MEDIA_CACHE
          if(self.axisPpaError!=Status::Ok){self.canvas.detach();return self.axisPpaError;}
#endif
          continue;}}
      if(self.rasterFrame.current.page->nodes[i].kind==Kind::Media){
        const Status overlay=self.paintRgb565MediaOverlay(i,clipped,alpha);
        if(overlay==Status::Ok){result=self.finishOverlayPaintState(i,clipped,area,pixels,stride,alpha);if(result!=Status::Ok){self.canvas.detach();return result;}continue;}
        if(overlay!=Status::Unsupported){self.canvas.detach();return overlay;}
      }
      if(self.rasterFrame.current.page->nodes[i].kind==Kind::Media){
        const Status overlay=self.paintKeyedMediaOverlay(i,clipped,alpha);
        if(overlay==Status::Ok){result=self.finishOverlayPaintState(i,clipped,area,pixels,stride,alpha);if(result!=Status::Ok){self.canvas.detach();return result;}continue;}
        if(overlay!=Status::Unsupported){self.canvas.detach();return overlay;}
      }
      if(self.rasterFrame.current.page->nodes[i].kind==Kind::Shape){
        Status overlay=self.paintShapeWire(i,clipped,area,pixels,stride,alpha);
#if NICE_GFX_UI_STAGE_SHAPE_NATIVE_SPANS
        if(overlay==Status::Unsupported)overlay=self.paintStageShapeNative(i,clipped,area,pixels,stride,alpha);
#endif
        if(overlay==Status::Unsupported)overlay=self.paintShapeNative(i,clipped,alpha);
        if(overlay==Status::Unsupported)overlay=self.paintShapeOverlay(i,clipped,alpha);
        if(overlay==Status::Ok){const uint32_t finishStarted=self.profilingClock?self.profilingClock():0;result=self.finishOverlayPaintState(i,clipped,area,pixels,stride,alpha);if(self.profilingClock)self.stats.shapeFinishMicros+=self.profilingClock()-finishStarted;if(result!=Status::Ok){self.canvas.detach();return result;}continue;}
        if(overlay!=Status::Unsupported){self.canvas.detach();return overlay;}
      }
      if(self.rasterFrame.current.page->nodes[i].kind==Kind::Text||self.rasterFrame.current.page->nodes[i].kind==Kind::Label){
#if NICE_GFX_UI_TEXT_MASK_NATIVE
        Status overlay=Status::Unsupported;
#if NICE_GFX_UI_TEXT_ALPHA_PPA
        // Stationary opaque text stays on the CPU ink-only path; animated
        // opacity may use A8 hardware without making PPA a required backend.
        if(alpha<255)overlay=self.paintTextMaskOverlay(i,clipped,alpha);
#endif
        if(overlay==Status::Unsupported)overlay=self.paintTextMaskNative(i,clipped,alpha);
#else
        const Status overlay=self.paintTextMaskOverlay(i,clipped,alpha);
#endif
        if(overlay==Status::Ok){const uint32_t finishStarted=self.profilingClock?self.profilingClock():0;result=self.finishOverlayPaintState(i,clipped,area,pixels,stride,alpha);if(self.profilingClock)self.stats.textFinishMicros+=self.profilingClock()-finishStarted;if(result!=Status::Ok){self.canvas.detach();return result;}continue;}
        if(overlay!=Status::Unsupported){self.canvas.detach();return overlay;}
      }
      if(alpha<255&&self.rasterFrame.current.page->nodes[i].kind==Kind::Container){
        const Status overlay=self.paintContainerAlphaOverlay(i,clipped,area,pixels,stride,alpha);
        if(overlay==Status::Ok){result=self.finishOverlayPaintState(i,clipped,area,pixels,stride,alpha);if(result!=Status::Ok){self.canvas.detach();return result;}continue;}
        if(overlay!=Status::Unsupported){self.canvas.detach();return overlay;}
      }
      if(alpha<255&&self.rasterFrame.current.page->nodes[i].kind==Kind::Media&&
         self.paintRawMediaAlpha(i,clipped,area,pixels,stride,alpha))continue;
#if NICE_GFX_UI_FLAT_TEXT_ALPHA
      if(alpha<255&&self.paintFlatTextAlpha(i,clipped,area,pixels,stride,alpha))continue;
#endif
      if(alpha<255&&self.renderOrder==PixelOrder::Native){
        result=self.paintNativeAlphaNode(i,clipped,area,pixels,stride,alpha);
        if(result!=Status::Ok){self.canvas.detach();return result;}continue;
      }
      if(alpha<255) {
        const size_t needed=size_t(clipped.width)*clipped.height;
        if(self.underlay.size()<needed)self.underlay.resize(needed);
        for(int32_t y=0;y<clipped.height;++y)memcpy(self.underlay.data()+size_t(y)*clipped.width,pixels+size_t(clipped.y-area.y+y)*stride+clipped.x-area.x,size_t(clipped.width)*2);
      }
      self.canvas.setViewport(clipped.x,clipped.y,clipped.width,clipped.height,false);
#if defined(CONFIG_IDF_TARGET_ESP32S31) && NICE_GFX_S31_PPA_BLEND_AVAILABLE && NICE_GFX_UI_SYSTEM_MODAL_BASE_LEASE && NICE_GFX_UI_SYSTEM_MODAL_PANEL_CACHE && NICE_GFX_UI_SYSTEM_PANEL_PAGE_SOURCE
      Status nativeBackgroundStatus=Status::Ok;
      self.paintNode(i,clipped,modalBaseApplied&&modalPanelApplied&&alpha==255?&nativeBackgroundStatus:nullptr);
      if(nativeBackgroundStatus!=Status::Ok){self.canvas.detach();return nativeBackgroundStatus;}
#else
      self.paintNode(i,clipped);
#endif
#if (NICE_GFX_P4_NATIVE_PRESENTATION_OPAQUE_FILL && NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE) || (defined(CONFIG_IDF_TARGET_ESP32S31) && NICE_GFX_S31_PPA_BLEND_AVAILABLE)
    if(self.nativeOpaqueFillFailure!=Status::Ok){self.canvas.detach();return self.nativeOpaqueFillFailure;}
#endif
      if(alpha<255)rgb565_blend::wireRows(
        pixels+size_t(clipped.y-area.y)*stride+size_t(clipped.x-area.x),stride,
        self.underlay.data(),size_t(clipped.width),size_t(clipped.width),size_t(clipped.height),alpha);
      }
      }
      if(!self.snapshotMenuEnabled&&!self.foregroundCapture&&self.snapshotInput&&!self.snapshotTransition&&!self.rasterFrame.preview.page&&self.snapshotFocus>=0&&cache.nodes[self.snapshotFocus].inputVisible) {
        const Geometry &g=cache.nodes[self.snapshotFocus];const Rect r=g.rect,c=intersect(r,g.clip);
        self.canvas.setViewport(c.x,c.y,c.width,c.height,false);
        const int32_t radius=std::min<int32_t>(self.styleFor(*self.rasterFrame.current.page,self.rasterFrame.current.values,self.snapshotFocus).radius,std::min(r.width,r.height)/2);
        if(self.displayPolicy.monochrome){
          const Rect visible=intersect(c,{area.x,area.y,area.width,area.height});
          auto contrast=[&](int32_t x,int32_t y){
            if(x<visible.x||y<visible.y||x>=visible.x+visible.width||y>=visible.y+visible.height)return;
            uint16_t &pixel=pixels[size_t(y-area.y)*stride+size_t(x-area.x)];
            const uint16_t value=self.renderOrder==PixelOrder::Wire?rgb565_blend::fromWire(pixel):pixel;
            const uint16_t outline=DisplayPolicy::foregroundOn(value);
            pixel=self.renderOrder==PixelOrder::Wire?rgb565_blend::toWire(outline):outline;
          };
          // Each pixel belongs to one inset ring, so rounded corner joins
          // cannot invert the same monochrome destination pixel twice.
          const auto inside=[](int32_t x,int32_t y,Rect box,int32_t curve){
            if(box.width<=0||box.height<=0||x<box.x||y<box.y||x>=box.x+box.width||y>=box.y+box.height)return false;
            const int32_t dx=std::max<int32_t>(0,std::max(std::min(box.x+curve,box.x+box.width-curve-1)-x,x-std::max(box.x+curve,box.x+box.width-curve-1)));
            const int32_t dy=std::max<int32_t>(0,std::max(std::min(box.y+curve,box.y+box.height-curve-1)-y,y-std::max(box.y+curve,box.y+box.height-curve-1)));
            return dx*dx+dy*dy<=curve*curve;
          };
          for(int n=0;n<self.snapshotFocusWidth&&2*n<std::min(r.width,r.height);++n){
            const Rect ring{r.x+n,r.y+n,r.width-2*n,r.height-2*n},inner{ring.x+1,ring.y+1,ring.width-2,ring.height-2};
            const int32_t curve=std::min<int32_t>(std::max<int32_t>(0,radius-n),std::min(ring.width,ring.height)/2);
            const int32_t innerCurve=std::min<int32_t>(std::max<int32_t>(0,curve-1),std::max<int32_t>(0,std::min(inner.width,inner.height)/2));
            const Rect clipped=intersect(ring,visible);
            for(int32_t y=clipped.y;y<clipped.y+clipped.height;++y)for(int32_t x=clipped.x;x<clipped.x+clipped.width;++x)
              if(inside(x,y,ring,curve)&&!inside(x,y,inner,innerCurve))contrast(x,y);
          }
        }else for(int n=0;n<self.snapshotFocusWidth&&2*n<std::min(r.width,r.height);++n){
          const int32_t width=r.width-2*n,height=r.height-2*n,curve=std::min<int32_t>(std::max<int32_t>(0,radius-n),std::min(width,height)/2);
          self.canvas.drawRoundRect(r.x+n,r.y+n,width,height,curve,self.snapshotFocusColor);
        }
      }
      }
      if(fade){
        if(self.transition.config.effect==TransitionEffect::Dissolve){const auto &d=self.transition.config;
          for(int32_t y=0;y<area.height;++y)for(int32_t x=0;x<area.width;++x)
            if(!dissolveIncoming(area.x+x,area.y+y,self.transitionFrame.opacity,d.seed,d.grain))pixels[size_t(y)*stride+x]=self.transitionUnderlay[size_t(y)*area.width+x];
        }else rgb565_blend::wireRows(pixels,stride,self.transitionUnderlay.data(),size_t(area.width),size_t(area.width),size_t(area.height),self.transitionFrame.opacity);
      }
      if(self.rasterFrame.preview.page&&layer==0) { self.rasterFrame.current.page=activePage;self.rasterFrame.current.values.swap(self.rasterFrame.preview.values);self.rasterFrame.current.motions.swap(self.rasterFrame.preview.motions); }
    }
    self.snapshotPressed=activePressed;
    if(!self.foregroundCapture&&self.snapshotGroupIndex>=0&&self.snapshotGroup.indicatorHeight){
      const auto &g=self.snapshotGroup;const Rect footer=self.navigationFooter(g);const int width=footer.width,top=footer.y;
      const int step=int(g.dotSize)+g.dotGap,left=footer.x+(width-(int(g.count)*step-g.dotGap))/2;
      const int dotsY=top+(g.indicatorHeight-g.dotSize)/2;
      self.canvas.setViewport(area.x,area.y,area.width,area.height,false);
      if(!g.backgroundTransparent)self.canvas.fillRect(footer.x,top,width,g.indicatorHeight,g.background);
      for(uint16_t i=0;i<g.count;++i){
        const uint16_t color=self.displayPolicy.monochrome?DisplayPolicy::foregroundOn(g.background):i==self.snapshotGroupIndex?g.fillColor:g.trackColor;
        if(self.displayPolicy.monochrome&&i!=self.snapshotGroupIndex)self.canvas.drawRoundRect(left+i*step,dotsY,g.dotSize,g.dotSize,g.dotSize/2,color);
        else self.canvas.fillRoundRect(left+i*step,dotsY,g.dotSize,g.dotSize,g.dotSize/2,color);
      }
    }
    if(!self.buildingSurface&&!self.snapshotTransition&&!self.rasterFrame.preview.page)self.paintKeyboard({requestedArea.x,requestedArea.y,requestedArea.width,requestedArea.height});
    // Mirror an ordinary raster refresh into its unpublished retained slot.
    // A partial refresh is eligible only when its base was the immediately
    // preceding completed screen frame. Never rerasterize to build this copy.
    if(self.seedSurface&&!self.buildingSurface){
      const size_t width=self.rasterFrame.current.page->width;
      for(int32_t row=0;row<area.height;++row){
        auto *dst=self.seedSurface->pixels+size_t(area.y+row)*width+area.x;
        const auto *src=pixels+size_t(row)*stride;
        if(self.renderOrder==self.cacheOrder)memcpy(dst,src,size_t(area.width)*2);
        else for(int32_t col=0;col<area.width;++col)dst[col]=uint16_t((src[col]<<8)|(src[col]>>8));
      }
      self.seedPixels+=uint64_t(area.width)*area.height;
    }
    self.finishMonochromeTile(requestedArea,pixels,stride,modalBaseBackdropFused?&modalBaseBackdropRemainder:nullptr);
#if (NICE_GFX_P4_NATIVE_PRESENTATION_OPAQUE_FILL && NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE) || (defined(CONFIG_IDF_TARGET_ESP32S31) && NICE_GFX_S31_PPA_BLEND_AVAILABLE)
    if(self.nativeOpaqueFillFailure!=Status::Ok){self.canvas.detach();return self.nativeOpaqueFillFailure;}
#endif
    self.canvas.detach();return Status::Ok;
  }
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
  Status start(bool pipeline=false) {
    rgb565_ppa::Context::CpuOnlyNativePaint pipelineCpuPaint(ppaFade,pipeline);
    if(nativePipelineFatal)return Status::IoError;
#else
  Status start() {
#endif
    if(!project)return Status::NotInitialized;
    if(nativeStageFailureStatus()!=Status::Ok)return Status::IoError;
    if(processing)return Status::Busy;
    if(display.width()!=page().width||display.height()!=page().height)return Status::InvalidArgument;
    #if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
    if((refresh->busy()&&!pipeline)||!refresh->dirty()||updateDepth)return Status::Ok;
#else
    if(refresh->busy()||!refresh->dirty()||updateDepth)return Status::Ok;
#endif
    repaintDiscardFrame=false;
    ProfileTimer snapshotTimer(*this,&RuntimeStats::snapshotMicros);
    snapshotMonochromeEffects=monochromeEffects;
    snapshotBehaviors();
    seedSurface=nullptr;seedPixels=seedExpectedPixels=0;
    surfaceWarmPending=!surfaces.empty();++surfaceMutationEpoch;
    ensureInputAvailable();
    if(input&&focusNode>=0&&!focusable(focusNode,false)){
      processing=true;invalidate(focusNode);focusNode=-1;moveFocus(InputAction::Next);endProcessing();
      // Focus callbacks can batch mutations or queue navigation. Recheck gates
      // before publishing a frame; callbacks cannot recursively start snapshots.
      if(display.width()!=page().width||display.height()!=page().height)return Status::InvalidArgument;
      #if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
    if((refresh->busy()&&!pipeline)||!refresh->dirty()||updateDepth)return Status::Ok;
#else
    if(refresh->busy()||!refresh->dirty()||updateDepth)return Status::Ok;
#endif
    }
    // Sketch-global Views precede display initialization and rotation. Sync the
    // virtual raster bounds only when starting a validated, idle refresh.
    if(canvas.width()!=page().width||canvas.height()!=page().height)canvas.resize(page().width,page().height);
    snapshotFocus=focusNode;snapshotFocusId=focusNode>=0?page().nodes[focusNode].id:0;
    snapshotMenuMotion();
    snapshotAdvancedMenu();
    snapshotKeyInput=keyInput;
    snapshotInput=input!=nullptr&&snapshotKeyInput&&!transition.active;snapshotFocusColor=input?input->color:0;snapshotFocusWidth=input?input->width:0;
    cacheResourcePage(history.back());
    rasterFrame.current.page=&page();rasterFrame.current.values=state();rasterFrame.current.motions=pageMotions[history.back()];snapshotPressed=down&&!canceled&&activeControl>=0&&nodeContains(activeControl,x,y)?activeControl:-1;
    nativeStageCaptureSnapshot();
    if(buildKeyboard(softInput))invalidateKeyboardKeys();snapshotKeyboard=softInput;snapshotKeyboard.pressed=keyboardPressed;
    prepareKeyboardText();
    snapshotGroupIndex=-1;if(const auto *g=navigationGroup(page().path,&snapshotGroupIndex))snapshotGroup=*g;
    rasterFrame.preview.page=nullptr;snapshotBackOffset=backOffset;snapshotTransition=transition.active&&transition.phase!=TransitionPhase::Components;transitionFrame=transition.sample;
    snapshotPhase=transition.phase;snapshotBackground=transition.background;
    snapshotMotionRect=transitionViewport();
    snapshotTransitionEpoch=transition.active?transitionEpoch:0;
    if(!snapshotTransition&&coldFadeSurface&&(coldFadePage!=rasterFrame.current.page||coldFadeEpoch!=transitionEpoch))releaseColdFadeSurface();
    if(transition.active) {
      if(transition.phase==TransitionPhase::Paired){rasterFrame.preview.page=transition.from;rasterFrame.preview.values=transition.outgoing;rasterFrame.preview.motions=transition.outgoingMotions;}
      snapshotPressed=-1;
    }
    if(backPrepared&&backOffset>0&&history.size()>1) { const uint16_t previous=history[history.size()-2];rasterFrame.preview.page=&project->pages[previous];rasterFrame.preview.values=pages[previous];rasterFrame.preview.motions=pageMotions[previous];
      snapshotMotionRect=navigationViewport(page().path,rasterFrame.preview.page->path);snapshotTransition=true;snapshotPhase=TransitionPhase::Paired;
      transitionFrame={0,0,backOffset,0,255,false};
    }
    if(groupMotion.active&&!groupMotion.sharedBackdrop){
      snapshotMotionRect=navigationViewport(page().path,project->pages[groupMotion.destination].path);
      const int32_t offset=groupMotion.offset;
      const int32_t incoming=offset-groupMotion.sign*motionDistance();
      snapshotTransition=true;snapshotPhase=TransitionPhase::Paired;snapshotBackground=page().background;
      transitionFrame=groupMotion.vertical?TransitionSample{0,offset,0,incoming,255,true}:TransitionSample{offset,0,incoming,0,255,true};snapshotPressed=-1;
      if(groupMotion.prepared){rasterFrame.preview.page=&page();rasterFrame.preview.values=groupMotion.outgoing;rasterFrame.preview.motions=groupMotion.outgoingMotions;rasterFrame.current.page=&project->pages[groupMotion.destination];rasterFrame.current.values=pages[groupMotion.destination];rasterFrame.current.motions=pageMotions[groupMotion.destination];}
    }
    snapshotSurface=previewSurface=nullptr;snapshotAnimationSurface=nullptr;snapshotAnimationLayerCount=0;
    prepareSystemModalBaseFrame();
    snapshotTimer.stop();
    ProfileTimer framePrepareTimer(*this,&RuntimeStats::framePrepareMicros);
    directJpegCover=-1;directJpegTarget=nullptr;
    struct DirectJpegMarkerGuard {Impl &self;~DirectJpegMarkerGuard(){self.directJpegCover=-1;self.directJpegTarget=nullptr;}} directJpegMarkerGuard{*this};
    detail::JpegMediaCache::FramebufferPlan directJpegPlan;
    const int directJpegIndex=planDirectJpegFrame(directJpegPlan);
    const bool directJpegPlanned=directJpegIndex>=0;
    // Hydrate a retained terminal key before exact-match and animation-atlas
    // checks. The other page remains protected throughout a paired snapshot.
    // CPU RGB fades can merge exact cold run streams without decoding two
    // complete pages merely to read them back for this one transition.
    if(!systemModalBaseHasFrame()&&!directJpegPlanned&&!renderBudget.maxPixelsPerTick&&!compressedFadeKeysReady()){
      restoreSurfacePage(*rasterFrame.current.page,rasterFrame.preview.page);
      if(rasterFrame.preview.page)restoreSurfacePage(*rasterFrame.preview.page,rasterFrame.current.page);
    }
    const bool keyboardCovers=snapshotKeyboard.visible&&snapshotKeyboard.fullscreen&&!snapshotTransition&&!rasterFrame.preview.page;
    const bool plainFocus=(snapshotInput||styledFocus())&&!snapshotTransition&&!rasterFrame.preview.page&&snapshotFocus>=0;
    if(!systemModalBaseHasFrame()&&!directJpegPlanned&&!keyboardCovers&&snapshotPressed<0&&!plainFocus)snapshotSurface=findSurface(*rasterFrame.current.page,rasterFrame.current.values);
    if(rasterFrame.preview.page)previewSurface=findSurface(*rasterFrame.preview.page,rasterFrame.preview.values);
    const bool exactBeforePatch=snapshotSurface!=nullptr;
    // With an exact outgoing surface, prefetch beside its cheap first copy.
    // A stale first frame already pays for repair; defer its prefetch until
    // the following PPA fade step to keep both completion intervals bounded.
    if(!renderBudget.maxPixelsPerTick&&snapshotTransition&&snapshotPhase==TransitionPhase::Exit&&transition.config.effect==TransitionEffect::Fade&&
       transitionFrame.opacity>0&&(transitionFrame.opacity<255||exactBeforePatch)&&transition.destination>=0)
      restoreSurfacePage(project->pages[transition.destination],rasterFrame.current.page,rasterFrame.preview.page);
    if(!systemModalBaseHasFrame()&&!directJpegPlanned&&!snapshotSurface&&!renderBudget.maxPixelsPerTick)findAnimationSurface();
    // A direct JPEG plan has already sampled this immutable frame's geometry.
    bool geometryPrepared=directJpegPlanned,previewGeometryPrepared=false,transitionRepairAttempted=false;
    // Prefer a bounded retained repair during page motion. Native rendering
    // remains first for ordinary live maps, while changed route labels must
    // not force a static page to rerasterize at every slide sample.
    if(!systemModalBaseHasFrame()&&!directJpegPlanned&&!renderBudget.maxPixelsPerTick&&!keyboardCovers&&!snapshotSurface&&!snapshotAnimationSurface&&snapshotPressed<0&&!plainFocus){
      uint64_t repairBudget=0;
      // A claimed gesture freezes its key. A paired fade also retains both
      // complete source surfaces, so even a large incoming animation can repair
      // just its changed region instead of recapturing every static node.
      // Ordinary live frames keep the small cap; no animation clock is frozen.
      if((groupMotion.active&&groupMotion.prepared)||
         (snapshotTransition&&snapshotPhase==TransitionPhase::Paired&&transition.config.effect==TransitionEffect::Fade))
        repairBudget=uint64_t(rasterFrame.current.page->width)*rasterFrame.current.page->height;
      if(snapshotTransition&&snapshotPhase==TransitionPhase::Enter&&!rasterFrame.preview.page&&transitionFrame.opacity==255&&
         !snapshotMotionRect.x&&!snapshotMotionRect.y&&snapshotMotionRect.width==rasterFrame.current.page->width&&snapshotMotionRect.height==rasterFrame.current.page->height){
        const Rect visible=intersect({0,0,rasterFrame.current.page->width,rasterFrame.current.page->height},
          {transitionFrame.inX,transitionFrame.inY,rasterFrame.current.page->width,rasterFrame.current.page->height});
        if(visible.width<=0||visible.height<=0)repairBudget=std::max<uint64_t>(uint64_t(capacity)*4,uint64_t(rasterFrame.current.page->width)*rasterFrame.current.page->height/2);
      }
      // Always keep supported slide roots in their terminal pose in a retained
      // atlas. The larger first-entry budget is available only while the whole
      // page is outside the screen; visible/component-only repairs stay small.
      if(repairTerminalAnimationSurface(geometryPrepared,repairBudget))findAnimationSurface();
      if(snapshotTransition&&!snapshotAnimationSurface){
        snapshotSurface=patchSurface(geometryPrepared,repairBudget);transitionRepairAttempted=true;
        if(!snapshotSurface)snapshotSurface=captureGestureSurface(geometryPrepared);
      }
    }
    // Native/PPA composition can write before Refresh::start. Keep those
    // writes in the same whole-frame transaction as the eventual refresh;
    // only the scheduler may publish after every region has completed.
    auto *framePanel=nice_gfx::detail::DisplayAccess::panel(display);
    if(!framePanel)return Status::NotInitialized;
    if(directJpegPlanned)refresh->invalidateAll();
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
    DirtyRect frameDamage[16];size_t frameDamageCount=0;
    if(pipeline){
      if(!nativePipelineSnapshotEligible(directJpegPlanned))return Status::Unsupported;
      nativePipelineReservation=nativeStageSnapshotSerial;
      const Status reserved=refresh->reserveNativeDamage(nativePipelineReservation);
      if(reserved!=Status::Ok)return reserved;
      frameDamageCount=refresh->preparedNativeRegions(nativePipelineReservation);
      if(!frameDamageCount||frameDamageCount>16)return Status::InvalidArgument;
      for(size_t i=0;i<frameDamageCount;++i)
        if(!refresh->preparedNativeRegion(nativePipelineReservation,i,frameDamage[i]))return Status::InvalidArgument;
    }else{
      frameDamageCount=refresh->pendingRegions();
      if(frameDamageCount>16)return Status::InvalidArgument;
      for(size_t i=0;i<frameDamageCount;++i)if(!refresh->pendingRegion(i,frameDamage[i]))return Status::InvalidArgument;
    }
#else
    DirtyRect frameDamage[16];const size_t frameDamageCount=refresh->pendingRegions();
    if(frameDamageCount>16)return Status::InvalidArgument;
    for(size_t i=0;i<frameDamageCount;++i)if(!refresh->pendingRegion(i,frameDamage[i]))return Status::InvalidArgument;
#endif
    // P4's raw fade helpers can write fixed chrome/full mapped rows outside
    // pending dirty regions. Declare their whole possible target to the atomic
    // transport before the FIRST map. Ordinary native rendering remains bounded.
    // Keep frameDamage unchanged: refresh/seed accounting describes actual
    // renderer work, not this conservative driver inheritance declaration.
    bool rawNativeDamageFull=false;
#if defined(CONFIG_IDF_TARGET_ESP32P4)
    rawNativeDamageFull=(snapshotTransition&&transition.config.effect==TransitionEffect::Fade)||coldFadeSurface;
#endif
    const DirtyRect rawNativeFullDamage(0,0,rasterFrame.current.page->width,rasterFrame.current.page->height);
#if NICE_GFX_P4_MIPI_INHERIT_PREFETCH || NICE_GFX_UI_INCREMENTAL_FRAME_GEOMETRY || NICE_GFX_UI_RETAINED_CHART_INK
    // Snapshot is immutable. These helpers only build metadata/UTF8 metrics;
    // no native target, framebuffer read/write or SDK operation occurs here.
    if(!geometryPrepared&&!rawNativeDamageFull&&!directJpegPlanned&&!renderBudget.maxPixelsPerTick&&
       !snapshotTransition&&!rasterFrame.preview.page&&!snapshotSurface&&!snapshotAnimationSurface&&
       !snapshotKeyboard.visible&&!snapshotInput&&!circularKeyboard&&!buildingSurface&&!foregroundCapture&&
       !renderingViewport&&!groupMotion.active&&!groupMotion.prepared&&!coldFadeSurface&&!viewportOverlayLevels&&nativePrefixLimit<0){
      prepareFrameGeometry(*rasterFrame.current.page,rasterFrame.current.values,rasterFrame.current.geometry,-1,framePanel->supportsFrameBlit());
      geometryPrepared=true;++stats.earlyGeometryFrames;
    }
#endif
    // Plan every stripe and allocate owned memory before opening a frame.
#if NICE_GFX_UI_NATIVE_STAGE && NICE_GFX_UI_RETAINED_CHART_CPU_CONTEXT
    NativeStagePlan stagePlan;const bool stagePlanned=planNativeStage(*framePanel,frameDamage,frameDamageCount,stagePlan);
#else
    constexpr bool stagePlanned=false;
#endif
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
    Status frameStarted=Status::Unsupported;
    if(pipeline){
      nativePipelineLease=nice_gfx::NativeDrawingLease{};
      frameStarted=framePanel->beginNativeDrawing(frameDamage,frameDamageCount,true,nativePipelineLease);
      nativePipelineDrawingAcquired=frameStarted==Status::Ok;
      repaintDiscardFrame=frameStarted==Status::Ok;
    }else{
      frameStarted=stagePlanned?framePanel->beginRepaintFrame(frameDamage,frameDamageCount):Status::Unsupported;
#else
    Status frameStarted=stagePlanned?framePanel->beginRepaintFrame(frameDamage,frameDamageCount):Status::Unsupported;
#endif
#if NICE_GFX_UI_RETAINED_CHART_INK
    if(frameStarted==Status::Unsupported&&!rawNativeDamageFull&&!directJpegPlanned)frameStarted=retainedBegin(*framePanel,frameDamage,frameDamageCount);
#endif
#if NICE_GFX_FRAME_REPAINT_INHERIT
    // An immutable ordinary page snapshot will clear each supplied pixel
    // before any alpha/RMW painter. All other/native unknown paths preserve.
    if(frameStarted==Status::Unsupported&&!rawNativeDamageFull&&!directJpegPlanned&&!renderBudget.maxPixelsPerTick&&!snapshotTransition&&
       !rasterFrame.preview.page&&!snapshotSurface&&!snapshotAnimationSurface&&!snapshotKeyboard.visible&&
       !snapshotInput&&!circularKeyboard&&!buildingSurface&&!foregroundCapture&&!renderingViewport&&
       !groupMotion.active&&!groupMotion.prepared&&!coldFadeSurface&&!viewportOverlayLevels&&nativePrefixLimit<0&&frameDamageCount)
      frameStarted=framePanel->beginRepaintFrame(frameDamage,frameDamageCount);
    repaintDiscardFrame=frameStarted==Status::Ok;
#endif
    if(frameStarted==Status::Unsupported)frameStarted=framePanel->beginFrame(rawNativeDamageFull?&rawNativeFullDamage:frameDamage,
      rawNativeDamageFull?1:frameDamageCount);
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
    }
#endif
    if(frameStarted!=Status::Ok)return frameStarted;
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
    struct SnapshotFrameGuard {
      nice_gfx::Backend *panel;bool &discard;Impl &owner;bool pipeline;
      ~SnapshotFrameGuard(){if(panel){
        owner.nativeStageAbortedFrame();
        if(pipeline){++owner.nativePipelineStats.typedAborts;panel->abortNativeDrawing(owner.nativePipelineLease);}
        else panel->abortFrame();
        discard=false;
      }}
    } frameGuard{framePanel,repaintDiscardFrame,*this,pipeline};
#else
    struct SnapshotFrameGuard {
      nice_gfx::Backend *panel;bool &discard;Impl &owner;
      ~SnapshotFrameGuard(){if(panel){owner.nativeStageAbortedFrame();panel->abortFrame();discard=false;}}
    } frameGuard{framePanel,repaintDiscardFrame,*this};
#endif
#if NICE_GFX_UI_SYSTEM_MODAL_SOURCE_COMPOSITION
    if(!stagePlanned&&!retainedActive&&!directJpegPlanned
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
       &&!pipeline
#endif
       ){
      const Status composed=composeSystemModalSourceFrame(*framePanel,geometryPrepared);
      if(composed!=Status::Unsupported){
        if(composed!=Status::Ok)return composed;
        markSystemModalPanelColdFeedbackFrame();
        ++stats.frames;framePrepareTimer.stop();
        FrameSlice written;written.area={0,0,rasterFrame.current.page->width,rasterFrame.current.page->height};written.written=true;
        const Status started=refresh->start(render,this,PixelOrder::Wire,nullptr,&written,1);
        if(started==Status::Ok&&refresh->busy())frameGuard.panel=nullptr;
        return started;
      }
    }
#endif
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
    NativeRenderFrame nativeFrame;
    const Status nativeStatus=pipeline?prepareNativePipelineRender(nativeFrame):
      (renderBudget.maxPixelsPerTick?Status::Unsupported:prepareNativeRender(nativeFrame));
    if(pipeline&&nativeStatus!=Status::Ok)return nativeStatus;
#else
    NativeRenderFrame nativeFrame;const Status nativeStatus=renderBudget.maxPixelsPerTick?Status::Unsupported:prepareNativeRender(nativeFrame);
#endif
    if(nativeStatus!=Status::Ok&&nativeStatus!=Status::Unsupported)return nativeStatus;
    bool nativeReady=nativeStatus==Status::Ok;
    if(retainedActive&&(!nativeReady||!retainedBind(nativeFrame.frame)||framePanel->validateRetainedRepaintFrame(retainedReceipts,retainedCount)!=Status::Ok)){
      const auto aborted=framePanel->abortFrame();if(aborted!=Status::Ok)return aborted;retainedForgetFrame();
      const auto fallback=framePanel->beginRepaintFrame(frameDamage,frameDamageCount);repaintDiscardFrame=fallback==Status::Ok;
      if(fallback!=Status::Ok)return fallback;
      nativeFrame=NativeRenderFrame{};const auto remapped=prepareNativeRender(nativeFrame);
      if(remapped!=Status::Ok&&remapped!=Status::Unsupported)return remapped;nativeReady=remapped==Status::Ok;
    }
    if(!nativeReady)invalidateSystemModalPanelLive();
    repairSystemModalBaseFrame(nativeReady?&nativeFrame.frame:nullptr);
    ++stats.frames;
    if(nativeReady){
      if(!geometryPrepared){prepareFrameGeometry(*rasterFrame.current.page,rasterFrame.current.values,rasterFrame.current.geometry);geometryPrepared=true;}
      if(rasterFrame.preview.page&&!previewSurface){prepareFrameGeometry(*rasterFrame.preview.page,rasterFrame.preview.values,rasterFrame.preview.geometry);previewGeometryPrepared=true;}
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
      if(!pipeline)
#endif
        markSystemModalPanelColdFeedbackFrame();
      prepareSystemModalPanelFrame(&nativeFrame.frame);
      bool decodedToFrame=false;
      if(directJpegPlanned&&nativeFrame.frame.stride==nativeFrame.frame.width&&nativeFrame.count==1&&
         !nativeFrame.regions[0].x&&!nativeFrame.regions[0].y&&nativeFrame.regions[0].width==nativeFrame.frame.width&&nativeFrame.regions[0].height==nativeFrame.frame.height){
        const size_t bytes=nativeFrame.frame.stride*nativeFrame.frame.height*2;
        JpegDecodeTarget target{nativeFrame.frame.pixels,bytes,nativeFrame.frame.stride*2,uint16_t(nativeFrame.frame.width),uint16_t(nativeFrame.frame.height)};
        Status decoded=Status::Unsupported;
        detail::JpegPrefetch::Lease prefetched;
        if(jpegPrefetch.acquire(directJpegPlan,prefetched)){
          ProfileTimer copyTimer(*this,&RuntimeStats::jpegPrefetchCopyMicros);
          const PixelView pixels(prefetched.pixels,prefetched.width,prefetched.height,prefetched.strideBytes/2,PixelOrder::Native);
#if NICE_GFX_UI_JPEG_PREFETCH_COPY == 2
          if(prefetched.width==target.width&&prefetched.height==target.height&&prefetched.strideBytes==target.strideBytes&&prefetched.capacityBytes>=bytes)
            decoded=nice_gfx::platform::rgb565_native_copy::copy(nativeFrame.frame.pixels,prefetched.pixels,bytes/2).ok?Status::Ok:Status::Unsupported;
#elif NICE_GFX_UI_JPEG_PREFETCH_COPY == 1
          if(prefetched.width==target.width&&prefetched.height==target.height&&prefetched.strideBytes==target.strideBytes&&prefetched.capacityBytes>=bytes){
            memcpy(nativeFrame.frame.pixels,prefetched.pixels,bytes);decoded=Status::Ok;
          }
#elif NICE_GFX_UI_JPEG_PREFETCH_COPY == 3
          // This is the actual mapped display frame inside beginFrame, never
          // an offscreen cache repair. Reuse the transport's contiguous GDMA
          // copy and retain the source lease until any hinted async read ends.
          if(framePanel->canFrameBlit(pixels)){
            decoded=framePanel->flush(0,0,pixels,false);
            if(decoded==Status::Ok&&framePanel->busy())decoded=framePanel->finish();
            if(decoded!=Status::Ok&&decoded!=Status::Unsupported)framePanel->finish();
          }
          if(decoded==Status::Unsupported)decoded=ppaFade.copyIntoNative(nativeFrame.frame,0,0,pixels,prefetched.capacityBytes,0,0,prefetched.width,prefetched.height);
#else
          decoded=ppaFade.copyIntoNative(nativeFrame.frame,0,0,pixels,prefetched.capacityBytes,0,0,prefetched.width,prefetched.height);
#endif
          if(decoded==Status::Ok)++stats.jpegPrefetchBlits;
        }
        if(decoded==Status::Unsupported)decoded=jpegCache.decodeFramebuffer(directJpegPlan,target);
        if(decoded==Status::Ok&&plannedJpegMedia)requestJpegAhead(*plannedJpegMedia,plannedJpegFrame);
        if(decoded==Status::Ok){directJpegCover=directJpegIndex;directJpegTarget=nativeFrame.frame.pixels;decodedToFrame=true;}
        else if(decoded!=Status::Unsupported)return decoded;
      }
      if(!decodedToFrame&&!retainedActive)partitionNativeRegions(nativeFrame);
    }
    framePrepareTimer.stop();
    if(!renderBudget.maxPixelsPerTick&&!nativeReady&&rasterFrame.preview.page&&!previewSurface&&snapshotTransition&&snapshotPhase==TransitionPhase::Paired){
      // A dynamic image can be published just before navigation, before its
      // page surface receives a normal refresh. Repair against the captured
      // outgoing key once, using the same bounded policy as the incoming page.
      // start() has already rejected live DMA frames; both immutable snapshot
      // leases survive these swaps. Never substitute the old page's live state.
      const PageDesc *incomingPage=rasterFrame.current.page,*outgoingPage=rasterFrame.preview.page;
      FrameCache *savedPaint=paintCache;
      const int savedPressed=snapshotPressed,savedFocus=snapshotFocus,savedGroup=snapshotGroupIndex;
      rasterFrame.current.values.swap(rasterFrame.preview.values);rasterFrame.current.motions.swap(rasterFrame.preview.motions);std::swap(rasterFrame.current.geometry,rasterFrame.preview.geometry);
      rasterFrame.current.page=outgoingPage;rasterFrame.preview.page=nullptr;paintCache=&rasterFrame.current.geometry;
      snapshotPressed=snapshotFocus=snapshotGroupIndex=-1;
      const bool retainedMotion=(groupMotion.active&&groupMotion.prepared)||transition.config.effect==TransitionEffect::Fade;
      previewSurface=patchSurface(previewGeometryPrepared,retainedMotion?uint64_t(outgoingPage->width)*outgoingPage->height:0);
      if(!previewSurface)previewSurface=captureGestureSurface(previewGeometryPrepared);
      rasterFrame.current.values.swap(rasterFrame.preview.values);rasterFrame.current.motions.swap(rasterFrame.preview.motions);std::swap(rasterFrame.current.geometry,rasterFrame.preview.geometry);
      rasterFrame.current.page=incomingPage;rasterFrame.preview.page=outgoingPage;paintCache=savedPaint;
      snapshotPressed=savedPressed;snapshotFocus=savedFocus;snapshotGroupIndex=savedGroup;
    }
    if(!systemModalBaseHasFrame()&&!renderBudget.maxPixelsPerTick&&!nativeReady&&!transitionRepairAttempted&&!keyboardCovers&&!snapshotSurface&&!snapshotAnimationSurface&&snapshotPressed<0&&!plainFocus)snapshotSurface=patchSurface(geometryPrepared);
    if(!keyboardCovers&&!snapshotSurface&&!snapshotAnimationSurface&&!geometryPrepared)prepareFrameGeometry(*rasterFrame.current.page,rasterFrame.current.values,rasterFrame.current.geometry,
      snapshotKeyboard.visible&&!snapshotTransition&&!rasterFrame.preview.page?snapshotKeyboard.rect.y:-1);
    if(rasterFrame.preview.page&&!previewSurface&&!previewGeometryPrepared)prepareCache(*rasterFrame.preview.page,rasterFrame.preview.values,rasterFrame.preview.geometry);
    // Retained pixels exclude transient backdrop/frame finishers. Keep the
    // cached body, but pass active behaviors through render() to add them.
    bool direct=snapshotSurface&&behaviorVisuals.empty()&&!snapshotTransition&&!rasterFrame.preview.page&&!snapshotKeyboard.visible&&!snapshotInput&&
      (snapshotGroupIndex<0||!snapshotGroup.indicatorHeight);
    PixelView surface(direct?snapshotSurface->pixels:nullptr,rasterFrame.current.page->width,rasterFrame.current.page->height,0,cacheOrder);
    FrameSlice pieces[6];size_t count=0;
    const int32_t w=rasterFrame.current.page->width,h=rasterFrame.current.page->height;
    const Rect viewport=snapshotMotionRect;
    const auto &t=transitionFrame;
    const Status backendFade=!direct?prepareBackendFade(pieces,count):Status::Unsupported;
    if(backendFade!=Status::Ok&&backendFade!=Status::Unsupported)return backendFade;
    const bool preparedFade=backendFade==Status::Ok;
    if(!renderBudget.maxPixelsPerTick&&!direct&&!preparedFade){const Status cpuFade=prepareCpuCachedFade(surface);
      if(cpuFade==Status::Ok)direct=true;
      else if(cpuFade!=Status::Unsupported)return cpuFade;}
    // Preserve authored fade timing. Cached immutable pages permit one PPA
    // transaction for the whole frame. Refresh borrows scratch until complete;
    // the start() busy gate prevents reuse while a transfer still reads it.
    const bool pairedFade=cacheOrder==PixelOrder::Wire&&!preparedFade&&ppaBlendEnabled&&rgb565_ppa::Context::available()&&snapshotTransition&&
      snapshotPhase==TransitionPhase::Paired&&transition.config.effect==TransitionEffect::Fade&&
      snapshotSurface&&previewSurface&&rasterFrame.preview.page&&rasterFrame.preview.page->width==w&&rasterFrame.preview.page->height==h&&
      viewport.x==0&&viewport.y==0&&viewport.width==w&&viewport.height==h&&
      t.outX==0&&t.outY==0&&t.inX==0&&t.inY==0&&
      !snapshotKeyboard.visible&&!snapshotInput&&(snapshotGroupIndex<0||!snapshotGroup.indicatorHeight);
    const bool singleFade=cacheOrder==PixelOrder::Wire&&!preparedFade&&ppaBlendEnabled&&rgb565_ppa::Context::available()&&snapshotTransition&&!rasterFrame.preview.page&&snapshotSurface&&
      (snapshotPhase==TransitionPhase::Enter||snapshotPhase==TransitionPhase::Exit)&&transition.config.effect==TransitionEffect::Fade&&
      viewport.x==0&&viewport.y==0&&viewport.width==w&&viewport.height==h&&
      !t.outX&&!t.outY&&!t.inX&&!t.inY&&!snapshotKeyboard.visible&&!snapshotInput&&
      (snapshotGroupIndex<0||!snapshotGroup.indicatorHeight);
    if(!direct&&!preparedFade){const Status cachedFade=prepareCachedViewportFade(pieces,count);
      if(cachedFade!=Status::Ok&&cachedFade!=Status::Unsupported)return cachedFade;
      if(cachedFade==Status::Unsupported)prepareViewportFade(pieces,count);
    }
    if(!renderBudget.maxPixelsPerTick&&(pairedFade||singleFade)){
      auto *panel=nice_gfx::detail::DisplayAccess::panel(display);
      const PixelView incoming(snapshotSurface->pixels,w,h,0,PixelOrder::Wire);
      const PixelView outgoing(previewSurface?previewSurface->pixels:nullptr,w,h,0,PixelOrder::Wire);
      if(panel&&panel->supportsFrameBlit()&&panel->canFrameBlit(incoming)&&(!pairedFade||panel->canFrameBlit(outgoing))){
        if(!t.opacity&&singleFade){
          pieces[0].area=DirtyRect(0,0,w,h);pieces[0].solid=true;pieces[0].color=snapshotBackground;count=1;
        }
        else if(!t.opacity&&pairedFade){surface=outgoing;direct=true;}
        else if(t.opacity==255){surface=incoming;direct=true;}
        else if(panel->canFrameBlit(PixelView(snapshotSurface->pixels,w,h,0,PixelOrder::Native))) {
          RenderTimer timer(*this);
          const uint32_t started=profilingClock?profilingClock():0;
          nice_gfx::NativeFramebuffer frame{};
          const Status mapStatus=panel->mapNativeFramebuffer(frame);
          if(mapStatus!=Status::Ok&&mapStatus!=Status::Unsupported)return mapStatus;
          const bool mapped=mapStatus==Status::Ok;
          const uint16_t *blended=nullptr;
          if(mapped){
            const bool okay=singleFade?ppaFade.blendSolidIntoNative(frame.pixels,frame.stride,frame.bytes,
              snapshotSurface->pixels,w,w,h,snapshotBackground,t.opacity):
              ppaFade.blendSourcesIntoNative(frame.pixels,frame.stride,frame.bytes,snapshotSurface->pixels,w,
                previewSurface->pixels,w,w,h,t.opacity);
            if(okay)blended=frame.pixels;
          }else if(pairedFade)blended=ppaFade.blendSourcesToNative(snapshotSurface->pixels,w,previewSurface->pixels,w,w,h,t.opacity);
          if(blended){
            surface=PixelView(blended,w,h,mapped?frame.stride:0,PixelOrder::Native);direct=true;
            ++stats.ppaBlendCalls;stats.ppaBlendPixels+=uint64_t(w)*h;
            if(profilingClock)stats.ppaBlendMicros+=profilingClock()-started;
          }else if(mapped||pairedFade)++stats.ppaBlendFailures; // Exact original CPU tile fallback.
        }
      }
    }
    const bool horizontal=t.outY==0&&t.inY==0&&abs(t.outX-t.inX)==viewport.width&&abs(t.outX)<=viewport.width&&abs(t.inX)<=viewport.width;
    const bool vertical=t.outX==0&&t.inX==0&&abs(t.outY-t.inY)==viewport.height&&abs(t.outY)<=viewport.height&&abs(t.inY)<=viewport.height;
    if(snapshotTransition&&snapshotPhase==TransitionPhase::Paired&&snapshotSurface&&previewSurface&&t.opacity==255&&
       (horizontal||vertical)&&!snapshotKeyboard.visible&&!snapshotInput&&(snapshotGroupIndex<0||!snapshotGroup.indicatorHeight)){
      const Surface *fixedSurface=groupMotion.active&&groupMotion.prepared?previewSurface:snapshotSurface;
      const auto fixed=[&](Rect r){if(r.width<=0||r.height<=0)return;auto &p=pieces[count++];p.area=DirtyRect(r.x,r.y,r.width,r.height);
        p.pixels=PixelView(fixedSurface->pixels+size_t(r.y)*w+r.x,r.width,r.height,w,cacheOrder);};
      fixed({0,0,w,viewport.y});fixed({0,viewport.y+viewport.height,w,h-viewport.y-viewport.height});
      fixed({0,viewport.y,viewport.x,viewport.height});fixed({viewport.x+viewport.width,viewport.y,w-viewport.x-viewport.width,viewport.height});
      const auto add=[&](const Surface *s,int32_t x,int32_t y){
        const int32_t left=std::max<int32_t>(viewport.x,viewport.x+x),top=std::max<int32_t>(viewport.y,viewport.y+y);
        const int32_t right=std::min<int32_t>(viewport.x+viewport.width,viewport.x+x+viewport.width),bottom=std::min<int32_t>(viewport.y+viewport.height,viewport.y+y+viewport.height);
        if(right<=left||bottom<=top)return;
        auto &piece=pieces[count++];piece.area=DirtyRect(left,top,right-left,bottom-top);
        piece.pixels=PixelView(s->pixels+size_t(top-y)*w+size_t(left-x),size_t(right-left),size_t(bottom-top),size_t(w),cacheOrder);
      };
      add(previewSurface,t.outX,t.outY);add(snapshotSurface,t.inX,t.inY);
    }
    if(!renderBudget.maxPixelsPerTick&&!direct&&!count&&snapshotTransition&&!rasterFrame.preview.page&&
       snapshotSurface&&snapshotSurface->ready&&snapshotSurface->pixels&&snapshotSurface->page==rasterFrame.current.page&&
       (snapshotPhase==TransitionPhase::Enter||snapshotPhase==TransitionPhase::Exit)&&t.opacity==255&&
       (transition.config.effect==TransitionEffect::SlideUp||transition.config.effect==TransitionEffect::SlideDown)&&
       !t.outX&&!t.inX&&!viewport.x&&viewport.width==w&&viewport.y>=0&&viewport.height>0&&
       int64_t(viewport.y)+viewport.height<=h&&(viewport.y||viewport.height!=h)&&
       !snapshotKeyboard.visible&&!snapshotInput&&snapshotPressed<0&&!styledFocus()&&
       behaviorVisuals.empty()&&!snapshotMenuVisible&&!snapshotMarqueeId&&
       (snapshotGroupIndex<0||!snapshotGroup.indicatorHeight)&&
       sameSurfaceState(snapshotSurface->key,rasterFrame.current.values)){
      const int32_t y=snapshotPhase==TransitionPhase::Enter?t.inY:t.outY;
      if(y>=-viewport.height&&y<=viewport.height){
        // Fixed chrome must use the complete accepted key, including current
        // time/indicator text. A body-only warm key cannot supply these strips.
        const auto fixed=[&](int32_t top,int32_t height){if(height<=0)return;
          auto &piece=pieces[count++];piece.area=DirtyRect(0,top,w,height);
          piece.pixels=PixelView(snapshotSurface->pixels+size_t(top)*w,w,height,w,cacheOrder);
        };
        fixed(0,viewport.y);fixed(viewport.y+viewport.height,h-viewport.y-viewport.height);
        const Rect visible=intersect(viewport,{0,viewport.y+y,w,viewport.height});
        const auto solid=[&](int32_t top,int32_t height){if(height<=0)return;
          auto &piece=pieces[count++];piece.area=DirtyRect(0,top,w,height);
          piece.solid=true;piece.color=snapshotBackground;
        };
        if(visible.height<=0)solid(viewport.y,viewport.height);
        else{
          solid(viewport.y,visible.y-viewport.y);
          solid(visible.y+visible.height,viewport.y+viewport.height-visible.y-visible.height);
          auto &piece=pieces[count++];piece.area=DirtyRect(0,visible.y,w,visible.height);
          piece.pixels=PixelView(snapshotSurface->pixels+size_t(visible.y-y)*w,w,visible.height,w,cacheOrder);
        }
      }
    }
    if(snapshotTransition&&!rasterFrame.preview.page&&snapshotSurface&&
       (snapshotPhase==TransitionPhase::Enter||snapshotPhase==TransitionPhase::Exit)&&t.opacity==255&&
       transition.config.effect!=TransitionEffect::Fade&&
       viewport.x==0&&viewport.y==0&&viewport.width==w&&viewport.height==h&&
       !snapshotKeyboard.visible&&!snapshotInput&&(snapshotGroupIndex<0||!snapshotGroup.indicatorHeight)){
      const int32_t x=snapshotPhase==TransitionPhase::Enter?t.inX:t.outX;
      const int32_t y=snapshotPhase==TransitionPhase::Enter?t.inY:t.outY;
      const Rect visible=intersect({0,0,w,h},{x,y,w,h});
      const auto solid=[&](Rect r){if(r.width<=0||r.height<=0)return;auto &p=pieces[count++];
        p.area=DirtyRect(r.x,r.y,r.width,r.height);p.solid=true;p.color=snapshotBackground;};
      if(visible.width<=0||visible.height<=0)solid({0,0,w,h});
      else {
        solid({0,0,w,visible.y});solid({0,visible.y+visible.height,w,h-visible.y-visible.height});
        solid({0,visible.y,visible.x,visible.height});solid({visible.x+visible.width,visible.y,w-visible.x-visible.width,visible.height});
        auto &p=pieces[count++];p.area=DirtyRect(visible.x,visible.y,visible.width,visible.height);
        p.pixels=PixelView(snapshotSurface->pixels+size_t(visible.y-y)*w+size_t(visible.x-x),
          visible.width,visible.height,w,cacheOrder);
      }
    }
    if(!renderBudget.maxPixelsPerTick&&!direct&&!count){
      const Status coldFade=prepareColdFade(surface);
      if(coldFade==Status::Ok)direct=true;
      else if(coldFade!=Status::Unsupported)return coldFade;
    }
    // Translation-only component layers reuse a terminal page as an atlas.
    // Compose directly into a mapped native framebuffer when available. The
    // transport performs conversion and card copies with PPA on ESP32-S31.
    // Other displays can borrow an existing cache slot for one full-frame blit.
    if(!renderBudget.maxPixelsPerTick&&!direct&&!count&&snapshotAnimationSurface&&(!snapshotTransition||t.opacity==255)){
      auto *panel=nice_gfx::detail::DisplayAccess::panel(display);
      if(panel&&panel->supportsFrameBlit()){
        nice_gfx::NativeFramebuffer native{};Status composed;
        {RenderTimer timer(*this);composed=composeAnimationNative(*panel,native);}
        if(composed==Status::Ok){surface=PixelView(native.pixels,w,h,native.stride,PixelOrder::Native);direct=true;}
        else if(composed!=Status::Unsupported)return composed;
      }
      if(!direct&&panel&&panel->supportsFrameBlit())if(auto *frame=acquireFrameSurface(snapshotAnimationSurface,snapshotSurface,previewSurface)){
        const PixelView candidate(frame->pixels,w,h,0,cacheOrder);
        if(panel->canFrameBlit(candidate)){
          renderingFullFrameSurface=true;const Status composed=render(this,DirtyRect(0,0,w,h),frame->pixels,w);renderingFullFrameSurface=false;
          if(composed==Status::Ok){if(cacheOrder==PixelOrder::Native)for(size_t i=0;i<size_t(w)*h;++i)frame->pixels[i]=rgb565_blend::fromWire(frame->pixels[i]);surface=candidate;direct=true;}
        }
      }
    }
    if(!direct&&!count&&nativeReady){
#if NICE_GFX_UI_NATIVE_STAGE && NICE_GFX_UI_RETAINED_CHART_CPU_CONTEXT
      const Status painted=stagePlanned&&canPaintStagedNative(*framePanel,nativeFrame,stagePlan)?paintStagedNativeRender(*framePanel,nativeFrame,stagePlan):paintNativeRender(nativeFrame);if(painted!=Status::Ok)return painted;
#else
      const Status painted=paintNativeRender(nativeFrame);if(painted!=Status::Ok)return painted;
#endif
      const Status certified=retainedCertify(*framePanel);if(certified!=Status::Ok)return certified;
      surface=PixelView(nativeFrame.frame.pixels,w,h,nativeFrame.frame.stride,PixelOrder::Native);direct=true;
    }
    // A full rectangle seeds the page. Subsequent dirty tiles may update it
    // only with an exact preceding-screen-frame fence; unchanged pixels then
    // have a proved base. Any intervening input/transition/native frame breaks
    // this chain. Both paths publish only after complete LCD frame completion.
    if(!direct&&!count&&!nativeReady&&!snapshotTransition&&!rasterFrame.preview.page&&
       !snapshotSurface&&!snapshotAnimationSurface&&!snapshotKeyboard.visible&&!snapshotInput&&!styledFocus()&&!behaviorVisualMoving()&&snapshotPressed<0&&
       (snapshotGroupIndex<0||!snapshotGroup.indicatorHeight)&&!circularKeyboard){
      seedFullPage=frameDamageCount==1&&frameDamage[0].x==0&&frameDamage[0].y==0&&frameDamage[0].width==w&&frameDamage[0].height==h;
      for(auto &slot:surfaces)if(slot.ready&&slot.page==rasterFrame.current.page&&slot.presentedAtFrame==stats.completedFrames){if(systemModalBasePinned(slot)){++stats.systemModalBaseWriterPinSkips;continue;}seedSurface=&slot;break;}
      if(!seedSurface&&seedFullPage)seedSurface=acquireFrameSurface(nullptr,nullptr,nullptr);
      if(seedSurface){
        seedSurface->ready=false;seedSurface->paused=false;seedSurface->compressionAttempted=false;++seedSurface->pixelsRevision;
        seedSurface->page=rasterFrame.current.page;seedSurface->key=rasterFrame.current.values;seedSurface->geometry=rasterFrame.current.geometry;
        for(size_t i=0;i<frameDamageCount;++i)seedExpectedPixels+=frameDamage[i].pixels();
      }
    }
    const auto bindSource=[&](FrameSlice &slice){
      if(slice.written||slice.solid||!slice.pixels.valid())return false;
      for(const Surface *cached:{snapshotSurface,previewSurface})if(cached&&cached->pixels&&cached->page&&slice.pixels.order==cacheOrder&&slice.pixels.stride==cached->page->width){
        const uintptr_t base=reinterpret_cast<uintptr_t>(cached->pixels),address=reinterpret_cast<uintptr_t>(slice.pixels.data);
        const size_t pixels=size_t(cached->page->width)*cached->page->height;
        if(address>=base&&address-base<pixels*2&&!((address-base)&1)){
          const size_t offset=(address-base)/2,x=offset%cached->page->width,y=offset/cached->page->width;
          if(slice.pixels.width<=size_t(cached->page->width)-x&&slice.pixels.height<=size_t(cached->page->height)-y){slice.source=backendFrameSource(*cached);slice.sourceX=x;slice.sourceY=y;return true;}
        }
      }return false;
    };
    if(direct){FrameSlice retained;retained.area={0,0,w,h};retained.pixels=surface;if(bindSource(retained)){pieces[0]=retained;count=1;direct=false;}}
    else for(size_t i=0;i<count;++i)bindSource(pieces[i]);
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
    if(pipeline){
      if(!direct||count||surface.data!=reinterpret_cast<const uint8_t*>(nativeFrame.frame.pixels)||
         surface.order!=PixelOrder::Native)return Status::Unsupported;
      const Status prepared=refresh->prepareNativeSurface(nativePipelineReservation,nativePipelineLease,nativeFrame.frame);
      if(prepared!=Status::Ok)return prepared;
      nativePipelineReady={nativePipelineLease,nativePipelineReservation,nativeStageFrameAccepted};
      nativeStageFrameAccepted=false;
      ++nativePipelineStats.preparedFrames;++nativePipelineStats.cpuOnlyFrames;
      if(!stagePlanned)++nativePipelineStats.nonStageFrames;
      frameGuard.panel=nullptr;
      return Status::Ok;
    }
#endif
    const Status started=refresh->start(render,this,PixelOrder::Wire,direct?&surface:nullptr,count?pieces:nullptr,count);
    if(started!=Status::Ok)seedSurface=nullptr;
    if(started==Status::Ok&&refresh->busy())frameGuard.panel=nullptr;
    return started;
  }
  void releaseSnapshot() { if(!refresh->busy()){retainedForgetFrame();repaintDiscardFrame=false;releaseSystemModalPanelColdFeedbackFrame();rasterFrame.release();snapshotSurface=previewSurface=nullptr;releaseSystemModalBaseFrame();} }
  void completedFrame() {
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
    if(nativePipelinePending.snapshot){completeNativePipelineTicket();return;}
#endif
    nativeStageCompletedFrame();
    retainedCompleted();
    if(seedSurface){
      if(seedPixels==seedExpectedPixels){
        seedSurface->ready=true;seedSurface->row=rasterFrame.current.page->height;
        seedSurface->presentedAtFrame=stats.completedFrames+1;
        releaseSurfaceText(*seedSurface);if(seedFullPage)++surfaceStats.seededPages;else ++surfaceStats.mirroredFrames;surfaceStats.seededPixels+=seedPixels;
      }
      seedSurface=nullptr;seedPixels=seedExpectedPixels=0;
    }
    if(snapshotSurface&&!snapshotTransition&&!rasterFrame.preview.page&&!snapshotKeyboard.visible&&!snapshotInput&&snapshotPressed<0&&
       (snapshotGroupIndex<0||!snapshotGroup.indicatorHeight)&&!circularKeyboard)
      const_cast<Surface*>(snapshotSurface)->presentedAtFrame=stats.completedFrames+1;
    completeSystemModalPanelColdFeedbackFrame();
    ++stats.completedFrames;
    completeMotionFrame();
    if(snapshotTransitionEpoch&&snapshotTransitionEpoch==transitionEpoch&&!transitionFrame.active)completedTransitionEpoch=transitionEpoch;
  }
  void settleTransition() {
    if(!transition.active||transition.pending||transition.sample.active||refresh->busy()||completedTransitionEpoch!=transitionEpoch)return;
    if(transition.phase==TransitionPhase::Components){finishComponents();return;}
    if(transition.phase==TransitionPhase::Exit){commitDeparture();return;}
    transition.active=false;transition.from=nullptr;transition.outgoing.clear();
    // The terminal transition frame omits the transient ring. Restore it
    // once that frame is on the display, using only the focused dirty area.
    if(keyInput&&input)invalidate(focusNode);
  }
  #include "detail/MenuMotionRuntime.inc.h"
  #include "detail/BehaviorRuntime.inc.h"
  #include "detail/MenuAdvancedRuntime.inc.h"
};

#include "detail/Dynamic.inc.h"
#include "detail/BehaviorApi.inc.h"

View::View(NICE_GFX &d,size_t n):impl_(new Impl(d,n)) {}
View::View(NICE_GFX &d,uint16_t *a,uint16_t *b,size_t n):impl_(new Impl(d,a,b,n)) {}
View::~View(){
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
  if(impl_&&impl_->cancelNativePipelineWork()!=Status::Ok&&impl_->nativePipelineOwnsWork()){
    // An accepted uncertain display lease has no fabricated destructor fence.
    // Keep its owning Impl and all borrowed raster resources alive.
    return;
  }
#endif
  delete impl_;
}
bool View::configureResourceCache(size_t bytes,size_t reserve){
  auto &s=*impl_;if(s.project||s.refresh->busy())return false;
  s.resourceCacheBudget=bytes;s.resourceReserve=reserve;return true;
}
Status View::create(NICE_GFX &d,View *&output,size_t n) noexcept {
#if NICE_GFX_UI_MONOCHROME_ONLY
  if(!d.displayCapabilities().monochrome())return Status::Unsupported;
#endif
  if(n>std::vector<uint16_t>().max_size())return Status::InvalidArgument;
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  try {
#endif
    std::unique_ptr<Impl> prepared(new(std::nothrow) Impl(d,n));
    if(!prepared)return Status::NoMemory;
    View *created=new(std::nothrow) View(prepared.get());
    if(!created)return Status::NoMemory;
    prepared.release();output=created;return Status::Ok;
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  } catch(const std::bad_alloc &) {return Status::NoMemory;}
#endif
}
Status View::create(NICE_GFX &d,uint16_t *a,uint16_t *b,size_t n,View *&output) noexcept {
#if NICE_GFX_UI_MONOCHROME_ONLY
  if(!d.displayCapabilities().monochrome())return Status::Unsupported;
#endif
  if(!a||!n||a==b||n>std::vector<uint16_t>().max_size())return Status::InvalidArgument;
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  try {
#endif
    std::unique_ptr<Impl> prepared(new(std::nothrow) Impl(d,a,b,n));
    if(!prepared)return Status::NoMemory;
    View *created=new(std::nothrow) View(prepared.get());
    if(!created)return Status::NoMemory;
    prepared.release();output=created;return Status::Ok;
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  } catch(const std::bad_alloc &) {return Status::NoMemory;}
#endif
}
Status View::mount(const ProjectDesc &p) {
  Impl &s=*impl_;
#if NICE_GFX_UI_MONOCHROME_ONLY
  if(!s.display.displayCapabilities().monochrome())return Status::Unsupported;
#endif
  if(s.processing)return Status::Busy;
  if(p.version!=ResourceVersion||!p.pages||!p.count||p.initial>=p.count||!validTransition(p.transition))return Status::InvalidArgument;
  std::unique_ptr<Impl> prepared;
#if NICE_GFX_UI_FRAME_GEOMETRY_EARLY_RESERVE
  size_t maximumGeometryNodes=0;
#endif
  std::vector<std::vector<int>> cancellationPaths;
  std::vector<uint16_t> cancellationLookup;
  std::vector<Impl::ComponentLink> cancellationLinks;
  std::vector<int32_t> cancellationNodes;
  const PageDesc *cancellationPage=nullptr;
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  try {
#endif
  std::vector<const MonoFont *> validatedFonts;std::vector<Id> validatedIds;size_t chartSamples=0;
  for(uint16_t j=0;j<p.count;++j) {
    const PageDesc &page=p.pages[j];if(!page.path||!*page.path||!page.nodes||!page.count||page.width!=s.display.width()||page.height!=s.display.height()||
      (page.enterTransition&&!validTransition(*page.enterTransition))||(page.exitTransition&&!validTransition(*page.exitTransition)))return Status::InvalidArgument;
#if NICE_GFX_UI_FRAME_GEOMETRY_EARLY_RESERVE
    maximumGeometryNodes=std::max(maximumGeometryNodes,size_t(page.count));
#endif
    for(uint16_t k=0;k<j;++k)if(strcmp(p.pages[k].path,page.path)==0)return Status::InvalidArgument;
    for(uint16_t i=0;i<page.count;++i) {
      const NodeDesc &n=page.nodes[i];
      if(uint8_t(n.kind)>uint8_t(Kind::InputField)||!validShape(n))return Status::InvalidArgument;
      if(n.kind==Kind::InputField||(n.kind==Kind::Spinbox&&n.inputField)){const auto *d=n.inputField;if(!d||uint8_t(d->type)>uint8_t(InputFieldType::Tel)||uint8_t(d->enterAction)>uint8_t(InputEnterAction::Search)||uint8_t(d->keyboardTheme)>uint8_t(KeyboardTheme::Dark)||d->decimals>6||d->pinLength>32||d->maxBytes>512||d->maxLength>512||(d->hasMin&&d->hasMax&&d->min>d->max)||d->min<INT32_MIN||d->min>INT32_MAX||d->max<INT32_MIN||d->max>INT32_MAX||(n.kind==Kind::Spinbox&&d->type!=InputFieldType::Number))return Status::InvalidArgument;
        const char *initial=d->value?d->value:"";if(strlen(initial)>(d->maxBytes?d->maxBytes:256))return Status::InvalidArgument;int32_t initialNumber=0;if(*s.inputValidationError(initial,*d,false,initialNumber,n.kind==Kind::Spinbox?&n:nullptr))return Status::InvalidArgument;
        if(d->keyboardFont&&std::find(validatedFonts.begin(),validatedFonts.end(),d->keyboardFont)==validatedFonts.end()){if(!validFont(*d->keyboardFont))return Status::InvalidArgument;validatedFonts.push_back(d->keyboardFont);}}
      else if(n.inputField)return Status::InvalidArgument;
      if(!validInstrument(n))return Status::InvalidArgument;
      if(chartKind(n.kind)){chartSamples+=size_t(n.instrument->capacity)*n.instrument->seriesCount;if(chartSamples>4096)return Status::InvalidArgument;}
      if(!validAnimation(n.animation)||!validMotion(n.animation,n.motion))return Status::InvalidArgument;
      if(n.motion&&(n.motion->pivotX!=50||n.motion->pivotY!=50)&&n.kind!=Kind::Media&&n.kind!=Kind::Icon&&n.kind!=Kind::Shape)return Status::InvalidArgument;
      if(n.motion&&n.motion->count){for(uint8_t f=0;f<n.motion->count;++f)if(n.motion->frames[f].scale!=1024||n.motion->frames[f].rotation){
        if(n.kind!=Kind::Media&&n.kind!=Kind::Icon&&n.kind!=Kind::Shape)return Status::InvalidArgument;
        // A shape's transformed contour has no intermediate local bitmap.
        // Partial local reveal would require clipping both fills and strokes
        // before transformation, so keep this unsupported combination explicit.
        if(n.kind==Kind::Shape)for(uint8_t revealFrame=0;revealFrame<n.motion->count;++revealFrame)
          if(n.motion->frames[revealFrame].reveal!=1024)return Status::InvalidArgument;
        for(uint16_t child=uint16_t(i+1);child<page.count;++child)if(page.nodes[child].parent==i)return Status::InvalidArgument;
        break;
      }}
      if(n.exitAnimation&&!validExitAnimation(*n.exitAnimation))return Status::InvalidArgument;
      if(!n.id||(n.parent!=NoParent&&n.parent>=i)||n.rect.width<0||n.rect.height<0||n.rect.x<-32768||n.rect.y<-32768||int64_t(n.rect.x)+n.rect.width>32767||int64_t(n.rect.y)+n.rect.height>32767||n.style.fontSize>7)return Status::InvalidArgument;
      if(!validGradient(n.style.gradient,n.rect))return Status::InvalidArgument;
      if(hasText(n.kind)&&(!n.style.font||!n.style.fontSize))return Status::InvalidArgument;
      if(n.style.font&&std::find(validatedFonts.begin(),validatedFonts.end(),n.style.font)==validatedFonts.end()) {
        if(!validFont(*n.style.font))return Status::InvalidArgument;
        if(n.style.font->fallback) {
          const MonoFont *fallback=n.style.font->fallback;
          if(fallback==n.style.font||fallback->fallback||!validFont(*fallback))return Status::InvalidArgument;
        }
        validatedFonts.push_back(n.style.font);
      }
      if((n.kind==Kind::Range||n.kind==Kind::Progress)&&(n.maximum<=n.minimum||n.step<=0))return Status::InvalidArgument;
      if(n.kind==Kind::Media&&n.media){if(!validMedia(*n.media))return Status::InvalidArgument;if(mediaJpeg(*n.media)&&!s.jpegCache.available())return Status::Unsupported;}
      if(n.kind==Kind::Icon && !(n.flags&Hidden) && (!n.icon || !n.icon->bitmap || !n.icon->width || !n.icon->height || n.icon->width>128 || n.icon->height>128 || (n.icon->bitsPerPixel!=0&&n.icon->bitsPerPixel!=1&&n.icon->bitsPerPixel!=4) || n.icon->bytes!=(n.icon->bitsPerPixel==4?(n.icon->width+1)/2:(n.icon->width+7)/8)*n.icon->height))return Status::InvalidArgument;
      if(n.kind==Kind::Pager&&contentBox(n.rect,n.style).width<=0)return Status::InvalidArgument;
      if(n.pager.indicatorHeight) {
        if(n.kind!=Kind::Pager||n.pager.indicatorHeight<6||n.pager.dotSize>n.pager.indicatorHeight||n.pager.indicatorHeight>80||n.pager.dotSize<2||n.pager.dotSize>12||n.pager.dotGap<2||n.pager.dotGap>20)return Status::InvalidArgument;
        const Rect area=contentBox(n.rect,n.style);int count=0;
        for(uint16_t k=0;k<page.count;++k)if(page.nodes[k].parent==i)++count;
        if(!count||area.height<=n.pager.indicatorHeight||count*n.pager.dotSize+(count-1)*n.pager.dotGap>area.width)return Status::InvalidArgument;
      }
    }
    validatedIds.resize(page.count);for(uint16_t i=0;i<page.count;++i)validatedIds[i]=page.nodes[i].id;
    std::sort(validatedIds.begin(),validatedIds.end());if(std::adjacent_find(validatedIds.begin(),validatedIds.end())!=validatedIds.end())return Status::InvalidArgument;
  }
  // Borrow the existing tiles during preparation. Nothing here writes pixels
  // or submits a frame, so an old DMA transfer can safely keep borrowing them.
  prepared.reset(new(std::nothrow) Impl(s.display,s.tileBuffers[0],s.tileBuffers[1],s.capacity));
  if(!prepared)return Status::NoMemory;
  auto &next=*prepared;
  // Global Views can precede DMA initialization. Restore owned ping-pong tiles
  // once the transport becomes asynchronous; caller-owned single tiles stay single.
  if(s.first.size()&&!s.second.size()&&s.display.asynchronous()){
    OwnedUiTile second(s.capacity);next.second.swap(second);
    next.tileBuffers[1]=next.second.data();next.tileCount=2;
    std::unique_ptr<RefreshScheduler<16>> scheduler(new RefreshScheduler<16>(s.display,next.tileBuffers[0],next.tileBuffers[1],s.capacity));
    delete next.refresh;next.refresh=scheduler.release();
  }
  if(s.down&&s.target>=0&&!s.canceled) {
    size_t depth=0;for(int i=s.target;i>=0;i=s.parent(i))++depth;
    cancellationPaths.resize(1);cancellationPaths.front().reserve(depth);
    // PointerCancel resolves public component IDs. Prepare those indexes off
    // to the side as well, rather than allocating after the commit begins.
    if(s.componentCount) {
      next.project=s.project;next.history.assign(1,s.history.back());
      next.components=s.components;next.componentCount=s.componentCount;
      next.prepareComponents();cancellationPage=&s.page();
      cancellationLookup.swap(next.lookup);cancellationLinks.swap(next.componentLinks);
      cancellationNodes.swap(next.componentNodes);
      next.components=nullptr;next.componentCount=0;next.componentsPage=nullptr;next.lookupPage=nullptr;
    }
  }
  next.jpegCache.configure(s.jpegCache.handler(),s.jpegCache.context());
  next.resourceCacheBudget=s.resourceCacheBudget;next.resourceReserve=s.resourceReserve;
  next.attachResourceProject(p);next.pages.resize(p.count);next.pageMotions.resize(p.count);
  next.history.assign(1,p.initial);next.pageFocus.assign(p.count,0);
  next.initializeActive();next.enterAnimations();next.refresh->invalidateAll();
  if(!next.displayPolicy.slowRefresh&&next.page().enterTransition&&next.page().enterTransition->effect!=TransitionEffect::None) {
    next.transition.background=next.page().background;
    next.pagePhase(Impl::TransitionPhase::Enter,*next.page().enterTransition);
  }
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  } catch(const std::bad_alloc &) {return Status::NoMemory;}
#endif
  // All library-owned allocations completed. Keep application callback
  // exceptions outside the allocation boundary: they must not be mistaken for
  // a failed preparation after PointerCancel has already been delivered.
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
  const Status canceled=s.cancelNativePipelineWork();
  if(canceled!=Status::Ok)return canceled;
#endif
  if(!cancellationPaths.empty())s.eventPaths.swap(cancellationPaths);
  if(cancellationPage) {
    s.lookup.swap(cancellationLookup);s.lookupPage=cancellationPage;
    s.componentLinks.swap(cancellationLinks);s.componentNodes.swap(cancellationNodes);s.componentsPage=cancellationPage;
  }
#if !NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
  s.refresh->cancel();
#endif
  {
    struct ProcessingGuard {bool &flag;explicit ProcessingGuard(bool &v):flag(v){flag=true;}~ProcessingGuard(){flag=false;}} guard(s.processing);
    s.cancel();
  }
  auto &mounted=*prepared;
  mounted.listeners=s.listeners;mounted.guard=s.guard;mounted.guardContext=s.guardContext;
  mounted.edgeBack=s.edgeBack;mounted.swipeHome=s.swipeHome;
  mounted.circularKeyboard=s.circularKeyboard;mounted.refresh->configureCircularClip(s.circularKeyboard);
  mounted.keyboardThemeOverride=s.keyboardThemeOverride;
  mounted.profilingClock=s.profilingClock;mounted.refresh->setProfilingClock(s.profilingClock);
  mounted.stats=s.stats;mounted.inputEpoch=s.inputEpoch+1;mounted.inputLayoutEpoch=s.inputLayoutEpoch;
  mounted.renderBudget=s.renderBudget;
  mounted.ppaBlendEnabled=s.ppaBlendEnabled;
  mounted.refresh->configurePixelBudget(s.refresh->pixelBudget());
  mounted.refresh->configureMergeSlack(s.refresh->mergeSlack());
  // Transfer tile ownership only after accepted DMA has drained. The new
  // scheduler already points at these same buffers; no framebuffer is copied.
  mounted.first.swap(s.first);if(!mounted.second.size())mounted.second.swap(s.second);
  impl_=prepared.release();delete &s;
#if NICE_GFX_UI_FRAME_GEOMETRY_EARLY_RESERVE
  // Optional capacity after the old frame/stage owner has drained and released.
  // reserve preserves size and the cache object's valid/borrowed identity.
  (void)detail::reserveFrameGeometryNodes(impl_->rasterFrame.current.geometry.nodes,maximumGeometryNodes);
#endif
#if defined(NICE_GFX_UI_NATIVE_RESOURCE_PREWARM) && NICE_GFX_UI_NATIVE_RESOURCE_PREWARM
  // Compiled-default resource warming is independent of generated action,
  // shared-media and input configuration, which load() installs after mount.
  // Failure is best-effort and does not change a successfully mounted project.
  (void)impl_->warmNativeResources();
#endif
  return Status::Ok;
}
bool View::on(Id id,EventType type,EventHandler cb,void *context,bool capture) { if(!cb)return false;impl_->addListener({id,type,cb,context,capture});return true; }
bool View::onScoped(Id id,EventType type,EventHandler cb,void *context){if(!cb)return false;impl_->addListener({id,type,cb,context,false,false,true});return true;}
bool View::onUi(Id id,EventType type,EventHandler cb,void *context){
  if(!cb)return false;auto &s=*impl_;
  for(const auto &listener:*s.listeners)if(listener.uiAction&&listener.id==id&&listener.type==type&&listener.callback==cb&&listener.context==context)return true;
  s.addListener({id,type,cb,context,false,type==EventType::Click,true,true});return true;
}
bool View::onDefault(Id id,EventType type,EventHandler cb,void *context){
  if(!cb)return false;
  for(const auto &l:*impl_->listeners)if(l.id==id&&l.type==type&&l.callback==cb&&l.context==context&&l.defaultAction)return true;
  impl_->addListener({id,type,cb,context,false,true,true});return true;
}
bool View::configureInteraction(InteractionVersion version,const InteractionDesc *policies,size_t count){
  auto &s=*impl_;if(!s.project||s.processing||s.down||count>4096||(count&&!policies)||(version!=InteractionVersion::Legacy&&version!=InteractionVersion::Coordinated))return false;
  std::vector<InteractionDesc> prepared;if(count)prepared.assign(policies,policies+count);
  std::sort(prepared.begin(),prepared.end(),[](const InteractionDesc &a,const InteractionDesc &b){return a.id<b.id;});
  for(size_t i=0;i<count;++i){const auto &d=prepared[i];const auto &p=d.policy;
    if(!d.id||(i&&prepared[i-1].id==d.id)||uint8_t(p.axis)>3||uint8_t(p.ownership)>2||uint8_t(p.eventScope)>2)return false;
    bool exists=false;for(uint16_t page=0;page<s.project->count;++page)for(uint16_t n=0;n<s.project->pages[page].count;++n)if(s.project->pages[page].nodes[n].id==d.id)exists=true;
    if(!exists)return false;
  }
  s.interactionPolicies.swap(prepared);s.interactionVersion=version;return true;
}
InteractionVersion View::interactionVersion()const{return impl_->interactionVersion;}
bool View::setGesturePolicy(Id id,const GesturePolicy &policy){
  auto &s=*impl_;if(s.processing||s.down||s.find(id)<0||uint8_t(policy.axis)>3||uint8_t(policy.ownership)>2||uint8_t(policy.eventScope)>2)return false;
  const auto at=std::lower_bound(s.interactionPolicies.begin(),s.interactionPolicies.end(),id,[](const InteractionDesc &a,Id b){return a.id<b;});
  if(at!=s.interactionPolicies.end()&&at->id==id)at->policy=policy;else s.interactionPolicies.insert(at,{id,policy});return true;
}
bool View::claimPointer(Id id,GestureAxis axis){
  auto &s=*impl_;const int i=s.find(id);
  if(!s.coordinated()||!s.down||!s.pointerDispatch||s.drag!=Impl::Drag::None||i<0||axis==GestureAxis::None||uint8_t(axis)>3||!s.descendant(s.target,i)||!s.inInputLayer(i))return false;
  if(s.pointerClaim<0||s.descendant(i,s.pointerClaim)){s.pointerClaim=i;s.pointerClaimAxis=axis;}
  return true;
}
Id View::gestureOwner()const{
  const auto &s=*impl_;if(!s.down)return 0;if(s.owner>=0)return s.page().nodes[s.owner].id;
  if(s.gesture)return s.gesture->id;if(s.drag==Impl::Drag::Group)if(const auto *g=s.navigationGroup(s.page().path))return g->id;return 0;
}
bool View::acceptsEvent(const Event &event)const{return impl_->acceptsEvent(event);}
bool View::onOnce(Id id,EventType type,EventHandler cb,void *context,bool capture) {
  if(!cb)return false;
  const Impl::Listener key{id,type,cb,context,capture};
  const auto first=std::lower_bound(impl_->listeners->begin(),impl_->listeners->end(),key,Impl::listenerBefore);
  for(auto at=first;at!=impl_->listeners->end()&&at->id==id&&at->type==type&&at->capture==capture;++at)if(at->callback==cb&&at->context==context)return true;
  return on(id,type,cb,context,capture);
}
bool View::configureComponents(const ComponentDesc *config,size_t count) {
  auto &s=*impl_;if(!s.project||s.processing||s.refresh->busy()||count>65535||(!config&&count))return false;
  std::vector<size_t> ordered(count);std::vector<uint8_t> found(count,0);std::vector<uint16_t> nodes;std::vector<Id> overlays;
  for(size_t c=0;c<count;++c){ordered[c]=c;const auto &d=config[c];
    if(!d.id||uint8_t(d.kind)>uint8_t(ComponentKind::Confirm)||d.optionCount>12||(d.optionCount&&!d.options)||(d.flags&~ComponentViewportOverlay)||((d.flags&ComponentViewportOverlay)&&!d.expandedId))return false;
    if(d.flags&ComponentViewportOverlay)overlays.push_back(d.expandedId);
    if((d.kind==ComponentKind::Select||d.kind==ComponentKind::Choice)&&(!d.optionCount||d.valueId))return false;
    const Id slots[]={d.valueId,d.textId,d.pageId,d.expandedId};
    for(size_t n=0;n<4;++n)if(slots[n])for(size_t j=0;j<n;++j)if(slots[n]==slots[j])return false;
    if(d.valueFormat){const size_t length=strlen(d.valueFormat);const char *placeholder=strstr(d.valueFormat,"{value}");if(length>128||!placeholder||strstr(placeholder+7,"{value}")||!d.valueId||!d.textId)return false;}
    for(uint8_t n=0;n<d.optionCount;++n){if(!d.options[n].id||!d.options[n].label||strlen(d.options[n].label)>128)return false;for(Id slot:slots)if(slot&&slot==d.options[n].id)return false;for(uint8_t j=0;j<n;++j)if(d.options[n].id==d.options[j].id)return false;}
  }
  std::sort(ordered.begin(),ordered.end(),[&](size_t a,size_t b){return config[a].id<config[b].id;});
  for(size_t c=1;c<count;++c)if(config[ordered[c-1]].id==config[ordered[c]].id)return false;
  for(uint16_t p=0;p<s.project->count;++p){const auto &page=s.project->pages[p];nodes.resize(page.count);for(uint16_t n=0;n<page.count;++n)nodes[n]=n;
    std::sort(nodes.begin(),nodes.end(),[&](uint16_t a,uint16_t b){return page.nodes[a].id<page.nodes[b].id;});
    const auto find=[&](Id id)->int{if(!id)return -1;const auto at=std::lower_bound(nodes.begin(),nodes.end(),id,[&](uint16_t index,Id key){return page.nodes[index].id<key;});return at!=nodes.end()&&page.nodes[*at].id==id?int(*at):-1;};
    for(uint16_t root=0;root<page.count;++root){const Id id=page.nodes[root].id;const auto at=std::lower_bound(ordered.begin(),ordered.end(),id,[&](size_t index,Id key){return config[index].id<key;});if(at==ordered.end()||config[*at].id!=id)continue;
      const size_t c=*at;const auto &d=config[c];if(found[c]++)return false;
      if(page.nodes[root].kind!=Kind::Container&&page.nodes[root].kind!=Kind::Scroll)return false;
      const auto descendant=[&](int index){if(index<0||index==root)return false;for(uint16_t parent=page.nodes[index].parent;parent!=NoParent;parent=page.nodes[parent].parent)if(parent==root)return true;return false;};
      for(Id target:{d.valueId,d.textId,d.pageId,d.expandedId})if(target&&!descendant(find(target)))return false;
      if(d.textId&&!hasText(page.nodes[find(d.textId)].kind))return false;
      if(d.pageId&&page.nodes[find(d.pageId)].kind!=Kind::Pager)return false;
      if(d.expandedId&&page.nodes[find(d.expandedId)].kind!=Kind::Container&&page.nodes[find(d.expandedId)].kind!=Kind::Scroll)return false;
      if(d.valueId&&!control(page.nodes[find(d.valueId)].kind))return false;
      if(d.kind==ComponentKind::SettingRow&&(!d.valueId||(page.nodes[find(d.valueId)].kind!=Kind::Switch&&page.nodes[find(d.valueId)].kind!=Kind::Checkbox)))return false;
      if(d.kind==ComponentKind::ValueSlider&&(!d.valueId||page.nodes[find(d.valueId)].kind!=Kind::Range||!d.valueFormat))return false;
      if(d.kind==ComponentKind::Tabs&&!d.pageId)return false;
      if((d.kind==ComponentKind::Select||d.kind==ComponentKind::Popup||d.kind==ComponentKind::Confirm)&&!d.expandedId)return false;
      if(d.kind==ComponentKind::Select&&(!d.textId||page.nodes[find(d.textId)].kind!=Kind::Button))return false;
      for(uint8_t n=0;n<d.optionCount;++n){const int index=find(d.options[n].id);if(!descendant(index)||page.nodes[index].kind!=Kind::Button)return false;}
      if((d.kind==ComponentKind::Select||d.kind==ComponentKind::Choice)&&(page.nodes[root].value<0||page.nodes[root].value>=d.optionCount))return false;
    }
  }
  if(std::find(found.begin(),found.end(),uint8_t(0))!=found.end())return false;
  s.components=config;s.componentCount=count;s.componentsPage=nullptr;
  std::sort(overlays.begin(),overlays.end());overlays.erase(std::unique(overlays.begin(),overlays.end()),overlays.end());s.viewportOverlayIds.swap(overlays);
  s.updateOverlayLevels();
  s.clearFrameGeometryMemo();for(uint16_t p=0;p<s.project->count;++p)s.clearSurfacePage(&s.project->pages[p]);s.refresh->invalidateAll();
  for(auto &index:s.componentPaintIndices)index=detail::ComponentPaintIndex{};
  s.componentPaintUsed[0]=s.componentPaintUsed[1]=0;s.componentPaintClock=0;
  s.prepareComponents();
  for(uint16_t p=0;p<s.project->count;++p)for(size_t component=0;component<count;++component)
    s.revealComponentSelection(s.project->pages[p],s.pages[p],config[component]);
  return true;
}
bool View::setExpanded(Id id,bool expanded) {auto &s=*impl_;const auto *link=s.publicComponent(id);return link&&link->panel>=0&&(expanded?show(s.page().nodes[link->panel].id):hide(s.page().nodes[link->panel].id));}
bool View::expanded(Id id) const {auto &s=*impl_;const auto *link=s.publicComponent(id);return link&&link->panel>=0&&!(s.state()[link->panel].flags&Hidden);}
bool View::configureFocusStyles(const FocusStyleDesc *styles,size_t count) {
  auto &s=*impl_;if(s.processing||s.refresh->busy()||count>65535||(count&&!styles))return false;
  for(size_t i=0;i<count;++i){const auto &d=styles[i];
    if(!d.target||!d.owner||(d.flags&~uint16_t(1023))||(i&&(styles[i-1].target>d.target||(styles[i-1].target==d.target&&styles[i-1].owner>=d.owner))))return false;
  }
  s.invalidateSystemModalBaseLive();s.focusStyles=styles;s.focusStyleCount=count;s.surfaceWarmPending=!s.surfaces.empty();++s.surfaceMutationEpoch;s.refresh->invalidateAll();return true;
}
void View::configureInput(const InputConfig &config) {
  auto &s=*impl_;const bool nested=s.processing;s.processing=true;
  s.invalidateSystemModalBaseLive();
  s.input=&config;s.focusIndexConfig=nullptr;++s.inputLayoutEpoch;
  if(s.project){s.restoreFocus();s.refresh->invalidateAll();}
  if(!nested)s.endProcessing();
}
bool View::configureMenuMotion(const MenuMotionConfig &config) {
  auto &s=*impl_;if(s.processing||!validMenuMotion(config))return false;
  s.processing=true;
  s.baseMenuMotionConfig=config;s.baseMenuMotionConfigured=true;s.applyPageMenuMotion();s.resetMenuMotion();
  s.compactMenuRows();
  if(s.project){if(s.focusNode>=0){s.reveal(s.focusNode);s.aimMenuIndicator(true);}s.refresh->invalidateAll();}
  s.endProcessing();
  return true;
}
bool View::setMenuMotionPreset(MenuPreset preset) {
  if(uint8_t(preset)>uint8_t(MenuPreset::Reduced))return false;
  auto config=impl_->baseMenuMotionConfigured?impl_->baseMenuMotionConfig:impl_->menuMotionConfig;config.preset=preset;
  if(preset!=MenuPreset::Custom)config.mode=preset==MenuPreset::Reduced?MenuMotionMode::Instant:MenuMotionMode::Spring;
  config.camera=config.position=config.size=config.decoration=config.overlay=MenuDynamics{};
  if(impl_->processing){auto &s=*impl_;s.baseMenuMotionConfig=config;s.baseMenuMotionConfigured=true;s.applyPageMenuMotion();s.resetMenuMotion();s.compactMenuRows();if(s.project){if(s.focusNode>=0){s.reveal(s.focusNode);s.aimMenuIndicator(true);}s.refresh->invalidateAll();}return true;}
  return configureMenuMotion(config);
}
bool View::setLightMode(bool enabled) {
  auto &s=*impl_;
  if(s.monochromeLightMode!=enabled){s.monochromeLightMode=enabled;if(s.project)s.refresh->invalidateAll();}return true;
}
bool View::lightMode() const { return impl_->monochromeLightMode; }
bool View::setMenuIndicator(MenuIndicator indicator) {
  if(uint8_t(indicator)>uint8_t(MenuIndicator::Underline))return false;
  auto &s=*impl_;s.menuMotionConfig.indicator=indicator;if(s.project)s.refresh->invalidateAll();return true;
}
bool View::configureMonochromeEffects(MonochromeEffects effects) {
  auto &s=*impl_;
  if(s.processing||s.refresh->busy()||uint8_t(effects)>uint8_t(MonochromeEffects::Bayer4))return false;
  s.monochromeEffects=effects;
  if(s.project)s.refresh->invalidateAll();
  return true;
}
bool View::configureNavigation(const NavigationDesc *config){
  auto &s=*impl_;if(s.processing||(config&&!s.validNavigation(*config)))return false;
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
  if(!s.nativePipelineConfigurationReady())return false;
#endif
  s.processing=true;s.invalidateSystemModalBaseLive();s.cancel();s.navigation=config;s.refresh->invalidateAll();s.endProcessing();return true;
}
bool View::configureSystemChrome(const Id *bars,size_t barCount,const Id *panels,size_t panelCount){
  auto &s=*impl_;if(!s.project||s.processing||s.refresh->busy()||barCount>128||panelCount>128||(barCount&&!bars)||(panelCount&&!panels)||(panelCount&&panelCount!=barCount))return false;
  std::vector<Id> preparedBars,preparedPanels;
  std::vector<Impl::SystemChromePair> pairs;
  for(size_t n=0;n<panelCount;++n){int bar=-1,panel=-1,which=-1;for(uint16_t p=0;p<s.project->count;++p)for(uint16_t i=0;i<s.project->pages[p].count;++i){const auto id=s.project->pages[p].nodes[i].id;if(id==bars[n]){bar=i;which=p;}if(id==panels[n]){panel=i;if(which>=0&&which!=p)return false;which=p;}}
    if(bar<0||panel<0||which<0)return false;const auto &page=s.project->pages[which];if(page.nodes[bar].id!=bars[n]||page.nodes[panel].id!=panels[n])return false;
    int handle=panel;for(uint16_t i=uint16_t(panel+1);i<page.count;++i)if(page.nodes[i].parent==panel&&page.nodes[i].kind==Kind::Button)handle=i;
    pairs.push_back({uint16_t(which),uint16_t(bar),uint16_t(panel),uint16_t(handle)});
  }
  if(barCount)preparedBars.assign(bars,bars+barCount);if(panelCount)preparedPanels.assign(panels,panels+panelCount);
  for(auto *list:{&preparedBars,&preparedPanels}){std::sort(list->begin(),list->end());if(std::adjacent_find(list->begin(),list->end())!=list->end())return false;
    for(Id id:*list){if(!id)return false;bool found=false;for(uint16_t p=0;p<s.project->count;++p)for(uint16_t i=0;i<s.project->pages[p].count;++i)if(s.project->pages[p].nodes[i].id==id){if(found||s.project->pages[p].nodes[i].kind!=Kind::Container)return false;found=true;}if(!found)return false;}}
  for(Id id:preparedBars)if(std::binary_search(preparedPanels.begin(),preparedPanels.end(),id))return false;
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
  if(!s.nativePipelineConfigurationReady())return false;
#endif
  s.clearSystemModalPanelColdFeedback();s.systemBarRoots.swap(preparedBars);s.systemPanelRoots.swap(preparedPanels);s.systemChromePairs.swap(pairs);s.systemChromePanel=-1;s.updateOverlayLevels();s.clearFrameGeometryMemo();
  for(uint16_t p=0;p<s.project->count;++p)s.clearSurfacePage(&s.project->pages[p]);s.refresh->invalidateAll();return true;
}
bool View::configureSystemPanelLinks(const SystemPanelLink *links,size_t count){
  auto &s=*impl_;if(!s.project||s.processing||s.refresh->busy()||count>65535||(count&&!links))return false;
  std::vector<Impl::SystemPanelMember> prepared;prepared.reserve(count);
  for(size_t n=0;n<count;++n){const auto &link=links[n];if(link.page>=s.project->count||!link.clone||!link.source)return false;int node=-1,sourceNode=-1,sourcePage=-1;
    const auto &page=s.project->pages[link.page];for(uint16_t i=0;i<page.count;++i)if(page.nodes[i].id==link.clone){node=i;break;}
    for(uint16_t p=0;p<s.project->count;++p)for(uint16_t i=0;i<s.project->pages[p].count;++i)if(s.project->pages[p].nodes[i].id==link.source){if(sourcePage>=0)return false;sourcePage=p;sourceNode=i;}
    if(node<0||sourcePage<0||page.nodes[node].kind!=s.project->pages[sourcePage].nodes[sourceNode].kind)return false;
    for(const auto &old:prepared)if(old.page==link.page&&old.node==node)return false;
    prepared.push_back({link.page,uint16_t(node),uint16_t(sourcePage),uint16_t(sourceNode)});
  }
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
  if(!s.nativePipelineConfigurationReady())return false;
#endif
  s.systemPanelMembers.swap(prepared);return true;
}
bool View::configureRenderCache(uint16_t *storage,size_t pixels,size_t compressedPixels){return impl_->configureSurfaces(storage,pixels,compressedPixels);}
bool View::prewarmRenderCache(const char *path){return impl_->prewarmSurface(path);}
bool View::prewarmNativeResources(){return impl_->warmNativeResources();}
bool View::stepRenderCache(uint32_t now,size_t maxPixels){
  auto &s=*impl_;if(!s.project||!maxPixels||s.softInput.visible)return false;
  const size_t saved=s.renderBudget.maxPixelsPerTick;
  s.renderBudget.maxPixelsPerTick=std::min(maxPixels,saved?saved:s.capacity);
  const auto before=s.surfaceStats.warmedTiles;
  s.warmSurfaceTile(now,true);
  s.renderBudget.maxPixelsPerTick=saved;
  return s.surfaceStats.warmedTiles!=before;
}
bool View::configureCircularClip(bool enabled){
  auto &s=*impl_;if(s.processing||s.refresh->busy()||s.groupMotion.active||s.transition.active||s.backAnimation.active||s.animation.active||s.inertia.active||s.animationsRunning())return false;
  const bool result=s.refresh->configureCircularClip(enabled);if(result){s.releaseKeyboardPointer();s.circularKeyboard=enabled;s.refresh->invalidateAll();}return result;
}
bool View::renderCacheReady(const char *path)const{return impl_->project&&impl_->surfaceReady(impl_->routeIndex(path));}
RenderCacheStats View::renderCacheStats()const{
  const auto &s=*impl_;RenderCacheStats result=s.surfaceStats;result.readyPages=result.bodyReadyPages=result.chromePendingPages=0;result.warming=false;
  result.compressedReadyPages=result.compressedBodyReadyPages=result.compressedChromePendingPages=0;
  for(const auto &entry:s.compressedSurfaces)if(entry.page&&entry.words){
    const int index=int(entry.page-s.project->pages);
    const bool exact=s.compressedSurfaceReady(index),body=s.compressedSurfaceReady(index,true);
    if(body){++result.compressedBodyReadyPages;if(exact)++result.compressedReadyPages;else ++result.compressedChromePendingPages;}
  }
  if(s.compressedStorage){
    // A complete raw body may still await its bounded RLE count/encode pass.
    // Terminal attempts (including same-body reuse or rejected entropy) mark
    // compressionAttempted, so they cannot leave a permanent pending flag.
    if(s.compressedBuilder.surface)result.warming=true;
    else for(const auto &entry:s.surfaces)if(entry.ready&&!entry.compressionAttempted){result.warming=true;break;}
  }
  for(const auto &entry:s.surfaces)if(entry.page){const int index=int(entry.page-s.project->pages);
    const bool exact=s.surfaceReady(index),body=s.surfaceReady(index,true);
    if(body){
      // Idle replacement can retain one complete key beside a partial builder.
      // Count the route once rather than counting both of its raw pixel slots.
      bool counted=false;for(const auto &previous:s.surfaces){if(&previous==&entry)break;if(previous.page==entry.page){counted=true;break;}}
      if(!counted){++result.bodyReadyPages;if(exact)++result.readyPages;else ++result.chromePendingPages;}
    }else if(!entry.paused)result.warming=true;
  }
  if(result.bodyReadyPages<result.capacityPages&&s.surfaceWarmPending&&s.surfaceWanted.empty())result.warming=true;
  // A new request may replace a complete automatically selected cache before
  // any idle tick has assigned its slot. Report that queued work as warming.
  if(s.surfaceWarmPending)for(int index:s.surfacePrewarm)if(!s.surfaceReady(index,true)){
    bool paused=false;for(const auto &entry:s.surfaces)if(entry.page==&s.project->pages[index]){paused=entry.paused;break;}
    if(!paused){result.warming=true;break;}
  }
  return result;
}
bool View::hasKeyBinding(Id key,KeyGesture gesture) const { const auto *b=impl_->keyBinding(key,gesture);return b&&b->action!=InputAction::None; }
bool View::inputButton(Id key,KeyGesture gesture,uint32_t now) { auto &s=*impl_;if(!s.project||s.processing||gesture==KeyGesture::None)return false;s.setKeyInput(true);const auto *b=s.keyBinding(key,gesture);return b&&inputAction(b->action,b->target,b->route,now); }
bool View::inputAction(InputAction action,Id target,const char *route,uint32_t now) {
  auto &s=*impl_;if(!s.project||s.processing)return false;s.setKeyInput(true);
  if(s.transition.active||s.backAnimation.active||s.groupMotion.settling)return false;
  s.processing=true;s.cancel();s.animation.active=s.inertia.active=false;
  const bool result=s.keyAction(action,target,route,now);s.endProcessing();return result;
}
bool View::focus(Id id) { auto &s=*impl_;if(!s.project||s.processing)return false;s.processing=true;bool ok=s.setFocus(s.focusTarget(id));s.endProcessing();return ok; }
Id View::focused() const { return impl_->focusNode<0?0:impl_->page().nodes[impl_->focusNode].id; }
uint32_t View::inputGeneration() const { return impl_->inputEpoch; }
void View::setRouteGuard(RouteGuard cb,void *context){impl_->guard=cb;impl_->guardContext=context;}
bool View::push(const char *path){return impl_->navigate(path,RouteAction::Push);}
bool View::transitioning()const{return impl_->transition.active||impl_->groupMotion.settling;}
bool View::transitionFramePending()const{return impl_->transition.active&&impl_->transition.pending;}
bool View::cancelNavigation(){return !impl_->processing&&impl_->cancelDeparture();}
bool View::replace(const char *path){return impl_->navigate(path,RouteAction::Replace);}
bool View::replace(const char *path,const TransitionDesc &transition){return validTransition(transition)&&impl_->navigate(path,RouteAction::Replace,&transition);}
bool View::back(){return impl_->navigate(nullptr,RouteAction::Back);}
bool View::home(){return impl_->navigate(nullptr,RouteAction::Home);}
const char *View::path()const{return impl_->project?impl_->page().path:"";}
size_t View::historySize()const{return impl_->history.size();}
bool View::setText(Id id,const char *text){int i=impl_->textTarget(id);if(i<0)return false;auto &s=*impl_;if(s.page().nodes[i].kind==Kind::InputField)return s.setInputText(i,text);if(s.state()[i].text.set(text))s.invalidate(i);return true;}
bool View::setPageText(const char *path,Id id,const char *text){
  auto &s=*impl_;const int pageIndex=s.routeIndex(path);if(pageIndex<0)return false;const auto &page=s.project->pages[pageIndex];int node=-1;
  for(uint16_t i=0;i<page.count;++i)if(page.nodes[i].id==id){node=i;break;}
  if(node<0||(page.nodes[node].kind!=Kind::Text&&page.nodes[node].kind!=Kind::Label&&page.nodes[node].kind!=Kind::Button))return false;
  Impl::PageTextOverride *entry=nullptr;for(auto &item:s.pageTextOverrides)if(item.page==pageIndex&&item.node==node){entry=&item;break;}
  if(!entry){Impl::PageTextOverride item;item.page=uint16_t(pageIndex);item.node=uint16_t(node);item.text.init(page.nodes[node].text);s.pageTextOverrides.push_back(std::move(item));entry=&s.pageTextOverrides.back();}
  const bool changed=entry->text.set(text);if(!s.pages[pageIndex].empty()){const bool liveChanged=strcmp(s.pages[pageIndex][node].text.c_str(),entry->text.c_str())!=0;s.pages[pageIndex][node].text=entry->text;if(liveChanged&&pageIndex==s.history.back())s.invalidate(node);}
  if(changed&&pageIndex!=s.history.back()){s.surfaceWarmPending=!s.surfaces.empty();++s.surfaceMutationEpoch;}return true;
}
bool View::setPageProgress(const char *path,Id id,int32_t value){
  auto &s=*impl_;const int pageIndex=s.routeIndex(path);if(pageIndex<0)return false;const auto &page=s.project->pages[pageIndex];int node=-1;
  for(uint16_t i=0;i<page.count;++i)if(page.nodes[i].id==id){node=i;break;}
  if(node<0||page.nodes[node].kind!=Kind::Progress)return false;const auto &desc=page.nodes[node];
  value=clamp32(value,desc.minimum,desc.maximum);Impl::PageProgressOverride *entry=nullptr;for(auto &item:s.pageProgressOverrides)if(item.page==pageIndex&&item.node==node){entry=&item;break;}
  if(!entry){s.pageProgressOverrides.push_back({uint16_t(pageIndex),uint16_t(node),value});entry=&s.pageProgressOverrides.back();}else entry->value=value;
  if(!s.pages[pageIndex].empty()&&s.pages[pageIndex][node].value!=value){s.pages[pageIndex][node].value=value;if(pageIndex==s.history.back())s.invalidate(node);}
  if(pageIndex!=s.history.back()){s.surfaceWarmPending=!s.surfaces.empty();++s.surfaceMutationEpoch;}return true;
}
const char *View::text(Id id)const{int i=impl_->textTarget(id);return i<0?"":impl_->state()[i].text.c_str();}
bool View::beginTextInput(Id id){auto &s=*impl_;if(!s.project||s.processing||s.transition.active)return false;s.processing=true;s.cancel();const bool result=s.softBegin(s.find(id));s.endProcessing();return result;}
const char *View::inputDraft()const{return impl_->softInput.draft.c_str();}
size_t View::inputCursor()const{return impl_->softInput.cursor;}
bool View::inputText(const char *text){auto &s=*impl_;if(s.processing)return false;s.processing=true;const bool result=s.softInsert(text);s.endProcessing();return result;}
bool View::moveInputCursor(int32_t count){return !impl_->processing&&impl_->softMove(count);}
bool View::backspaceInput(){auto &s=*impl_;if(s.processing)return false;s.processing=true;const bool result=s.softBackspace();s.endProcessing();return result;}
bool View::deleteInput(){auto &s=*impl_;if(s.processing)return false;s.processing=true;const bool result=s.softDelete();s.endProcessing();return result;}
bool View::clearInput(){auto &s=*impl_;if(s.processing)return false;s.processing=true;const bool result=s.softClear();s.endProcessing();return result;}
bool View::keyboardKey(const char *key){auto &s=*impl_;if(s.processing||!key||!s.softInput.visible)return false;s.processing=true;bool result=false;
  s.buildKeyboard(s.softInput);if(s.softInput.keys)for(const auto &item:*s.softInput.keys)if(item.id==key||s.keyboardLabelMatches(s.softInput,item,key)||item.action==key){result=s.softPress(item);break;}s.endProcessing();return result;}
bool View::commitTextInput(bool next){auto &s=*impl_;if(s.processing)return false;s.processing=true;const bool result=s.softCommit(next);s.endProcessing();return result;}
bool View::cancelTextInput(){auto &s=*impl_;if(s.processing)return false;s.processing=true;const bool result=s.softCancel();s.endProcessing();return result;}
bool View::hideTextInput(){return !impl_->processing&&impl_->softHide();}
bool View::keyboardVisible()const{return impl_->softInput.visible;}
Id View::inputTarget()const{return impl_->softInput.node<0?0:impl_->page().nodes[impl_->softInput.node].id;}
const char *View::inputError()const{return impl_->softInput.error.c_str();}
Rect View::keyboardBounds()const{return impl_->softInput.visible?impl_->softInput.rect:Rect{0,0,0,0};}
Rect View::keyboardKeyBounds(const char *key)const{const auto &k=impl_->softInput;if(k.visible&&key&&k.keys)for(size_t i=0;i<k.keys->size();++i){const auto &item=(*k.keys)[i];if(item.id==key||impl_->keyboardLabelMatches(k,item,key))return k.picker&&i+2<k.keys->size()&&int(i)!=k.keyFocus?Rect{0,0,0,0}:item.rect;}return {0,0,0,0};}
void View::setKeyboardTheme(KeyboardTheme theme){auto &s=*impl_;if(uint8_t(theme)>uint8_t(KeyboardTheme::Dark))return;s.keyboardThemeOverride=int(theme);if(s.softInput.theme==theme)return;s.softInput.theme=theme;if(s.softInput.visible){const Rect r=s.softInput.rect;s.refresh->invalidate(DirtyRect(r.x,r.y,r.width,r.height));}}
void View::resetKeyboardTheme(){auto &s=*impl_;s.keyboardThemeOverride=-1;const KeyboardTheme theme=s.softInput.node<0?KeyboardTheme::Light:s.softInput.descriptor.keyboardTheme;if(s.softInput.theme==theme)return;s.softInput.theme=theme;if(s.softInput.visible){const Rect r=s.softInput.rect;s.refresh->invalidate(DirtyRect(r.x,r.y,r.width,r.height));}}
KeyboardTheme View::keyboardTheme()const{const auto &s=*impl_;return s.keyboardThemeOverride>=0?KeyboardTheme(s.keyboardThemeOverride):s.softInput.node>=0?s.softInput.theme:KeyboardTheme::Light;}
bool View::setValue(Id id,int32_t value,bool animateSwitch){auto &s=*impl_;const auto *link=s.publicComponent(id);if(link&&(link->desc->kind==ComponentKind::Select||link->desc->kind==ComponentKind::Choice))return s.selectOption(*link,value);int i=s.valueTarget(id);if(i>=0&&s.page().nodes[i].kind==Kind::Pager)return setPage(s.page().nodes[i].id,value);return s.setValue(i,value,animateSwitch);}
bool View::setChartValue(Id id,uint8_t series,uint16_t index,int32_t value){auto &s=*impl_;int i=s.find(id);if(i<0||!chartKind(s.page().nodes[i].kind))return false;const auto &a=*s.page().nodes[i].instrument;if(series>=a.seriesCount||index>=a.capacity)return false;const auto &old=*s.state()[i].instrument;if(index<old.count[series]&&old.value(a,series,index)==value)return true;bool ok=s.instrument(i).set(a,series,index,value);if(ok)s.invalidate(i);return ok;}
bool View::appendChartValue(Id id,uint8_t series,int32_t value){auto &s=*impl_;int i=s.find(id);if(i<0||!chartKind(s.page().nodes[i].kind))return false;bool ok=s.instrument(i).append(*s.page().nodes[i].instrument,series,value);if(ok)s.invalidate(i);return ok;}
bool View::setChartSeries(Id id,uint8_t series,const int32_t *values,size_t count){auto &s=*impl_;int i=s.find(id);if(i<0||!chartKind(s.page().nodes[i].kind))return false;const auto &a=*s.page().nodes[i].instrument;if(series>=a.seriesCount||count>a.capacity||(count&&!values))return false;const auto &old=*s.state()[i].instrument;bool same=old.count[series]==count;if(same)for(size_t j=0;j<count;++j)if(old.value(a,series,int(j))!=values[j]){same=false;break;}if(same)return true;auto &v=s.instrument(i);auto &data=v.writable();v.head[series]=0;v.count[series]=uint16_t(count);for(size_t j=0;j<a.capacity;++j)data[size_t(series)*a.capacity+j]=j<count?values[j]:ChartNone;s.invalidate(i);return true;}
int32_t View::chartValue(Id id,uint8_t series,uint16_t index)const{auto &s=*impl_;int i=s.find(id);return i<0||!chartKind(s.page().nodes[i].kind)?ChartNone:s.state()[i].instrument->value(*s.page().nodes[i].instrument,series,index);}
int32_t View::value(Id id)const{int i=impl_->valueTarget(id);return i<0?0:impl_->state()[i].value;}
Status View::setImage(Id id,const ImageView &source) {
  int index=-1;const Status target=impl_->imageTarget(id,index);if(target!=Status::Ok)return target;
  Image image;const Status result=Image::copyForView(source,image);if(result!=Status::Ok)return result;
  impl_->replaceImage(index,image,true);return Status::Ok;
}
Status View::setImage(Id id,const Image &image) {
  int index=-1;const Status target=impl_->imageTarget(id,index);if(target!=Status::Ok)return target;
  if(!image.valid())return Status::InvalidArgument;
  impl_->replaceImage(index,image,true);return Status::Ok;
}
bool View::hasImage(Id id,const Image &image) const {
  int index=-1;if(!image.impl_||impl_->imageTarget(id,index)!=Status::Ok)return false;
  const auto &value=impl_->state()[index];return value.imageOverride&&value.image.impl_==image.impl_;
}
Status View::setMediaResource(Id id,const MediaDesc &resource) {
  int index=-1;const Status target=impl_->imageTarget(id,index);if(target!=Status::Ok)return target;
  if(!validMedia(resource))return Status::InvalidArgument;
  if(mediaJpeg(resource)&&!impl_->jpegCache.available())return Status::Unsupported;
  const size_t bytes=mediaJpeg(resource)?detail::jpegResource(resource)->bytes:mediaFrameBytes(resource)*resource.frames;
  const ImageFormat format=mediaMasked(resource)?ImageFormat::Mono1Masked:mediaPacked(resource)?ImageFormat::Mono1:mediaPixelBytes(resource)==2?ImageFormat::RGB565:ImageFormat::RGB332;
  const ImageView view(mediaPixels(resource),resource.width,resource.height,bytes,format,PixelOrder::Wire,mediaStrideBytes(resource));
  auto *storage=new(std::nothrow) Image::Impl(view,nullptr,nullptr,0,true);if(!storage)return Status::NoMemory;
  storage->media=resource;Image image(storage);impl_->replaceImage(index,image,true);return Status::Ok;
}
bool View::configureJpegDecoder(JpegDecodeHandler handler,void *context){
  auto &s=*impl_;if(s.processing||s.refresh->busy())return false;s.jpegPaintLease.reset();s.jpegPrefetch.cancel();s.jpegCache.configure(handler,context);s.mediaDecodeStatus=Status::Ok;s.refresh->invalidateAll();return true;
}
JpegMediaStats View::jpegMediaStats()const{return impl_->jpegCache.stats();}
bool View::configureMediaPlayback(const MediaPlaybackDesc *policies,size_t count) {
  auto &s=*impl_;if(!s.project||s.processing||s.down||count>4096||(count&&!policies))return false;
  std::vector<MediaPlaybackDesc> prepared;if(count)prepared.assign(policies,policies+count);
  std::sort(prepared.begin(),prepared.end(),[](const MediaPlaybackDesc &a,const MediaPlaybackDesc &b){return a.id<b.id;});
  for(size_t at=0;at<prepared.size();++at){const auto &entry=prepared[at];
    if(!entry.id||(at&&prepared[at-1].id==entry.id))return false;
    bool found=false;
    for(uint16_t p=0;p<s.project->count;++p){const auto &page=s.project->pages[p];
      for(uint16_t i=0;i<page.count;++i)if(page.nodes[i].id==entry.id){
        found=true;if(!entry.scrubTarget)continue;
        bool target=false;for(uint16_t k=0;k<page.count;++k)if(page.nodes[k].id==entry.scrubTarget&&page.nodes[k].kind==Kind::Media){target=true;break;}
        if(!target)return false;
      }
    }
    if(!found)return false;
  }
  s.mediaPolicies.swap(prepared);s.mediaTweens.clear();
  for(uint16_t p=0;p<s.project->count;++p)if(!s.pages[p].empty())for(uint16_t i=0;i<s.project->pages[p].count;++i){
    const auto &node=s.project->pages[p].nodes[i];if(node.kind!=Kind::Media)continue;
    auto &v=s.pages[p][i];const uint16_t initial=s.initialMediaFrame(node,v);const bool changed=v.mediaFrame!=initial;
    v.mediaFrame=initial;v.mediaAt=0;v.mediaPending=true;v.mediaPaused=!s.mediaPlayback(node.id).autoplay;
    if(changed&&p==s.history.back())s.invalidate(i);
  }
  return true;
}
bool View::configureSharedMedia(const SharedMediaDesc *policies,size_t count) {
  auto &s=*impl_;if(!s.project||s.processing||s.down||count>4096||(count&&!policies))return false;
  std::vector<Impl::SharedMediaPolicy> prepared;prepared.reserve(count);
  for(size_t at=0;at<count;++at) {
    const auto &entry=policies[at];
    if(!entry.groupId||!entry.nodeId||entry.pageIndex>=s.project->count||entry.fps>60)return false;
    const auto &page=s.project->pages[entry.pageIndex];int node=-1;
    for(uint16_t i=0;i<page.count;++i)if(page.nodes[i].id==entry.nodeId){node=i;break;}
    if(node<0||page.nodes[node].kind!=Kind::Media||!page.nodes[node].media||entry.targetFrame>=page.nodes[node].media->frames)return false;
    for(const auto &previous:prepared) {
      if(previous.desc.pageIndex==entry.pageIndex&&(previous.desc.groupId==entry.groupId||previous.desc.nodeId==entry.nodeId))return false;
      if(previous.desc.groupId==entry.groupId&&s.project->pages[previous.desc.pageIndex].nodes[previous.node].media!=page.nodes[node].media)return false;
    }
    prepared.push_back({entry,uint16_t(node)});
  }
  std::sort(prepared.begin(),prepared.end(),[](const Impl::SharedMediaPolicy &a,const Impl::SharedMediaPolicy &b){return a.desc.groupId<b.desc.groupId||(a.desc.groupId==b.desc.groupId&&a.desc.pageIndex<b.desc.pageIndex);});
  s.sharedMediaPolicies.swap(prepared);s.sharedMediaCarry.clear();
  s.enterSharedMedia();
  return true;
}
bool View::setMediaFrame(Id id,int32_t frame) {
  auto &s=*impl_;int i=-1;return s.imageTarget(id,i)==Status::Ok&&s.seekMedia(i,frame);
}
bool View::playMediaTo(Id id,int32_t frame,uint32_t duration) {
  auto &s=*impl_;int i=-1;if(duration<1||duration>60000||s.imageTarget(id,i)!=Status::Ok)return false;
  return s.beginMediaTween(i,frame,duration);
}
bool View::playMediaToFps(Id id,int32_t frame,uint16_t fps) {
  auto &s=*impl_;int i=-1;if(fps<1||fps>60||s.imageTarget(id,i)!=Status::Ok)return false;
  const auto *m=s.media(s.page().nodes[i],s.state()[i]);if(!m||frame<0||frame>=m->frames)return false;
  const uint32_t from=s.state()[i].mediaFrame,distance=frame>=int32_t(from)?uint32_t(frame)-from:from-uint32_t(frame);
  if(!distance)return s.seekMedia(i,frame);
  const uint32_t duration=uint32_t((uint64_t(distance)*1000+fps-1)/fps);
  return s.beginMediaTween(i,frame,duration);
}
bool View::playMedia(Id id) {
  auto &s=*impl_;int i=-1;if(s.imageTarget(id,i)!=Status::Ok||!s.media(s.page().nodes[i],s.state()[i]))return false;
  if(s.displayPolicy.slowRefresh)return s.seekMedia(i,0);
  s.removeMediaTween(i);auto &v=s.state()[i];v.mediaPaused=false;v.mediaPending=true;return true;
}
bool View::pauseMedia(Id id) {
  auto &s=*impl_;int i=-1;if(s.imageTarget(id,i)!=Status::Ok||!s.media(s.page().nodes[i],s.state()[i]))return false;
  s.removeMediaTween(i);auto &v=s.state()[i];v.mediaPaused=true;v.mediaPending=true;return true;
}
int32_t View::mediaFrame(Id id)const {
  const Impl &s=*impl_;int i=-1;return s.imageTarget(id,i)==Status::Ok&&s.media(s.page().nodes[i],s.state()[i])?s.state()[i].mediaFrame:-1;
}
bool View::mediaPlaying(Id id)const {
  const Impl &s=*impl_;int i=-1;if(s.displayPolicy.slowRefresh||s.imageTarget(id,i)!=Status::Ok)return false;
  const auto *m=s.media(s.page().nodes[i],s.state()[i]);return m&&m->frames>1&&(!s.state()[i].mediaPaused||s.hasMediaTween(i));
}
Status View::clearImage(Id id) {
  int index=-1;const Status target=impl_->imageTarget(id,index);if(target!=Status::Ok)return target;
  impl_->replaceImage(index,Image(),true);return Status::Ok;
}
Status View::clearImage(const char *path,Id id) {
  auto &s=*impl_;if(!s.project)return Status::NotInitialized;
  const int pageIndex=s.routeIndex(path);if(pageIndex<0||!id)return Status::InvalidArgument;
  if(pageIndex==s.history.back())return clearImage(id);
  const auto &page=s.project->pages[pageIndex];int index=-1;
  for(uint16_t i=0;i<page.count;++i)if(page.nodes[i].id==id){index=i;break;}
  if(index<0||page.nodes[index].kind!=Kind::Media)return Status::InvalidArgument;
  if(s.pages[size_t(pageIndex)].empty())return Status::Ok;
  auto &value=s.pages[size_t(pageIndex)][size_t(index)];
  const bool changed=!value.imageOverride||value.image.valid();
  value.image=Image();value.imageOverride=true;
  value.mediaFrame=0;value.mediaAt=0;value.mediaPending=true;
  value.mediaPaused=!s.mediaPlayback(page.nodes[index].id).autoplay;
  s.mediaTweens.erase(std::remove_if(s.mediaTweens.begin(),s.mediaTweens.end(),
    [&](const Impl::MediaTween &t){return t.page==&page&&t.node==index;}),s.mediaTweens.end());
  // Preserve immutable outgoing/retained keys while a transition can read
  // them. Inactive cache warming or eviction releases those leases normally.
  if(changed){s.surfaceWarmPending=!s.surfaces.empty();++s.surfaceMutationEpoch;}
  return Status::Ok;
}
Status View::resetImage(Id id) {
  int index=-1;const Status target=impl_->imageTarget(id,index);if(target!=Status::Ok)return target;
  impl_->replaceImage(index,Image(),false);return Status::Ok;
}
bool View::clearPageImages(const char *path) {
  auto &s=*impl_;const int index=s.routeIndex(path);
  if(index<0||index==s.history.back())return false;
  bool changed=false;
  for(auto &value:s.pages[size_t(index)])if(value.imageOverride&&value.image.impl_&&!value.image.impl_->flash){
    value.image=Image();value.mediaFrame=0;value.mediaAt=0;value.mediaPending=true;changed=true;
  }
  // Do not invalidate the outgoing immutable key: paired fades may still
  // sample its retained pixels. Once the transition drains, inactive warming
  // compares this cleared state and releases that old key before its first
  // replacement tile. LRU eviction also releases the lease normally.
  if(changed){s.surfaceWarmPending=!s.surfaces.empty();++s.surfaceMutationEpoch;}
  return true;
}
bool View::setVisible(Id id,bool visible){
  auto &s=*impl_;const int i=s.find(id);if(i<0)return false;
  auto *m=s.liveMotion(i);
  if(bool(!(s.state()[i].flags&Hidden))==visible&&(!m||!m->hiding))return true;
  // Cancellation and focus callbacks may request navigation. Complete this
  // operation on its original page before committing that request.
  const bool nested=s.processing;s.processing=true;
  if(visible)s.cancelVisibilityMotions(i);
  else {if(m&&m->hiding)s.state()[i].animating=s.state()[i].animationPending=false;s.resetMotion(i,false);}
  s.commitVisibility(i,visible);
  if(!nested)s.endProcessing();return true;
}
bool View::show(Id id){auto &s=*impl_;const int i=s.find(id);if(i<0)return false;const bool nested=s.processing;s.processing=true;const bool result=s.requestShow(i);if(!nested)s.endProcessing();return result;}
bool View::hide(Id id){auto &s=*impl_;const int i=s.find(id);if(i<0)return false;const bool nested=s.processing;s.processing=true;const bool result=s.requestHide(i);if(!nested)s.endProcessing();return result;}
bool View::setEnabled(Id id,bool enabled){
  auto &s=*impl_;const int i=s.find(id);if(i<0)return false;
  if(bool(!(s.state()[i].flags&Disabled))==enabled)return true;
  const bool nested=s.processing;s.processing=true;++s.inputLayoutEpoch;
  if(enabled)s.state()[i].flags&=~Disabled;else s.state()[i].flags|=Disabled;s.invalidateSubtree(i);
  s.cancelUnavailableEditors();
  if(s.down&&!s.pointerAvailable())pointerCancel();s.ensureInputAvailable();
  if(s.buildKeyboard(s.softInput))s.invalidateKeyboardKeys();
  if(!nested)s.endProcessing();return true;
}
bool View::setFocusable(Id id,bool enabled){
  auto &s=*impl_;const int i=s.find(id);if(i<0)return false;const int8_t wanted=enabled?1:0;if(s.state()[i].focusOverride==wanted)return true;
  const bool nested=s.processing;s.processing=true;s.state()[i].focusOverride=wanted;++s.inputLayoutEpoch;s.invalidate(i);
  if(!enabled){
    for(auto &b:s.behaviors)if(b.page==s.history.back()&&b.open&&b.running&&b.desc.kind==BehaviorKind::Editor&&s.valueTarget(b.desc.target)==i){s.finishBehaviorState(b,false);if(b.closing)s.completeBehaviorClose(b);}
    if(s.editNode==i){s.editNode=-1;s.setValue(i,s.editValue);s.dispatch(i,EventType::Input);}
    if(s.softInput.node==i)s.softCancel();
    if(s.focusNode==i){s.focusScope=s.scopeFor(i);s.focusNode=-1;s.dispatch(i,EventType::Blur);if(s.focusNode<0)s.moveFocus(InputAction::Next);}
  }
  s.ensureInputAvailable();if(!nested)s.endProcessing();return true;
}
bool View::scrollTo(Id id,int32_t x,int32_t y){Impl &s=*impl_;int i=s.find(id);if(s.inertia.node==i)s.inertia.active=false;if(s.animation.node==i)s.animation.active=false;bool nested=s.processing;s.processing=true;bool r=s.scroll(i,x,y);if(!nested)s.endProcessing();return r;}
bool View::setPage(Id id,int32_t page){Impl &s=*impl_;int i=s.pageTarget(id);if(s.animation.node==i)s.animation.active=false;bool nested=s.processing;s.processing=true;bool r=s.setPage(i,page,true);if(!nested)s.endProcessing();return r;}
int32_t View::pageIndex(Id id)const{return value(id);}
void View::setEdgeBackEnabled(bool enabled){impl_->edgeBack=enabled;}
void View::setSwipeHomeEnabled(bool enabled){impl_->swipeHome=enabled;}
Rect View::bounds(Id id)const{int i=impl_->find(id);return i<0?Rect{0,0,0,0}:impl_->rect(i);}
Id View::hitTest(int32_t x,int32_t y)const{const auto &s=*impl_;if(!s.project||s.history.empty())return 0;const int i=s.hit(x,y);return i<0?0:s.page().nodes[i].id;}
bool View::releasePage(const char *path){auto &s=*impl_;if(!s.project||!path||s.processing)return false;
  for(uint16_t i=0;i<s.project->count;++i)if(!strcmp(s.project->pages[i].path,path)){
    if(std::find(s.history.begin(),s.history.end(),i)!=s.history.end()||s.transition.from==&s.project->pages[i]||(s.transition.active&&s.transition.destination==i)||(s.groupMotion.active&&s.groupMotion.destination==i)||(s.refresh->busy()&&(s.rasterFrame.current.page==&s.project->pages[i]||s.rasterFrame.preview.page==&s.project->pages[i])))return false;
    s.clearSurfacePage(&s.project->pages[i]);
#if NICE_GFX_UI_NATIVE_CACHE_REPAIR && NICE_GFX_UI_NATIVE_PREFIX_CACHE && NICE_GFX_UI_ROI_PREFIX_BOOTSTRAP
    s.clearNativeRoiPrefixes(&s.project->pages[i]);
#endif
#if NICE_GFX_UI_TEXT_LAYOUT_MEMO
    s.clearTextLayoutMemoPage(&s.project->pages[i]);
#endif
    RetainedVector<Impl::State>().swap(s.pages[i]);std::vector<Impl::MotionRuntime>().swap(s.pageMotions[i]);s.pageFocus[i]=0;return true;
  }return false;
}
nice_gfx::NativePresentationStats View::nativePresentationStats()const{
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
  const auto &s=*impl_;auto result=s.nativePipelineStats;
  result.pending=s.refresh->nativePresentationPending();result.ready=s.nativePipelineReady.snapshot!=0;
  result.fatal=s.nativePipelineFatal||s.nativeStageFailureStatus()!=Status::Ok;return result;
#else
  return {};
#endif
}
RuntimeStats View::runtimeStats()const{
  const auto &s=*impl_;RuntimeStats result=s.stats;const auto &overlay=s.ppaFade.stats();
  result.copyWireAttempts=overlay.wireCopy.attempts;
  result.copyWireCompleted=overlay.wireCopy.completed;
  result.copyWireRejected=overlay.wireCopy.rejected;
  result.copyWireErrors=overlay.wireCopy.errors;
  result.copyWireDriverCalls=overlay.wireCopy.driverCalls;
  result.copyWireDriverMicros=overlay.wireCopy.driverMicros;
  result.copyWirePixels=overlay.wireCopy.pixels;
  result.copyWireRejectedFootprint=overlay.wireCopy.rejectedFootprint;
  result.copyWireRejectedContext=overlay.wireCopy.rejectedContext;
  result.copyWireRejectedSourceCache=overlay.wireCopy.rejectedSourceCache;
  result.copyWireRejectedMemory=overlay.wireCopy.rejectedMemory;
  result.copyWireRejectedDmaAlignment=overlay.wireCopy.rejectedDmaAlignment;
  result.copyWireRejectedPlatform=overlay.wireCopy.rejectedPlatform;
  result.systemModalBaseLivePins=s.systemModalBaseLive.owner==s.frameSourceOwner&&s.systemModalBaseLive.slot<s.surfaces.size()?1:0;
  result.systemModalBaseFramePins=s.systemModalBaseFrame.owner==s.frameSourceOwner&&s.systemModalBaseFrame.slot<s.surfaces.size()?1:0;
#if NICE_GFX_UI_KEYED_PIE
  const auto &pie=s.keyedPieUi;result.uiPieEnabled=true;result.uiPieGateOpen=nice_gfx::platform::rgb565_p4_blend::verified();
  result.uiPieRows=pie.rows;result.uiPieEligibleBlocks=pie.eligibleBlocks;result.uiPieAcceptedBlocks=pie.acceptedBlocks;result.uiPieFallbackBlocks=pie.fallbackBlocks;
  result.uiPieKeyBlocks=pie.keyBlocks;result.uiPiePeelPixels=pie.peelPixels;result.uiPiePathMicros=pie.pathMicros;
#if NICE_GFX_UI_AXIS_MEDIA_PIE
  result.axisPieDraws=pie.axisDraws;result.axisPieRuns=pie.axisRuns;result.axisPieDirectBlocks=pie.axisDirectBlocks;result.axisPieScratchBlocks=pie.axisScratchBlocks;result.axisPieAcceptedBlocks=pie.axisAcceptedBlocks;result.axisPieFallbackBlocks=pie.axisFallbackBlocks;
#endif
  result.uiPiePixels=pie.safe.pixels;result.uiPieMaskedCycles=pie.safe.maskedCycles;result.uiPieChunks=pie.safe.chunks;
  result.uiPieMaximumMaskedCycles=pie.safe.maximumMaskedCycles;result.uiPieMaximumEnvelopeCycles=pie.safe.maximumEnvelopeCycles;
  result.uiPieRejectedContext=pie.safe.rejectedContext;result.uiPieRejectedArguments=pie.safe.rejectedArguments;result.uiPieRejectedUnverified=pie.safe.rejectedUnverified;
#endif
  for(unsigned i=0;i<5;++i)result.chartPhaseMicros[i]=s.canvas.chartPhaseMicros(i);
#if NICE_GFX_UI_AXIS_MEDIA_CACHE
  const auto &axis=s.motionAxisCache.statistics();result.axisMediaCacheHits=axis.hits;result.axisMediaCacheMisses=axis.misses;
  result.axisMediaCacheEvictions=axis.evictions;result.axisMediaCacheRejected=axis.rejected;result.axisMediaCacheBytes=axis.bytes;
  result.axisMediaRunBuilds=axis.runBuilds;result.axisMediaRunBuildRejected=axis.runBuildRejected;result.axisMediaRunPaints=axis.runPaints;result.axisMediaRunSegments=axis.runSegments;
  result.axisMediaRunOpaquePixels=axis.runOpaquePixels;result.axisMediaRunBlendPixels=axis.runBlendPixels;result.axisMediaRunSkippedPixels=axis.runSkippedPixels;result.axisMediaRunMetadataBytes=axis.runMetadataBytes;
  result.axisMediaFullFootprintPaints=axis.fullFootprintPaints;result.axisMediaFullFootprintFallbacks=axis.fullFootprintFallbacks;result.axisMediaFullFootprintSubsetPixels=axis.fullFootprintSubsetPixels;
#endif
  const auto prefetched=s.jpegPrefetch.stats();
  result.jpegPrefetchHits=prefetched.hits;result.jpegPrefetchMisses=prefetched.misses;result.jpegPrefetchRequests=prefetched.requests;
  result.jpegPrefetchDecoded=prefetched.completed;result.jpegPrefetchFailures=prefetched.failures;
  result.jpegPrefetchDecodeMicros=prefetched.decodeMicros;result.jpegPrefetchBytes=prefetched.outputBytes;
  result.controlBytes=sizeof(Impl);
#if NICE_GFX_UI_OUTLINED_MASK
  const auto foreground=s.motionOutlinedMask.foregroundLutStats();
  result.outlinedForegroundLutDraws=foreground.draws;result.outlinedForegroundLutBuilds=foreground.builds;result.outlinedForegroundLutReuses=foreground.reuses;
  result.outlinedForegroundLutSmallBypasses=foreground.smallBypasses;result.outlinedForegroundLutPixels=foreground.pixels;
  result.outlinedForegroundLutProbeWords=foreground.probeWords;result.outlinedForegroundLutBuildMicros=foreground.buildMicros;
  const auto outlined=s.motionOutlinedMask.geometryCacheStats();
  result.outlinedCacheHits=outlined.hits;result.outlinedCacheMisses=outlined.misses;result.outlinedCacheRejected=outlined.rejected;result.outlinedCacheBoundaryRejected=outlined.boundaryRejected;result.outlinedCacheEvictions=outlined.evictions;result.outlinedCacheAllocationFailures=outlined.allocationFailures;
  result.outlinedCacheColdReuses=outlined.coldReuses;result.outlinedCacheSmallRoiBypasses=outlined.smallRoiBypasses;
  result.outlinedCacheFullBuildMicros=outlined.fullBuildMicros;result.outlinedCacheRoiBuildMicros=outlined.roiBuildMicros;result.outlinedCacheExtractMicros=outlined.extractMicros;
  result.outlinedCacheFullBuildMaskBytes=outlined.fullBuildMaskBytes;result.outlinedCacheRequestedMaskBytes=outlined.requestedMaskBytes;result.outlinedCacheExtractedBytes=outlined.extractedBytes;
  result.outlinedCacheBytes=outlined.arenaBytes;result.outlinedCacheUsedBytes=outlined.usedBytes;result.outlinedCachePeakBytes=outlined.peakUsedBytes;result.outlinedCacheEntries=outlined.entries;
#endif
#if NICE_GFX_UI_FIXED_CHROME_PPA
  result.fixedChromeBytes=s.fixedChromeCache.bytes()+s.fixedChromeKeyScratch.capacity();
#endif
#if defined(CONFIG_IDF_TARGET_ESP32S31)
  result.controlInternal=esp_ptr_internal(impl_);
#endif
  result.overlayPreflightMicros=overlay.overlayPreflightMicros;result.overlayDriverMicros=overlay.overlayDriverMicros;
  result.overlaySyncMicros=overlay.overlaySyncMicros;result.overlayPreflightCalls=overlay.overlayPreflightCalls;
  result.overlaySyncCalls=overlay.overlaySyncCalls;result.overlaySyncedBytes=overlay.overlaySyncedBytes;
#if NICE_GFX_UI_SHAPE_OVERLAY_PPA || NICE_GFX_UI_SHAPE_NATIVE_SPANS || NICE_GFX_UI_SHAPE_WIRE_SPANS
  result.shapeTextureHits=s.shapeOverlayCache.hits()-s.shapeTextureHitsBase;
  result.shapeTextureMisses=s.shapeOverlayCache.misses()-s.shapeTextureMissesBase;
  result.shapeTextureEvictions=s.shapeOverlayCache.evictions()-s.shapeTextureEvictionsBase;
  result.shapeTextureBytes=s.shapeOverlayCache.bytes();
#endif
  return result;
}
void View::resetRuntimeStats(){auto &s=*impl_;s.stats=RuntimeStats{};s.ppaFade.resetOverlayStats();s.ppaFade.resetWireCopyStats();
#if NICE_GFX_UI_OUTLINED_MASK
  s.motionOutlinedMask.resetGeometryCacheStats();s.motionOutlinedMask.resetForegroundLutStats();
#endif

#if NICE_GFX_UI_KEYED_PIE
  s.keyedPieUi=Impl::KeyedPieUiStats{};
#endif
#if NICE_GFX_UI_SHAPE_OVERLAY_PPA || NICE_GFX_UI_SHAPE_NATIVE_SPANS || NICE_GFX_UI_SHAPE_WIRE_SPANS
  s.shapeTextureHitsBase=s.shapeOverlayCache.hits();s.shapeTextureMissesBase=s.shapeOverlayCache.misses();
  s.shapeTextureEvictionsBase=s.shapeOverlayCache.evictions();
#endif
}
void View::setProfilingClock(uint32_t (*clock)()){impl_->profilingClock=clock;impl_->refresh->setProfilingClock(clock);}
bool View::configureRenderBudget(const RenderBudget &budget){
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
  auto &s=*impl_;
  if(s.processing||!s.refresh->validPixelBudget(budget.maxPixelsPerTick)||!s.nativePipelineConfigurationReady())return false;
  if(!s.refresh->configurePixelBudget(budget.maxPixelsPerTick))return false;
#else
  auto &s=*impl_;if(s.processing||!s.refresh->configurePixelBudget(budget.maxPixelsPerTick))return false;
#endif
  s.renderBudget=budget;return true;
}
RenderBudget View::renderBudget()const{return impl_->renderBudget;}
bool View::configurePpaBlend(bool enabled){
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
  auto &s=*impl_;if(s.processing||!s.nativePipelineConfigurationReady()||s.refresh->busy())return false;
  s.ppaBlendEnabled=enabled;return true;
#else
  auto &s=*impl_;if(s.processing||s.refresh->busy())return false;s.ppaBlendEnabled=enabled;return true;
#endif
}
bool View::ppaBlendEnabled()const{return impl_->ppaBlendEnabled;}
bool View::configureMergeSlack(uint32_t pixels){
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
  auto &s=*impl_;return !s.processing&&s.nativePipelineConfigurationReady()&&s.refresh->configureMergeSlack(pixels);
#else
  return !impl_->processing&&impl_->refresh->configureMergeSlack(pixels);
#endif
}
uint32_t View::mergeSlack()const{return impl_->refresh->mergeSlack();}
#if NICE_GFX_UI_FRAME_GEOMETRY_EARLY_RESERVE
FrameGeometryStorageSnapshot View::frameGeometryStorage() const noexcept {
  FrameGeometryStorageSnapshot result;
  if(!impl_)return result;
  const auto observe=[](const auto &nodes){
    FrameGeometryStorageSnapshot::OwnedNodes value;
    value.address=reinterpret_cast<uintptr_t>(nodes.data());
    value.size=nodes.size();value.capacity=nodes.capacity();
    value.activeBytes=value.size*sizeof(detail::FrameGeometry);
    value.allocatedBytes=value.capacity*sizeof(detail::FrameGeometry);
    value.owned=value.capacity!=0;
    const auto domain=detail::frameGeometryMemoryDomain(nodes.data(),value.allocatedBytes);
    value.spanValid=domain.spanValid;value.internal=domain.internal;value.external=domain.external;
    return value;
  };
  result.current=observe(impl_->rasterFrame.current.geometry.nodes);
  result.preview=observe(impl_->rasterFrame.preview.geometry.nodes);
  result.memo=observe(impl_->geometryMemoCache.nodes);
#if NICE_GFX_UI_FRAME_GEOMETRY_IN_PLACE
  result.memoBorrowsCurrent=impl_->geometryMemoFrame==&impl_->rasterFrame.current.geometry;
#endif
  result.internalAllocatorAvailable=detail::FrameGeometryInternalAllocatorAvailable;
  result.earlyReserveAvailable=detail::FrameGeometryEarlyReserveAvailable;
  return result;
}
#endif
MemoryStats View::memoryStats()const{const auto &s=*impl_;MemoryStats m;
  // Object sizes already include inline string storage. Count only separately
  // stored character capacity; allocator/shared-ownership metadata is excluded.
  const auto stringStorage=[](const std::string &value)->size_t {
    const uintptr_t object=reinterpret_cast<uintptr_t>(&value),data=reinterpret_cast<uintptr_t>(value.c_str());
    return data>=object&&data-object<sizeof(value)?0:value.capacity()+1;
  };
  m.stateBytes=s.resourceCacheBytes()+(s.compiledStyles?s.compiledStyleLimit*sizeof(Style):0)+sizeof(Impl)+sizeof(*s.refresh)+s.pages.capacity()*sizeof(RetainedVector<Impl::State>)+s.history.capacity()*sizeof(uint16_t)+sizeof(std::vector<Impl::Listener>)+s.listeners->capacity()*sizeof(Impl::Listener)+s.pageFocus.capacity()*sizeof(Id)+stringStorage(s.pendingPath)+s.lookup.capacity()*sizeof(uint16_t)+s.eventPaths.capacity()*sizeof(std::vector<int>);
  for(const auto &path:s.eventPaths)m.stateBytes+=path.capacity()*sizeof(int);
  m.stateBytes+=s.focusIndex.capacity()*sizeof(size_t)+s.focusItems.capacity()*sizeof(int)+s.menuScrollMotions.capacity()*sizeof(Impl::MenuScrollMotion);
  m.stateBytes+=(s.animationNodes.capacity()+s.mediaNodes.capacity())*sizeof(uint16_t);
  m.stateBytes+=s.pageMotions.capacity()*sizeof(std::vector<Impl::MotionRuntime>);
  for(const auto &poses:s.pageMotions)m.stateBytes+=poses.capacity()*sizeof(Impl::MotionRuntime);
  m.snapshotBytes+=(s.rasterFrame.current.motions.capacity()+s.rasterFrame.preview.motions.capacity()+s.transition.outgoingMotions.capacity()+s.groupMotion.outgoingMotions.capacity())*sizeof(Impl::MotionRuntime);
  m.stateBytes+=s.interactionPolicies.capacity()*sizeof(InteractionDesc);
  m.stateBytes+=(s.systemBarRoots.capacity()+s.systemPanelRoots.capacity())*sizeof(Id)+s.systemChromePairs.capacity()*sizeof(Impl::SystemChromePair)+s.systemPanelMembers.capacity()*sizeof(Impl::SystemPanelMember)+s.pageTextOverrides.capacity()*sizeof(Impl::PageTextOverride)+s.pageProgressOverrides.capacity()*sizeof(Impl::PageProgressOverride);
  m.stateBytes+=s.mediaPolicies.capacity()*sizeof(MediaPlaybackDesc)+s.mediaTweens.capacity()*sizeof(Impl::MediaTween);
  m.stateBytes+=s.sharedMediaPolicies.capacity()*sizeof(Impl::SharedMediaPolicy)+s.sharedMediaCarry.capacity()*sizeof(Impl::SharedMediaCarry);
  m.stateBytes+=s.componentLinks.capacity()*sizeof(Impl::ComponentLink)+s.componentNodes.capacity()*sizeof(int32_t);
  std::vector<const void *> seen;
  for(const auto &entry:s.pageTextOverrides)if(entry.text.owned&&std::find(seen.begin(),seen.end(),entry.text.owned.get())==seen.end()){seen.push_back(entry.text.owned.get());m.payloadBytes+=sizeof(std::string)+stringStorage(*entry.text.owned);}
  auto payload=[&](const RetainedVector<Impl::State> &items){for(const auto &v:items){
    if(v.image.impl_&&std::find(seen.begin(),seen.end(),v.image.impl_)==seen.end()){seen.push_back(v.image.impl_);m.payloadBytes+=sizeof(Image::Impl)+v.image.storageBytes();}
    if(v.text.owned&&std::find(seen.begin(),seen.end(),v.text.owned.get())==seen.end()){seen.push_back(v.text.owned.get());m.payloadBytes+=sizeof(std::string)+stringStorage(*v.text.owned);}
    if(v.implicitInput&&std::find(seen.begin(),seen.end(),v.implicitInput.get())==seen.end()){seen.push_back(v.implicitInput.get());m.payloadBytes+=sizeof(InputFieldDesc);}
    if(v.instrument){if(std::find(seen.begin(),seen.end(),v.instrument.get())==seen.end()){seen.push_back(v.instrument.get());m.payloadBytes+=sizeof(InstrumentState);}
      const auto &data=v.instrument->data;if(data&&std::find(seen.begin(),seen.end(),data.get())==seen.end()){seen.push_back(data.get());m.payloadBytes+=sizeof(std::vector<int32_t>)+data->capacity()*sizeof(int32_t);}}
    if(v.dynamic&&std::find(seen.begin(),seen.end(),v.dynamic.get())==seen.end()){seen.push_back(v.dynamic.get());m.payloadBytes+=sizeof(DynamicNode);}
  }};
  for(const auto &p:s.pages){if(!p.empty())++m.residentPages;m.residentNodes+=p.size();m.stateBytes+=p.capacity()*sizeof(Impl::State);payload(p);}
  for(const auto *p:{&s.rasterFrame.current.values,&s.rasterFrame.preview.values,&s.transition.outgoing,&s.groupMotion.outgoing}){m.snapshotBytes+=p->capacity()*sizeof(Impl::State);payload(*p);}
  for(const auto &key:s.geometryMemoKey)if(key.dynamic&&std::find(seen.begin(),seen.end(),key.dynamic.get())==seen.end()){seen.push_back(key.dynamic.get());m.payloadBytes+=sizeof(DynamicNode);}
  m.stateBytes+=s.geometryMemoKey.capacity()*sizeof(Impl::FrameGeometryKey)+s.subtreeEndsCapacity*sizeof(uint16_t);
  for(const auto *c:{&s.rasterFrame.current.geometry,&s.rasterFrame.preview.geometry,&s.geometryMemoCache})m.geometryBytes+=c->nodes.capacity()*sizeof(Impl::Geometry)+c->lines.capacity()*sizeof(Impl::TextLine)+c->bands.capacity()*sizeof(uint32_t);
  // Both SoftInput objects themselves are already part of sizeof(Impl).
  if(s.suspendedSystemInput)m.stateBytes+=sizeof(Impl::SoftInput);
  for(const auto *k:{&s.softInput,&s.snapshotKeyboard,static_cast<const Impl::SoftInput*>(s.suspendedSystemInput.get())}){if(!k)continue;m.payloadBytes+=stringStorage(k->draft)+stringStorage(k->error)+stringStorage(k->pressed);if(k->keys&&std::find(seen.begin(),seen.end(),k->keys.get())==seen.end()){seen.push_back(k->keys.get());m.geometryBytes+=sizeof(std::vector<Impl::SoftKey>)+k->keys->capacity()*sizeof(Impl::SoftKey);for(const auto &key:*k->keys)m.payloadBytes+=stringStorage(key.id)+stringStorage(key.label)+stringStorage(key.value)+stringStorage(key.action);}}
  m.payloadBytes+=stringStorage(s.keyboardTextFrame.draft)+stringStorage(s.keyboardTextFrame.title);
  m.stateBytes+=s.surfaces.capacity()*sizeof(Impl::Surface)+(s.surfaceWanted.capacity()+s.surfacePrewarm.capacity())*sizeof(int);
  m.stateBytes+=s.systemModalBaseLiveKinds.capacity()+s.systemModalBaseFrameKinds.capacity();
  for(const auto &entry:s.surfaces){m.snapshotBytes+=entry.key.capacity()*sizeof(Impl::State);payload(entry.key);
    const auto &c=entry.geometry;m.geometryBytes+=c.nodes.capacity()*sizeof(Impl::Geometry)+c.lines.capacity()*sizeof(Impl::TextLine)+c.bands.capacity()*sizeof(uint32_t);
  }
  m.stateBytes+=s.compressedSurfaces.capacity()*sizeof(Impl::CompressedSurface);
  for(const auto &entry:s.compressedSurfaces){m.snapshotBytes+=entry.key.capacity()*sizeof(Impl::State);payload(entry.key);
    const auto &c=entry.geometry;m.geometryBytes+=c.nodes.capacity()*sizeof(Impl::Geometry)+c.lines.capacity()*sizeof(Impl::TextLine)+c.bands.capacity()*sizeof(uint32_t);
  }
  m.cacheBytes=s.surfacePixels*sizeof(uint16_t)+s.jpegPrefetch.stats().outputBytes;
#if NICE_GFX_UI_CONTAINER_PRIMITIVE_TEMPLATE
  // Inline template metadata is already counted by sizeof(Impl) in stateBytes.
  if(s.containerPrimitiveTemplate.pixels)m.cacheBytes+=Impl::ContainerPrimitiveTemplate::Bytes;
#endif
  for(const auto&buffer:s.retainedBuffers)for(const auto&chart:buffer.charts)if(chart.bits)m.retainedMaskBytes+=chart.bytes;
  m.cacheBytes+=m.retainedMaskBytes;
  if(auto*panel=nice_gfx::detail::DisplayAccess::panel(s.display))m.retainedProtocolBytes=panel->retainedRepaintStorageBytes();
#if NICE_GFX_UI_AXIS_MEDIA_CACHE
  m.cacheBytes+=s.motionAxisCache.statistics().bytes;
#endif
#if NICE_GFX_UI_FIXED_CHROME_PPA
  m.cacheBytes+=s.fixedChromeCache.bytes()+s.fixedChromeKeyScratch.capacity();
#endif
#if NICE_GFX_UI_KEYED_MEDIA_PPA
  m.cacheBytes+=s.keyedMediaPpaCache.bytes();
#endif
#if NICE_GFX_UI_SHAPE_OVERLAY_PPA || NICE_GFX_UI_SHAPE_NATIVE_SPANS || NICE_GFX_UI_SHAPE_WIRE_SPANS
  m.cacheBytes+=s.shapeOverlayCache.bytes();
#endif
#if NICE_GFX_UI_TEXT_MASK_PPA || NICE_GFX_UI_TEXT_MASK_NATIVE
  m.cacheBytes+=s.textMaskCache.bytes();
#endif
  m.nativeStageBytes=s.nativeStageAllocatedBytes();
  m.tileBytes=s.capacity*s.tileCount*sizeof(uint16_t);m.scratchBytes=(s.underlay.capacity()+s.transitionUnderlay.capacity()+s.mediaRow.capacity())*sizeof(uint16_t)+s.ppaFade.stats().scratchBytes+s.jpegCache.stats().outputBytes+s.jpegCache.stats().inputBytes;
#if NICE_GFX_UI_OUTLINED_MASK
  m.scratchBytes+=s.motionOutlinedMask.bytes();m.cacheBytes+=s.motionOutlinedMask.geometryCacheBytes();
#endif
#if NICE_GFX_UI_NATIVE_CACHE_REPAIR
  m.cacheBytes+=s.rgb565MediaCache.bytes();m.scratchBytes+=s.nativeCacheScratch.bytes;
#if NICE_GFX_UI_NATIVE_PREFIX_CACHE
  m.cacheBytes+=s.nativePrefixBytes();
#endif
#endif
#if defined(CONFIG_IDF_TARGET_ESP32S31) && NICE_GFX_S31_MOTION_TEXTURE_BUDGET > 0
  m.scratchBytes+=s.motionTextureCache.bytes();
#endif
#if NICE_GFX_UI_TEXT_LAYOUT_MEMO
  m.geometryBytes+=s.textLayoutMemoBytes();
#endif
  return m;
}

bool View::playAnimation(Id id){auto &s=*impl_;if(s.transition.active&&(s.transition.phase==Impl::TransitionPhase::Components||s.transition.phase==Impl::TransitionPhase::Exit))return false;return s.startAnimation(s.find(id),0,true);}
bool View::playAnimation(Id id,uint32_t now){auto &s=*impl_;if(s.transition.active&&(s.transition.phase==Impl::TransitionPhase::Components||s.transition.phase==Impl::TransitionPhase::Exit))return false;return s.startAnimation(s.find(id),now);}
bool View::stopAnimation(Id id){auto &s=*impl_;if(s.transition.active&&(s.transition.phase==Impl::TransitionPhase::Components||s.transition.phase==Impl::TransitionPhase::Exit))return false;int i=s.find(id);if(i<0)return false;s.invalidateSubtree(i);auto &v=s.state()[i];v.animating=v.animationPending=false;v.animationX=v.animationY=0;v.opacity=255;s.resetMotion(i);s.invalidateSubtree(i);return true;}
bool View::animationRunning(Id id)const{int i=impl_->find(id);return i>=0&&impl_->state()[i].animating&&impl_->visible(impl_->page(),impl_->state(),i);}
void View::pointerDown(int32_t x,int32_t y,uint32_t now){auto &s=*impl_;if(s.processing||(s.behaviorInputBlocked()&&!s.systemChromeCanInterruptClosing(x,y)))return;s.processing=true;const bool consumed=s.dismissInputToasts();s.endProcessing();if(consumed)return;s.setKeyInput(false);if(s.project&&s.softInput.visible&&!s.softInput.fullscreen)for(const auto &pair:s.systemChromePairs)if(pair.page==s.history.back()&&s.nodeContains(pair.bar,x,y)&&s.inInputLayer(pair.bar)){s.softHide();break;}if(s.softInput.visible){
  s.processing=true;s.releaseKeyboardPointer();s.cancel();++s.inputEpoch;if(s.buildKeyboard(s.softInput))s.invalidateKeyboardKeys();s.keyboardPointer=true;s.keyboardTrackpad=false;s.keyboardSpacePointer=false;s.keyboardPickerPointer=false;s.keyboardPickerVertical=false;s.keyboardFieldPointer=contains(s.softInput.fieldRect,x,y);s.keyboardPointerX=x;s.keyboardPointerY=y;
  s.invalidateKeyboardKey(s.softInput.keyFocus);const int i=s.keyboardFieldPointer?-1:s.keyboardHit(s.softInput,x,y);s.keyboardPressed=i<0?"":(*s.softInput.keys)[i].id;if(i>=0){const bool choice=!s.softInput.picker||i+2<int(s.softInput.keys->size());if(choice)s.softInput.keyFocus=i;else s.invalidateKeyboardKey(i);s.keyboardPickerPointer=s.softInput.picker&&choice;s.keyboardSpacePointer=s.keyboardPressed=="space";}
  if(s.keyboardFieldPointer)s.softPlaceCursor(x);s.keyboardPointerCursor=s.softInput.cursor;s.keyboardPointerCell=s.keyboardCursorCell();s.invalidateKeyboardKey(s.softInput.keyFocus);if(s.keyboardPressed=="backspace")s.beginKeyboardBackspace(now);s.endProcessing();return;
  }s.keyboardPointer=false;s.keyboardFieldPointer=s.keyboardSpacePointer=s.keyboardTrackpad=s.keyboardPickerPointer=false;if(!s.backAnimation.active&&(!s.transition.active||s.sharedNavigationHit(x,y)>=0)&&!s.groupMotion.settling)s.pointerDown(x,y,now);}
void View::pointerMove(int32_t x,int32_t y,uint32_t now){auto &s=*impl_;if(s.processing)return;if(s.keyboardPointer){
  if(!s.softInput.visible)return;const int32_t dx=x-s.keyboardPointerX,dy=y-s.keyboardPointerY;
  if(s.keyboardPickerPointer&&!s.keyboardSpacePointer){
    if(!s.keyboardTrackpad&&std::max(abs(dx),abs(dy))>=8){s.keyboardTrackpad=true;s.keyboardPickerVertical=abs(dy)>abs(dx);}
    if(s.keyboardTrackpad){s.invalidateKeyboardPressed();s.keyboardPressed.clear();s.keyboardBackspaceRepeat=s.keyboardBackspacePaused=false;const int32_t steps=(s.keyboardPickerVertical?dy:dx)/24;
      if(steps){s.softPickerStep(steps);if(s.keyboardPickerVertical)s.keyboardPointerY+=steps*24;else s.keyboardPointerX+=steps*24;}s.invalidateKeyboardKey(s.softInput.keyFocus);return;}
  }
  if((s.keyboardFieldPointer||s.keyboardSpacePointer)&&!s.keyboardTrackpad&&abs(dx)>=8&&abs(dx)>=abs(dy)){s.keyboardTrackpad=true;s.keyboardPressed.clear();s.invalidateKeyboardKey(s.softInput.keyFocus);}
  if(s.keyboardTrackpad){s.softPointerCursor(dx);return;}if(s.keyboardFieldPointer)return;
  const int i=s.keyboardHit(s.softInput,x,y);if(!s.keyboardPressed.empty()&&(i<0||(*s.softInput.keys)[i].id!=s.keyboardPressed)){s.keyboardBackspaceRepeat=false;s.invalidateKeyboardPressed();s.keyboardPressed.clear();s.keyboardSpacePointer=false;s.invalidateKeyboardKey(s.softInput.keyFocus);}return;
  }if(!s.backAnimation.active&&(!s.transition.active||(s.sharedComponentDeparture()&&s.down))&&!s.groupMotion.settling)s.pointerMove(x,y,now);}
void View::pointerUp(int32_t x,int32_t y,uint32_t now){auto &s=*impl_;if(s.processing)return;if(s.keyboardPointer){
  // A final release coordinate can activate the trackpad even without moves.
  pointerMove(x,y,now);s.processing=true;s.keyboardPointer=s.keyboardBackspaceRepeat=s.keyboardBackspacePaused=false;const bool trackpad=s.keyboardTrackpad,field=s.keyboardFieldPointer;s.keyboardFieldPointer=s.keyboardSpacePointer=s.keyboardTrackpad=s.keyboardPickerPointer=false;const int i=s.softInput.visible?s.keyboardHit(s.softInput,x,y):-1;const std::string pressed=s.keyboardPressed;s.invalidateKeyboardPressed();s.keyboardPressed.clear();s.invalidateKeyboardKey(s.softInput.keyFocus);
  if(s.softInput.visible&&!trackpad&&!field&&i>=0&&(*s.softInput.keys)[i].id==pressed&&pressed!="backspace")s.softPress((*s.softInput.keys)[i]);s.endProcessing();return;
  }if(!s.backAnimation.active&&(!s.transition.active||(s.sharedComponentDeparture()&&s.down))&&!s.groupMotion.settling)s.pointerUp(x,y,now);}
void View::pointerCancel(){Impl &s=*impl_;bool nested=s.processing;s.processing=true;s.releaseKeyboardPointer();s.cancel(true);if(s.backAnimation.active){s.backAnimation.active=false;s.backOffset=0;s.backPrepared=false;s.refresh->invalidateAll();s.routeEvent(EventType::NavigationCancel,s.page().path,s.project->pages[s.history[s.history.size()-2]].path,RouteAction::Back);}if(!nested)s.endProcessing();}
void View::stopPointerRepeat(){impl_->keyboardBackspaceRepeat=impl_->keyboardBackspacePaused=false;}
void View::pausePointerRepeat(bool paused){impl_->keyboardBackspacePaused=paused;}
Status View::tick(uint32_t now){
  Impl &s=*impl_;Impl::NativeStageTickGuard stageTickGuard{s};if(!s.project)return Status::NotInitialized;if(s.processing)return Status::Busy;
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
  struct NativePollGuard {
    Impl &self;
    explicit NativePollGuard(Impl &owner)noexcept:self(owner){self.nativePipelineInPoll=true;self.nativePipelinePublishedInPoll=false;}
    ~NativePollGuard()noexcept{self.nativePipelineInPoll=false;}
  } nativePollGuard{s};
#endif
  s.processing=true;s.advanceBehaviors(now);s.endProcessing();
  // Slow-refresh policy can change at runtime (for example, partial updates
  // disabled for cleanup). Keep its cadence current without adding capability
  // queries to the unchanged immediate/color hot path.
  if(s.displayPolicy.slowRefresh)s.displayPolicy=DisplayPolicy::from(s.display.displayCapabilities());
  const Status displayStatus=s.display.pollDisplay(now);
  Impl::TickTimer timer(s);const uint64_t rasterBefore=s.stats.renderedTiles;
  if(s.pendingNavigation&&!s.transition.active)s.endProcessing();
  if(s.groupMotion.active&&s.groupMotion.settling){
    const uint32_t elapsed=uint32_t(now-s.groupMotion.started);
    s.groupMotion.updated=now;
    if(s.displayPolicy.slowRefresh||elapsed>=s.groupMotion.duration)s.finishGroup();
    else {
      if(s.groupMotion.follow){const uint32_t progress=(elapsed<<10)/s.groupMotion.duration;const int32_t q=int32_t(animation_detail::easeQ10(progress,s.groupMotion.config.easing));s.groupMotion.offset=s.groupMotion.from+int32_t((int64_t(s.groupMotion.to)-s.groupMotion.from)*q/1024);}
      else {const int64_t remaining=180-elapsed;s.groupMotion.offset=int32_t(s.groupMotion.to+(int64_t(s.groupMotion.from)-s.groupMotion.to)*remaining*remaining*remaining/(180*180*180));}
      s.refresh->invalidateAll();
    }
  }
  if(s.transition.active) {
    if(s.transition.phase==Impl::TransitionPhase::Components)s.advanceDeparture(now);
    else {
      if(s.transition.pending) {s.transition.started=now;s.transition.pending=false;}
      const TransitionSample old=s.transition.sample;
      s.transition.sample=s.pageSample(uint32_t(now-s.transition.started));
      const auto &next=s.transition.sample;
      if(old.outX!=next.outX||old.outY!=next.outY||old.inX!=next.inX||old.inY!=next.inY||old.opacity!=next.opacity||old.active!=next.active)s.invalidateMotion(s.transitionViewport());
    }
  }
  if(s.backAnimation.active) {
    const uint32_t elapsed=uint32_t(now-s.backAnimation.started);
    if(elapsed>=180) {
      const bool commit=s.backAnimation.commit;s.backAnimation.active=false;
      if(commit)s.navigate(nullptr,RouteAction::Back);
      else { s.processing=true;s.backPrepared=false;s.backOffset=0;s.refresh->invalidateAll();s.routeEvent(EventType::NavigationCancel,s.page().path,s.project->pages[s.history[s.history.size()-2]].path,RouteAction::Back);s.endProcessing(); }
    } else { const int64_t remaining=180-elapsed;s.backOffset=int32_t(s.backAnimation.to+(int64_t(s.backAnimation.from)-s.backAnimation.to)*remaining*remaining*remaining/(180*180*180));s.refresh->invalidateAll(); }
  }
  if(s.inertia.active) {
    s.processing=true;uint32_t elapsed=uint32_t(now-s.inertia.last);if(elapsed>128)elapsed=128;
    const int steps=int(elapsed/16);
    for(int step=0;step<steps&&s.inertia.active;++step) {
      const int i=s.inertia.node;const bool horizontal=(s.state()[i].flags&Horizontal)!=0;
      const int32_t before=horizontal?s.state()[i].scrollX:s.state()[i].scrollY;
      s.inertia.remainder+=s.inertia.velocity*16;const int32_t distance=s.inertia.remainder/1000;s.inertia.remainder%=1000;
      s.scroll(i,horizontal?before+distance:0,horizontal?0:before+distance,true);
      s.inertia.velocity=s.inertia.velocity*90/100;
      const int32_t after=horizontal?s.state()[i].scrollX:s.state()[i].scrollY;
      if(abs(s.inertia.velocity)<20||(distance&&before==after))s.inertia.active=false;
    }
    s.inertia.last+=uint32_t(steps*16);if(uint32_t(now-s.inertia.last)>128)s.inertia.last=now;
    s.endProcessing();
  }
  if(s.animation.active) {
    s.processing=true;const uint32_t elapsed=uint32_t(now-s.animation.started);
    if(elapsed>=180) { const int node=s.animation.node,index=s.animation.index;s.animation.active=false;s.setPage(node,index,true); }
    else { const int64_t remaining=180-elapsed;const int64_t denominator=180*180*180;
      const int32_t position=int32_t(s.animation.to+(int64_t(s.animation.from)-s.animation.to)*remaining*remaining*remaining/denominator);
      s.scroll(s.animation.node,position,0,true); }
    s.endProcessing();
  }
  if(s.down&&!s.canceled&&(!s.coordinated()||!s.defaultsBlocked)&&!s.moved&&s.drag==Impl::Drag::None&&s.activeControl>=0&&s.page().nodes[s.activeControl].kind==Kind::Spinbox&&int32_t(now-s.spinRepeatAt)>=0){s.processing=true;s.longPressed=true;s.spinRepeatAt=now+120;s.spinbox(s.activeControl,s.x,true);s.endProcessing();}
  if(s.down&&!s.canceled&&(!s.coordinated()||!s.defaultsBlocked)&&!s.longPressed&&!s.moved&&s.drag==Impl::Drag::None&&uint32_t(now-s.downAt)>=600){s.processing=true;s.longPressed=true;s.dispatch(s.target,EventType::LongPress);s.endProcessing();}
  const bool departing=s.transition.active&&(s.transition.phase==Impl::TransitionPhase::Components||s.transition.phase==Impl::TransitionPhase::Exit);
  s.ensureTickNodes();
  if(!departing||s.sharedComponentDeparture())s.advanceMediaTweens(now);
  if((!departing||s.sharedComponentDeparture())&&!s.displayPolicy.slowRefresh)for(uint16_t i:s.mediaNodes) {
    const MediaDesc *m=s.media(s.page().nodes[i],s.state()[i]);if(!m||m->frames<2||!s.visible(s.page(),s.state(),i))continue;
    auto &v=s.state()[i];if(v.mediaPaused)continue;
    if(v.mediaPending){v.mediaAt=now-uint32_t((uint32_t(v.mediaFrame)*1000+m->fps-1)/m->fps);v.mediaPending=false;}
    const uint32_t elapsed=uint32_t(now-v.mediaAt);const uint32_t step=uint32_t(uint64_t(elapsed)*m->fps/1000);
    const uint16_t frame=uint16_t(step%m->frames);
    if(frame!=v.mediaFrame){v.mediaFrame=frame;s.invalidate(i);}
  }
  if(!departing){s.advanceAnimations(now);}
  s.ensureInputAvailable();
  s.repeatKeyboardBackspace(now);
  s.processing=true;s.stepMenuMotion(now);s.endProcessing();
  // A failed display must not stop menu timers, callbacks or logical state.
  // Preserve pending damage for recovery and report the physical error without
  // attempting another raster submission against the faulted backend.
  if(displayStatus!=Status::Ok&&displayStatus!=Status::Busy){s.nativeStageRefreshResult(displayStatus,true);return displayStatus;}
  // Slow displays receive the latest logical state after their active waveform
  // completes. Input, callbacks and clocks above remain responsive throughout.
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
  const auto pipeline=s.nativePipelineTick(now);
  if(pipeline.handled){
    if(pipeline.status==Status::Ok){s.settleMotions();s.settleTransition();}
    const Status result=s.mediaDecodeStatus!=Status::Ok?s.mediaDecodeStatus:pipeline.status;
    s.mediaDecodeStatus=Status::Ok;return result;
  }
#endif
  const bool rasterActive=s.refresh->busy();
  const bool movingFrame=s.menuMoving()||s.behaviorMoving()||s.animationsRunning()||s.transition.active;
  const uint32_t motionInterval=s.menuMotionConfigured&&movingFrame?(1000u+s.menuMotionConfig.maxFps-1)/s.menuMotionConfig.maxFps:0;
  const uint32_t frameInterval=std::max(s.displayPolicy.frameIntervalMs,motionInterval);
  const bool cadenceReady=!s.displayFrameStarted||!frameInterval||uint32_t(now-s.lastDisplayFrameAt)>=frameInterval;
  const bool mayStart=rasterActive||(!s.display.displayRefreshing()&&cadenceReady);
  const bool starting=mayStart&&!rasterActive&&s.refresh->dirty()&&!s.updateDepth;
  Status status=mayStart?s.start():Status::Ok;const bool frameActive=s.refresh->busy();
  if(starting&&status==Status::Ok){s.lastDisplayFrameAt=now;s.displayFrameStarted=true;}
  const uint64_t submitBefore=s.refresh->stats().submitMicros;
  const auto beforeDirect=s.refresh->stats();
  {Impl::ProfileTimer refreshTimer(s,&RuntimeStats::refreshStepMicros);if(status==Status::Ok)status=s.refresh->step();}
  s.nativeStageRefreshResult(status,!s.refresh->busy());
  s.stats.submitMicros+=s.refresh->stats().submitMicros-submitBefore;
  s.stats.directBlits+=s.refresh->stats().directBlits-beforeDirect.directBlits;
  s.stats.directPixels+=s.refresh->stats().directPixels-beforeDirect.directPixels;
  if(frameActive&&!s.refresh->busy()&&status==Status::Ok)s.completedFrame();
  if(status==Status::Ok){s.settleMotions();s.settleTransition();}s.releaseSnapshot();
  if(status==Status::Ok&&mayStart&&!s.softInput.visible){
    if(timer.expired()||(s.renderBudget.maxPixelsPerTick&&s.stats.renderedTiles!=rasterBefore))++s.stats.budgetDeferrals;
    else s.warmSurfaceTile(now);
  }
  const Status result=s.mediaDecodeStatus!=Status::Ok?s.mediaDecodeStatus:status;s.mediaDecodeStatus=Status::Ok;return result;
}
Status View::finish(){
  Impl &s=*impl_;Impl::NativeStageTickGuard stageTickGuard{s};
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
  const Status pipeline=s.finishNativePipelineWork();
  if(pipeline!=Status::Ok)return pipeline;
  if(s.nativePipelineInPoll&&s.nativePipelinePublishedInPoll)return Status::Busy;
#endif
  Status status=s.start();if(status!=Status::Ok)return status;
  const bool frameActive=s.refresh->busy();const auto before=s.refresh->stats();
  status=s.refresh->finish();s.nativeStageRefreshResult(status,!s.refresh->busy());const auto after=s.refresh->stats();
  s.stats.submitMicros+=after.submitMicros-before.submitMicros;
  s.stats.directBlits+=after.directBlits-before.directBlits;s.stats.directPixels+=after.directPixels-before.directPixels;
  if(frameActive&&!s.refresh->busy()&&status==Status::Ok)s.completedFrame();
  if(status==Status::Ok){s.settleMotions();s.settleTransition();}s.releaseSnapshot();const Status result=s.mediaDecodeStatus!=Status::Ok?s.mediaDecodeStatus:status;s.mediaDecodeStatus=Status::Ok;return result;
}
bool View::busy()const{return impl_->refresh->busy()||impl_->animationsRunning()||impl_->motionsPending()||!impl_->mediaTweens.empty()||impl_->animation.active||impl_->inertia.active||impl_->backAnimation.active||impl_->transition.active||impl_->groupMotion.active||impl_->menuMoving()||impl_->behaviorMoving();}
bool View::refreshing()const{return impl_->display.displayRefreshing()||impl_->refresh->busy();}
bool View::dirty()const{return impl_->refresh->dirty();}
void View::invalidate(){impl_->refresh->invalidateAll();}

} }
#endif // Non-AVR compiled UI runtime
