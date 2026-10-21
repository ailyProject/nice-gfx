#pragma once
#include "../../Display/Transport.h"
#include "../../Core/PixelCodec.h"
#include <new>
namespace nice_gfx {
// Shared bounded-memory DMA pipeline for controllers without a worker task.
// Conversion of the next chunk overlaps DMA of the current chunk. Async returns
// after submitting the final chunk; wait() completes every accepted transfer.
// The source is never changed and CS remains asserted across every chunk.
template<class Base> class BufferedDmaBus : public Base {
 public:
  template<class... Args> explicit BufferedDmaBus(Args&&... args) : Base(static_cast<Args&&>(args)...) {}
  Status pixels(uint8_t c, PixelView v, bool async) override { return image(c,8,v,async); }
  Status pixels16(uint16_t c, PixelView v, bool async) override { return image(c,16,v,async); }
  Status dataPixels(PixelView v, bool async) override { return image(0,0,v,async); }
  Status fill(uint8_t c, uint16_t color, size_t count) override { return solid(c,8,color,count); }
  Status fill16(uint16_t c, uint16_t color, size_t count) override { return solid(c,16,color,count); }
  Status dataFill(uint16_t color, size_t count) override { return solid(0,0,color,count); }
  bool asynchronous() const override { return dmaAvailable_; }
  bool busy() const override { return const_cast<BufferedDmaBus *>(this)->poll() == Status::Busy; }
  Status wait() override { Status s; do { s=poll(); } while (s==Status::Busy); return s; }
  Status setPixelFormat(PixelFormat format) override {
    Status s=wait(); return s==Status::Ok ? Base::setPixelFormat(format) : s;
  }
 protected:
  virtual Status dmaWrite(const uint8_t *data, size_t bytes)=0;
  virtual Status dmaPoll()=0; // Must drain/stop DMA before returning any terminal status.
  virtual bool dmaDirect(const uint8_t *) const { return true; }
  Status allocateDmaBuffers(size_t bytes=1536) {
    if (bytes < 12 || bytes > 65532 || bytes%12) return Status::InvalidArgument;
    buffer_=new(std::nothrow) uint8_t[bytes*2];
    if (!buffer_) return Status::NoMemory;
    bufferBytes_=bytes; dmaAvailable_=true; result_=Status::Ok; return Status::Ok;
  }
  void releaseDmaBuffers() { wait(); delete[] buffer_; buffer_=nullptr; dmaAvailable_=false; }
  Status start(uint16_t c,uint8_t bits,bool reading) override {
    Status s=wait(); return s==Status::Ok ? Base::start(c,bits,reading) : s;
  }
  bool dmaAvailable_=false;
 private:
  Status poll() {
    if (!open_) return result_;
    Status s=dmaPoll();
    if (s==Status::Busy) return s;
    Base::finish(); open_=false; return result_=s;
  }
  Status drainChunk() { Status s; do { s=dmaPoll(); } while (s==Status::Busy); return s; }
  Status image(uint16_t command,uint8_t bits,PixelView view,bool async) {
    if (!dmaAvailable_) {
      if (!bits) return Base::dataPixels(view,async);
      return bits==16 ? Base::pixels16(command,view,async) : Base::pixels(uint8_t(command),view,async);
    }
    if (!this->begun_) return Status::NotInitialized;
    if (!view.valid()) return Status::InvalidArgument;
    Status s=start(command,bits,false); if (s!=Status::Ok) return s;
    open_=true; bool pending=false; unsigned slot=0;
    const size_t bpp=wirePixelBytes(this->format_);
    const bool contiguous=view.stride==view.width;
    const size_t rows=contiguous ? 1 : view.height, columns=contiguous ? view.width*view.height : view.width;
    for (size_t row=0; row<rows && s==Status::Ok; ++row) {
      const uint8_t *source=view.data+2*row*view.stride; size_t remaining=columns;
      while (remaining && s==Status::Ok) {
        bool direct=view.order==PixelOrder::Wire && this->format_==PixelFormat::RGB565 && dmaDirect(source);
        size_t capacity=direct ? 65532/bpp : bufferBytes_/bpp;
        size_t count=remaining < capacity ? remaining : capacity;
        const uint8_t *tx=source;
        if (!direct) { uint8_t *stage=buffer_+slot*bufferBytes_; encodePixels(stage,source,count,view.order,this->format_); tx=stage; }
        if (pending) s=drainChunk();
        if (s==Status::Ok) s=dmaWrite(tx,count*bpp);
        pending=s==Status::Ok; source+=count*2; remaining-=count; slot ^= 1;
      }
    }
    if (s!=Status::Ok) { if (pending) drainChunk(); Base::finish(); open_=false; return result_=s; }
    return async ? Status::Ok : wait();
  }
  Status solid(uint16_t command,uint8_t bits,uint16_t color,size_t count) {
    if (!dmaAvailable_ || (this->writeStrobe_.number>=0 && this->format_==PixelFormat::RGB565)) {
      const Status pending=wait();
      if (pending!=Status::Ok) return pending;
      if (!bits) return Base::dataFill(color,count);
      return bits==16 ? Base::fill16(command,color,count) : Base::fill(uint8_t(command),color,count);
    }
    if (!this->begun_) return Status::NotInitialized;
    if (!count) return Status::Ok;
    Status s=start(command,bits,false); if (s!=Status::Ok) return s;
    open_=true; const size_t bpp=wirePixelBytes(this->format_);
    encodePixels(buffer_,reinterpret_cast<const uint8_t *>(&color),1,PixelOrder::Native,this->format_);
    for (size_t i=bpp; i<bufferBytes_; i+=bpp) memcpy(buffer_+i,buffer_,bpp);
    while (count && s==Status::Ok) {
      size_t n=count < bufferBytes_/bpp ? count : bufferBytes_/bpp;
      s=dmaWrite(buffer_,n*bpp); if (s==Status::Ok) s=drainChunk(); count-=n;
    }
    Base::finish(); open_=false; return result_=s;
  }
  uint8_t *buffer_=nullptr;
  size_t bufferBytes_=0;
  bool open_=false;
  Status result_=Status::Ok;
};
}
