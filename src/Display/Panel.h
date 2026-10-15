#pragma once
#ifndef GFX_RUNTIME_PANEL_H
#define GFX_RUNTIME_PANEL_H
#include "ControllerProfile.h"

namespace nice_gfx {

struct InitCommand {
  uint8_t command, count;
  uint16_t delayMs;
  uint8_t data[16];
};

struct PanelConfig {
  uint16_t width, height;
  uint32_t id;
  const InitCommand *init;
  size_t initCount;
  uint8_t madctl[4];
  uint16_t xOffset[4], yOffset[4];
  uint8_t rotationMask; // Supported rotations, not a promise of software rotation.
};

class Panel {
 public:
  Panel(Transport &bus, const PanelConfig &config, int reset = -1,
        uint8_t windowAlignmentX = 1, uint8_t windowAlignmentY = 1)
      : bus_(bus), config_(config), reset_(reset),
        windowAlignmentX_(windowAlignmentX), windowAlignmentY_(windowAlignmentY) {}
  Panel(Transport &bus, const ControllerProfile &profile, int reset = -1,
        PanelOptions options = PanelOptions(), uint8_t windowAlignmentX = 1,
        uint8_t windowAlignmentY = 1)
      : bus_(bus), config_{}, reset_(reset), profile_(profile), options_(options), profiled_(true),
        windowAlignmentX_(windowAlignmentX), windowAlignmentY_(windowAlignmentY) {
    config_.width = options.width ? options.width : profile.width;
    config_.height = options.height ? options.height : profile.height;
    config_.id = profile.id;
    config_.rotationMask = profile.rotations>0&&profile.rotations<=8 ? uint8_t((1U << profile.rotations)-1) : 0;
  }
  ~Panel() { if (ready_) finish(); }
  Panel(const Panel &) = delete;
  Panel &operator=(const Panel &) = delete;

  Status begin() {
    if (ready_) return record(synchronize());
    if (!config_.width || !config_.height || config_.width > INT16_MAX || config_.height > INT16_MAX ||
        !windowAlignmentX_ || !windowAlignmentY_ ||
        config_.width % windowAlignmentX_ || config_.height % windowAlignmentY_ ||
        !(config_.rotationMask & 1) || (config_.initCount && !config_.init) || reset_ < -1 ||
        (reset_ >= 0 && !bus_.validOutputPin(reset_))) return record(Status::InvalidArgument);
    if((!profiled_&&(config_.rotationMask&0xF0)) ||
       (profiled_&&(!profile_.initialize||!profile_.rotate||options_.inversion < -1||options_.inversion > 1))) return record(Status::InvalidArgument);
    if(reset_>=0) {
      const TransportInfo pins=bus_.info();
      if(reset_==pins.clock||reset_==pins.cs||reset_==pins.dc||reset_==pins.read||reset_==pins.write) return record(Status::InvalidArgument);
      for(unsigned i=0;i<16;++i)if(reset_==pins.data[i])return record(Status::InvalidArgument);
    }
    for (size_t i=0; i<config_.initCount; ++i)
      if (config_.init[i].count > sizeof(config_.init[i].data)) return record(Status::InvalidArgument);
    status_ = bus_.begin();
    if (status_ != Status::Ok) return status_;
    // PanelConfig programs use RGB565. Reusing a transport after an RGB666 or
    // RGB888 profile must restore its encoder before writing that program.
    status_ = bus_.setPixelFormat(profiled_ ? profile_.pixelFormat : PixelFormat::RGB565);
    if (status_ != Status::Ok) { bus_.end(); return status_; }
    inverted_ = false;
    if (reset_ >= 0) {
      bus_.output(reset_, true); bus_.delayMs(5);
      bus_.output(reset_, false); bus_.delayMs(20);
      bus_.output(reset_, true);
    } else if (!profiled_ || profile_.softReset) {
      status_ = profiled_ && is16() ? bus_.command16(profile_.softReset,nullptr,0)
                                    : bus_.command(profiled_ ? uint8_t(profile_.softReset) : 0x01, nullptr, 0);
    }
    bus_.delayMs(150);
    if (profiled_ && status_ == Status::Ok) {
      PanelSequence s = sequence(); profile_.initialize(s); status_ = s.finish();
      xOffset_=s.colstart; yOffset_=s.rowstart;
    }
    for (size_t i=0; i<config_.initCount && status_ == Status::Ok; ++i) {
      const InitCommand &c = config_.init[i];
      status_ = bus_.command(c.command, c.data, c.count);
      if (c.delayMs) bus_.delayMs(c.delayMs);
    }
    if (status_ != Status::Ok) { bus_.end(); return status_; }
    ready_ = true;
    Status s = setRotation(0);
    if(s==Status::Ok && profiled_ && options_.inversion>=0) s=invert(options_.inversion!=0);
    if (s != Status::Ok) { bus_.end(); ready_ = false; }
    return record(s);
  }
  void end() { finish(); bus_.end(); ready_ = false; window_ = false; }
  bool ready() const { return ready_; }
  uint16_t width() const { return rotation_ & 1 ? config_.height : config_.width; }
  uint16_t height() const { return rotation_ & 1 ? config_.width : config_.height; }
  uint8_t rotation() const { return rotation_; }
  // Transfer windows must begin and end on this logical pixel grid. A partial
  // refresh renderer may expand dirty regions and repaint the added pixels;
  // low-level window()/flush() callers must supply an aligned pixel payload.
  uint8_t windowAlignmentX() const { return rotation_ & 1 ? windowAlignmentY_ : windowAlignmentX_; }
  uint8_t windowAlignmentY() const { return rotation_ & 1 ? windowAlignmentX_ : windowAlignmentY_; }
  uint32_t id() const { return config_.id; }
  int resetPin() const { return reset_; }
  TransportInfo transportInfo() const { return bus_.info(); }
  bool supportsFrameBlit() const { return bus_.supportsFrameBlit(); }
  bool canFrameBlit(PixelView view) const { return bus_.canFrameBlit(view); }
  Status setPixelClockHz(uint32_t hz) {
    if(!ready_)return record(Status::NotInitialized);
    const Status s=finish();return s==Status::Ok?record(bus_.setPixelClockHz(hz)):s;
  }
  Status status() const { return status_; }
  bool asynchronous() const { return bus_.asynchronous(); }
  bool busy() const { return bus_.busy(); }

