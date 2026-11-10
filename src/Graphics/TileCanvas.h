#pragma once
#include "Gfx.h"
#include "ChartIram.h"
#include "ChartBackdropGrid.h"
#ifndef NICE_GFX_UI_THIN_SOLID_FILL
#define NICE_GFX_UI_THIN_SOLID_FILL 1
#endif
#define NICE_GFX_UI_THIN_SOLID_FILL_VERSION 1
#include "../Fonts/Text.h"
#include "../Core/Geometry.h"
#include "../Display/Refresh.h"
#include "../Platform/ESP32/Rgb565P4Memory.h"
#include "WordFill16.h"
#ifndef NICE_GFX_TILE_WORD_FILL
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#define NICE_GFX_TILE_WORD_FILL 1
#else
#define NICE_GFX_TILE_WORD_FILL 0
#endif
#endif
#ifndef NICE_GFX_UI_CHART_ROW_MAJOR_STEEP
#define NICE_GFX_UI_CHART_ROW_MAJOR_STEEP 1
#endif
#ifndef NICE_GFX_UI_CHART_ROW_MAJOR_GRID
#define NICE_GFX_UI_CHART_ROW_MAJOR_GRID 1
#endif
#ifndef NICE_GFX_UI_CHART_COLUMN_STROKES
#define NICE_GFX_UI_CHART_COLUMN_STROKES 1
#endif
#ifndef NICE_GFX_TILE_AA_PALETTE
#if defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32P4)
#define NICE_GFX_TILE_AA_PALETTE 1
#else
#define NICE_GFX_TILE_AA_PALETTE 0
#endif
#endif
#ifndef NICE_GFX_TILE_ROUNDED_CENTER
#define NICE_GFX_TILE_ROUNDED_CENTER 1
#endif
#if (defined(ARDUINO_ARCH_ESP32) || NICE_GFX_NATIVE_IDF) && defined(CONFIG_IDF_TARGET_ESP32S31)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"
#endif

namespace nice_gfx {

// Non-owning RGB565 tile sink. Logical coordinates always refer to the whole
// display, while memory only contains the current clipped tile. All geometry
// and fonts use the normal NICE_GFX rasterizer; no Sprite or allocation needed.
class TileCanvas : public NICE_GFX {
 public:
  // Diagnostic-only, per-canvas chart component clocks. Clock-free calls are
  // inert. Scope closure prevents time outside renderChart being attributed.
  using ChartPhaseClock=uint32_t (*)();
  void setChartPhaseClock(ChartPhaseClock clock){if(chartPhaseClock_!=clock){chartPhaseClock_=clock;chartPhase_=-1;}}
  void chartProfilePhase(int phase){if(!chartPhaseClock_)return;const uint32_t now=chartPhaseClock_();
    if(chartPhase_>=0)chartPhaseUs_[chartPhase_]+=uint32_t(now-chartPhaseAt_);
    chartPhase_=phase>=0&&phase<5?phase:-1;chartPhaseAt_=now;}
  uint64_t chartPhaseMicros(unsigned phase)const{return phase<5?chartPhaseUs_[phase]:0;}
 private:
  ChartPhaseClock chartPhaseClock_=nullptr;int chartPhase_=-1;uint32_t chartPhaseAt_=0;uint64_t chartPhaseUs_[5]{};
 public:
  using NativeFillHandler=bool (*)(void *,const DirtyRect &,uint16_t *,size_t,uint16_t);
  // Optional blocking native writer. False requests the complete CPU fallback;
  // implementations must drain accepted writes and leave CPU-coherent output.
  void setNativeFillHandler(NativeFillHandler handler,void *context){nativeFill_=handler;nativeFillContext_=context;}
  // Exact factory only. View separately proves either PPA is unavailable or
  // this is its serialized fenced SDK back mapping before omitting the hook.
  // A capability authorizes only the currently mapped, fenced MIPI inactive
  // back allocation. Match the exact attached ROI rather than accepting a
  // same-sized generic/native/staging pointer. This query never touches pixels.
  bool chartBackdropMatchesFencedFrame(const NativeFramebuffer &frame)const{
#if NICE_GFX_UI_CHART_FUSED_PPA_FRAME
    if(chartColumnOwner_!=this||!data_||order_!=PixelOrder::Native||
       nativeFill_!=chartColumnPureFill_||nativeFillContext_!=chartColumnPureContext_||
       !frame.fencedAtomicMipiRmw||!frame.pixels||!frame.width||!frame.height||
       frame.stride<frame.width||frame.height>SIZE_MAX/frame.stride/sizeof(uint16_t)||
       tile_.x<0||tile_.y<0||tile_.width<=0||tile_.height<=0||
       uint64_t(tile_.x)+uint32_t(tile_.width)>frame.width||
       uint64_t(tile_.y)+uint32_t(tile_.height)>frame.height||stride_!=frame.stride)return false;
    const size_t bytes=frame.stride*frame.height*sizeof(uint16_t);
    const uintptr_t base=reinterpret_cast<uintptr_t>(frame.pixels);
    if(frame.bytes<bytes||base%alignof(uint16_t)||base>UINTPTR_MAX-bytes)return false;
    const size_t offset=(size_t(tile_.y)*frame.stride+size_t(tile_.x))*sizeof(uint16_t);
    return reinterpret_cast<uintptr_t>(data_)==base+offset;
#else
    (void)frame;return false;
#endif
  }
  bool prepareChartBackdropGrid(int32_t x,int32_t y,int32_t width,int32_t height,unsigned radius,
      unsigned horizontal,unsigned vertical,unsigned minor,uint16_t background,
      uint16_t majorColor,uint16_t minorColor,ChartBackdropGridPlan &out)const{
#if NICE_GFX_UI_CHART_FUSED_BACKDROP_GRID
    if(chartColumnOwner_!=this||!data_||order_!=PixelOrder::Native||
       nativeFill_!=chartColumnPureFill_||nativeFillContext_!=chartColumnPureContext_||
       width<2||height<2||width>2048||height>2048||radius>255||radius>unsigned(width/2)||radius>unsigned(height/2)||minor>5||horizontal>10||vertical>24||
       x<-32760||y<-32760||int64_t(x)+width>32760||int64_t(y)+height>32760||
       _xDatum||_yDatum)return false;
    const unsigned hs=horizontal*(minor+1),vs=vertical*(minor+1);
    if((horizontal&&hs>=64)||(vertical&&vs>=96))return false;
    auto &p=out;p.owner=this;p.target=data_;p.stride=stride_;p.x=x;p.y=y;p.width=width;p.height=height;p.radius=uint16_t(radius);
    p.tileX=tile_.x;p.tileY=tile_.y;p.background=stored(background);p.rowCount=p.columnCount=0;
    p.left=std::max(x,std::max(_vpX,tile_.x));p.right=std::min(x+width,std::min(_vpW,tile_.x+tile_.width));
    p.top=std::max(y,std::max(_vpY,tile_.y));p.bottom=std::min(y+height,std::min(_vpH,tile_.y+tile_.height));
    if(_vpOoB)p.right=p.left;
    if(horizontal)for(unsigned k=0;k<=hs;++k){p.rows[p.rowCount]=uint16_t((height-1)*k/hs);p.rowColors[p.rowCount++]=stored(k%(minor+1)==0?majorColor:minorColor);}
    if(vertical)for(unsigned k=0;k<=vs;++k){p.columns[p.columnCount]=uint16_t((width-1)*k/vs);p.columnColors[p.columnCount++]=stored(k%(minor+1)==0?majorColor:minorColor);}
    return true;
#else
    (void)x;(void)y;(void)width;(void)height;(void)radius;(void)horizontal;(void)vertical;(void)minor;(void)background;(void)majorColor;(void)minorColor;(void)out;return false;
#endif
  }
#if defined(CONFIG_IDF_TARGET_ESP32P4) && defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("O3","no-tree-vectorize","no-tree-slp-vectorize")))
#endif
  void paintChartBackdropGrid(const ChartBackdropGridPlan &p,const DirtyRect &cover,ChartBackdropGridResult &result){
#if NICE_GFX_UI_CHART_FUSED_BACKDROP_GRID
    // Only prepared plans from this still-attached exact canvas are accepted.
    if(p.owner!=this||p.target!=data_||p.stride!=stride_||p.tileX!=tile_.x||p.tileY!=tile_.y||
       p.right<=p.left||p.bottom<=p.top)return;
    const int32_t first=p.left-p.x,last=p.right-p.x;
    const int64_t coverEndX=int64_t(cover.x)+cover.width,coverEndY=int64_t(cover.y)+cover.height;
    const auto fill=[&](uint16_t *out,int32_t count,uint16_t color){
      if(count<=0)return;
      result.pixels+=uint32_t(count);
#if NICE_GFX_UI_RETAINED_CHART_INK
      if(retainedBackground_){const size_t offset=size_t(out-data_);const int32_t y=tile_.y+int32_t(offset/stride_),x=tile_.x+int32_t(offset%stride_);
        for(int32_t i=0;i<count;++i){if(!retainedBackground_->contains(x+i,y)||retainedBackground_->test(x+i,y)){out[i]=color;if(retainedBackground_->contains(x+i,y))++retainedRestored_;}else ++retainedSkipped_;}return;}
#endif
      if(reinterpret_cast<uintptr_t>(out)&2u){*out++=color;--count;}
      const uint32_t pair=uint32_t(color)|(uint32_t(color)<<16);
      // memcpy gives a defined alias-safe word store; alignment is explicit.
#if defined(__GNUC__) || defined(__clang__)
      auto *aligned=static_cast<uint16_t*>(__builtin_assume_aligned(out,4));
#else
      auto *aligned=out;
#endif
      while(count>=8){memcpy(aligned,&pair,4);memcpy(aligned+2,&pair,4);memcpy(aligned+4,&pair,4);memcpy(aligned+6,&pair,4);aligned+=8;count-=8;}
      while(count>=2){memcpy(aligned,&pair,4);aligned+=2;count-=2;}
      if(count)*aligned=color;
    };
    unsigned rowAt=0;
    for(int32_t y=p.top;y<p.bottom;++y){const int32_t localY=y-p.y;uint16_t horizontalColor=0;bool horizontal=false;
      // Original H-grid order: duplicate row positions keep the last color.
      while(rowAt<p.rowCount&&p.rows[rowAt]<=localY){if(p.rows[rowAt]==localY){horizontal=true;horizontalColor=p.rowColors[rowAt];}++rowAt;}
      const uint32_t distance=localY<p.radius?uint32_t(p.radius-localY):localY>=p.height-p.radius?uint32_t(localY-(p.height-p.radius-1)):0;
      const int32_t inset=distance?int32_t(roundInset(p.radius,distance)):0;
      const bool hole=cover.width>0&&cover.height>0&&y>=cover.y&&int64_t(y)<coverEndY;
      const int32_t holeLeft=hole?int32_t(std::min<int64_t>(last,std::max<int64_t>(first,int64_t(cover.x)-p.x))):last;
      const int32_t holeRight=hole?int32_t(std::max<int64_t>(first,std::min<int64_t>(last,coverEndX-p.x))):first;
      auto *row=p.target+size_t(y-p.tileY)*p.stride+size_t(p.left-p.tileX);
      const auto base=[&](int32_t a,int32_t b){if(b<=a)return;
        // Horizontal lines stop before the final x pixel (original width=pw).
        if(horizontal){const int32_t gridEnd=std::min(b,p.width-1);if(gridEnd>a)fill(row+a-first,gridEnd-a,horizontalColor);a=std::max(a,gridEnd);}
        if(b<=a)return;
        const int32_t begin=std::max(a,inset),end=std::min(b,p.width-inset);
        if(end<=begin){result.holes+=uint32_t(b-a);return;}
        result.holes+=uint32_t(begin-a+b-end);a=begin;b=end;
        if(hole&&holeRight>holeLeft){const int32_t l=std::max(a,holeLeft),r=std::min(b,holeRight);
          if(r>l){fill(row+a-first,l-a,p.background);fill(row+r-first,b-r,p.background);result.holes+=uint32_t(r-l);return;}}
        fill(row+a-first,b-a,p.background);
      };
      int32_t cursor=first;
      // Original V-grid comes after H-grid, and omits the final y row.
      if(localY<p.height-1)for(unsigned k=0;k<p.columnCount;){const int32_t x=p.columns[k];uint16_t color=p.columnColors[k++];
        while(k<p.columnCount&&p.columns[k]==x)color=p.columnColors[k++];
        if(x<first)continue;if(x>=last)break;base(cursor,x);fill(row+x-first,1,color);cursor=x+1;
      }
      base(cursor,last);++result.rows;
    }
#else
    (void)p;(void)cover;(void)result;
#endif
  }

