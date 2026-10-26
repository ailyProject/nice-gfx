#pragma once
#include "../Backend.h"
#include "../../Graphics/MonoConvert.h"
#include <stdlib.h>

namespace nice_gfx {

enum class MonoPacking : uint8_t { PageLsb, RowMsb };

// One means white/lit in both formats. RGB565 tiles are consumed synchronously;
// the physical refresh owns only these small, private packed buffers.
class PackedMonoPanel : public Backend {
 public:
  ~PackedMonoPanel() override { free(storage_); free(backup_); }
  bool ready() const override { return ready_; }
  uint16_t width() const override { return rotation_ & 1 ? nativeHeight_ : nativeWidth_; }
  uint16_t height() const override { return rotation_ & 1 ? nativeWidth_ : nativeHeight_; }
  uint8_t rotation() const override { return rotation_; }
  int resetPin() const override { return reset_; }
  TransportInfo transportInfo() const override { return bus_.info(); }
  Status status() const override { return status_; }
  bool frameOpen() const override { return frameOpen_; }
  size_t packedBytes() const { return bytes_; }
  const uint8_t *packedPixels() const { return frame_; }
  // Changing conversion applies to future RGB writes, never silently mutates
  // already composed pixels; callers should invalidate their UI after changes.
  void setMonoOptions(MonoOptions options) { options_ = options; }
  MonoOptions monoOptions() const { return options_; }
  Status setRotation(uint8_t rotation) override {
    if (!ready_) return record(Status::NotInitialized);
    if (rotation > 3) return record(Status::Unsupported);
    if (frameOpen_) return record(Status::Busy);
    rotation_ = rotation; window_ = false; return record(Status::Ok);
  }
  Status window(int32_t x0, int32_t y0, int32_t x1, int32_t y1) override {
    if (!ready_) return record(Status::NotInitialized);
    if (x0 < 0 || y0 < 0 || x1 < x0 || y1 < y0 || x1 >= width() || y1 >= height())
      return record(Status::InvalidArgument);
    left_ = uint16_t(x0); top_ = uint16_t(y0); spanWidth_ = uint16_t(x1-x0+1);
    spanHeight_ = uint16_t(y1-y0+1); cursor_ = 0; window_ = true;
    return record(Status::Ok);
  }
  Status pixels(PixelView view, bool = false) override {
    if (!ready_) return record(Status::NotInitialized);
    if (!view.valid() || (view.order!=PixelOrder::Native&&view.order!=PixelOrder::Wire) ||
        view.height > SIZE_MAX/view.width || !window_ ||
        view.width * view.height > uint32_t(spanWidth_) * spanHeight_ - cursor_)
      return record(Status::InvalidArgument);
    for (size_t row=0; row<view.height; ++row) {
      const uint8_t *source=view.data+row*view.stride*2;
      for (size_t col=0; col<view.width;) {
        uint16_t x=uint16_t(left_+cursor_%spanWidth_),y=uint16_t(top_+cursor_/spanWidth_);
        size_t n=spanWidth_-cursor_%spanWidth_;if(n>view.width-col)n=view.width-col;
        if(!rotation_&&packing_==MonoPacking::PageLsb){
          const size_t base=size_t(y/8)*nativeWidth_+x;const uint8_t m=uint8_t(1U<<(y&7));
          for(size_t i=0;i<n;++i){const bool white=monoWhite(monoRead565(source+(col+i)*2,view.order),options_,x+unsigned(i),y);
            masked(base+i,m,white);}
        }else if(!rotation_&&packing_==MonoPacking::RowMsb){
          size_t used=0;
          while(used<n){const unsigned px=x+unsigned(used),shift=px&7;
            const size_t take=n-used<8-shift?n-used:8-shift;uint8_t value=0,m=0;
            for(size_t i=0;i<take;++i){const uint8_t bit=uint8_t(0x80U>>(shift+i));m|=bit;
              if(monoWhite(monoRead565(source+(col+used+i)*2,view.order),options_,px+unsigned(i),y))value|=bit;}
            const size_t at=size_t(y)*(nativeWidth_/8)+px/8;value|=uint8_t(frame_[at]&~m);
            if(frame_[at]!=value){frame_[at]=value;dirty_=true;}used+=take;}
        }else{
          for(size_t i=0;i<n;++i){uint16_t px=uint16_t(x+i),py=y;native(px,py);
            put(px,py,monoWhite(monoRead565(source+(col+i)*2,view.order),options_,px,py));}
        }
        cursor_+=uint32_t(n);col+=n;
      }
    }
    return record(Status::Ok);
  }
  Status fill(uint16_t color, uint32_t count) override {
    if (!ready_) return record(Status::NotInitialized);
    if (!window_ || count > uint32_t(spanWidth_)*spanHeight_-cursor_) return record(Status::InvalidArgument);
    if (!count) return Status::Ok;
    // Full-screen clears/fills touch packed bytes directly (8 pixels per store).
    if (!cursor_ && count == uint32_t(width())*height() && spanWidth_==width() && spanHeight_==height() &&
        (options_.dither==MonoDither::None || !color || color==0xFFFF)) {
      const uint8_t value=monoWhite(color,options_,0,0)?0xFF:0;
      const bool padded=packing_==MonoPacking::PageLsb&&(nativeHeight_&7);
      const uint8_t lastMask=uint8_t((1U<<(nativeHeight_&7))-1);
      for(size_t i=0;i<bytes_;++i){const uint8_t v=padded&&i>=bytes_-nativeWidth_?uint8_t(value&lastMask):value;
        if(frame_[i]!=v){frame_[i]=v;dirty_=true;}}
      cursor_+=count; return record(Status::Ok);
    }
    const bool solid=options_.dither==MonoDither::None || !color || color==0xFFFF;
    const bool white=monoWhite(color,options_,0,0);
    if(packing_==MonoPacking::PageLsb&&!rotation_&&solid&&!cursor_&&count==uint32_t(spanWidth_)*spanHeight_){
      const unsigned bottom=top_+spanHeight_,last=(bottom-1)/8;
      for(unsigned page=top_/8;page<=last;++page){uint8_t m=0xFF;
        if(page==top_/8)m&=uint8_t(0xFFU<<(top_&7));
        if(page==last&&(bottom&7))m&=uint8_t((1U<<(bottom&7))-1);
        const size_t base=size_t(page)*nativeWidth_+left_;
        for(unsigned x=0;x<spanWidth_;++x)masked(base+x,m,white);}
      cursor_+=count;return record(Status::Ok);
    }
    // Common unrotated horizontal solid spans use masks at the edges and
    // byte stores in the middle. Text and rectangle backgrounds hit this path.
    if (packing_==MonoPacking::RowMsb && !rotation_ && solid) {
      while(count){const uint16_t x=uint16_t(left_+cursor_%spanWidth_),y=uint16_t(top_+cursor_/spanWidth_);
        const uint32_t room=spanWidth_-cursor_%spanWidth_,n=count<room?count:room;
        rowFill(x,y,uint16_t(n),white);cursor_+=n;count-=n;}
    } else while(count--){uint16_t x=uint16_t(left_+cursor_%spanWidth_),y=uint16_t(top_+cursor_/spanWidth_);
      native(x,y);put(x,y,solid?white:monoWhite(color,options_,x,y));++cursor_;}
    return record(Status::Ok);
  }
  void append(uint16_t color) override { fill(color,1); }
  Status append(uint16_t color,uint32_t count) override { return fill(color,count); }
  Status flush(int32_t x,int32_t y,PixelView view,bool async=true) override {
    if(!ready_)return record(Status::NotInitialized);
    if(!view.valid()||view.width>INT32_MAX||view.height>INT32_MAX)return record(Status::InvalidArgument);
    const int64_t right=int64_t(x)+view.width,bottom=int64_t(y)+view.height;
    if(right<=0||bottom<=0||x>=width()||y>=height())return Status::Ok;
    const size_t skipX=x<0?size_t(-int64_t(x)):0,skipY=y<0?size_t(-int64_t(y)):0;
    view.data+=2*(skipY*view.stride+skipX);
    const int32_t left=x<0?0:x,top=y<0?0:y;
    view.width=size_t((right<width()?right:width())-left);view.height=size_t((bottom<height()?bottom:height())-top);
    Status s=window(left,top,left+int32_t(view.width)-1,top+int32_t(view.height)-1);
    return s==Status::Ok?pixels(view,async):s;
  }
  Status readRect(int32_t x,int32_t y,size_t w,size_t h,uint16_t *out) override {
    if(!ready_)return record(Status::NotInitialized);
    if(!out||!w||!h||x<0||y<0||w>width()||h>height()||size_t(x)>width()-w||size_t(y)>height()-h)
      return record(Status::InvalidArgument);
    for(size_t row=0;row<h;++row)for(size_t col=0;col<w;++col){uint16_t px=uint16_t(x+col),py=uint16_t(y+row);
      native(px,py);*out++=pixelIsWhite(px,py)?0xFFFF:0;}
    return record(Status::Ok);
  }
  Status beginFrame(const DirtyRect *regions,size_t count) override {
    if(!ready_)return record(Status::NotInitialized);
    if(count&&!regions)return record(Status::InvalidArgument);
    for(size_t i=0;i<count;++i){const DirtyRect&r=regions[i];if(r.x<0||r.y<0||r.width<=0||r.height<=0||
      int64_t(r.x)+r.width>width()||int64_t(r.y)+r.height>height())return record(Status::InvalidArgument);}
    if(!count||frameOpen_)return Status::Ok;
    // Allocate only on the first transactional UI frame, then reuse forever.
    if(!backup_){backup_=static_cast<uint8_t*>(malloc(bytes_));if(!backup_)return record(Status::NoMemory);}
    memcpy(backup_,frame_,bytes_);backupDirty_=dirty_;frameOpen_=true;return record(Status::Ok);
  }
  Status commitFrame() override {frameOpen_=false;window_=false;return finish();}
  Status abortFrame() override {
    if(frameOpen_){memcpy(frame_,backup_,bytes_);dirty_=backupDirty_;}
    frameOpen_=false;window_=false;return record(Status::Ok);
  }
 protected:
  PackedMonoPanel(Transport &bus,uint16_t w,uint16_t h,MonoPacking packing,int reset=-1)
      :bus_(bus),nativeWidth_(w),nativeHeight_(h),packing_(packing),reset_(reset) {}
  Status allocate(uint8_t initial) {
    if(!nativeWidth_||!nativeHeight_||nativeWidth_>INT16_MAX||nativeHeight_>INT16_MAX||
       (packing_==MonoPacking::RowMsb && nativeWidth_%8))return record(Status::InvalidArgument);
    const size_t row=packing_==MonoPacking::PageLsb?nativeWidth_:nativeWidth_/8;
    const size_t rows=packing_==MonoPacking::PageLsb?(size_t(nativeHeight_)+7)/8:nativeHeight_;
    if(rows>SIZE_MAX/row/2)return record(Status::InvalidArgument);
    bytes_=row*rows;
    if(!storage_){storage_=static_cast<uint8_t*>(malloc(bytes_*2));if(!storage_)return record(Status::NoMemory);}
    frame_=storage_;sent_=storage_+bytes_;memset(frame_,initial,bytes_);memset(sent_,initial,bytes_);
    dirty_=true;sentValid_=false;frameOpen_=false;window_=false;rotation_=0;return record(Status::Ok);
  }
  Status record(Status s){status_=s;return s;}
  bool validReset()const{
    if(reset_<-1||(reset_>=0&&!bus_.validOutputPin(reset_)))return false;
    if(reset_<0)return true;
    const TransportInfo pins=bus_.info();
    if(reset_==pins.clock||reset_==pins.cs||reset_==pins.dc||reset_==pins.read||reset_==pins.write)return false;
    for(unsigned i=0;i<16;++i)if(reset_==pins.data[i])return false;
    return true;
  }
  void native(uint16_t &x,uint16_t &y) const {
    const uint16_t original=x;
    switch(rotation_){case 1:x=nativeWidth_-1-y;y=original;break;
      case 2:x=nativeWidth_-1-x;y=nativeHeight_-1-y;break;
      case 3:x=y;y=nativeHeight_-1-original;break;default:break;}
  }
  size_t offset(uint16_t x,uint16_t y) const {return packing_==MonoPacking::PageLsb?size_t(y/8)*nativeWidth_+x:size_t(y)*(nativeWidth_/8)+x/8;}
  uint8_t mask(uint16_t x,uint16_t y) const {return packing_==MonoPacking::PageLsb?uint8_t(1U<<(y&7)):uint8_t(0x80U>>(x&7));}
  bool pixelIsWhite(uint16_t x,uint16_t y)const{return (frame_[offset(x,y)]&mask(x,y))!=0;}
  void put(uint16_t x,uint16_t y,bool white){const size_t at=offset(x,y);const uint8_t m=mask(x,y);
    const uint8_t value=white?uint8_t(frame_[at]|m):uint8_t(frame_[at]&~m);
    if(frame_[at]!=value){frame_[at]=value;dirty_=true;}}
  void masked(size_t at,uint8_t m,bool white){const uint8_t value=white?uint8_t(frame_[at]|m):uint8_t(frame_[at]&~m);
    if(frame_[at]!=value){frame_[at]=value;dirty_=true;}}
  void rowFill(uint16_t x,uint16_t y,uint16_t n,bool white){size_t at=size_t(y)*(nativeWidth_/8)+x/8;
    if(x&7){const unsigned take=n<8-(x&7)?n:8-(x&7);masked(at++,uint8_t((0xFFU>>(x&7))&(0xFFU<<(8-(x&7)-take))),white);n=uint16_t(n-take);}
    const uint8_t v=white?0xFF:0;while(n>=8){if(frame_[at]!=v){frame_[at]=v;dirty_=true;}++at;n-=8;}
    if(n)masked(at,uint8_t(0xFFU<<(8-n)),white);
  }
  Transport &bus_;
  uint16_t nativeWidth_,nativeHeight_;
  MonoPacking packing_;
  int reset_;
  MonoOptions options_;
  uint8_t *frame_=nullptr,*sent_=nullptr;
  size_t bytes_=0;
  bool ready_=false,dirty_=false,sentValid_=false,frameOpen_=false;
 private:
  uint8_t *storage_=nullptr,*backup_=nullptr;
  bool backupDirty_=false,window_=false;
  uint8_t rotation_=0;
  uint16_t left_=0,top_=0,spanWidth_=0,spanHeight_=0;
  uint32_t cursor_=0;
  Status status_=Status::Ok;
};

} // namespace nice_gfx