  Status setRotation(uint8_t r) {
    if (!ready_) return record(Status::NotInitialized);
    if (r > 7 || !(config_.rotationMask & (1U << r))) return record(Status::Unsupported);
    if (profiled_) {
      Status waited=synchronize(); if(waited!=Status::Ok) return record(waited);
      window_=false;
      PanelSequence s=sequence(); s.colstart=xOffset_; s.rowstart=yOffset_;
      profile_.rotate(s,r); Status result=s.finish();
      if(result==Status::Ok) {
        rotation_=s.rotation; xOffset_=s.colstart; yOffset_=s.rowstart;
        if(options_.overrideOffsets) { xOffset_=options_.xOffsets[r]; yOffset_=options_.yOffsets[r]; }
        // HX8347C/HX8352C store inversion in the register their rotation program
        // rewrites. Preserve an explicitly enabled inverse display mode.
        if(inverted_ && (strcmp(profile_.name,"HX8347C")==0 || strcmp(profile_.name,"HX8352C")==0)) result=invert(true);
      }
      return record(result);
    }
    Status s = command(0x36, &config_.madctl[r], 1);
    if (s == Status::Ok) rotation_ = r;
    return s;
  }
  Status command(uint16_t c, const uint8_t *data = nullptr, size_t length = 0) {
    if (!ready_) return record(Status::NotInitialized);
    if (length && !data) return record(Status::InvalidArgument);
    Status s = synchronize();
    if (s != Status::Ok) return s;
    window_ = false;
    return record(is16() || c>255 ? bus_.command16(c,data,length) : bus_.command(uint8_t(c), data, length));
  }
  Status command16(uint16_t c, const uint8_t *data = nullptr, size_t length = 0) {
    if(!ready_) return record(Status::NotInitialized);
    if(length&&!data) return record(Status::InvalidArgument);
    Status s=synchronize(); window_=false;
    return s==Status::Ok ? record(bus_.command16(c,data,length)) : s;
  }
  Status data(const uint8_t *data, size_t length) {
    if(!ready_) return record(Status::NotInitialized);
    if(length&&!data) return record(Status::InvalidArgument);
    Status s=synchronize(); window_=false;
    return s==Status::Ok ? record(bus_.data(data,length)) : s;
  }
  Status writeRegister16(uint16_t c,uint16_t value) {
    if(!ready_)return record(Status::NotInitialized);
    Status s=synchronize();window_=false;
    return s==Status::Ok?record(bus_.register16(c,value,16)):s;
  }
  Status window(int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
    if (!ready_) return record(Status::NotInitialized);
    Status s = synchronize();
    window_ = false;
    if (s != Status::Ok) return s;
    if (x0 < 0 || y0 < 0 || x1 < x0 || y1 < y0 || x1 >= width() || y1 >= height())
      return record(Status::InvalidArgument);
    if (x0 % windowAlignmentX() || y0 % windowAlignmentY() ||
        (x1 - x0 + 1) % windowAlignmentX() || (y1 - y0 + 1) % windowAlignmentY())
      return record(Status::InvalidArgument);
    if(profiled_) {
      s=controllerWindow(x0,y0,x1,y1);
      if(s==Status::Ok) { window_=true; started_=false; remaining_=uint32_t(x1-x0+1)*uint32_t(y1-y0+1); }
      return record(s);
    }
    const uint32_t xs = x0 + config_.xOffset[rotation_], xe = x1 + config_.xOffset[rotation_];
    const uint32_t ys = y0 + config_.yOffset[rotation_], ye = y1 + config_.yOffset[rotation_];
    if (xe > UINT16_MAX || ye > UINT16_MAX) return record(Status::InvalidArgument);
    uint8_t column[4] = {uint8_t(xs >> 8), uint8_t(xs), uint8_t(xe >> 8), uint8_t(xe)};
    uint8_t row[4] = {uint8_t(ys >> 8), uint8_t(ys), uint8_t(ye >> 8), uint8_t(ye)};
    window_ = false;
    s = bus_.command(0x2A, column, 4);
    if (s == Status::Ok) s = bus_.command(0x2B, row, 4);
    if (s == Status::Ok) {
      window_ = true; started_ = false;
      remaining_ = uint32_t(x1 - x0 + 1) * uint32_t(y1 - y0 + 1);
    }
    return record(s);
  }
  Status pixels(PixelView view, bool async = false) {
    if(!ready_)return record(Status::NotInitialized);
    if (!view.valid() || view.height > SIZE_MAX / view.width) return record(Status::InvalidArgument);
    const size_t count = view.width * view.height;
    if (!window_ || used_>remaining_ || count > remaining_ - used_) return record(Status::InvalidArgument);
    Status s = drain();
    if (s == Status::Ok) s = bus_.wait();
    if (s == Status::Ok) s = transmit(view, async);
    if (s == Status::Ok) { started_ = true; remaining_ -= count; }
    else window_ = false;
    return record(s);
  }
  Status fill(uint16_t color, uint32_t count) {
    if(!ready_)return record(Status::NotInitialized);
    if (!count) return Status::Ok;
    if (!window_ || used_>remaining_ || count > remaining_ - used_) return record(Status::InvalidArgument);
    Status s = drain();
    if (s == Status::Ok) s = bus_.wait();
    // A display window can exceed a 16-bit MCU's address space even though
    // each transfer span must fit size_t. Filled pixels need no framebuffer.
    while (s == Status::Ok && count) {
      const size_t span = count > SIZE_MAX ? SIZE_MAX : size_t(count);
      if(profiled_ && started_) s=bus_.dataFill(color,span);
      else if(profiled_ && profile_.protocol==Protocol::Ssd1331) s=bus_.dataFill(color,span);
      else if(is16()) s=bus_.fill16(profile_.ramWrite,color,span);
      else s=bus_.fill(started_ ? 0x3C : profiled_ ? uint8_t(profile_.ramWrite) : 0x2C,color,span);
      if (s == Status::Ok) { remaining_ -= span; count -= span; started_ = true; }
    }
    if (s != Status::Ok) window_ = false;
    return record(s);
  }
  // Consecutive glyph pixels are collected into spans to batch transport calls.
  void append(uint16_t color) {
    if (!window_ || used_ >= remaining_) { record(Status::InvalidArgument); return; }
    buffer_[used_++] = color;
    if (used_ == sizeof(buffer_)/sizeof(buffer_[0])) drain();
  }
  // Collect short glyph runs in the existing buffer. Sending each foreground
  // or background run as a separate fill costs a transaction per color change.
  Status append(uint16_t color, uint32_t count) {
    if (!ready_) return record(Status::NotInitialized);
    if (!window_ || used_ > remaining_ || count > remaining_ - used_)
      return record(Status::InvalidArgument);
    const size_t capacity = sizeof(buffer_) / sizeof(buffer_[0]);
    while (count) {
      const size_t room = capacity - used_;
      const size_t n = count < room ? size_t(count) : room;
      for (size_t i = 0; i < n; ++i) buffer_[used_ + i] = color;
      used_ += n; count -= n;
      if (used_ == capacity) {
        const Status s = drain();
        if (s != Status::Ok) return s;
      }
    }
    return record(Status::Ok);
  }
  Status finish() {
    const Status previous=status_;
    Status s=synchronize();
    if (s==Status::Ok) s=bus_.present();
    // The return value describes this synchronization attempt, allowing a
    // caller to retry a drained transfer. Keep an earlier operation error in
    // status() until the next explicit successful command/window/pixel write.
    record(s==Status::Ok?previous:s);
    return s;
  }
  // Clipped partial refresh. Source dimensions/stride describe the original area.
  Status flush(int32_t x, int32_t y, PixelView view, bool async = true) {
    if (!ready_) return record(Status::NotInitialized);
    if (!view.valid() || view.width > INT32_MAX || view.height > INT32_MAX) return record(Status::InvalidArgument);
    const int64_t right = int64_t(x) + int64_t(view.width), bottom = int64_t(y) + int64_t(view.height);
    if (right <= 0 || bottom <= 0 || x >= width() || y >= height()) return finish();
    const size_t skipX = x < 0 ? size_t(-int64_t(x)) : 0;
    const size_t skipY = y < 0 ? size_t(-int64_t(y)) : 0;
    view.data += 2 * (skipY * view.stride + skipX);
    const int32_t left = x < 0 ? 0 : x, top = y < 0 ? 0 : y;
    view.width = size_t((right < width() ? right : width()) - left);
    view.height = size_t((bottom < height() ? bottom : height()) - top);
    Status s = window(left, top, left + int32_t(view.width) - 1, top + int32_t(view.height) - 1);
    return s == Status::Ok ? pixels(view, async) : s;
  }
  Status readCommand(uint16_t c,uint8_t *data,size_t bytes,uint8_t dummy=1,uint8_t commandBits=8) {
    if(!ready_) return record(Status::NotInitialized);
    if(bytes&&!data) return record(Status::InvalidArgument);
    Status s=synchronize(); window_=false;
    return s==Status::Ok ? record(bus_.read(c,data,bytes,dummy,is16()?16:commandBits)) : s;
  }
  Status readPixel(int32_t x,int32_t y,uint16_t &out) { return readRect(x,y,1,1,&out); }
  Status readIndexedCommand(uint16_t c,uint8_t index,uint8_t &value) {
    if(!ready_)return record(Status::NotInitialized);
    if(profiled_&&profile_.indexCommand&&bus_.info().lanes<=1) {
      const uint8_t selector=uint8_t(0x10+(index&15));
      Status s=command(profile_.indexCommand,&selector,1);if(s!=Status::Ok)return s;
      return readCommand(c,&value,1,0);
    }
    // Parallel controllers select a parameter by clocking preceding bytes.
    return readCommand(c,&value,1,index);
  }
  Status readRect(int32_t x,int32_t y,size_t w,size_t h,uint16_t *out) {
    if(!ready_) return record(Status::NotInitialized);
    if(!out || !w || !h || w>SIZE_MAX/2 || h>(SIZE_MAX/2)/w || x<0 || y<0 || w>width() || h>height() ||
       uint32_t(x)>width()-w || uint32_t(y)>height()-h) return record(Status::InvalidArgument);
    Status s=synchronize(); if(s!=Status::Ok) return record(s);
    const uint16_t cmd=profiled_?profile_.ramRead:0;
    if(!cmd) return record(Status::Unsupported);
    // SSD1351/SSD1357 and SEPS525 RAM readback needs their MCU parallel bus.
    // Their serial interfaces do not return RAM data, even with an MCU MISO pin.
    if((profile_.protocol==Protocol::Ssd1351 || profile_.protocol==Protocol::Seps525) && bus_.info().lanes<8)
      return record(Status::Unsupported);
    const bool word=profile_.readFormat==PixelReadFormat::RGB565 || profile_.readFormat==PixelReadFormat::BGR565;
    uint8_t bytes[192];
    uint16_t words[64];
    for(size_t row=0;row<h;++row) for(size_t col=0;col<w;col+=64) {
      const size_t n=w-col<64?w-col:64;
      s=window(x+int32_t(col),y+int32_t(row),x+int32_t(col+n)-1,y+int32_t(row));
      if(s!=Status::Ok) return s;
      const bool wideWord=word&&bus_.info().lanes==16;
      s=wideWord?bus_.readWords(cmd,words,n,profile_.readDummy,is16()?16:8):bus_.read(cmd,bytes,n*(word?2:3),profile_.readDummy,is16()?16:8);
      if(s!=Status::Ok) {window_=false;return record(s);}
      for(size_t i=0;i<n;++i) {
        uint16_t color;
        if(word) { color=wideWord?words[i]:uint16_t(bytes[2*i]<<8|bytes[2*i+1]); if(profile_.readFormat==PixelReadFormat::BGR565)color=uint16_t(color>>11|color<<11|(color&0x07E0)); }
        else {
          uint8_t r=bytes[3*i],g=bytes[3*i+1],b=bytes[3*i+2];
          if(profile_.readFormat==PixelReadFormat::RGB777) {r=uint8_t(r<<1);g=uint8_t(g<<1);b=uint8_t(b<<1);}
          color=uint16_t((r&0xF8)<<8 | (g&0xFC)<<3 | b>>3);
        }
        *out++=color;
      }
    }
    window_=false; return record(Status::Ok);
  }
  Status invert(bool on) {
    if(!profiled_) return command(on?0x21:0x20);
    switch(profile_.protocol) {
      case Protocol::Ssd1331: return command(on?0xA7:0xA4);
      case Protocol::Ssd1351: return command(on?0xA7:0xA6);
      case Protocol::Hx8347: {
        if(strcmp(profile_.name,"HX8347D")==0) {uint8_t d=on?2:0;return command(1,&d,1);}
        const uint8_t mode=uint8_t(((rotation_==0||rotation_==3)?7:3)|(on?0x10:0));
        const uint8_t scan[4]={0x40,0x60,0,0x20};
        Status s=command(0x36,&mode,1);
        if(s==Status::Ok) s=command(0x16,&scan[rotation_&3],1);
        if(s==Status::Ok && (strcmp(profile_.name,"HX8347C")==0 || strcmp(profile_.name,"HX8352C")==0)) inverted_=on;
        return s;
      }
      case Protocol::Hx8357a: { uint8_t d=on?2:0; return command(1,&d,1); }
      case Protocol::Ili9331: return writeRegister16(0x61,on?1:0);
      case Protocol::Jbt6k71: return writeRegister16(7,on?0x4004:0x4000);
      case Protocol::Ssd1283: return record(Status::Unsupported);
      case Protocol::Seps525: return record(Status::Unsupported); // Controller has no inversion operation.
      case Protocol::Dcs16: return strcmp(profile_.name,"RM68120")==0?command(on?0x2100:0x2000):record(Status::Unsupported);
      default: return command(on?0x21:0x20);
    }
  }
 private:
  Status synchronize() {
    Status s=drain();
    const Status waited=bus_.wait();
    if(s==Status::Ok)s=waited;
    if(s!=Status::Ok)record(s);
    return s;
  }
  bool is16() const { return profiled_ && (profile_.protocol==Protocol::Dcs16 || profile_.protocol==Protocol::Ili9331 || profile_.protocol==Protocol::Jbt6k71); }
  PanelSequence sequence() {
    const uint8_t order=options_.colorOrder==ColorOrder::RGB ? profile_.rgbOrder :
                        options_.colorOrder==ColorOrder::BGR ? profile_.bgrOrder : profile_.defaultColorOrder;
    return PanelSequence(bus_,config_.width,config_.height,order,uint8_t(options_.tab),options_.xOffset,options_.yOffset,profile_.protocol==Protocol::Ili9225);
  }
  Status reg8(uint16_t c,uint8_t v) { return is16()?bus_.command16(c,&v,1):bus_.command(uint8_t(c),&v,1); }
  Status reg16(uint16_t c,uint16_t v) {
    return bus_.register16(c,v,is16()?16:8);
  }
  static void swap(int32_t &a,int32_t &b) { const int32_t t=a;a=b;b=t; }
  Status controllerWindow(int32_t x0,int32_t y0,int32_t x1,int32_t y1) {
    x0+=xOffset_; x1+=xOffset_; y0+=yOffset_; y1+=yOffset_;
    // GC9D01 modules deliberately use signed column offsets. The controller
    // accepts the resulting 16-bit wrapped register address.
    const Protocol p=profile_.protocol;
    if((p==Protocol::Ili9225 || p==Protocol::Ssd1331 || p==Protocol::Ssd1351) && (rotation_&1)) { swap(x0,y0);swap(x1,y1); }
    if(p==Protocol::Ssd1963 && !(rotation_&1)) { swap(x0,y0);swap(x1,y1); }
    Status s=Status::Ok;
    if(p==Protocol::Dcs || p==Protocol::Ssd1963) {
      const uint8_t col[]={uint8_t(x0>>8),uint8_t(x0),uint8_t(x1>>8),uint8_t(x1)};
      const uint8_t row[]={uint8_t(y0>>8),uint8_t(y0),uint8_t(y1>>8),uint8_t(y1)};
      s=bus_.command(0x2A,col,4); if(s==Status::Ok) s=bus_.command(0x2B,row,4);
    } else if(p==Protocol::Dcs16) {
      const uint16_t v[]={uint16_t(x0>>8),uint16_t(x0&255),uint16_t(x1>>8),uint16_t(x1&255),uint16_t(y0>>8),uint16_t(y0&255),uint16_t(y1>>8),uint16_t(y1&255)};
      for(unsigned i=0;i<8 && s==Status::Ok;++i) s=reg16(uint16_t((i<4?0x2A00:0x2B00)+(i&3)),v[i]);
    } else if(p==Protocol::Ili9225) {
      const uint8_t c[]={0x37,0x36,0x39,0x38,0x20,0x21};
      const uint16_t v[]={uint16_t(x0),uint16_t(x1),uint16_t(y0),uint16_t(y1),uint16_t(x0),uint16_t(y0)};
      for(unsigned i=0;i<6 && s==Status::Ok;++i) s=reg16(c[i],v[i]);
    } else if(p==Protocol::Ili9331) {
      const uint8_t even[]={0x20,0x50,0x51,0x21,0x52,0x53},odd[]={0x21,0x52,0x53,0x20,0x50,0x51};
      const uint8_t *c=(rotation_&1)?odd:even;
      const uint16_t v[]={uint16_t(x0),uint16_t(x0),uint16_t(x1),uint16_t(y0),uint16_t(y0),uint16_t(y1)};
      for(unsigned i=0;i<6 && s==Status::Ok;++i) s=reg16(c[i],v[i]);
    } else if(p==Protocol::Jbt6k71) {
      const uint16_t even[]={0x406,0x407,0x200,0x408,0x409,0x201},odd[]={0x408,0x409,0x201,0x406,0x407,0x200};
      const uint16_t *c=(rotation_&1)?odd:even;
      if(rotation_==1) { const int32_t t=x0; x0=config_.height-x1-1;x1=config_.height-t-1; }
      if(rotation_==0) { const int32_t t=y0; y0=config_.height-y1-1;y1=config_.height-t-1; }
      const uint16_t v[]={uint16_t(x0),uint16_t(x1),uint16_t(rotation_==1?x1:x0),uint16_t(y0),uint16_t(y1),uint16_t(rotation_==0?y1:y0)};
      for(unsigned i=0;i<6 && s==Status::Ok;++i) s=reg16(c[i],v[i]);
    } else if(p==Protocol::Hx8347 || p==Protocol::Hx8357a) {
      const uint8_t v[]={uint8_t(x0>>8),uint8_t(x0),uint8_t(x1>>8),uint8_t(x1),uint8_t(y0>>8),uint8_t(y0),uint8_t(y1>>8),uint8_t(y1)};
      if(p==Protocol::Hx8357a) { const uint8_t a[]={v[0],v[1],v[4],v[5]}; for(unsigned i=0;i<4 && s==Status::Ok;++i) s=reg8(uint16_t(0x80+i),a[i]); }
      for(unsigned i=0;i<8 && s==Status::Ok;++i) s=reg8(uint16_t(2+i),v[i]);
    } else if(p==Protocol::Seps525) {
      const uint8_t even[]={0x17,0x20,0x18,0x19,0x21,0x1A},odd[]={0x19,0x21,0x1A,0x17,0x20,0x18};
      const uint8_t *c=(rotation_&1)?odd:even;
      const uint16_t v[]={uint16_t(x0),uint16_t(x0),uint16_t(x1),uint16_t(y0),uint16_t(y0),uint16_t(y1)};
      for(unsigned i=0;i<6 && s==Status::Ok;++i) s=reg16(c[i],v[i]);
    } else if(p==Protocol::Ssd1283) {
      uint8_t v1,v2,v3,h1,h2,h3;
      switch(rotation_&3) {
        case 1:v1=uint8_t(x1+2);v2=uint8_t(x0+2);v3=v2;h1=uint8_t(131-y0);h2=uint8_t(131-y1);h3=h1;break;
        case 2:v1=uint8_t(131-y0);v2=uint8_t(131-y1);v3=v1;h1=uint8_t(131-x0);h2=uint8_t(131-x1);h3=h1;break;
        case 3:v1=uint8_t(131-x0);v2=uint8_t(131-x1);v3=v1;h1=uint8_t(y1+2);h2=uint8_t(y0+2);h3=h2;break;
        default:v1=uint8_t(y1+2);v2=uint8_t(y0+2);v3=v2;h1=uint8_t(x1+2);h2=uint8_t(x0+2);h3=h2;break;
      }
      s=reg16(0x44,uint16_t(h1<<8|h2));if(s==Status::Ok)s=reg16(0x45,uint16_t(v1<<8|v2));if(s==Status::Ok)s=reg16(0x21,uint16_t(v3<<8|h3));
    } else if(p==Protocol::Ssd1331) {
      const uint8_t c[]={0x15,uint8_t(x0),uint8_t(x1),0x75,uint8_t(y0),uint8_t(y1)};
      for(unsigned i=0;i<6&&s==Status::Ok;++i)s=bus_.command(c[i],nullptr,0);
    } else if(p==Protocol::Ssd1351) {
      // These are two 8-bit parameters, not a 16-bit register value. On a
      // 16-bit MCU bus each parameter still needs its own write cycle.
      const uint8_t col[]={uint8_t(x0),uint8_t(x1)},row[]={uint8_t(y0),uint8_t(y1)};
      s=bus_.command(0x15,col,2);if(s==Status::Ok)s=bus_.command(0x75,row,2);
    }
    return s;
  }
  Status transmit(PixelView view,bool async) {
    if(!profiled_)return bus_.pixels(started_?0x3C:0x2C,view,async);
    if(started_||profile_.protocol==Protocol::Ssd1331)return bus_.dataPixels(view,async);
    if(is16())return bus_.pixels16(profile_.ramWrite,view,async);
    return bus_.pixels(started_?0x3C:uint8_t(profile_.ramWrite),view,async);
  }
  Status record(Status s) { status_ = s; return s; }
  Status drain() {
    if (!used_) return Status::Ok;
    const size_t n = used_; used_ = 0;
    Status s = bus_.wait();
    if (s == Status::Ok) s = transmit(PixelView(buffer_, n), false);
    if (s == Status::Ok) { started_ = true; remaining_ -= n; }
    else window_ = false;
    return record(s);
  }
  Transport &bus_;
  PanelConfig config_;
  int reset_;
  Status status_ = Status::NotInitialized;
  uint8_t rotation_ = 0;
  bool ready_ = false, window_ = false, started_ = false;
  bool inverted_ = false;
  uint32_t remaining_ = 0;
  size_t used_ = 0;
  uint16_t buffer_[pixelBufferPixels];
  ControllerProfile profile_{};
  PanelOptions options_;
  bool profiled_=false;
  int16_t xOffset_=0,yOffset_=0;
  uint8_t windowAlignmentX_=1,windowAlignmentY_=1;
};
} // namespace nice_gfx
#endif
