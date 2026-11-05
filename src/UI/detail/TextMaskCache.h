#pragma once
#include "../../Graphics/Gfx.h"
#include "../../Fonts/Text.h"
#include "FrameContext.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <limits.h>
#include <new>
#if defined(CONFIG_IDF_TARGET_ESP32S31) && (defined(ESP32)||defined(ARDUINO_ARCH_ESP32)||defined(ESP_PLATFORM))
#include <esp_heap_caps.h>
#endif

namespace nice_gfx { namespace htmlui { namespace detail {
// A position-independent text layout. Color, translation and inherited opacity
// belong to the eventual compositor, not to these immutable coverage pixels.
struct TextMaskLayout {
  const MonoFont *font=nullptr;
  const char *text=nullptr;
  const FrameTextLine *lines=nullptr;
  size_t lineCount=0;
  int32_t width=0,height=0,textY=0;
  uint8_t fontSize=1,textAlign=0;
  bool antialias=true;
};

struct TextMaskView {
  const uint8_t *data=nullptr;
  size_t width=0,height=0,stride=0,bytes=0;
  bool empty=true;

  TextMaskView()=default;

  TextMaskView(const uint8_t *pixels,size_t w,size_t h,size_t pitch,size_t allocated,bool noInk)
    :data(pixels),width(w),height(h),stride(pitch),bytes(allocated),empty(noInk) {}

  // Only this cache can certify immutable output from TextMaskCanvas. Raw
  // borrowed masks retain validation even when their bytes happen to be A8.
  bool trustedCoverage4() const {return data&&data==trustedData_&&bytes==trustedBytes_;}

  bool inkBounds(Rect &bounds) const {
    if(!trustedCoverage4())return false;bounds=inkBounds_;return true;
  }
 private:
  friend class TextMaskCache;
  const uint8_t *trustedData_=nullptr;
  size_t trustedBytes_=0;
  Rect inkBounds_{0,0,0,0};
};

// Reuse the existing drawChar/fallback/raster-run implementation. This sink
// receives its exact final 0..15 coverage; it never reads or stores RGB565.
class TextMaskCanvas final : public NICE_GFX {
 public:
  TextMaskCanvas(int16_t width,int16_t height,uint8_t *pixels,size_t stride)
    : NICE_GFX(width,height),pixels_(pixels),stride_(stride) {
    setViewport(0,0,width,height,false);
    setTextColor(0xffff); // Equal foreground/background selects transparency.
  }

  bool valid() const {return valid_;}

  bool empty() const {return !ink_;}

  Rect inkBounds() const {return inkBounds_;}

  void begin_nin_write() override {}
  void end_nin_write() override {}
 protected:
  bool textRenderSpan(int32_t x,int32_t y,int32_t width,int32_t height,
                      uint16_t,uint8_t coverage) override {
    if(!valid_||!coverage)return true;

    if(!pixels_||x<0||y<0||width<=0||height<=0||x>_width||y>_height||
       width>_width-x||height>_height-y||coverage>15){valid_=false;return true;}

    const uint8_t alpha=uint8_t(coverage*17);

    for(int32_t row=0;row<height;++row){
      uint8_t *target=pixels_+size_t(y+row)*stride_+size_t(x);

      for(int32_t col=0;col<width;++col){
        const uint8_t previous=target[col];

        // Two fractional glyph samples cannot be folded into one A8 sample
        // while preserving RGB565's intermediate rounding for every backdrop.
        // Reject that layout and keep its original ordered glyph painter.
        if(previous&&previous!=255&&alpha!=255){valid_=false;return true;}

        target[col]=previous==255?255:alpha;
      }
    }

    if(!ink_)inkBounds_={x,y,width,height};

    else {const int32_t right=std::max(inkBounds_.x+inkBounds_.width,x+width),bottom=std::max(inkBounds_.y+inkBounds_.height,y+height);
      inkBounds_.x=std::min(inkBounds_.x,x);inkBounds_.y=std::min(inkBounds_.y,y);inkBounds_.width=right-inkBounds_.x;inkBounds_.height=bottom-inkBounds_.y;}

    ink_=true;return true;
  }

 private:
  uint8_t *pixels_=nullptr;
  size_t stride_=0;
  bool valid_=true,ink_=false;
  Rect inkBounds_{0,0,0,0};
};

// Fixed metadata and a hard allocation budget. One aligned PSRAM allocation
// owns each mask, its UTF-8 key and its line descriptors. A View owns the cache;
// replacing/mounting that View destroys every allocation. Returned pixels are
// borrowed only until the next resolve/clear call, for a blocking PPA transfer.
class TextMaskCache {
 public:
  static constexpr size_t DefaultBudget=1024*1024,MaxEntries=64;

