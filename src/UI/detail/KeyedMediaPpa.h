#pragma once
#include "../Compiled.h"
#include "../../Platform/Common/Overlay.h"
#include <stdlib.h>
#include <string.h>
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#include <esp_heap_caps.h>
#include <esp_cache.h>
#endif
#ifndef NICE_GFX_UI_KEYED_MEDIA_PPA
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#define NICE_GFX_UI_KEYED_MEDIA_PPA 1
#else
#define NICE_GFX_UI_KEYED_MEDIA_PPA 0
#endif
#endif
namespace nice_gfx { namespace htmlui { namespace detail {
// Immutable compiled Wire565 images are copied once into DMA-friendly native
// RGB565. One owner task borrows a resolved slot only through a blocking PPA
// operation; no State or asynchronous transfer retains these cache pointers.
class KeyedMediaPpaCache {
 public:
  using Allocate=void *(*)(size_t,size_t);using Free=void (*)(void *);using Sync=bool (*)(void *,size_t);
  static constexpr size_t Budget=512u*1024u,Slots=4,Alignment=64;
  explicit KeyedMediaPpaCache(size_t budget=Budget,Allocate alloc=allocate,Free release=freeMemory,Sync sync=synchronize)
    :budget_(budget),allocate_(alloc),free_(release),sync_(sync){}
  ~KeyedMediaPpaCache(){clear();}
  KeyedMediaPpaCache(const KeyedMediaPpaCache &)=delete;KeyedMediaPpaCache &operator=(const KeyedMediaPpaCache &)=delete;
  size_t bytes()const{return bytes_;}uint64_t copies()const{return copies_;}
  void clear(){for(auto &slot:slots_)discard(slot);clock_=copies_=0;}
  bool resolve(const MediaDesc &media,const uint8_t *frame,size_t stride,platform::OverlayView &out){
    out=platform::OverlayView{};
    if(!allocate_||!free_||!sync_||!media.pixels||!frame||!mediaColorKeyed(media)||media.frames!=1||
       !media.width||!media.height||media.width>=0x4000||media.height>=0x4000||stride<size_t(media.width)*2||
       (stride&1)||size_t(media.height)>SIZE_MAX/stride)return false;
    const size_t sourceBytes=stride*media.height;const uintptr_t source=reinterpret_cast<uintptr_t>(frame);
    if(source>UINTPTR_MAX-sourceBytes)return false;
    // Cached outputs cannot become future immutable inputs to their own LRU.
    for(const auto &slot:slots_)if(slot.pixels){const uintptr_t p=reinterpret_cast<uintptr_t>(slot.pixels);
      if(source<p+slot.bytes&&p<source+sourceBytes)return false;}
    const uint16_t key=uint16_t(uint16_t(media.pixels[0])<<8|media.pixels[1]);Slot *found=nullptr;
    for(auto &slot:slots_)if(slot.pixels&&slot.source==frame&&slot.sourceStride==stride&&slot.width==media.width&&slot.height==media.height&&slot.key==key){found=&slot;break;}
    if(!found){
      const size_t rowBytes=(size_t(media.width)*2+Alignment-1)&~(Alignment-1);
      if(size_t(media.height)>SIZE_MAX/rowBytes)return false;const size_t needed=rowBytes*media.height;
      if(needed>budget_)return false;
      while(bytes_>budget_-needed){auto *oldest=lru();if(!oldest)return false;discard(*oldest);}
      for(auto &slot:slots_)if(!slot.pixels){found=&slot;break;}
      if(!found){found=lru();discard(*found);}
      auto *pixels=static_cast<uint8_t *>(allocate_(Alignment,needed));if(!pixels)return false;
      if(reinterpret_cast<uintptr_t>(pixels)%Alignment){free_(pixels);return false;}
      for(size_t y=0;y<media.height;++y){const uint8_t *input=frame+y*stride;auto *row=reinterpret_cast<uint16_t *>(pixels+y*rowBytes);
        for(size_t x=0;x<media.width;++x)row[x]=uint16_t(uint16_t(input[2*x])<<8|input[2*x+1]);
        for(size_t x=media.width;x<rowBytes/2;++x)row[x]=key;
      }
      if(!sync_(pixels,needed)){free_(pixels);return false;}
      found->pixels=pixels;found->source=frame;found->sourceStride=stride;found->stride=rowBytes/2;found->bytes=needed;
      found->width=media.width;found->height=media.height;found->key=key;bytes_+=needed;++copies_;
    }
    found->used=++clock_;out.data=found->pixels;out.width=found->width;out.height=found->height;out.stride=found->stride;out.bytes=found->bytes;
    out.format=platform::OverlayFormat::Rgb565;out.colorKey=true;out.key=found->key;return true;
  }
 private:
  struct Slot {uint8_t *pixels=nullptr;const uint8_t *source=nullptr;size_t sourceStride=0,stride=0,bytes=0;uint16_t width=0,height=0,key=0;uint64_t used=0;};
  Slot *lru(){Slot *oldest=nullptr;for(auto &slot:slots_)if(slot.pixels&&(!oldest||slot.used<oldest->used))oldest=&slot;return oldest;}
  void discard(Slot &slot){if(slot.pixels){free_(slot.pixels);bytes_-=slot.bytes;}slot=Slot{};}
  static void *allocate(size_t alignment,size_t bytes){
#if defined(CONFIG_IDF_TARGET_ESP32S31)
    return heap_caps_aligned_alloc(alignment,bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
#else
    if(bytes>SIZE_MAX-alignment-sizeof(void *))return nullptr;void *raw=::malloc(bytes+alignment+sizeof(void *));if(!raw)return nullptr;
    const uintptr_t p=(reinterpret_cast<uintptr_t>(raw)+sizeof(void *)+alignment-1)&~(alignment-1);reinterpret_cast<void **>(p)[-1]=raw;return reinterpret_cast<void *>(p);
#endif
  }
  static void freeMemory(void *memory){
#if defined(CONFIG_IDF_TARGET_ESP32S31)
    heap_caps_free(memory);
#else
    ::free(reinterpret_cast<void **>(memory)[-1]);
#endif
  }
  static bool synchronize(void *pixels,size_t bytes){
#if defined(CONFIG_IDF_TARGET_ESP32S31)
    return esp_cache_msync(pixels,bytes,ESP_CACHE_MSYNC_FLAG_DIR_C2M)==ESP_OK;
#else
    (void)pixels;(void)bytes;return true;
#endif
  }
  Slot slots_[Slots];size_t budget_=0,bytes_=0;uint64_t clock_=0,copies_=0;Allocate allocate_;Free free_;Sync sync_;
};
}}}
