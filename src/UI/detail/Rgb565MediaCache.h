#pragma once
#include "../Compiled.h"
#include "../../Platform/Common/Overlay.h"
#include "AlignedMemory.h"
#include <new>
#include <string.h>
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#include <esp_heap_caps.h>
#include <esp_cache.h>
#endif
namespace nice_gfx { namespace htmlui { namespace detail {
// Immutable compiled RGB565 assets, scaled once with the ordinary nearest
// neighbor rule. Native aligned rows can then feed PPA without Flash reads or
// repeated pixel conversion. The caller only supplies immutable sources;
// frame selection follows the existing media clock, independently of caching.
class Rgb565MediaCache {
 public:
  explicit Rgb565MediaCache(size_t budget=3u*1024u*1024u):budget_(budget){}
  ~Rgb565MediaCache(){for(auto &s:slots_)discard(s);}
  Rgb565MediaCache(const Rgb565MediaCache&)=delete;Rgb565MediaCache&operator=(const Rgb565MediaCache&)=delete;
  size_t bytes()const{return bytes_;}
  bool resolveFrame(const MediaDesc &media,size_t frame,size_t width,size_t height,platform::OverlayView &out){
    out=platform::OverlayView{};
    if(!media.pixels||media.bytesPerPixel!=2||!media.width||!media.height||frame>=media.frames)return false;
    const size_t rowBytes=size_t(media.width)*2;
    if(size_t(media.height)>SIZE_MAX/rowBytes)return false;
    const size_t frameBytes=rowBytes*media.height;
    const uintptr_t address=reinterpret_cast<uintptr_t>(media.pixels);
    if(size_t(media.frames)>(UINTPTR_MAX-address)/frameBytes)return false;
    const MediaDesc selected{media.pixels+frame*frameBytes,media.width,media.height,1,media.fps,2};
    return resolve(selected,width,height,out);
  }
  bool resolve(const MediaDesc &media,size_t width,size_t height,platform::OverlayView &out){
    out=platform::OverlayView{};
    if(!media.pixels||media.bytesPerPixel!=2||media.frames!=1||!media.width||!media.height||!width||!height||width>=0x4000||height>=0x4000)return false;
    const size_t stride=(width+31)&~size_t(31);if(height>SIZE_MAX/stride/2)return false;const size_t needed=stride*height*2;if(needed>budget_)return false;
    Slot *slot=nullptr;for(auto &s:slots_)if(s.pixels&&s.source==media.pixels&&s.sourceWidth==media.width&&s.sourceHeight==media.height&&s.width==width&&s.height==height){slot=&s;break;}
    if(!slot){
      while(bytes_>budget_-needed){auto *s=lru();if(!s)return false;discard(*s);}
      for(auto &s:slots_)if(!s.pixels){slot=&s;break;}if(!slot){slot=lru();discard(*slot);}
#if defined(CONFIG_IDF_TARGET_ESP32P4)
      if(heap_caps_get_free_size(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT)<needed+2u*1024u*1024u)return false;
      auto *pixels=static_cast<uint16_t*>(heap_caps_aligned_alloc(64,needed,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
#else
      auto *pixels=static_cast<uint16_t*>(allocateAlignedMemory<64>(needed));
#endif
      if(!pixels)return false;
      for(size_t y=0;y<height;++y){const auto *row=media.pixels+(y*media.height/height)*size_t(media.width)*2;auto *target=pixels+y*stride;
        for(size_t x=0;x<width;++x){const size_t sx=x*media.width/width;target[x]=uint16_t(uint16_t(row[sx*2])<<8|row[sx*2+1]);}
        for(size_t x=width;x<stride;++x)target[x]=0;
      }
#if defined(CONFIG_IDF_TARGET_ESP32P4)
      if(esp_cache_msync(pixels,needed,ESP_CACHE_MSYNC_FLAG_DIR_C2M)!=ESP_OK){heap_caps_free(pixels);return false;}
#endif
      slot->pixels=pixels;slot->source=media.pixels;slot->sourceWidth=media.width;slot->sourceHeight=media.height;slot->width=width;slot->height=height;slot->stride=stride;slot->bytes=needed;bytes_+=needed;
    }
    slot->used=++clock_;out.data=reinterpret_cast<const uint8_t*>(slot->pixels);out.width=width;out.height=height;out.stride=slot->stride;out.bytes=slot->bytes;out.format=platform::OverlayFormat::Rgb565;return true;
  }
 private:
  struct Slot{uint16_t *pixels=nullptr;const uint8_t *source=nullptr;size_t sourceWidth=0,sourceHeight=0,width=0,height=0,stride=0,bytes=0;uint64_t used=0;};
  Slot *lru(){Slot *s=nullptr;for(auto &candidate:slots_)if(candidate.pixels&&(!s||candidate.used<s->used))s=&candidate;return s;}
  void discard(Slot &s){if(s.pixels){
#if defined(CONFIG_IDF_TARGET_ESP32P4)
    heap_caps_free(s.pixels);
#else
    freeAlignedMemory<64>(s.pixels);
#endif
    bytes_-=s.bytes;}s=Slot{};}
  Slot slots_[16];size_t budget_=0,bytes_=0;uint64_t clock_=0;
};
}}}