  explicit TextMaskCache(size_t budget=DefaultBudget):budget_(budget) {}

  ~TextMaskCache(){clear();}
  TextMaskCache(const TextMaskCache &)=delete;
  TextMaskCache &operator=(const TextMaskCache &)=delete;

  size_t bytes() const {return resident_;}

  size_t entries() const {size_t count=0;for(const auto &slot:slots_)if(slot.raw)++count;return count;}

  uint64_t hits() const {return hits_;}

  uint64_t builds() const {return builds_;}
  uint64_t rejected() const {return rejected_;}

  void clear(){for(auto &slot:slots_)discard(slot);}

  bool resolve(const TextMaskLayout &layout,TextMaskView &output) {
    output=TextMaskView{};

    size_t length=0;if(!validLayout(layout,length))return false;

    for(auto &slot:slots_)if(slot.raw&&slot.matches(layout,length)){
      slot.used=++clock_;++hits_;if(slot.unsupported)return false;
      output=slot.view();output.trustedData_=output.data;output.trustedBytes_=output.bytes;output.inkBounds_=slot.inkBounds;return true;
    }

    const size_t width=size_t(layout.width),height=size_t(layout.height);
    const size_t stride=(width+63)&~size_t(63);

    if(height>SIZE_MAX/stride)return false;

    const size_t maskBytes=stride*height,lineBytes=layout.lineCount*sizeof(FrameTextLine);

    if(maskBytes>SIZE_MAX-lineBytes||maskBytes+lineBytes>SIZE_MAX-length-1)return false;

    const size_t payload=maskBytes+lineBytes+length+1;

    if(payload>SIZE_MAX-126)return false;

    const size_t aligned=(payload+63)&~size_t(63),allocated=aligned+63;

    if(allocated>budget_)return false;

    Slot *slot=nullptr;

    for(auto &candidate:slots_)if(!candidate.raw){slot=&candidate;break;}

    if(!slot){slot=oldest(nullptr);if(!slot)return false;discard(*slot);}

    while(resident_>budget_-allocated){Slot *victim=oldest(slot);if(!victim)return false;discard(*victim);}

    void *raw=allocate(allocated);if(!raw)return false;
    uint8_t *pixels=reinterpret_cast<uint8_t *>((reinterpret_cast<uintptr_t>(raw)+63)&~uintptr_t(63));
    slot->raw=raw;slot->pixels=pixels;slot->allocated=allocated;slot->maskBytes=maskBytes;
    slot->stride=stride;slot->width=layout.width;slot->height=layout.height;slot->textY=layout.textY;
    slot->font=layout.font;slot->primary=*layout.font;slot->hasFallback=layout.font->fallback!=nullptr;

    if(slot->hasFallback)slot->fallback=*layout.font->fallback;

    slot->fontSize=layout.fontSize;slot->textAlign=layout.textAlign;slot->antialias=layout.antialias;
    slot->lineCount=layout.lineCount;slot->length=length;slot->used=++clock_;
    slot->lines=reinterpret_cast<FrameTextLine *>(pixels+maskBytes);

    for(size_t i=0;i<layout.lineCount;++i)new(slot->lines+i) FrameTextLine(layout.lines[i]);

    slot->text=reinterpret_cast<char *>(pixels+maskBytes+lineBytes);

    std::memcpy(slot->text,layout.text,length+1);resident_+=allocated;
    std::memset(pixels,0,maskBytes);
    TextMaskCanvas canvas(int16_t(layout.width),int16_t(layout.height),pixels,stride);
    canvas.setFont(layout.font);canvas.setTextSize(layout.fontSize);canvas.setAntialiasing(layout.antialias);

    int32_t y=layout.textY;const int32_t advance=int32_t(layout.font->height)*layout.fontSize;

    for(size_t n=0;n<layout.lineCount&&canvas.valid();++n){const auto &line=layout.lines[n];
      int32_t x=0;if(layout.textAlign==1)x=std::max<int32_t>(0,(layout.width-line.width)/2);

      else if(layout.textAlign==2)x=std::max<int32_t>(0,layout.width-line.width);

      for(size_t at=line.begin;at<line.end&&canvas.valid();){
        if(layout.text[at]=='\r'){++at;continue;}

        uint16_t consumed=0;const uint16_t cp=canvas.decodeUTF8(
          reinterpret_cast<uint8_t *>(const_cast<char *>(layout.text+at)),&consumed,
          uint16_t(std::min<size_t>(line.end-at,4)));

        if(!consumed){slot->unsupported=true;break;}at+=consumed;
        if(cp>=32&&cp!=127)x+=canvas.drawChar(cp,x,y);
      }

      y+=advance;
    }

    ++builds_;slot->unsupported=slot->unsupported||!canvas.valid();slot->empty=canvas.empty();slot->inkBounds=canvas.inkBounds();

    if(slot->unsupported){++rejected_;return false;}

    output=slot->view();output.trustedData_=output.data;output.trustedBytes_=output.bytes;output.inkBounds_=slot->inkBounds;return true;
  }