  struct RetainedInkMask {DirtyRect rect{};uint8_t *bits=nullptr;
    bool contains(int32_t x,int32_t y)const{return bits&&x>=rect.x&&y>=rect.y&&int64_t(x)<int64_t(rect.x)+rect.width&&int64_t(y)<int64_t(rect.y)+rect.height;}
    size_t position(int32_t x,int32_t y)const{return size_t(y-rect.y)*size_t(rect.width)+size_t(x-rect.x);}
    bool test(int32_t x,int32_t y)const{if(!contains(x,y))return false;const size_t p=position(x,y);return (bits[p/8]>>(p%8))&1;}
    void mark(int32_t x,int32_t y){if(contains(x,y)){const size_t p=position(x,y);bits[p/8]|=uint8_t(1u<<(p%8));}}
  };
  void setRetainedExclusions(const DirtyRect *rects,size_t count){retainedExclude_=rects;retainedExcludeCount_=count;}
  void setRetainedBackgroundMask(RetainedInkMask *mask){retainedBackground_=mask;}
  void setRetainedInstrumentMask(RetainedInkMask *mask){retainedInstrument_=mask;}
  uint64_t retainedSkippedStores()const{return retainedSkipped_;}
  uint64_t retainedInkStores()const{return retainedInk_;}
  uint64_t retainedRestoredStores()const{return retainedRestored_;}
 private:
  const DirtyRect *retainedExclude_=nullptr;size_t retainedExcludeCount_=0;
  RetainedInkMask *retainedBackground_=nullptr,*retainedInstrument_=nullptr;
  uint64_t retainedSkipped_=0,retainedInk_=0,retainedRestored_=0;
  bool retainedExcluded(int32_t x,int32_t y)const{
#if NICE_GFX_UI_RETAINED_CHART_INK
    for(size_t i=0;i<retainedExcludeCount_;++i){const auto&r=retainedExclude_[i];if(x>=r.x&&y>=r.y&&int64_t(x)<int64_t(r.x)+r.width&&int64_t(y)<int64_t(r.y)+r.height)return true;}
#else
    (void)x;(void)y;
#endif
    return false;}
  bool retainedIntersects(const DirtyRect&r)const{
#if NICE_GFX_UI_RETAINED_CHART_INK
    for(size_t i=0;i<retainedExcludeCount_;++i){const auto&b=retainedExclude_[i];if(int64_t(r.x)<int64_t(b.x)+b.width&&int64_t(b.x)<int64_t(r.x)+r.width&&int64_t(r.y)<int64_t(b.y)+b.height&&int64_t(b.y)<int64_t(r.y)+r.height)return true;}
#else
    (void)r;
#endif
    return false;}
  void retainedMarkSpan(int32_t x,int32_t y,int32_t width){
#if NICE_GFX_UI_RETAINED_CHART_INK
    if(!retainedInstrument_)return;for(int32_t i=0;i<width;++i)if(retainedInstrument_->contains(x+i,y)){retainedInstrument_->mark(x+i,y);++retainedInk_;}
#else
    (void)x;(void)y;(void)width;
#endif
  }
 public:
  TileCanvas(int16_t width, int16_t height) : NICE_GFX(width, height) {}
  void resize(int16_t w, int16_t h) {
    _init_width = _width = w; _init_height = _height = h; detach();
  }
  Status attach(const DirtyRect &area, uint16_t *pixels, size_t capacity,
                size_t stride, PixelOrder order = PixelOrder::Wire) {
    detach();
    if (!pixels || reinterpret_cast<uintptr_t>(pixels) % alignof(uint16_t) ||
        area.x < 0 || area.y < 0 || area.width <= 0 || area.height <= 0 ||
        int64_t(area.x)+area.width > _width || int64_t(area.y)+area.height > _height ||
        stride < size_t(area.width) || size_t(area.width) > capacity ||
        size_t(area.height-1) > (capacity-size_t(area.width))/stride ||
        capacity > SIZE_MAX/sizeof(uint16_t) ||
        reinterpret_cast<uintptr_t>(pixels)>UINTPTR_MAX-capacity*sizeof(uint16_t)) return Status::InvalidArgument;
    data_ = pixels; stride_ = stride; tile_ = area; order_ = order;
    setViewport(area.x, area.y, area.width, area.height, false);
    return Status::Ok;
  }
  void detach() { data_ = nullptr; stream_ = false; resetViewport(); }
  bool attached() const { return data_ != nullptr; }
  // A text-only compositor may bypass this sink only when its checked native
  // framebuffer is the same backing region. Offscreen motion/cache canvases
  // must retain their own painter even while a screen target is also mapped.
  bool targetsNativeFramebuffer(const NativeFramebuffer &target) const {
    if(!data_||order_!=PixelOrder::Native||!target.pixels||!target.width||!target.height||
       target.stride<target.width||target.height>SIZE_MAX/target.stride/2||
       target.bytes<target.stride*target.height*2||tile_.x<0||tile_.y<0||
       int64_t(tile_.x)+tile_.width>int64_t(target.width)||
       int64_t(tile_.y)+tile_.height>int64_t(target.height)||stride_!=target.stride)return false;
    const size_t bytes=target.stride*target.height*2;
    if(reinterpret_cast<uintptr_t>(target.pixels)>UINTPTR_MAX-bytes)return false;
    const size_t offset=(size_t(tile_.y)*target.stride+size_t(tile_.x))*2;
    return reinterpret_cast<uintptr_t>(data_)==reinterpret_cast<uintptr_t>(target.pixels)+offset;
  }
  // Actual attached-tile and viewport intersection (right/bottom exclusive),
  // expressed in drawing coordinates even when a viewport datum is enabled.
  bool rasterClip(int &left,int &top,int &right,int &bottom) const {
    int32_t x0=0,y0=0,x1=0,y1=0;
    if(!textRenderClip(x0,y0,x1,y1))return false;
    left=int(x0-_xDatum);right=int(x1-_xDatum);top=int(y0-_yDatum);bottom=int(y1-_yDatum);return true;
  }
  // Exact RGB565-only fast path for the established radius-one chart brush.
  // Explicit owner authorization excludes derived/borrowed copies and unknown
  // native callbacks; no generic graphics primitive changes behavior.
  void enableChartColumnStrokesForExactCanvas(NativeFillHandler pureSmallFill=nullptr,void *context=nullptr){
    chartColumnOwner_=this;chartColumnPureFill_=pureSmallFill;chartColumnPureContext_=context;
  }
  // Consumes only the chart caller's bounded scratch arrays on success.
  // False leaves both arrays unchanged and retains every legacy callback.
  uint64_t chartGridBatches() const{return chartGridBatches_;}
  uint64_t chartGridPixels() const{return chartGridPixels_;}
#if defined(CONFIG_IDF_TARGET_ESP32P4) && defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("O3")))
#endif
  NICE_GFX_UI_CHART_IRAM_ATTR bool chartGridVerticalBatch(int32_t top,int32_t height,int32_t *positions,uint16_t *colors,size_t count){
#if NICE_GFX_UI_CHART_ROW_MAJOR_GRID
    if(chartColumnOwner_!=this||!data_||!positions||!colors||!count||count>96||
       top<-32760||top>32760||height<=0||height>=4096)return false;
    if(order_==PixelOrder::Native&&nativeFill_&&
       (nativeFill_!=chartColumnPureFill_||nativeFillContext_!=chartColumnPureContext_))return false;
    // Every original width-one VLine has fewer than 4096 pixels. The
    // factory's explicitly authorized Native handler purely rejects it.
    for(size_t i=0;i<count;++i)if(positions[i]<-32760||positions[i]>32760)return false;
    if(_vpOoB)return true;
    const int32_t left=std::max(_vpX,tile_.x),right=std::min(_vpW,tile_.x+tile_.width);
    const int32_t firstBound=std::max(_vpY,tile_.y),lastBound=std::min(_vpH,tile_.y+tile_.height);
    const int64_t first=std::max<int64_t>(int64_t(top)+_yDatum,firstBound),last=std::min<int64_t>(int64_t(top)+height+_yDatum,lastBound);
    if(left>=right||first>=last)return true;
    size_t visible=0;
    for(size_t i=0;i<count;++i){const int64_t x=int64_t(positions[i])+_xDatum;if(x<left||x>=right)continue;
      positions[visible]=int32_t(x)-tile_.x;colors[visible]=stored(colors[i]);++visible;
    }
    if(!visible)return true;
    const int32_t rows=int32_t(last-first),rowStart=int32_t(first)-tile_.y;
    // Preserve the source column order inside each row. Repeated x values
    // therefore retain the last authored grid color, including minor lines.
    for(int32_t y=0;y<rows;++y){uint16_t *row=data_+size_t(rowStart+y)*stride_;
      for(size_t i=0;i<visible;++i)row[size_t(positions[i])]=colors[i];
    }
    ++chartGridBatches_;chartGridPixels_+=uint64_t(uint32_t(rows))*visible;return true;
#else
    (void)top;(void)height;(void)positions;(void)colors;(void)count;return false;
#endif
  }

