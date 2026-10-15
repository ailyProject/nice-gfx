#pragma once
#include "../../../Display/Transport.h"
#include <stdlib.h>
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
#include <esp_heap_caps.h>
#endif
namespace nice_gfx { namespace controllers {
// Wrappers own no bus; the containing panel's holder outlives its Panel base.
class ForwardTransport : public Transport {
 public:
  explicit ForwardTransport(Transport &bus) : bus_(bus) {}
  Status begin() override { return bus_.begin(); }
  void end() override { bus_.end(); }
  Status command(uint8_t c,const uint8_t *d,size_t n) override { return bus_.command(c,d,n); }
  Status command16(uint16_t c,const uint8_t *d,size_t n) override { return bus_.command16(c,d,n); }
  Status read(uint16_t c,uint8_t *d,size_t n,uint8_t dummy=1,uint8_t bits=8) override { return bus_.read(c,d,n,dummy,bits); }
  Status setPixelFormat(PixelFormat f) override { return bus_.setPixelFormat(f); }
  Status pixels(uint8_t c,PixelView v,bool a) override { return bus_.pixels(c,v,a); }
  Status fill(uint8_t c,uint16_t v,size_t n) override { return bus_.fill(c,v,n); }
  Status wait() override { return bus_.wait(); }
  bool busy() const override { return bus_.busy(); }
  bool asynchronous() const override { return bus_.asynchronous(); }
  bool validOutputPin(int p) const override { return bus_.validOutputPin(p); }
  TransportInfo info() const override { return bus_.info(); }
  void output(int p,bool h) override { bus_.output(p,h); }
  void delayMs(uint32_t ms) override { bus_.delayMs(ms); }
 protected: Transport &bus_;
};

// PT6891 v1.0.0: 95x28 visible pixels, mirror(1,0). Logical rows 0..3
// occupy RAM rows 24..27 and rows 4..27 occupy RAM rows 0..23. RAM's other
// four rows are not visible. Native row length is pixel count minus one.
class Pt6891Transport : public ForwardTransport {
 public:
  explicit Pt6891Transport(Transport &bus,uint16_t w=95,uint16_t h=28)
      : ForwardTransport(bus),width_(w),height_(h) {}
  Status begin() override {
    if (width_!=95 || height_!=28) return Status::Unsupported;
    return bus_.begin();
  }
  Status command(uint8_t c,const uint8_t *d,size_t n) override {
    if (n && !d) return Status::InvalidArgument;
    if(c==0xA5 && !n) {
      Status s=bus_.command(c,nullptr,0);
      // Software reset retains RAM; clear all physical rows as upstream does.
      const uint8_t x=0,length=127;
      for(uint8_t y=0;s==Status::Ok && y<32;++y) {
        s=bus_.command(0x80,&y,1);
        if(s==Status::Ok) s=bus_.command(0x81,&x,1);
        if(s==Status::Ok) s=bus_.command(0x82,&length,1);
        if(s==Status::Ok) s=bus_.fill(0x83,0,128);
      }
      if(s==Status::Ok) bus_.delayMs(10);
      return s;
    }
    if ((c==0x2A || c==0x2B) && n==4) {
      const uint16_t first=uint16_t(d[0]<<8 | d[1]),last=uint16_t(d[2]<<8 | d[3]);
      if (first>last || last>=(c==0x2A ? width_ : height_)) return Status::InvalidArgument;
      if (c==0x2A) { x0_=first; x1_=last; validX_=true; }
      else { y0_=first; y1_=last; validY_=true; cursor_=0; }
      return Status::Ok;
    }
    // 0x2A/0x29 with 64 parameters are native gamma tables, not DCS windows.
    if (!n) {
      if (c==0x20 || c==0x21) return bus_.command(c==0x21 ? 0x79 : 0x78,nullptr,0);
      if (c==0x28 || c==0x29) return bus_.command(c==0x29 ? 0x5B : 0x58,nullptr,0);
      if (c==0x10 || c==0x11) return bus_.command(c==0x11 ? 0xAA : 0x49,nullptr,0);
    }
    return bus_.command(c,d,n);
  }
  Status setPixelFormat(PixelFormat f) override {
    return f==PixelFormat::RGB565 ? bus_.setPixelFormat(f) : Status::Unsupported;
  }
  Status pixels(uint8_t c,PixelView v,bool) override {
    if (c!=0x83) return Status::Unsupported;
    return image(v);
  }
  Status dataPixels(PixelView v,bool) override { return image(v); }
  Status fill(uint8_t c,uint16_t color,size_t count) override {
    if (c!=0x83) return Status::Unsupported;
    return solid(color,count);
  }
  Status dataFill(uint16_t color,size_t count) override { return solid(color,count); }
  bool asynchronous() const override { return false; }
 private:
  Status check(size_t count) const {
    if (!validX_ || !validY_) return Status::NotInitialized;
    const size_t area=size_t(x1_-x0_+1)*(y1_-y0_+1);
    return cursor_<=area && count<=area-cursor_ ? Status::Ok : Status::InvalidArgument;
  }
  Status row(size_t count) {
    Status s=bus_.wait(); if (s!=Status::Ok) return s;
    const size_t span=x1_-x0_+1;
    uint8_t y=uint8_t(y0_+cursor_/span); y=uint8_t(y<4 ? y+24 : y-4);
    const uint8_t x=uint8_t(x0_+cursor_%span),length=uint8_t(count-1);
    s=bus_.command(0x80,&y,1);
    if(s==Status::Ok) s=bus_.command(0x81,&x,1);
    if(s==Status::Ok) s=bus_.command(0x82,&length,1);
    return s;
  }
  Status image(PixelView view) {
    if(!view.valid() || view.height>SIZE_MAX/view.width) return Status::InvalidArgument;
    Status s=check(view.width*view.height); if(s!=Status::Ok) return s;
    for(size_t r=0;r<view.height;++r) {
      size_t column=0;
      while(column<view.width) {
        size_t n=x1_-x0_+1-cursor_%(x1_-x0_+1);
        if(n>view.width-column) n=view.width-column;
        s=row(n); if(s!=Status::Ok) return s;
        s=bus_.pixels(0x83,PixelView(view.data+2*(r*view.stride+column),n,1,0,view.order),false);
        if(s!=Status::Ok) return s;
        cursor_+=n; column+=n;
      }
    }
    return Status::Ok;
  }
  Status solid(uint16_t color,size_t count) {
    Status s=check(count); if(s!=Status::Ok) return s;
    while(count) {
      size_t n=x1_-x0_+1-cursor_%(x1_-x0_+1); if(n>count) n=count;
      s=row(n); if(s==Status::Ok) s=bus_.fill(0x83,color,n);
      if(s!=Status::Ok) return s;
      cursor_+=n; count-=n;
    }
    return Status::Ok;
  }
  uint16_t width_,height_,x0_=0,x1_=0,y0_=0,y1_=0;
  size_t cursor_=0;
  bool validX_=false,validY_=false;
};

// AXS15231B ignores RASET in QSPI mode. NV3041A has no RAMWRC command at
// all. Compose dirty windows in an owned RGB565 frame and submit a complete
// RAMWR transaction. The latter also sends RASET. DMA chunks keep CS active;
// no RAMWRC header is inserted between chunks. wait() never presents glyphs.
class Axs15231bTransport : public ForwardTransport {
 public:
  Axs15231bTransport(Transport &bus,uint16_t width,uint16_t height,bool rowAddress=false,uint8_t alignmentX=4)
      : ForwardTransport(bus),width_(width),height_(height),rowAddress_(rowAddress),alignmentX_(alignmentX) {}
  ~Axs15231bTransport() override { bus_.wait(); freeFrame(); }
  Status begin() override {
    if(!width_ || !height_ || !alignmentX_ || width_%alignmentX_ || size_t(height_)>SIZE_MAX/2/width_) return Status::InvalidArgument;
    if(!frame_) {
      const size_t bytes=size_t(width_)*height_*2;
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
      frame_=static_cast<uint8_t *>(heap_caps_calloc(1,bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
      if(!frame_) frame_=static_cast<uint8_t *>(heap_caps_calloc(1,bytes,MALLOC_CAP_8BIT));
#else
      frame_=static_cast<uint8_t *>(calloc(1,bytes));
#endif
      if(!frame_) return Status::NoMemory;
    }
    const Status s=bus_.begin(); if(s!=Status::Ok) freeFrame(); return s;
  }
  void end() override { bus_.wait(); bus_.end(); dirty_=false; freeFrame(); }
  Status setPixelFormat(PixelFormat f) override {
    return f==PixelFormat::RGB565 ? bus_.setPixelFormat(f) : Status::Unsupported;
  }
  Status command(uint8_t c,const uint8_t *d,size_t n) override {
    if(n && !d) return Status::InvalidArgument;
    if((c==0x2A || c==0x2B) && n==4) {
      if(!frame_) return Status::NotInitialized;
      const uint16_t first=uint16_t(d[0]<<8|d[1]),last=uint16_t(d[2]<<8|d[3]);
      if(first>last || last>=(c==0x2A ? width_ : height_)) return Status::InvalidArgument;
      Status s=bus_.wait(); if(s!=Status::Ok) return s;
      if(c==0x2A) { x0_=first; x1_=last; validX_=true; }
      else { y0_=first; y1_=last; validY_=true; cursor_=0; }
      return Status::Ok;
    }
    return bus_.command(c,d,n);
  }
  Status pixels(uint8_t c,PixelView v,bool) override {
    if(c!=0x2C) return Status::Unsupported;
    return image(v);
  }
  Status dataPixels(PixelView v,bool) override { return image(v); }
  Status fill(uint8_t c,uint16_t color,size_t count) override {
    return c==0x2C ? solid(color,count) : Status::Unsupported;
  }
  Status dataFill(uint16_t color,size_t count) override { return solid(color,count); }
  Status present() override {
    Status s=bus_.wait(); if(s!=Status::Ok || !dirty_) return s;
    const uint16_t end=width_-1;
    const uint8_t columns[]={0,0,uint8_t(end>>8),uint8_t(end)};
    s=bus_.command(0x2A,columns,4);
    if(s==Status::Ok && rowAddress_) {
      const uint16_t bottom=height_-1;
      const uint8_t rows[]={0,0,uint8_t(bottom>>8),uint8_t(bottom)};
      s=bus_.command(0x2B,rows,4);
    }
    if(s==Status::Ok) s=bus_.pixels(0x2C,PixelView(frame_,width_,height_,width_,PixelOrder::Wire),false);
    if(s==Status::Ok) dirty_=false;
    return s;
  }
  // Writes are copied immediately; a caller may reuse its source on return.
  bool asynchronous() const override { return false; }
 private:
  void freeFrame() {
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
    if(frame_) heap_caps_free(frame_);
#else
    free(frame_);
#endif
    frame_=nullptr; validX_=validY_=false; cursor_=0;
  }
  Status check(size_t count) {
    if(!frame_ || !validX_ || !validY_) return Status::NotInitialized;
    const size_t area=size_t(x1_-x0_+1)*(y1_-y0_+1);
    if(cursor_>area || count>area-cursor_) return Status::InvalidArgument;
    return bus_.wait();
  }
  uint8_t *destination() { return frame_+2*((y0_+cursor_/(x1_-x0_+1))*width_+x0_+cursor_%(x1_-x0_+1)); }
  Status image(PixelView view) {
    if(!view.valid() || view.height>SIZE_MAX/view.width) return Status::InvalidArgument;
    Status s=check(view.width*view.height); if(s!=Status::Ok) return s;
    for(size_t row=0;row<view.height;++row) {
      size_t column=0;
      while(column<view.width) {
        size_t n=x1_-x0_+1-cursor_%(x1_-x0_+1); if(n>view.width-column) n=view.width-column;
        copyPixels(destination(),view.data+2*(row*view.stride+column),n,view.order);
        cursor_+=n; column+=n; dirty_=true;
      }
    }
    return complete();
  }
  Status solid(uint16_t color,size_t count) {
    Status s=check(count); if(s!=Status::Ok) return s;
    while(count) {
      size_t n=x1_-x0_+1-cursor_%(x1_-x0_+1); if(n>count) n=count;
      uint8_t *out=destination();
      for(size_t i=0;i<n;++i) { *out++=uint8_t(color>>8); *out++=uint8_t(color); }
      cursor_+=n; count-=n; dirty_=true;
    }
    return complete();
  }
  Status complete() { return cursor_==size_t(x1_-x0_+1)*(y1_-y0_+1) ? present() : Status::Ok; }
  uint8_t *frame_=nullptr;
  uint16_t width_,height_,x0_=0,x1_=0,y0_=0,y1_=0;
  size_t cursor_=0;
  bool validX_=false,validY_=false,dirty_=false;
  bool rowAddress_;
  uint8_t alignmentX_;
};
} } // namespace nice_gfx::controllers
