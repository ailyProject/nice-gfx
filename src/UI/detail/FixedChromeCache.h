#pragma once
#include "../../Platform/Common/Overlay.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <new>
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#include <esp_heap_caps.h>
#endif

namespace nice_gfx { namespace htmlui { namespace detail {
// Immutable foreground only. The owner must restrict its paint callback to
// componentwise monotone 1-Lipschitz RGB565 operations, freeze every paint/key
// input during resolve(), and restore its own canvas/render state each time.
// Never include a sampled/JPEG backdrop in the captured foreground.
//
// Source leases end at the next resolve()/clear(): consume each successful
// OverlayView synchronously before resolving another strip. The callbacks are
// synchronous CPU painters and must not retain a scratch pointer or launch DMA.
class FixedChromeCache {
 public:
  struct WireSurface {uint16_t *pixels=nullptr;size_t width=0,height=0,stride=0,bytes=0;};
  using BuildCallback=Status (*)(void *,const WireSurface &,uint16_t backgroundWord);
  using Allocate=void *(*)(size_t);using Free=void (*)(void *);
  static constexpr size_t SlotCount=8,DefaultBudget=2u*1024u*1024u,MaxScratchBytes=1024u*1024u,Alignment=64,PaletteBytes=65536u/8;
  explicit FixedChromeCache(size_t budget=DefaultBudget,size_t scratchBudget=MaxScratchBytes,
      Allocate allocate=allocateMemory,Free release=freeMemory)
    :budget_(budget<DefaultBudget?budget:DefaultBudget),scratchBudget_(scratchBudget<MaxScratchBytes?scratchBudget:MaxScratchBytes),
     allocate_(allocate),free_(release){}
  ~FixedChromeCache(){clear();}
  FixedChromeCache(const FixedChromeCache &)=delete;FixedChromeCache &operator=(const FixedChromeCache &)=delete;
  size_t bytes()const{return resident_+scratchBytes();}
  size_t persistentBytes()const{return resident_;}
  size_t scratchBytes()const{return scratchCapacity_*2+(palette_?PaletteBytes:0);}
  uint64_t hits()const{return hits_;}uint64_t misses()const{return misses_;}
  uint64_t builds()const{return builds_;}uint64_t evictions()const{return evictions_;}
  uint64_t partialRejects()const{return partialRejects_;}
  void clear(){for(auto &slot:slots_)discard(slot);releaseScratch();clock_=hits_=misses_=builds_=evictions_=partialRejects_=0;}

