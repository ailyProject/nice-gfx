#pragma once
#include "../Compiled.h"
#include "../JpegDecoder.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <memory>
#include <vector>
#include <new>
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
#include <soc/soc_caps.h>
#endif
#if defined(SOC_JPEG_DECODE_SUPPORTED) && SOC_JPEG_DECODE_SUPPORTED
#include <driver/jpeg_decode.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <esp_idf_version.h>
#define NICE_GFX_UI_HAS_HARDWARE_JPEG 1
#else
#define NICE_GFX_UI_HAS_HARDWARE_JPEG 0
#endif
#ifndef NICE_GFX_UI_JPEG_FLASH_DIRECT
#define NICE_GFX_UI_JPEG_FLASH_DIRECT 1
#endif
// Limit borrowed Flash input to the S31 driver/API revision validated by the
// SDK contract. Other targets/revisions and encrypted builds keep scratch.
#if NICE_GFX_UI_HAS_HARDWARE_JPEG && NICE_GFX_UI_JPEG_FLASH_DIRECT && defined(CONFIG_IDF_TARGET_ESP32S31)
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6,1,0) && !CONFIG_SECURE_FLASH_ENC_ENABLED
#include <esp_memory_utils.h>
#include <esp_efuse.h>
#include <spi_flash_mmap.h>
#include <esp_private/dma2d.h>
#define NICE_GFX_UI_CAN_BORROW_JPEG_FLASH 1
#endif
#endif
#ifndef NICE_GFX_UI_CAN_BORROW_JPEG_FLASH
#define NICE_GFX_UI_CAN_BORROW_JPEG_FLASH 0
#endif
#if NICE_GFX_UI_HAS_HARDWARE_JPEG && defined(CONFIG_IDF_TARGET_ESP32P4)
#include <esp_memory_utils.h>
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6,0,0)
#include <esp_cache.h>
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6,1,0)
#include <esp_private/dma2d.h>
#endif
#else
#include <esp_dma_utils.h>
#endif
#define NICE_GFX_UI_CAN_DECODE_JPEG_FRAMEBUFFER 1
#else
#define NICE_GFX_UI_CAN_DECODE_JPEG_FRAMEBUFFER NICE_GFX_UI_CAN_BORROW_JPEG_FLASH
#endif
namespace nice_gfx { namespace htmlui { namespace detail {
inline bool jpegCanBorrowFlash(const uint8_t *bytes,size_t count){
#if NICE_GFX_UI_CAN_BORROW_JPEG_FLASH
  if(!bytes||!count||count>UINT32_MAX)return false;
  const uintptr_t first=reinterpret_cast<uintptr_t>(bytes);
  if(first>UINTPTR_MAX-(count-1)||esp_efuse_is_flash_encryption_enabled())return false;
  const void *last=reinterpret_cast<const void *>(first+count-1);
  if(!esp_ptr_in_drom(bytes)||!esp_ptr_in_drom(last)||esp_ptr_external_ram(bytes)||esp_ptr_external_ram(last)||
     dma2d_get_buffer_alignment_constraint(bytes)>1)return false;
  const size_t physical=spi_flash_cache2phys(bytes),end=spi_flash_cache2phys(last);
  return physical!=SPI_FLASH_CACHE2PHYS_FAIL&&end!=SPI_FLASH_CACHE2PHYS_FAIL&&end>=physical&&end-physical==count-1;
#else
  (void)bytes;(void)count;return false;
#endif
}
// P4 copies compressed Flash input into its decoder-owned DMA scratch. This
// eligibility check permits immutable look-ahead without claiming direct Flash
// DMA support on the P4 driver or retaining application-owned mutable memory.
inline bool jpegImmutableFramebufferSource(const uint8_t *bytes,size_t count){
#if NICE_GFX_UI_HAS_HARDWARE_JPEG && defined(CONFIG_IDF_TARGET_ESP32P4)
  const uintptr_t at=reinterpret_cast<uintptr_t>(bytes);
  return bytes&&count&&count<=UINT32_MAX&&at<=UINTPTR_MAX-(count-1)&&esp_ptr_in_drom(bytes)&&esp_ptr_in_drom(reinterpret_cast<const void*>(at+count-1));
#else
  return jpegCanBorrowFlash(bytes,count);
#endif
}
inline bool jpegFramebufferAligned(const void *pixels,size_t bytes){
#if NICE_GFX_UI_HAS_HARDWARE_JPEG && defined(CONFIG_IDF_TARGET_ESP32P4)
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6,0,0)
  // IDF 6 removed esp_dma_utils' public helpers. Match the JPEG driver's
  // cache-line (or 4-byte) alignment and DMA2D memory constraints instead.
  size_t alignment=esp_cache_get_line_size_by_addr(pixels);if(!alignment)alignment=4;
  if((reinterpret_cast<uintptr_t>(pixels)&(alignment-1))||(bytes&(alignment-1)))return false;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6,1,0)
  return dma2d_get_buffer_alignment_constraint(pixels)<=1;
#else
  return true;
#endif
#else
  esp_dma_mem_info_t memory{};memory.dma_alignment_bytes=4;
  return esp_dma_is_buffer_alignment_satisfied(pixels,bytes,memory);
#endif
#elif NICE_GFX_UI_CAN_BORROW_JPEG_FLASH
  (void)bytes;return dma2d_get_buffer_alignment_constraint(pixels)<=1;
#else
  (void)pixels;(void)bytes;return false;
#endif
}
inline uint32_t jpegRead32(const uint8_t *p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
inline uint16_t jpegRead16(const uint8_t *p){return uint16_t(uint16_t(p[0])<<8|p[1]);}
struct JpegHeader {uint16_t width=0,height=0;uint8_t components=0,sampling=0;};
inline bool jpegHeader(const uint8_t *bytes,size_t count,JpegHeader &header){
  if(!bytes||count<4||bytes[0]!=0xff||bytes[1]!=0xd8||bytes[count-2]!=0xff||bytes[count-1]!=0xd9)return false;
  JpegHeader result;size_t offset=2;
  while(offset<count){
    if(bytes[offset++]!=0xff)return false;
    while(offset<count&&bytes[offset]==0xff)++offset;
    if(offset>=count)return false;const uint8_t marker=bytes[offset++];
    if(!marker||marker==0xd8||marker==0xd9||marker==1||(marker>=0xd0&&marker<=0xd7)||count-offset<2)return false;
    const size_t length=jpegRead16(bytes+offset);if(length<2||length>count-offset)return false;
    const uint8_t *payload=bytes+offset+2;
    const bool sof=marker>=0xc0&&marker<=0xcf&&marker!=0xc4&&marker!=0xc8&&marker!=0xcc;
    if(sof){
      if(marker!=0xc0||result.components||length<8||payload[0]!=8)return false;
      result.components=payload[5];if(result.components!=3||length!=size_t(8+3*result.components))return false;
      result.height=jpegRead16(payload+1);result.width=jpegRead16(payload+3);result.sampling=payload[7];
      if(!result.width||!result.height||(result.sampling!=0x11&&result.sampling!=0x21&&result.sampling!=0x22))return false;
    }
    if(marker==0xda){
      if(!result.components||length!=size_t(6+2*result.components)||payload[0]!=result.components||payload[1+2*result.components]||payload[2+2*result.components]!=63||payload[3+2*result.components]||offset+length>=count-2)return false;
      header=result;return true;
    }
    offset+=length;
  }
  return false;
}
inline const JpegMediaResource *jpegResource(const MediaDesc &m){return mediaJpeg(m)?reinterpret_cast<const JpegMediaResource *>(m.pixels):nullptr;}
inline bool jpegFrame(const MediaDesc &m,uint16_t frame,const uint8_t *&bytes,size_t &count){
  const auto *resource=jpegResource(m);
  if(!resource||!resource->data||resource->bytes<12||resource->bytes>12u*1024u*1024u||reinterpret_cast<uintptr_t>(resource->data)>UINTPTR_MAX-resource->bytes||frame>=m.frames)return false;
  const uint8_t *data=resource->data;if(memcmp(data,"NGJ1",4)||jpegRead32(data+4)!=m.frames)return false;
  const size_t header=8+(size_t(m.frames)+1)*4;
  if(header>resource->bytes||jpegRead32(data+8)!=0||jpegRead32(data+8+size_t(m.frames)*4)!=resource->bytes-header)return false;
  const size_t first=jpegRead32(data+8+size_t(frame)*4),end=jpegRead32(data+8+size_t(frame+1)*4);
  if(first>=end||end>resource->bytes-header)return false;
  bytes=data+header+first;count=end-first;return true;
}
inline bool validJpegMedia(const MediaDesc &m){
  if(!mediaJpeg(m)||!m.pixels||!m.width||!m.height||m.width>4096||m.height>4096||!m.frames||m.frames>300||!m.fps||m.fps>30||uint64_t(m.width)*m.height*2>8388608)return false;
  for(uint16_t frame=0;frame<m.frames;++frame){const uint8_t *bytes=nullptr;size_t count=0;JpegHeader header;
    if(!jpegFrame(m,frame,bytes,count)||!jpegHeader(bytes,count,header)||header.width!=m.width||header.height!=m.height)return false;}
  return true;
}
// SDK revisions may return visible or MCU-padded RGB565 output. YUV422
// uses16x8 MCUs and YUV444 uses8x8; the allocation uses the16x16 upper bound.
inline bool jpegOutputStride(size_t width,size_t height,uint8_t sampling,size_t written,size_t capacity,size_t &stride){
  if(!width||!height||width>4096||height>4096||written>capacity)return false;
  const size_t blockX=size_t(sampling>>4)*8,blockY=size_t(sampling&15)*8;
  if((sampling!=0x11&&sampling!=0x21&&sampling!=0x22)||!blockX||!blockY)return false;
  const size_t mw=(width+blockX-1)/blockX*blockX,mh=(height+blockY-1)/blockY*blockY;
  const size_t pw=(width+15)&~size_t(15),ph=(height+15)&~size_t(15);
  const size_t layouts[][2]={{mw,mh},{pw,ph},{mw,height},{width,mh},{pw,height},{width,ph},{width,height}};
  for(const auto &layout:layouts)if(layout[0]*layout[1]*2==written&&layout[0]>=width&&layout[1]>=height){stride=layout[0]*2;return true;}
  return false;
}
// Bounded per-instance decode scratch belongs to one View owner task. It is never
// exposed as an immutable Image: painters copy it into their leased tile or
// framebuffer synchronously. Independent instances keep separate current frames;
// shared bindings retain their slot across routes. All slots share one engine
// and DMA input scratch, with an8MiB default LRU output budget.
class JpegMediaCache {
 public:
  struct FramebufferPlan {const uint8_t *bytes=nullptr;size_t count=0;uint16_t width=0,height=0;uint8_t sampling=0;};
  explicit JpegMediaCache(size_t budget=8388608):budget_(budget){}
  ~JpegMediaCache(){release();}
  JpegMediaCache(const JpegMediaCache &)=delete;
  JpegMediaCache &operator=(const JpegMediaCache &)=delete;
  bool available()const{return handler_||NICE_GFX_UI_HAS_HARDWARE_JPEG;}
  void configure(JpegDecodeHandler handler,void *context){handler_=handler;context_=context;for(auto &slot:slots_)slot->cached=nullptr;}
  JpegDecodeHandler handler()const{return handler_;}void *context()const{return context_;}
  const void *pixels()const{return slots_.empty()?nullptr:slots_[0]->pixels;}
  size_t capacity()const{return slots_.empty()?0:slots_[0]->capacity;}
  bool overlaps(uintptr_t begin,size_t bytes)const{
    const auto overlap=[&](const void *memory,size_t size){if(!memory||!size)return false;const uintptr_t at=reinterpret_cast<uintptr_t>(memory);return at>UINTPTR_MAX-size||begin>UINTPTR_MAX-bytes||(at<begin+bytes&&begin<at+size);};
    if(overlap(input_,inputBytes_))return true;for(const auto &slot:slots_)if(overlap(slot->pixels,slot->capacity))return true;return false;
  }
  JpegMediaStats stats()const{auto result=stats_;result.outputBytes=outputBytes_;result.inputBytes=inputBytes_;result.cachedInstances=slots_.size();result.cacheBudgetBytes=budget_;return result;}
  // Pure structural/input preflight. A plan never owns or retains an LCD
  // buffer, and external output never enters a decoded-image cache slot.
  bool planFramebuffer(const MediaDesc &m,uint16_t frame,FramebufferPlan &plan)const{
    plan=FramebufferPlan{};
#if NICE_GFX_UI_CAN_DECODE_JPEG_FRAMEBUFFER
    if(handler_||!m.width||!m.height||(m.width&15)||(m.height&15))return false;
    const uint8_t *bytes=nullptr;size_t count=0;JpegHeader header;
    if(!jpegFrame(m,frame,bytes,count)||!jpegHeader(bytes,count,header)||header.width!=m.width||header.height!=m.height||!jpegImmutableFramebufferSource(bytes,count))return false;
    plan={bytes,count,m.width,m.height,header.sampling};return true;
#else
    (void)m;(void)frame;return false;
#endif
  }
  Status decodeFramebuffer(const FramebufferPlan &plan,JpegDecodeTarget &target){
#if NICE_GFX_UI_CAN_DECODE_JPEG_FRAMEBUFFER
    const size_t bytes=size_t(plan.width)*plan.height*2;
    const uintptr_t at=reinterpret_cast<uintptr_t>(target.pixels);
    if(handler_||!plan.bytes||!plan.count||!plan.width||!plan.height||(plan.width&15)||(plan.height&15)||
       !target.pixels||(at&63)||(bytes&63)||target.capacityBytes!=bytes||target.strideBytes!=size_t(plan.width)*2||
       target.width!=plan.width||target.height!=plan.height||at>UINTPTR_MAX-bytes||overlaps(at,bytes)||
       !esp_ptr_external_ram(target.pixels)||!esp_ptr_external_ram(reinterpret_cast<const void *>(at+bytes-1))||
       !jpegFramebufferAligned(target.pixels,bytes)||!jpegImmutableFramebufferSource(plan.bytes,plan.count))return Status::Unsupported;
    const auto original=target;const Status status=hardware(plan.bytes,plan.count,target,plan.sampling);
    if(status!=Status::Ok)return failed(status);
    if(target.pixels!=original.pixels||target.capacityBytes!=bytes||target.strideBytes!=size_t(plan.width)*2||target.width!=plan.width||target.height!=plan.height)return failed(Status::IoError);
    ++stats_.decodedFrames;++stats_.directFramebufferFrames;return Status::Ok;
#else
    (void)plan;(void)target;return Status::Unsupported;
#endif
  }
  Status resolve(const MediaDesc &m,uint16_t frame,JpegDecodeTarget &out,uint64_t instance=0){
    const auto *resource=jpegResource(m);Slot *slot=nullptr;for(auto &entry:slots_)if(entry->instance==instance){slot=entry.get();break;}
    if(slot&&resource==slot->cached&&frame==slot->frame&&m.width==slot->width&&m.height==slot->height){++stats_.cacheHits;slot->used=++clock_;out={slot->pixels,slot->capacity,slot->stride,slot->width,slot->height};return Status::Ok;}
    const uint8_t *bytes=nullptr;size_t count=0;JpegHeader header;
    if(!jpegFrame(m,frame,bytes,count)||!jpegHeader(bytes,count,header)||header.width!=m.width||header.height!=m.height)return failed(Status::InvalidArgument);
    if(!available())return failed(Status::Unsupported);
    const size_t alignedWidth=(size_t(m.width)+15)&~size_t(15),alignedHeight=(size_t(m.height)+15)&~size_t(15);
    if(alignedWidth>8388608u/2/alignedHeight)return failed(Status::InvalidArgument);
    const size_t required=alignedWidth*alignedHeight*2;if(required>budget_)return failed(Status::NoMemory);
    if(!slot){std::unique_ptr<Slot> fresh(new(std::nothrow) Slot);if(!fresh)return failed(Status::NoMemory);fresh->instance=instance;slot=fresh.get();slots_.push_back(std::move(fresh));}
    if(required>slot->capacity||(required<=SIZE_MAX/2&&slot->capacity>required*2)){
      outputBytes_-=slot->capacity;freeMemory(slot->pixels);slot->pixels=nullptr;slot->capacity=0;slot->cached=nullptr;
      if(!makeRoom(required,slot))return failed(Status::NoMemory);
      size_t allocated=0;void *memory=allocate(required,false,allocated);if(!memory)return failed(Status::NoMemory);
      if(allocated>budget_||!makeRoom(allocated,slot)){freeMemory(memory);return failed(Status::NoMemory);}
      slot->pixels=static_cast<uint16_t *>(memory);slot->capacity=allocated;outputBytes_+=allocated;
    }
    slot->used=++clock_;JpegDecodeTarget target{slot->pixels,slot->capacity,size_t(m.width)*2,m.width,m.height};slot->cached=nullptr;
    const Status status=handler_?handler_(context_,bytes,count,target):hardware(bytes,count,target,header.sampling);
    if(status!=Status::Ok)return failed(status);
    if(target.pixels!=slot->pixels||target.capacityBytes!=slot->capacity||target.width!=m.width||target.height!=m.height||target.strideBytes<size_t(m.width)*2||(target.strideBytes&1)||target.strideBytes>slot->capacity||size_t(m.height-1)>(slot->capacity-size_t(m.width)*2)/target.strideBytes)return failed(Status::InvalidArgument);
    slot->cached=resource;slot->frame=frame;slot->width=m.width;slot->height=m.height;slot->stride=target.strideBytes;++stats_.decodedFrames;out=target;return Status::Ok;
  }
 private:
  struct Slot {uint64_t instance=0,used=0;uint16_t *pixels=nullptr;size_t capacity=0,stride=0;const JpegMediaResource *cached=nullptr;uint16_t frame=0,width=0,height=0;};
  Status failed(Status status){++stats_.decodeFailures;return status;}
  bool makeRoom(size_t bytes,Slot *keep){
    while(outputBytes_>budget_-bytes){size_t oldest=slots_.size();for(size_t i=0;i<slots_.size();++i)if(slots_[i].get()!=keep&&(oldest==slots_.size()||slots_[i]->used<slots_[oldest]->used))oldest=i;
      if(oldest==slots_.size())return false;auto &slot=*slots_[oldest];outputBytes_-=slot.capacity;freeMemory(slot.pixels);slots_.erase(slots_.begin()+oldest);++stats_.evictedInstances;
    }return true;
  }
  static void *allocate(size_t bytes,bool input,size_t &allocated){
#if NICE_GFX_UI_HAS_HARDWARE_JPEG
    jpeg_decode_memory_alloc_cfg_t config{};config.buffer_direction=input?JPEG_DEC_ALLOC_INPUT_BUFFER:JPEG_DEC_ALLOC_OUTPUT_BUFFER;
    void *memory=jpeg_alloc_decoder_mem(bytes,&config,&allocated);if(memory&&allocated<bytes){heap_caps_free(memory);return nullptr;}return memory;
#else
    (void)input;allocated=bytes;return ::malloc(bytes);
#endif
  }
  static void freeMemory(void *memory){
#if NICE_GFX_UI_HAS_HARDWARE_JPEG
    heap_caps_free(memory);
#else
    ::free(memory);
#endif
  }
  Status hardware(const uint8_t *bytes,size_t count,JpegDecodeTarget &target,uint8_t sampling){
#if NICE_GFX_UI_HAS_HARDWARE_JPEG
    if(count>UINT32_MAX||target.capacityBytes>UINT32_MAX)return Status::InvalidArgument;
    if(!engine_){jpeg_decode_engine_cfg_t config{};config.timeout_ms=1000;const auto status=jpeg_new_decoder_engine(&config,&engine_);if(status!=ESP_OK)return status==ESP_ERR_NO_MEM?Status::NoMemory:Status::IoError;}
    const bool direct=jpegCanBorrowFlash(bytes,count);const uint8_t *bitStream=bytes;
    const auto copyInput=[&]()->Status{
      if(count>inputBytes_){size_t allocated=0;void *memory=allocate(count,true,allocated);if(!memory)return Status::NoMemory;freeMemory(input_);input_=static_cast<uint8_t *>(memory);inputBytes_=allocated;}
      const int64_t copyAt=esp_timer_get_time();
      memcpy(input_,bytes,count);if(inputBytes_>count)memset(input_+count,0,inputBytes_-count);
      stats_.copyMicros+=uint64_t(esp_timer_get_time()-copyAt);stats_.copiedInputBytes+=count;++stats_.copiedInputCalls;
      bitStream=input_;return Status::Ok;
    };
    if(!direct){const Status copied=copyInput();if(copied!=Status::Ok)return copied;}
    else ++stats_.directFlashCalls;
    jpeg_decode_cfg_t config{};config.output_format=JPEG_DECODE_OUT_FORMAT_RGB565;
    // SDK BGR means native little-endian RGB565 words (same convention as
    // the existing Korvo Map JPEG adapter's verified four-color fixture).
    config.rgb_order=JPEG_DEC_RGB_ELEMENT_ORDER_BGR;config.conv_std=JPEG_YUV_RGB_CONV_STD_BT601;
    // The driver consumes bitStream synchronously. Its immutable compiled
    // resource remains leased by the View snapshot until this call returns.
    uint32_t written=0;
    const auto process=[&]()->esp_err_t{
      const int64_t decodeAt=esp_timer_get_time();++stats_.hardwareDecodeCalls;
      const auto status=jpeg_decoder_process(engine_,&config,bitStream,uint32_t(count),reinterpret_cast<uint8_t *>(target.pixels),uint32_t(target.capacityBytes),&written);
      stats_.decodeMicros+=uint64_t(esp_timer_get_time()-decodeAt);return status;
    };
    auto status=process();
    // A driver revision may impose an additional input restriction. An
    // argument rejection occurs before its DMA transaction; retry once using
    // the original scratch path without reporting a failed decoded frame.
    if(direct&&status==ESP_ERR_INVALID_ARG){
      ++stats_.directFlashFallbacks;const Status copied=copyInput();if(copied!=Status::Ok)return copied;
      status=process();
    }
    if(status!=ESP_OK)return status==ESP_ERR_NO_MEM?Status::NoMemory:status==ESP_ERR_NOT_SUPPORTED?Status::Unsupported:Status::IoError;
    if(!jpegOutputStride(target.width,target.height,sampling,written,target.capacityBytes,target.strideBytes))return Status::IoError;
    return Status::Ok;
#else
    (void)bytes;(void)count;(void)target;(void)sampling;return Status::Unsupported;
#endif
  }
  void release(){for(auto &slot:slots_)freeMemory(slot->pixels);slots_.clear();freeMemory(input_);
#if NICE_GFX_UI_HAS_HARDWARE_JPEG
    if(engine_)jpeg_del_decoder_engine(engine_);
#endif
  }
  std::vector<std::unique_ptr<Slot>> slots_;size_t budget_=8388608,outputBytes_=0;uint64_t clock_=0;
  uint8_t *input_=nullptr;size_t inputBytes_=0;
  JpegDecodeHandler handler_=nullptr;void *context_=nullptr;JpegMediaStats stats_;
#if NICE_GFX_UI_HAS_HARDWARE_JPEG
  jpeg_decoder_handle_t engine_=nullptr;
#endif
};
} } }