  bool chartHasExactOwner() const{return chartColumnOwner_==this;}
  uint64_t chartColumnBatches() const{return chartColumnBatches_;}
  uint64_t chartColumnPixels() const{return chartColumnPixels_;}
#if defined(CONFIG_IDF_TARGET_ESP32P4) && defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("O3")))
#endif
  NICE_GFX_UI_CHART_IRAM_ATTR bool chartStrokeRadiusOne(int32_t x1,int32_t y1,int32_t x2,int32_t y2,uint16_t color){
#if NICE_GFX_UI_CHART_COLUMN_STROKES
    if(chartColumnOwner_!=this||!data_||_vpOoB||x2<x1)return false;
    if(order_==PixelOrder::Native&&nativeFill_&&
       (nativeFill_!=chartColumnPureFill_||nativeFillContext_!=chartColumnPureContext_))return false;
    const int64_t wideDx=int64_t(x2)-x1,wideDy=y2>=y1?int64_t(y2)-y1:int64_t(y1)-y2;
    // Every original horizontal brush span has <4096 pixels in this domain.
    // View's explicitly authorized fill handler rejects those without work.
    if(wideDx>2048||wideDy>4096||x1<-32760||x2>32760||y1<-32760||y1>32760||y2<-32760||y2>32760)return false;
    const int32_t physicalLeft=std::max(_vpX,tile_.x),physicalTop=std::max(_vpY,tile_.y);
    const int32_t physicalRight=std::min(_vpW,tile_.x+tile_.width),physicalBottom=std::min(_vpH,tile_.y+tile_.height);
    const int64_t left64=int64_t(physicalLeft)-_xDatum,top64=int64_t(physicalTop)-_yDatum;
    const int64_t right64=int64_t(physicalRight)-_xDatum,bottom64=int64_t(physicalBottom)-_yDatum;
    if(left64<INT32_MIN||left64>INT32_MAX||right64<INT32_MIN||right64>INT32_MAX||
       top64<INT32_MIN||top64>INT32_MAX||bottom64<INT32_MIN||bottom64>INT32_MAX)return false;
    const int32_t left=int32_t(left64),top=int32_t(top64),right=int32_t(right64),bottom=int32_t(bottom64);
    if(left>=right||top>=bottom)return true;
    const int32_t dx=int32_t(wideDx),dy=int32_t(wideDy),sy=y2>=y1?1:-1;
    const int32_t first=std::max(x1-1,left),last=std::min(x2+1,right-1);
    if(first>last)return true;
#if NICE_GFX_UI_CHART_ROW_MAJOR_STEEP
    if(dy>dx){
      const int32_t minY=std::min(y1,y2),maxY=std::max(y1,y2);
      const uint16_t ink=stored(color);uint64_t pixels=0;
      const auto span=[&](int32_t y,int32_t center,int32_t radius){
        if(y<top||y>=bottom)return;
        const int32_t low=std::max(center-radius,left),high=std::min(center+radius,right-1);
        if(low>high)return;
        const int32_t px=int32_t(int64_t(low)+_xDatum),py=int32_t(int64_t(y)+_yDatum);
        uint16_t *target=data_+size_t(py-tile_.y)*stride_+size_t(px-tile_.x);
        retainedMarkSpan(px,py,high-low+1);for(int32_t x=low;x<=high;++x)*target++=ink;
        pixels+=uint32_t(high-low+1);
      };
      // A steep Bresenham segment has exactly one center in every integer
      // y row and adjacent centers differ in x by at most1. Consequently
      // each internal radius-one brush union is precisely centerX[-1,+1].
      // Only the two outside endpoint rows have a single covered texel.
      span(minY-1,sy>0?x1:x2,0);
      const int32_t rowFirst=std::max(minY,top),rowLast=std::min(maxY,bottom-1);
      if(rowFirst<=rowLast){
        const int32_t m=sy>0?rowFirst-y1:y1-rowFirst,den=2*dy,step=2*dx;
        const int32_t numerator=step*m+dy;
        int32_t offset=numerator/den,remainder=numerator%den;
        // All arithmetic is bounded by accepted endpoints: numerator<2^25,
        // den<=8192 and step<den. One carry/borrow replaces per-row division.
        for(int32_t y=rowFirst;y<=rowLast;++y){
          span(y,x1+offset,1);
          if(sy>0){remainder+=step;if(remainder>=den){remainder-=den;++offset;}}
          else {remainder-=step;if(remainder<0){remainder+=den;--offset;}}
        }
      }
      span(maxY+1,sy>0?x2:x1,0);
      ++chartColumnBatches_;chartColumnPixels_+=pixels;
#ifdef NICE_GFX_CHART_STEEP_TRACE
      NICE_GFX_CHART_STEEP_TRACE(dx,dy,pixels);
#endif
      return true;
    }
#endif

    struct Center {int32_t low=0,high=0;bool valid=false;};
    const auto center=[&](int32_t q){
      Center out;if(q<0||q>dx)return out;out.valid=true;
      if(!dx){out.low=std::min(y1,y2);out.high=std::max(y1,y2);return out;}
      if(dy<=dx){const int32_t offset=(2*dy*q+dx)/(2*dx);out.low=out.high=y1+sy*offset;return out;}
      // Exact inversion of x(m)=floor((2*dx*m+dy)/(2*dy)).
      // All products are bounded below2^25 by the accepted endpoint domain.
      const int32_t begin=q?std::max<int32_t>(0,(2*dy*q-dy+2*dx-1)/(2*dx)):0;
      const int32_t end=std::min(dy,(2*dy*q+dy-1)/(2*dx));
      const int32_t a=y1+sy*begin,b=y1+sy*end;out.low=std::min(a,b);out.high=std::max(a,b);return out;
    };
    int32_t pendingX=0,pendingWidth=0,pendingLow=0,pendingHigh=0;
    const uint16_t ink=stored(color);uint64_t pixels=0;
    const auto flush=[&](){
      if(!pendingWidth)return;
      const int32_t px=int32_t(int64_t(pendingX)+_xDatum),py=int32_t(int64_t(pendingLow)+_yDatum);
      uint16_t *firstPixel=data_+size_t(py-tile_.y)*stride_+size_t(px-tile_.x);
      const int32_t height=pendingHigh-pendingLow+1;
      for(int32_t y=0;y<height;++y){uint16_t *row=firstPixel+size_t(y)*stride_;retainedMarkSpan(px,py+y,pendingWidth);for(int32_t x=0;x<pendingWidth;++x)row[x]=ink;}
      pixels+=uint64_t(pendingWidth)*uint32_t(height);pendingWidth=0;
    };
    int32_t q=first-x1;Center previous=center(q-1),own=center(q),next=center(q+1);
    for(int32_t x=first;x<=last;++x,++q){
      int32_t low=INT32_MAX,high=INT32_MIN;
      if(own.valid){low=own.low-1;high=own.high+1;}
      if(previous.valid){low=std::min(low,previous.low);high=std::max(high,previous.high);}
      if(next.valid){low=std::min(low,next.low);high=std::max(high,next.high);}
      low=std::max(low,top);high=std::min(high,bottom-1);
      if(low<=high){
        if(pendingWidth&&low==pendingLow&&high==pendingHigh&&x==pendingX+pendingWidth)++pendingWidth;
        else {flush();pendingX=x;pendingWidth=1;pendingLow=low;pendingHigh=high;}
      }else flush();
      previous=own;own=next;next=center(q+2);
    }
    flush();++chartColumnBatches_;chartColumnPixels_+=pixels;
#ifdef NICE_GFX_CHART_COLUMN_TRACE
    NICE_GFX_CHART_COLUMN_TRACE(dx,dy,pixels);
#endif
    return true;
#else
    (void)x1;(void)y1;(void)x2;(void)y2;(void)color;return false;
#endif
  }