  // Wire stride is measured in pixels and participates in exact cache identity.
  // Native RGB565 is preferred: an unused foreground color becomes the key
  // for transparent pixels. Fully opaque strips need no key. If all 65536
  // colors are occupied, or the optional palette cannot be allocated within
  // the scratch budget, transparent strips use binary-alpha ARGB instead.
  // Output has an independently padded, 64-byte-aligned row stride.
  // Unsupported covers malformed input, partial dependence and optional-cache
  // allocation failure. Callback errors other than NoMemory are propagated.
  Status resolve(const void *key,size_t keyBytes,size_t width,size_t height,size_t stride,
      BuildCallback build,void *context,platform::OverlayView &output){
    output=platform::OverlayView{};
    if(!budget_||!scratchBudget_||!allocate_||!free_||!key||!keyBytes||!build||
       !width||!height||stride<width||stride>=0x4000||height>=0x4000||width>=0x4000||
       height>SIZE_MAX/stride/2||keyBytes>SIZE_MAX-(Alignment-1))return Status::Unsupported;
    const uintptr_t input=reinterpret_cast<uintptr_t>(key);
    if(keyBytes>UINTPTR_MAX-input||aliasesOwned(input,keyBytes))return Status::Unsupported;
    const size_t wireBytes=stride*height*2;
    if(wireBytes>SIZE_MAX-(Alignment-1))return Status::Unsupported;
    const size_t scratchNeeded=align(wireBytes),argbStride=(width+15)&~size_t(15),rgbStride=(width+31)&~size_t(31);
    if(argbStride>=0x4000||rgbStride>=0x4000||height>SIZE_MAX/argbStride/4||
       height>SIZE_MAX/rgbStride/2||scratchNeeded>scratchBudget_/2)return Status::Unsupported;
    const size_t paddedKey=align(keyBytes);if(paddedKey>budget_)return Status::Unsupported;
    const uint32_t hash=hashBytes(static_cast<const uint8_t *>(key),keyBytes);
    for(auto &slot:slots_)if(slot.raw&&slot.hash==hash&&slot.keyBytes==keyBytes&&slot.width==width&&slot.height==height&&
        slot.wireStride==stride&&!memcmp(slot.key,key,keyBytes)){
      ++hits_;slot.used=++clock_;if(slot.unsupported)return Status::Unsupported;
      publish(slot,output);return Status::Ok;
    }
    ++misses_;
    if(!reserveScratch(scratchNeeded))return Status::Unsupported;
    memset(black_,0,wireBytes);memset(white_,0xff,wireBytes);
    const WireSurface black{black_,width,height,stride,wireBytes},white{white_,width,height,stride,wireBytes};
    Status result=build(context,black,0);
    if(result!=Status::Ok)return result==Status::NoMemory?Status::Unsupported:result;
    result=build(context,white,0xffff);
    if(result!=Status::Ok)return result==Status::NoMemory?Status::Unsupported:result;
    ++builds_;
    bool supported=true,transparent=false;
    for(size_t y=0;y<height&&supported;++y)for(size_t x=0;x<width;++x){
      const uint16_t a=fromWire(black_[y*stride+x]),b=fromWire(white_[y*stride+x]);
      if(a==b)continue;
      if(a||b!=0xffff){supported=false;break;}
      transparent=true;
    }
    bool rgb=true;uint16_t colorKey=0;
    if(supported&&transparent){
      rgb=reservePalette();
      if(rgb){
        memset(palette_,0,PaletteBytes);
        for(size_t y=0;y<height;++y)for(size_t x=0;x<width;++x){
          const uint16_t a=fromWire(black_[y*stride+x]),b=fromWire(white_[y*stride+x]);
          if(a==b)palette_[a>>5]|=uint32_t(1)<<(a&31);
        }
        rgb=false;
        for(size_t word=0;word<PaletteBytes/4;++word)if(palette_[word]!=UINT32_MAX){
          unsigned bit=0;while(palette_[word]&(uint32_t(1)<<bit))++bit;
          colorKey=uint16_t(word*32+bit);rgb=true;break;
        }
      }
    }
    const size_t outputStride=rgb?rgbStride:argbStride;
    const size_t pixelsBytes=supported?outputStride*height*(rgb?2:4):0;
    if(pixelsBytes>budget_-paddedKey)return Status::Unsupported;
    // Remember a deterministic endpoint rejection under its exact immutable
    // key too, so an unsupported strip does not repaint two scratch images on
    // every frame. OOM/callback errors are deliberately never negative-cached.
    const size_t needed=paddedKey+pixelsBytes;
    Slot *slot=acquire(needed);if(!slot)return Status::Unsupported;
    auto *raw=static_cast<uint8_t *>(allocate_(needed));
    if(!raw)return Status::Unsupported;
    if(reinterpret_cast<uintptr_t>(raw)&(Alignment-1)){free_(raw);return Status::Unsupported;}
    const size_t storedPixels=pixelsBytes;
    if(storedPixels)memset(raw,0,storedPixels);
    memcpy(raw+storedPixels,key,keyBytes);
    if(supported&&rgb){auto *pixels=reinterpret_cast<uint16_t *>(raw);
      // Include row padding in the same safe foreground representation.
      if(transparent)for(size_t i=0;i<pixelsBytes/2;++i)pixels[i]=colorKey;
      for(size_t y=0;y<height;++y)for(size_t x=0;x<width;++x){
        const uint16_t a=fromWire(black_[y*stride+x]),b=fromWire(white_[y*stride+x]);
        pixels[y*outputStride+x]=a==b?a:colorKey;
      }
    }else if(supported){auto *argb=reinterpret_cast<uint32_t *>(raw);
      for(size_t y=0;y<height;++y)for(size_t x=0;x<width;++x){
        const uint16_t a=fromWire(black_[y*stride+x]),b=fromWire(white_[y*stride+x]);
        if(a==b)argb[y*outputStride+x]=argbColor(a);
      }
    }else ++partialRejects_;
    slot->raw=raw;slot->allocated=needed;slot->key=raw+storedPixels;slot->keyBytes=keyBytes;slot->hash=hash;
    slot->width=width;slot->height=height;slot->wireStride=stride;slot->outputStride=outputStride;
    slot->rgb=rgb;slot->colorKey=transparent&&rgb;slot->keyColor=colorKey;
    slot->pixelsBytes=storedPixels;slot->unsupported=!supported;slot->used=++clock_;resident_+=needed;
    if(!supported)return Status::Unsupported;
    publish(*slot,output);return Status::Ok;
  }