 private:
  static bool sameFont(const MonoFont &a,const MonoFont &b){
    return a.bitmap==b.bitmap&&a.codepoints==b.codepoints&&a.count==b.count&&a.width==b.width&&
      a.height==b.height&&a.baseline==b.baseline&&a.fallback==b.fallback&&a.bitsPerPixel==b.bitsPerPixel&&
      a.compactPunctuation==b.compactPunctuation&&a.advances==b.advances&&a.xOffset==b.xOffset;
  }

  struct Slot {
    void *raw=nullptr;uint8_t *pixels=nullptr;char *text=nullptr;FrameTextLine *lines=nullptr;
    const MonoFont *font=nullptr;MonoFont primary{},fallback{};
    size_t allocated=0,maskBytes=0,stride=0,lineCount=0,length=0;
    int32_t width=0,height=0,textY=0;
    Rect inkBounds{0,0,0,0};
    uint64_t used=0;uint8_t fontSize=0,textAlign=0;
    bool hasFallback=false,antialias=true,unsupported=false,empty=true;

    bool matches(const TextMaskLayout &value,size_t textLength) const {
      if(font!=value.font||!sameFont(primary,*value.font)||
         hasFallback!=(value.font->fallback!=nullptr)||(hasFallback&&!sameFont(fallback,*value.font->fallback))||
         width!=value.width||height!=value.height||textY!=value.textY||fontSize!=value.fontSize||
         textAlign!=value.textAlign||antialias!=value.antialias||lineCount!=value.lineCount||length!=textLength||
         std::memcmp(text,value.text,length)!=0)return false;

      for(size_t i=0;i<lineCount;++i)if(lines[i].begin!=value.lines[i].begin||lines[i].end!=value.lines[i].end||lines[i].width!=value.lines[i].width)return false;

      return true;
    }

    TextMaskView view() const {return{pixels,size_t(width),size_t(height),stride,maskBytes,empty};}
  };

  static bool validLayout(const TextMaskLayout &layout,size_t &length){
    if(!layout.font||!layout.font->bitmap||!layout.font->height||!layout.font->width||!layout.text||
       !layout.lines||!layout.lineCount||layout.lineCount>128||layout.width<=0||layout.height<=0||
       layout.width>INT16_MAX||layout.height>INT16_MAX||!layout.fontSize||layout.fontSize>7||
       layout.textAlign>2||layout.textY<0)return false;

    while(length<=2048&&layout.text[length])++length;if(length>2048)return false;

    for(size_t i=0;i<layout.lineCount;++i)if(layout.lines[i].begin>layout.lines[i].end||
       layout.lines[i].end>length||layout.lines[i].width<0)return false;

    return true;
  }

  static void *allocate(size_t bytes){
#if defined(CONFIG_IDF_TARGET_ESP32S31) && (defined(ESP32)||defined(ARDUINO_ARCH_ESP32)||defined(ESP_PLATFORM))
    return heap_caps_malloc(bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);

#else
    return std::malloc(bytes);

#endif
  }

  static void release(void *memory){
#if defined(CONFIG_IDF_TARGET_ESP32S31) && (defined(ESP32)||defined(ARDUINO_ARCH_ESP32)||defined(ESP_PLATFORM))
    heap_caps_free(memory);

#else
    std::free(memory);

#endif
  }

  void discard(Slot &slot){if(slot.raw){resident_-=slot.allocated;release(slot.raw);}slot=Slot{};}

  Slot *oldest(const Slot *protectedSlot){Slot *result=nullptr;for(auto &slot:slots_)if(slot.raw&& &slot!=protectedSlot&&(!result||slot.used<result->used))result=&slot;return result;}
  Slot slots_[MaxEntries];

  size_t budget_=0,resident_=0;uint64_t clock_=0,hits_=0,builds_=0,rejected_=0;
};
}}} // namespace nice_gfx::htmlui::detail