  // RGB565 images in either byte order, cropped before row copies. Source
  // storage is borrowed for this call and must not overlap the attached tile.
  Status blit(int32_t x, int32_t y, PixelView image) {
    if (!image.valid() || image.width>INT32_MAX || image.height>INT32_MAX) return Status::InvalidArgument;
    if (!data_ || _vpOoB) return Status::Ok;
    const uintptr_t source=reinterpret_cast<uintptr_t>(image.data), target=reinterpret_cast<uintptr_t>(data_);
    const size_t sourceBytes=((image.height-1)*image.stride+image.width)*2;
    const size_t targetBytes=((size_t(tile_.height)-1)*stride_+size_t(tile_.width))*2;
    if (source>UINTPTR_MAX-sourceBytes || target>UINTPTR_MAX-targetBytes ||
        (source<target+targetBytes && target<source+sourceBytes)) return Status::InvalidArgument;
    ClippedRect r;
    const int32_t left=_vpX>tile_.x?_vpX:tile_.x, top=_vpY>tile_.y?_vpY:tile_.y;
    const int32_t right=_vpW<tile_.x+tile_.width?_vpW:tile_.x+tile_.width;
    const int32_t bottom=_vpH<tile_.y+tile_.height?_vpH:tile_.y+tile_.height;
    if (!clipRect(x,y,int32_t(image.width),int32_t(image.height),_xDatum,_yDatum,left,top,right,bottom,r)) return Status::Ok;
    const uint8_t *src=image.data+2*(size_t(r.sourceY)*image.stride+size_t(r.sourceX));
    uint16_t *dst=data_+size_t(r.y-tile_.y)*stride_+size_t(r.x-tile_.x);
    for(int32_t row=0;row<r.height;++row) {
      if (image.order==order_) copyRow(dst,src,size_t(r.width));
      else {
#if defined(__GNUC__) && defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        swapCopyRow(dst,src,size_t(r.width));
#else
        for(int32_t col=0;col<r.width;++col) {
        uint16_t c;
        if (image.order==PixelOrder::Wire) c=uint16_t(uint16_t(src[2*col])<<8|src[2*col+1]);
        else memcpy(&c,src+2*col,sizeof(c));
        dst[col]=stored(c);
        }
#endif
      }
      if (row+1<r.height) { src+=image.stride*2; dst+=stride_; }
    }
    return Status::Ok;
  }
 protected:
  bool textRenderClip(int32_t &left,int32_t &top,int32_t &right,int32_t &bottom) const override {
    left=_vpX>tile_.x?_vpX:tile_.x;top=_vpY>tile_.y?_vpY:tile_.y;
    right=_vpW<tile_.x+tile_.width?_vpW:tile_.x+tile_.width;
    bottom=_vpH<tile_.y+tile_.height?_vpH:tile_.y+tile_.height;
    return data_ && !_vpOoB && right>left && bottom>top;
  }
  // Validate this complete glyph ROI once. Native or bounded bilinear coverage
  // touches the actual RGB565 underlay without per-run virtual span calls.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((optimize("O3")))
#endif
  bool textRenderGrayGlyph(const TextGlyph &glyph,uint16_t width,uint16_t height,int64_t left,int64_t top,
      const TextRasterClip &clip,uint16_t foreground,uint16_t background,bool opaque) override {
    if(!data_||_vpOoB||!glyph.data||glyph.encoding!=TextEncoding::Coverage4||
       !glyph.width||!glyph.height||!width||!height||glyph.stride<(uint32_t(glyph.width)+1)/2||
       clip.left>=clip.right||clip.top>=clip.bottom||clip.right>width||clip.bottom>height)return false;
    const bool resized=width!=glyph.width||height!=glyph.height;
    if(resized&&(width>256||height>256||glyph.width>256||glyph.height>256))return false;
    const size_t sourceRow=(uint32_t(glyph.width)+1)/2;
    if(size_t(glyph.height-1)>(SIZE_MAX-sourceRow)/glyph.stride)return false;
    const size_t sourceBytes=size_t(glyph.height-1)*glyph.stride+sourceRow;
    const size_t targetBytes=(size_t(tile_.height-1)*stride_+size_t(tile_.width))*2;
    const uintptr_t input=reinterpret_cast<uintptr_t>(glyph.data),output=reinterpret_cast<uintptr_t>(data_);
    if(input>UINTPTR_MAX-sourceBytes||output>UINTPTR_MAX-targetBytes||
       (input<output+targetBytes&&output<input+sourceBytes))return false;
    const int64_t x=left+clip.left,y=top+clip.top,right=left+clip.right,bottom=top+clip.bottom;
    if(x<tile_.x||y<tile_.y||right>int64_t(tile_.x)+tile_.width||bottom>int64_t(tile_.y)+tile_.height||
       x<_vpX||y<_vpY||right>_vpW||bottom>_vpH)return false;
    uint16_t palette[16];const uint16_t ink=stored(foreground);
    if(opaque)for(uint8_t c=0;c<16;++c)palette[c]=stored(textCoverageColor(foreground,background,c));
    uint16_t *target=data_+size_t(y-tile_.y)*stride_+size_t(x-tile_.x);
    if(resized){
      const int32_t dx=int32_t(width)*2,dy=int32_t(height)*2;
      const uint32_t den=uint32_t(dx)*dy;
      const auto floor32=[](int32_t n,int32_t d){return n>=0?n/d:-((-n+d-1)/d);};
      const int32_t nx=(int32_t(clip.left)*2+1)*glyph.width-width;
      const int32_t ny=(int32_t(clip.top)*2+1)*glyph.height-height;
      const int32_t firstSx=floor32(nx,dx);const uint32_t firstFx=uint32_t(nx-firstSx*dx);
      int32_t sy=floor32(ny,dy);uint32_t fy=uint32_t(ny-sy*dy);
      const int32_t stepX=int32_t(glyph.width)*2,stepY=int32_t(glyph.height)*2;
      const int32_t quotientX=stepX/dx,quotientY=stepY/dy;
      const uint32_t remainderX=uint32_t(stepX%dx),remainderY=uint32_t(stepY%dy);
      const auto sample=[&](int32_t sx,int32_t yy)
#if defined(__GNUC__) || defined(__clang__)
          __attribute__((always_inline))
#endif
          {
        if(sx<0||yy<0||sx>=glyph.width||yy>=glyph.height)return uint8_t(0);
        const uint8_t p=uint8_t(pgm_read_byte(glyph.data+size_t(yy)*glyph.stride+uint32_t(sx)/2));
        return uint8_t((sx&1)?p&15:p>>4);
      };
      for(uint16_t yy=clip.top;yy<clip.bottom;++yy){
        int32_t sx=firstSx;uint32_t fx=firstFx;uint16_t *dst=target;
        for(uint16_t xx=clip.left;xx<clip.right;++xx,++dst){
          const uint32_t a=uint32_t(sample(sx,sy))*(dx-fx)+uint32_t(sample(sx+1,sy))*fx;
          const uint32_t b=uint32_t(sample(sx,sy+1))*(dx-fx)+uint32_t(sample(sx+1,sy+1))*fx;
          const uint8_t coverage=uint8_t((a*(dy-fy)+b*fy+den/2)/den);
          if(opaque)*dst=palette[coverage];
          else if(coverage==15)*dst=ink;
          else if(coverage){
#if NICE_GFX_TILE_AA_PALETTE
            *dst=paletteBlend(foreground,*dst,coverage);
#else
            *dst=stored(textCoverageColor(foreground,stored(*dst),coverage));
#endif
          }
          sx+=quotientX;fx+=remainderX;if(fx>=uint32_t(dx)){fx-=dx;++sx;}
        }
        target+=stride_;sy+=quotientY;fy+=remainderY;if(fy>=uint32_t(dy)){fy-=dy;++sy;}
      }
      return true;
    }
    for(uint16_t sy=clip.top;sy<clip.bottom;++sy){
      const uint8_t *source=glyph.data+size_t(sy)*glyph.stride;
      uint16_t *dst=target;uint8_t packed=0;
      for(uint16_t sx=clip.left;sx<clip.right;++sx,++dst){
        if(!(sx&1)||sx==clip.left)packed=uint8_t(pgm_read_byte(source+sx/2));
        const uint8_t coverage=uint8_t((sx&1)?packed&15:packed>>4);
        if(opaque)*dst=palette[coverage];
        else if(coverage==15)*dst=ink;
        else if(coverage){
#if NICE_GFX_TILE_AA_PALETTE
          *dst=paletteBlend(foreground,*dst,coverage);
#else
          *dst=stored(textCoverageColor(foreground,stored(*dst),coverage));
#endif
        }
      }
      target+=stride_;
    }
    return true;
  }
  // Keep the span/palette hot loop optimized when sketches use -Os. The
  // checked clipping and exact coverage arithmetic remain unchanged.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((optimize("O3")))
#endif
  bool textRenderSpan(int32_t x,int32_t y,int32_t w,int32_t h,uint16_t color,uint8_t coverage) override {
    // renderTextGlyph supplies absolute, visible rectangles with a neutral
    // origin. Retain the borrowed tile's bounds even if a future caller uses
    // this hook outside that renderer; no pointer is formed before validation.
    if (!data_ || _vpOoB || w<=0 || h<=0 || x<tile_.x || y<tile_.y ||
        x>=tile_.x+tile_.width || y>=tile_.y+tile_.height ||
        w>tile_.x+tile_.width-x || h>tile_.y+tile_.height-y) return false;
    if (!coverage) return true;
    uint16_t *first=data_+size_t(y-tile_.y)*stride_+size_t(x-tile_.x);
    if (coverage==15) {
      const uint16_t c=stored(color);
      const size_t bytes=size_t(w)*sizeof(uint16_t);
#if NICE_GFX_TILE_WORD_FILL
      if(uint8_t(c)!=uint8_t(c>>8)&&size_t(w)*size_t(h)>=64&&
         detail::fillWordRect16(first,(size_t(tile_.height-1)*stride_+size_t(tile_.width))-size_t(first-data_),size_t(w),size_t(h),stride_,c))return true;
#endif
      if (uint8_t(c)==uint8_t(c>>8)) {
        if (size_t(w)==stride_) { memset(first,uint8_t(c),bytes*size_t(h)); return true; }
        for (int32_t row=0;row<h;++row) memset(first+size_t(row)*stride_,uint8_t(c),bytes);
      } else {
        for (int32_t col=0;col<w;++col) first[col]=c;
        for (int32_t row=1;row<h;++row) copyRow(first+size_t(row)*stride_,reinterpret_cast<const uint8_t *>(first),size_t(w));
      }
    } else {
      for (int32_t row=0;row<h;++row) {
        uint16_t *dst=first+size_t(row)*stride_;
        for (int32_t col=0;col<w;++col) {
#if NICE_GFX_TILE_AA_PALETTE // Exact AA palette memo.
          if(coverage<15) {dst[col]=paletteBlend(color,dst[col],coverage);continue;}
#endif
          dst[col]=stored(textCoverageColor(color,stored(dst[col]),coverage));
        }
      }
    }
    return true;
  }
 public:
  void begin_nin_write() override {}
  void end_nin_write() override {}
  void setWindow(int32_t x0,int32_t y0,int32_t x1,int32_t y1) override {
    stream_=data_ && x1>=x0 && y1>=y0;
    streamX_=streamLeft_=x0; streamY_=y0; streamRight_=x1; streamBottom_=y1;
  }
  void pushColor(uint16_t color) override {
    if (!stream_) return;
    // setWindow already receives absolute coordinates from clipWindow.
    if (!_vpOoB && streamX_>=_vpX && streamX_<_vpW && streamY_>=_vpY && streamY_<_vpH &&
        streamX_>=tile_.x && streamX_<tile_.x+tile_.width &&
        streamY_>=tile_.y && streamY_<tile_.y+tile_.height)
      {if(!retainedExcluded(streamX_,streamY_)){retainedMarkSpan(streamX_,streamY_,1);data_[size_t(streamY_-tile_.y)*stride_+size_t(streamX_-tile_.x)]=stored(color);}else ++retainedSkipped_;}
    if (streamX_==streamRight_) {
      streamX_=streamLeft_;
      if (streamY_==streamBottom_) stream_=false; else ++streamY_;
    } else ++streamX_;
  }