 private:
  struct Slot {uint8_t *raw=nullptr;const uint8_t *key=nullptr;size_t allocated=0,keyBytes=0,width=0,height=0,wireStride=0,outputStride=0,pixelsBytes=0;
    uint32_t hash=0;uint64_t used=0;uint16_t keyColor=0;bool unsupported=false,rgb=true,colorKey=false;};
  static size_t align(size_t bytes){return (bytes+Alignment-1)&~(Alignment-1);}
  static uint16_t fromWire(uint16_t word){const auto *p=reinterpret_cast<const uint8_t *>(&word);return uint16_t(uint16_t(p[0])<<8|p[1]);}
  static uint32_t argbColor(uint16_t color){const uint32_t r=color>>11,g=(color>>5)&63,b=color&31;
    return 0xff000000u|(((r<<3)|(r>>2))<<16)|(((g<<2)|(g>>4))<<8)|((b<<3)|(b>>2));}
  static uint32_t hashBytes(const uint8_t *key,size_t bytes){uint32_t hash=2166136261u;for(size_t i=0;i<bytes;++i)hash=(hash^key[i])*16777619u;return hash;}
  static bool overlaps(uintptr_t input,size_t bytes,const void *owned,size_t capacity){
    if(!owned||!capacity)return false;const uintptr_t p=reinterpret_cast<uintptr_t>(owned);
    return capacity>UINTPTR_MAX-p||(input<p+capacity&&p<input+bytes);
  }
  bool aliasesOwned(uintptr_t input,size_t bytes)const{
    if(overlaps(input,bytes,black_,scratchCapacity_)||overlaps(input,bytes,white_,scratchCapacity_)||
       overlaps(input,bytes,palette_,palette_?PaletteBytes:0))return true;
    for(const auto &slot:slots_)if(overlaps(input,bytes,slot.raw,slot.allocated))return true;return false;
  }
  bool reserveScratch(size_t bytes){
    if(bytes<=scratchCapacity_&&black_&&white_)return true;
    // No callback/source lease survives resolve. Release before growing so the
    // temporary peak, not just the eventual retained pair, stays <=1MiB.
    releaseScratch();
    auto *black=static_cast<uint16_t *>(allocate_(bytes));if(!black)return false;
    if(reinterpret_cast<uintptr_t>(black)&(Alignment-1)){free_(black);return false;}
    auto *white=static_cast<uint16_t *>(allocate_(bytes));
    if(!white){free_(black);return false;}
    if(reinterpret_cast<uintptr_t>(white)&(Alignment-1)){free_(white);free_(black);return false;}
    black_=black;white_=white;scratchCapacity_=bytes;return true;
  }
  bool reservePalette(){
    if(palette_)return true;
    // The palette is heap-backed, never an 8KiB task-stack temporary. Failure
    // only selects ARGB, and the pair plus palette never exceeds 1MiB.
    if(scratchBudget_<PaletteBytes||scratchCapacity_>(scratchBudget_-PaletteBytes)/2)return false;
    auto *memory=static_cast<uint32_t *>(allocate_(PaletteBytes));if(!memory)return false;
    if(reinterpret_cast<uintptr_t>(memory)&(Alignment-1)){free_(memory);return false;}
    palette_=memory;return true;
  }
  void releaseScratch(){if(black_)free_(black_);if(white_)free_(white_);if(palette_)free_(palette_);
    black_=white_=nullptr;palette_=nullptr;scratchCapacity_=0;}
  Slot *oldest(){Slot *result=nullptr;for(auto &slot:slots_)if(slot.raw&&(!result||slot.used<result->used))result=&slot;return result;}
  Slot *acquire(size_t bytes){
    while(resident_>budget_-bytes){auto *victim=oldest();if(!victim)return nullptr;discard(*victim);++evictions_;}
    for(auto &slot:slots_)if(!slot.raw)return &slot;
    auto *victim=oldest();if(victim){discard(*victim);++evictions_;}return victim;
  }
  void discard(Slot &slot){if(slot.raw){free_(slot.raw);resident_-=slot.allocated;}slot=Slot{};}
  static void publish(const Slot &slot,platform::OverlayView &output){
    output.data=slot.raw;output.width=slot.width;output.height=slot.height;output.stride=slot.outputStride;
    output.bytes=slot.pixelsBytes;output.format=slot.rgb?platform::OverlayFormat::Rgb565:platform::OverlayFormat::Argb8888;
    output.alpha=255;output.colorKey=slot.colorKey;output.key=slot.keyColor;
  }
  static void *allocateMemory(size_t bytes){
#if defined(CONFIG_IDF_TARGET_ESP32S31)
    return heap_caps_aligned_alloc(Alignment,bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
#else
    return ::operator new(bytes,std::align_val_t(Alignment),std::nothrow);
#endif
  }
  static void freeMemory(void *memory){
#if defined(CONFIG_IDF_TARGET_ESP32S31)
    heap_caps_free(memory);
#else
    ::operator delete(memory,std::align_val_t(Alignment));
#endif
  }
  Slot slots_[SlotCount];size_t budget_=0,scratchBudget_=0,resident_=0,scratchCapacity_=0;
  uint16_t *black_=nullptr,*white_=nullptr;uint32_t *palette_=nullptr;Allocate allocate_;Free free_;
  uint64_t clock_=0,hits_=0,misses_=0,builds_=0,evictions_=0,partialRejects_=0;
};
}}} // namespace nice_gfx::htmlui::detail