  using NICE_GFX::drawPixel;
  void drawPixel(int32_t x, int32_t y, uint32_t color) override {
    // Hot path for AA. Tile bounds remain authoritative even if the caller
    // resets the viewport inside its renderer.
    const int64_t px = int64_t(x)+_xDatum, py = int64_t(y)+_yDatum;
    if (!data_ || _vpOoB || px<_vpX || py<_vpY || px>=_vpW || py>=_vpH ||
        px<tile_.x || py<tile_.y || px>=tile_.x+tile_.width || py>=tile_.y+tile_.height) return;
    if(retainedExcluded(int32_t(px),int32_t(py))){++retainedSkipped_;return;}retainedMarkSpan(int32_t(px),int32_t(py),1);
    data_[size_t(py-tile_.y)*stride_+size_t(px-tile_.x)] = stored(uint16_t(color));
  }
  uint16_t readPixel(int32_t x, int32_t y) override {
    const int64_t px=int64_t(x)+_xDatum, py=int64_t(y)+_yDatum;
    if (!data_ || _vpOoB || px<_vpX || py<_vpY || px>=_vpW || py>=_vpH ||
        px<tile_.x || py<tile_.y || px>=tile_.x+tile_.width || py>=tile_.y+tile_.height) return 0;
    // Encoding is involutive: the same conversion decodes a stored word.
    return stored(data_[size_t(py-tile_.y)*stride_+size_t(px-tile_.x)]);
  }
  void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) override {
    if (!data_ || _vpOoB) return;
    ClippedRect r;
    const int32_t left=_vpX>tile_.x?_vpX:tile_.x, top=_vpY>tile_.y?_vpY:tile_.y;
    const int32_t right=_vpW<tile_.x+tile_.width?_vpW:tile_.x+tile_.width;
    const int32_t bottom=_vpH<tile_.y+tile_.height?_vpH:tile_.y+tile_.height;
    if (!clipRect(x,y,w,h,_xDatum,_yDatum,left,top,right,bottom,r)) return;
    const uint16_t c=stored(uint16_t(color));
    if(retainedIntersects(DirtyRect(r.x,r.y,r.width,r.height))){for(int32_t row=0;row<r.height;++row){auto*out=data_+size_t(r.y+row-tile_.y)*stride_+size_t(r.x-tile_.x);for(int32_t col=0;col<r.width;++col){if(retainedExcluded(r.x+col,r.y+row)){++retainedSkipped_;continue;}out[col]=c;}}return;}
    if(retainedInstrument_)for(int32_t row=0;row<r.height;++row)retainedMarkSpan(r.x,r.y+row,r.width);
    uint16_t *first=data_+size_t(r.y-tile_.y)*stride_+size_t(r.x-tile_.x);
    const size_t bytes=size_t(r.width)*sizeof(uint16_t);
    // Tiny raster spans cannot reach the native writer's 4096-pixel threshold
    // or PIE's 32-pixel minimum. Keep these frequent chart strokes local.
    if(r.height==1&&r.width<32){
      for(int32_t col=0;col<r.width;++col)first[col]=c;
      return;
    }
#if NICE_GFX_UI_THIN_SOLID_FILL
    // The exact View factory authorizes only its pure rejecting <4096-pixel
    // writer. Unknown/accepting hooks and any Wire hook retain their old path.
    // Dense spans may use the existing whole-span PIE fill; leave them alone.
    if(r.width<=2&&size_t(r.width)<stride_&&uint64_t(r.width)*uint32_t(r.height)<4096&&
       (!nativeFill_||(order_==PixelOrder::Native&&chartColumnOwner_==this&&
        nativeFill_==chartColumnPureFill_&&nativeFillContext_==chartColumnPureContext_))){
      for(int32_t row=0;row<r.height;++row){uint16_t *target=first+size_t(row)*stride_;target[0]=c;if(r.width==2)target[1]=c;}
#ifdef NICE_GFX_UI_THIN_SOLID_FILL_TRACE
      NICE_GFX_UI_THIN_SOLID_FILL_TRACE(r.width,r.height);
#endif
      return;
    }
#endif
    if(order_==PixelOrder::Native&&nativeFill_&&nativeFill_(nativeFillContext_,DirtyRect(r.x,r.y,r.width,r.height),first,stride_,uint16_t(color)))return;
    if(size_t(r.width)==stride_&&platform::rgb565_p4_memory::fill(first,size_t(r.width)*r.height,c))return;
    if(platform::rgb565_p4_memory::fill(first,size_t(r.width),c)){
      for(int32_t row=1;row<r.height;++row)if(!platform::rgb565_p4_memory::fill(first+size_t(row)*stride_,size_t(r.width),c))
        for(int32_t col=0;col<r.width;++col)first[size_t(row)*stride_+col]=c;
      return;
    }
#if NICE_GFX_TILE_WORD_FILL
    if(uint8_t(c)!=uint8_t(c>>8)&&size_t(r.width)*size_t(r.height)>=64&&
       detail::fillWordRect16(first,(size_t(tile_.height-1)*stride_+size_t(tile_.width))-size_t(first-data_),size_t(r.width),size_t(r.height),stride_,c))return;
#endif
    if (uint8_t(c)==uint8_t(c>>8)) {
      if (size_t(r.width)==stride_) { memset(first,uint8_t(c),bytes*size_t(r.height)); return; }
      for(int32_t row=0;row<r.height;++row) memset(first+size_t(row)*stride_,uint8_t(c),bytes);
    } else {
      for(int32_t col=0;col<r.width;++col) first[col]=c;
      for(int32_t row=1;row<r.height;++row) copyRow(first+size_t(row)*stride_,reinterpret_cast<const uint8_t *>(first),size_t(r.width));
    }
  }
  // Keep the original scanline raster for every intersecting triangle. Arc
  // segments outside a borrowed tile need no divisions or rejected spans,
  // even when a renderer temporarily enlarges the logical viewport.
  void fillTriangle(int32_t x0,int32_t y0,int32_t x1,int32_t y1,int32_t x2,int32_t y2,uint32_t color) {
    if(!data_ || _vpOoB)return;
    const int32_t top=_vpY>tile_.y?_vpY:tile_.y;
    const int32_t bottom=_vpH<tile_.y+tile_.height?_vpH:tile_.y+tile_.height;
    const int32_t minY=y0<y1?(y0<y2?y0:y2):(y1<y2?y1:y2);
    const int32_t maxY=y0>y1?(y0>y2?y0:y2):(y1>y2?y1:y2);
    if(bottom<=top || int64_t(maxY)+_yDatum<top || int64_t(minY)+_yDatum>=bottom)return;
    const int32_t left=_vpX>tile_.x?_vpX:tile_.x;
    const int32_t right=_vpW<tile_.x+tile_.width?_vpW:tile_.x+tile_.width;
    const int32_t minX=x0<x1?(x0<x2?x0:x2):(x1<x2?x1:x2);
    const int32_t maxX=x0>x1?(x0>x2?x0:x2):(x1>x2?x1:x2);
    if(right<=left || int64_t(maxX)+_xDatum<left || int64_t(minX)+_xDatum>=right)return;
    NICE_GFX::fillTriangle(x0,y0,x1,y1,x2,y2,color);
  }
  // Hide the nonvirtual display implementation: a tile needs only its visible
  // rows, rather than both complete midpoint arcs and hundreds of rejected
  // fillRect calls. The two octants below recover exactly those midpoint spans.
  void fillRoundRect(int32_t x,int32_t y,int32_t w,int32_t h,int32_t radius,uint32_t color) {
    if(!data_ || _vpOoB)return;
    ClippedRect r;
    const int32_t left=_vpX>tile_.x?_vpX:tile_.x,top=_vpY>tile_.y?_vpY:tile_.y;
    const int32_t right=_vpW<tile_.x+tile_.width?_vpW:tile_.x+tile_.width;
    const int32_t bottom=_vpH<tile_.y+tile_.height?_vpH:tile_.y+tile_.height;
    if(!clipRect(x,y,w,h,_xDatum,_yDatum,left,top,right,bottom,r))return;
    if(!radius){fillRect(x,y,w,h,color);return;}
    // Keep the existing display behavior for unsupported corner geometry.
    if(radius<0 || radius>w/2 || radius>h/2){NICE_GFX::fillRoundRect(x,y,w,h,radius,color);return;}
    const uint16_t c=stored(uint16_t(color));
    uint16_t *first=data_+size_t(r.y-tile_.y)*stride_+size_t(r.x-tile_.x);
#if NICE_GFX_TILE_ROUNDED_CENTER
    // The exact midpoint coverage is rectangular between the two corner
    // bands. Submit that visible interior once, sharing fillRect's bounded
    // native writer and row-copy fallbacks; only the arc rows need scanlines.
    const int32_t centerTop=r.sourceY>radius?r.sourceY:radius;
    const int32_t centerBottom=r.sourceY+r.height<h-radius?r.sourceY+r.height:h-radius;
    if(centerBottom>centerTop)fillRect(x+r.sourceX,y+centerTop,r.width,centerBottom-centerTop,color);
#endif
    for(int32_t row=0;row<r.height;++row) {
      const int32_t sy=r.sourceY+row;
#if NICE_GFX_TILE_ROUNDED_CENTER
      if(sy>=centerTop&&sy<centerBottom)continue;
#endif
      const uint32_t distance=sy<radius?uint32_t(radius-sy):sy>=h-radius?uint32_t(sy-(h-radius-1)):0;
      const int32_t inset=distance?int32_t(roundInset(uint32_t(radius),distance)):0;
      const int32_t begin=inset>r.sourceX?inset:r.sourceX;
      const int32_t end=w-inset<r.sourceX+r.width?w-inset:r.sourceX+r.width;
      if(end<=begin)continue;
      uint16_t *dst=first+size_t(row)*stride_+size_t(begin-r.sourceX);
      const int32_t count=end-begin;
      if(retainedIntersects(DirtyRect(x+begin+_xDatum,y+sy+_yDatum,count,1))){for(int32_t col=0;col<count;++col){if(retainedExcluded(x+begin+_xDatum+col,y+sy+_yDatum)){++retainedSkipped_;continue;}dst[col]=c;}continue;}
      retainedMarkSpan(x+begin+_xDatum,y+sy+_yDatum,count);
      if(uint8_t(c)==uint8_t(c>>8))memset(dst,uint8_t(c),size_t(count)*2);
      else for(int32_t col=0;col<count;++col)dst[col]=c;
    }
  }
  // Opaque linear background, using a compiler-generated flash RGB565 palette
  // and Q16 palette coordinates. Geometry must fit signed 32-bit arithmetic
  // over the original rectangle (the compiled UI validates it on mount).
  // No heap, temporary bitmap, trigonometry, division or virtual pixel calls.
  // Rounded coverage exactly matches NICE_GFX::fillRoundRect's midpoint spans.
  void fillLinearGradientRoundRect(int32_t x, int32_t y, int32_t w, int32_t h,
                                  uint8_t radius, const uint16_t *colors,
                                  int32_t origin, int32_t stepX, int32_t stepY,bool dither=false) {
    if (!data_ || _vpOoB || !colors || w<=0 || h<=0) return;
    ClippedRect visible;
    const int32_t left=_vpX>tile_.x?_vpX:tile_.x, top=_vpY>tile_.y?_vpY:tile_.y;
    const int32_t right=_vpW<tile_.x+tile_.width?_vpW:tile_.x+tile_.width;
    const int32_t bottom=_vpH<tile_.y+tile_.height?_vpH:tile_.y+tile_.height;
    if (!clipRect(x,y,w,h,_xDatum,_yDatum,left,top,right,bottom,visible)) return;
    int32_t r=radius;
    if(r>w/2)r=w/2;if(r>h/2)r=h/2;
    if(!r) { gradientRect(x,y,w,h,colors,origin,stepX,stepY,dither,0); return; }
    gradientRect(x,y+r,w,h-2*r,colors,origin+r*stepY,stepX,stepY,dither,uint8_t((r&1)<<1));
    // Most small dirty tiles lie entirely in the full-width central strip.
    // Do not walk either arc when no rounded row intersects this tile.
    if(visible.sourceY>=r&&visible.sourceY+visible.height<=h-r)return;
    const int32_t originalRadius=r;
    int32_t f=1-r,ddX=1,ddY=-2*r,dy=0;
    const auto span=[&](int32_t inset,int32_t row) {
      if(row<visible.sourceY||row>=visible.sourceY+visible.height||
         inset>=visible.sourceX+visible.width||w-inset<=visible.sourceX)return;
      gradientRect(x+inset,y+row,w-2*inset,1,colors,
                   origin+inset*stepX+row*stepY,stepX,stepY,dither,uint8_t((inset&1)|((row&1)<<1)));
    };
    while(dy<r) {
      if(f>=0) {
        span(originalRadius-dy,originalRadius-r);
        span(originalRadius-dy,h-originalRadius-1+r);
        --r;ddY+=2;f+=ddY;
      }
      ++dy;ddX+=2;f+=ddX;
      span(originalRadius-r,originalRadius-dy);
      span(originalRadius-r,h-originalRadius-1+dy);
    }
  }
 private:
  static uint32_t swapCopyPair(uint32_t value) {
    return ((value&0x00ff00ffu)<<8)|((value>>8)&0x00ff00ffu);
  }
  // Cross-order copy streams aligned word stores even when a clipped source
  // begins on the second halfword or an odd byte. No alignment-down read or
  // one-past source fetch is permitted; odd tails use exact two-byte reads.
  static void swapCopyRow(uint16_t *destination,const uint8_t *source,size_t pixels) {
#if defined(__GNUC__)
    typedef uint32_t CopyWord __attribute__((__may_alias__));
    if((reinterpret_cast<uintptr_t>(destination)&2)&&pixels){*destination++=uint16_t(source[0]<<8|source[1]);source+=2;--pixels;}
    auto *out=reinterpret_cast<CopyWord *>(destination);
    if(reinterpret_cast<uintptr_t>(source)&1){
      while(pixels>=2){*out++=uint32_t(source[0])<<8|source[1]|uint32_t(source[2])<<24|uint32_t(source[3])<<16;source+=4;pixels-=2;}
      if(pixels)*reinterpret_cast<uint16_t *>(out)=uint16_t(source[0]<<8|source[1]);return;
    }
    if(!(reinterpret_cast<uintptr_t>(source)&2)){
      const auto *in=reinterpret_cast<const CopyWord *>(source);
      while(pixels>=8){const uint32_t a=in[0],b=in[1],c=in[2],d=in[3];out[0]=swapCopyPair(a);out[1]=swapCopyPair(b);out[2]=swapCopyPair(c);out[3]=swapCopyPair(d);in+=4;out+=4;pixels-=8;}
      while(pixels>=2){*out++=swapCopyPair(*in++);pixels-=2;}
      if(pixels){const auto *last=reinterpret_cast<const uint8_t *>(in);*reinterpret_cast<uint16_t *>(out)=uint16_t(last[0]<<8|last[1]);}return;
    }
    uint16_t carry;__builtin_memcpy(&carry,__builtin_assume_aligned(source,2),2);
    const auto *in=reinterpret_cast<const CopyWord *>(source+2);
    while(pixels>=9){const uint32_t a=in[0],b=in[1],c=in[2],d=in[3];
      out[0]=swapCopyPair(uint32_t(carry)|(a<<16));out[1]=swapCopyPair((a>>16)|(b<<16));
      out[2]=swapCopyPair((b>>16)|(c<<16));out[3]=swapCopyPair((c>>16)|(d<<16));
      carry=uint16_t(d>>16);in+=4;out+=4;pixels-=8;
    }
    while(pixels>=3){const uint32_t value=*in++;*out++=swapCopyPair(uint32_t(carry)|(value<<16));carry=uint16_t(value>>16);pixels-=2;}
    if(pixels){auto *tail=reinterpret_cast<uint16_t *>(out);tail[0]=uint16_t(carry>>8|carry<<8);
      if(pixels==2){const auto *last=reinterpret_cast<const uint8_t *>(in);tail[1]=uint16_t(last[0]<<8|last[1]);}}
#else
    for(size_t i=0;i<pixels;++i)destination[i]=uint16_t(source[2*i]<<8|source[2*i+1]);
#endif
  }
#if (defined(ARDUINO_ARCH_ESP32) || NICE_GFX_NATIVE_IDF) && defined(CONFIG_IDF_TARGET_ESP32S31) && defined(__GNUC__)
  // Core 1 owns S31's PIE coprocessor. These caller-saved vector registers
  // are used only in a normal, yieldable task; IRQ/critical/Core 0 use C++.
  static __attribute__((noinline)) IRAM_ATTR void pieCopy64(uint16_t *&dst,const uint8_t *&src,size_t blocks) {
    asm volatile(
      "1:\n"
      "esp.vld.128.ip q0, %[src], 16\n"
      "esp.vld.128.ip q1, %[src], 16\n"
      "esp.vld.128.ip q2, %[src], 16\n"
      "esp.vld.128.ip q3, %[src], 16\n"
      "esp.vst.128.ip q0, %[dst], 16\n"
      "esp.vst.128.ip q1, %[dst], 16\n"
      "esp.vst.128.ip q2, %[dst], 16\n"
      "esp.vst.128.ip q3, %[dst], 16\n"
      "addi %[blocks], %[blocks], -1\n"
      "bnez %[blocks], 1b\n"
      : [dst] "+&r"(dst), [src] "+&r"(src), [blocks] "+&r"(blocks) : : "memory");
  }
#endif
  static void copyRow(uint16_t *dst,const uint8_t *src,size_t pixels) {
    if(!pixels)return;
    if(platform::rgb565_p4_memory::copy(dst,src,pixels))return;
#if (defined(ARDUINO_ARCH_ESP32) || NICE_GFX_NATIVE_IDF) && defined(CONFIG_IDF_TARGET_ESP32S31) && defined(__GNUC__)
    if(pixels>=32 && !((reinterpret_cast<uintptr_t>(dst)|reinterpret_cast<uintptr_t>(src))&15) &&
       xPortGetCoreID()==1 && xPortCanYield()) {
      const size_t copied=pixels&~size_t(31);pieCopy64(dst,src,copied/32);pixels-=copied;
      if(!pixels)return;
    }
#endif
#if (defined(ARDUINO_ARCH_ESP32) || NICE_GFX_NATIVE_IDF) && defined(__GNUC__) && defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    // Page surfaces may be two-byte offset after a slide. ESP's libc memcpy
    // then loses its aligned word path; stream aligned PSRAM words instead.
    // GCC's may_alias type permits RGB565 byte/halfword storage to be copied
    // as words without changing its representation or violating aliasing.
    typedef uint32_t CopyWord __attribute__((__may_alias__));
    if(reinterpret_cast<uintptr_t>(src)&1){memcpy(dst,src,pixels*2);return;}
    if(reinterpret_cast<uintptr_t>(dst)&2) {
      uint16_t first;__builtin_memcpy(&first,__builtin_assume_aligned(src,2),2);
      *dst++=first;src+=2;if(!--pixels)return;
    }
    CopyWord *out=reinterpret_cast<CopyWord *>(dst);
    if(!(reinterpret_cast<uintptr_t>(src)&2)) {
      const CopyWord *in=reinterpret_cast<const CopyWord *>(src);
      while(pixels>=8){out[0]=in[0];out[1]=in[1];out[2]=in[2];out[3]=in[3];out+=4;in+=4;pixels-=8;}
      while(pixels>=2){*out++=*in++;pixels-=2;}
      if(pixels){uint16_t last;__builtin_memcpy(&last,__builtin_assume_aligned(in,2),2);*reinterpret_cast<uint16_t *>(out)=last;}
      return;
    }
    uint16_t carry;__builtin_memcpy(&carry,__builtin_assume_aligned(src,2),2);
    const CopyWord *in=reinterpret_cast<const CopyWord *>(src+2);
    // Each word reads one future pixel, so the nine-pixel bound protects the
    // last row and the three-pixel bound protects its remaining pair loop.
    // Never align the source down or read before the borrowed image.
    while(pixels>=9) {
      const uint32_t a=in[0],b=in[1],c=in[2],d=in[3];
      out[0]=uint32_t(carry)|(a<<16);out[1]=(a>>16)|(b<<16);
      out[2]=(b>>16)|(c<<16);out[3]=(c>>16)|(d<<16);
      carry=uint16_t(d>>16);out+=4;in+=4;pixels-=8;
    }
    while(pixels>=3){const uint32_t word=*in++;*out++=uint32_t(carry)|(word<<16);carry=uint16_t(word>>16);pixels-=2;}
    uint16_t *tail=reinterpret_cast<uint16_t *>(out);tail[0]=carry;
    if(pixels==2){uint16_t last;__builtin_memcpy(&last,__builtin_assume_aligned(in,2),2);tail[1]=last;}
#else
    memcpy(dst,src,pixels*2);
#endif
  }
  template<class UInt> static uint32_t integerRoot(UInt value) {
    UInt root=0,bit=UInt(1)<<(sizeof(UInt)*8-2);
    while(bit>value)bit>>=2;
    while(bit) {
      if(value>=root+bit){value-=root+bit;root=(root>>1)+bit;}
      else root>>=1;
      bit>>=2;
    }
    return uint32_t(root);
  }
  template<class UInt> static uint32_t roundInsetImpl(uint32_t radius,uint32_t distance) {
    const UInt square=UInt(distance)*distance,remainder=UInt(radius)*radius-square;
    uint32_t extent;
    if(square<=remainder) {
      extent=integerRoot(remainder);
      if(remainder-UInt(extent)*extent>extent)++extent; // nearest integer in the side octant
    } else extent=integerRoot(remainder+distance-1); // widest span in the cap octant
    return radius-extent;
  }
  static uint32_t roundInset(uint32_t radius,uint32_t distance) {
    return radius<=65535?roundInsetImpl<uint32_t>(radius,distance):roundInsetImpl<uint64_t>(radius,distance);
  }
  static uint8_t gradientIndex(int32_t coordinate) {
    coordinate+=32768;
    return coordinate<=0?0:coordinate>=(255<<16)?255:uint8_t(coordinate>>16);
  }
  void gradientRect(int32_t x,int32_t y,int32_t w,int32_t h,const uint16_t *colors,
                    int32_t origin,int32_t stepX,int32_t stepY,bool dither,uint8_t phase) {
    ClippedRect r;
    const int32_t left=_vpX>tile_.x?_vpX:tile_.x,top=_vpY>tile_.y?_vpY:tile_.y;
    const int32_t right=_vpW<tile_.x+tile_.width?_vpW:tile_.x+tile_.width;
    const int32_t bottom=_vpH<tile_.y+tile_.height?_vpH:tile_.y+tile_.height;
    if(!clipRect(x,y,w,h,_xDatum,_yDatum,left,top,right,bottom,r))return;
    uint16_t *first=data_+size_t(r.y-tile_.y)*stride_+size_t(r.x-tile_.x);
    int32_t rowCoordinate=origin+r.sourceX*stepX+r.sourceY*stepY;
    phase^=uint8_t((r.sourceX&1)|((r.sourceY&1)<<1));
    // Horizontal gradients evaluate one row (two with ordered dithering),
    // then copy at memory bandwidth. Vertical gradients use one color or one
    // alternating color pair per row. Diagonals use prequantized flash LUTs.
    const int32_t period=dither?2:1,rows=stepY?r.height:(r.height<period?r.height:period);
    for(int32_t row=0;row<rows;++row) {
      uint16_t *dst=first+size_t(row)*stride_;
      if(!stepX) {
        const int index=gradientIndex(rowCoordinate);
        const uint16_t c=stored(pgm_read_word(colors+(dither?index*4+phase:index)));
        if(dither) {
          const uint16_t next=stored(pgm_read_word(colors+index*4+(phase^1)));
          int32_t col=0;
          for(;col+1<r.width;col+=2){dst[col]=c;dst[col+1]=next;}
          if(col<r.width)dst[col]=c;
        } else if(uint8_t(c)==uint8_t(c>>8))memset(dst,uint8_t(c),size_t(r.width)*2);
        else for(int32_t col=0;col<r.width;++col)dst[col]=c;
      } else {
        if(dither) {
          if(order_==PixelOrder::Native)gradientRow<false,true>(dst,r.width,colors,rowCoordinate,stepX,phase);
          else gradientRow<true,true>(dst,r.width,colors,rowCoordinate,stepX,phase);
        } else {
          if(order_==PixelOrder::Native)gradientRow<false,false>(dst,r.width,colors,rowCoordinate,stepX,0);
          else gradientRow<true,false>(dst,r.width,colors,rowCoordinate,stepX,0);
        }
      }
      rowCoordinate+=stepY;
      phase^=2;
    }
    if(!stepY)for(int32_t row=period;row<r.height;++row)
      copyRow(first+size_t(row)*stride_,reinterpret_cast<const uint8_t *>(first+size_t(dither?(row&1):0)*stride_),size_t(r.width));
  }
  template<bool Wire,bool Dither> static void gradientRow(uint16_t *dst,int32_t width,const uint16_t *colors,int32_t coordinate,int32_t step,uint8_t phase) {
    const int32_t rounded=coordinate+32768,last=rounded+(width-1)*step;
    // A linear span's endpoints bound every sample. Hoist clamping and pixel
    // order selection out of the normal loop instead of branching per pixel.
    if(uint32_t(rounded)<(256u<<16)&&uint32_t(last)<(256u<<16)) {
      uint32_t q=uint32_t(rounded);
      for(int32_t col=0;col<width;++col,q+=step) {
        const uint16_t c=pgm_read_word(colors+(Dither?((q>>16)*4+phase):(q>>16)));
        dst[col]=Wire?wireWord(c):c;
        if(Dither)phase^=1;
      }
    } else for(int32_t col=0;col<width;++col,coordinate+=step) {
      const int index=gradientIndex(coordinate);
      const uint16_t c=pgm_read_word(colors+(Dither?index*4+phase:index));
      dst[col]=Wire?wireWord(c):c;
      if(Dither)phase^=1;
    }
  }
  static uint16_t wireWord(uint16_t c) {
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    // ESP32 builds disable builtin memcpy: even a two-byte copy otherwise
    // becomes a libc call for every gradient pixel. Keep the wire bytes exact.
    return uint16_t((c<<8)|(c>>8));
#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return c;
#else
    const uint8_t bytes[2]={uint8_t(c>>8),uint8_t(c)};
    uint16_t result;memcpy(&result,bytes,sizeof(result));return result;
#endif
  }
  uint16_t stored(uint16_t c) const {
    if (order_ == PixelOrder::Native) return c;
    // Let the compiler resolve endianness, also on 8-bit targets.
    return wireWord(c);
  }
#if NICE_GFX_TILE_AA_PALETTE // Exact AA palette memo.
  // Each pixel still supplies its real stored underlay. Only an identical
  // foreground/background/order tuple can reuse exact RGB565 interpolation.
  uint16_t paletteBlend(uint16_t foreground,uint16_t background,uint8_t coverage) {
    const uint16_t bit=uint16_t(1u<<coverage);
    const bool same=foreground==paletteForeground_&&background==paletteBackground_&&order_==paletteOrder_;
    if(same&&(paletteValid_&bit))return palette_[coverage];
    const uint16_t result=stored(textCoverageColor(foreground,stored(background),coverage));
    if(!same){paletteForeground_=foreground;paletteBackground_=background;paletteOrder_=order_;paletteValid_=0;}
    palette_[coverage]=result;paletteValid_|=bit;return result;
  }
  uint16_t palette_[16]={},paletteForeground_=0,paletteBackground_=0,paletteValid_=0;
  PixelOrder paletteOrder_=PixelOrder::Wire;
#endif
  const TileCanvas *chartColumnOwner_=nullptr;
  NativeFillHandler chartColumnPureFill_=nullptr;void *chartColumnPureContext_=nullptr;
  uint64_t chartColumnBatches_=0,chartColumnPixels_=0;
  uint64_t chartGridBatches_=0,chartGridPixels_=0;
  NativeFillHandler nativeFill_=nullptr;void *nativeFillContext_=nullptr;
  uint16_t *data_ = nullptr;
  size_t stride_ = 0;
  DirtyRect tile_;
  PixelOrder order_ = PixelOrder::Wire;
  int32_t streamX_=0,streamY_=0,streamLeft_=0,streamRight_=0,streamBottom_=0;
  bool stream_=false;
};

} // namespace nice_gfx
