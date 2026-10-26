#pragma once
#include "../Arduino/Bus.h"
#include "../Common/BufferedDmaBus.h"
#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
#include <hardware/pio.h>
#include <hardware/dma.h>
#include <hardware/clocks.h>
#include <hardware/gpio.h>
namespace nice_gfx {
class Rp2040PioPort : public StreamBus {
 public:
  explicit Rp2040PioPort(const SpiConfig &config);
  Rp2040PioPort(const ParallelConfig &config,uint8_t width);
  Status begin() override;
  Status commands(const uint8_t *data,size_t bytes) override;
  void end() override;
  bool validOutputPin(int pin) const override;
  TransportInfo info() const override;
  Status register16(uint16_t command,uint16_t value,uint8_t bits=8) override;
  Status readWords(uint16_t command,uint16_t *data,size_t count,uint8_t dummy=1,uint8_t bits=8) override;
 protected:
  Status start(uint16_t command,uint8_t bits,bool reading) override;
  void finish() override;
  void writeBlock(const uint8_t *data,size_t bytes,bool pixelData) override;
  Status readBlock(uint8_t *data,size_t bytes) override;
  bool drained() const;
  void drain() const;
  uint8_t width_=1;
  PIO pio_=nullptr;
  int sm_=-1;
 private:
  void word(uint16_t value);
  void readMode(bool reading);
  uint8_t readByte();
  SpiConfig spiConfig_;
  ParallelConfig parallelConfig_;
  FastPin cs_,dc_,clock_,rd_;
  bool reading_=false;
  uint offset_=0;
  uint16_t instructions_[5]={};
  pio_program program_={};
};

// Runtime-selected PIO state machine and DMA channel. Parallel data pins must be
// consecutive (a PIO hardware constraint); use Parallel8/16 for arbitrary routes.
class Rp2040Pio : public BufferedDmaBus<Rp2040PioPort> {
 public:
  explicit Rp2040Pio(SpiPins pins) : BufferedDmaBus<Rp2040PioPort>(SpiConfig(pins)) {}
  explicit Rp2040Pio(const SpiConfig &config) : BufferedDmaBus<Rp2040PioPort>(config) {}
  Rp2040Pio(const ParallelConfig &config,uint8_t width) : BufferedDmaBus<Rp2040PioPort>(config,width) {}
  ~Rp2040Pio() override { end(); }
  Status begin() override;
  void end() override;
  Status fill(uint8_t c,uint16_t color,size_t count) override { return solidFast(c,8,color,count); }
  Status fill16(uint16_t c,uint16_t color,size_t count) override { return solidFast(c,16,color,count); }
  Status dataFill(uint16_t color,size_t count) override { return solidFast(0,0,color,count); }
 protected:
  Status dmaWrite(const uint8_t *data,size_t bytes) override;
  Status dmaPoll() override;
  bool dmaDirect(const uint8_t *data) const override {
    return width_!=16 || (reinterpret_cast<uintptr_t>(data) & 1)==0;
  }
 private:
  Status solidFast(uint16_t c,uint8_t bits,uint16_t color,size_t count);
  int channel_=-1;
  uint16_t *expanded_=nullptr;
  uint32_t started_=0;
  bool active_=false;
};
class Rp2040PioParallel8 : public Rp2040Pio {
 public:
  explicit Rp2040PioParallel8(ParallelPins pins) : Rp2040Pio(ParallelConfig(pins),8) {}
  explicit Rp2040PioParallel8(const ParallelConfig &config) : Rp2040Pio(config,8) {}
};
class Rp2040PioParallel16 : public Rp2040Pio {
 public:
  explicit Rp2040PioParallel16(ParallelPins pins) : Rp2040Pio(ParallelConfig(pins),16) {}
  explicit Rp2040PioParallel16(const ParallelConfig &config) : Rp2040Pio(config,16) {}
};
template<class GpioBus,uint8_t Width> class Rp2040BusAuto : public Transport {
 public:
  explicit Rp2040BusAuto(SpiPins pins) : Rp2040BusAuto(SpiConfig(pins)) {}
  explicit Rp2040BusAuto(const SpiConfig &config) : pio_(config),gpio_(config) {
    selected_=config.writeStrobePin>=0 ? static_cast<Transport *>(&gpio_) : static_cast<Transport *>(&pio_);
  }
  explicit Rp2040BusAuto(ParallelPins pins) : Rp2040BusAuto(ParallelConfig(pins)) {}
  explicit Rp2040BusAuto(const ParallelConfig &config) : pio_(config,Width),gpio_(config) {
    bool consecutive=true;
    for(uint8_t i=1;i<Width;++i) if(config.pins.data[i]!=config.pins.data[0]+i) consecutive=false;
    selected_=consecutive ? static_cast<Transport *>(&pio_) : static_cast<Transport *>(&gpio_);
  }
  ~Rp2040BusAuto() override { end(); }
  Status begin() override {
    Status status=selected_->begin();
    if(selected_==&pio_ && (status==Status::Busy || status==Status::NoMemory || status==Status::Unsupported)) {
      pio_.end(); selected_=&gpio_; status=selected_->begin();
    }
    return status;
  }
  void end() override { selected_->end(); }
  Status command(uint8_t c,const uint8_t *p,size_t n) override { return selected_->command(c,p,n); }
  Status commands(const uint8_t *p,size_t n) override { return selected_->commands(p,n); }
  Status command16(uint16_t c,const uint8_t *p,size_t n) override { return selected_->command16(c,p,n); }
  Status register16(uint16_t c,uint16_t value,uint8_t bits=8) override { return selected_->register16(c,value,bits); }
  Status data(const uint8_t *p,size_t n) override { return selected_->data(p,n); }
  Status pixels(uint8_t c,PixelView v,bool async) override { return selected_->pixels(c,v,async); }
  Status pixels16(uint16_t c,PixelView v,bool async) override { return selected_->pixels16(c,v,async); }
  Status dataPixels(PixelView v,bool async) override { return selected_->dataPixels(v,async); }
  Status fill(uint8_t c,uint16_t color,size_t n) override { return selected_->fill(c,color,n); }
  Status fill16(uint16_t c,uint16_t color,size_t n) override { return selected_->fill16(c,color,n); }
  Status dataFill(uint16_t color,size_t n) override { return selected_->dataFill(color,n); }
  Status setPixelFormat(PixelFormat format) override { return selected_->setPixelFormat(format); }
  Status read(uint16_t c,uint8_t *p,size_t n,uint8_t dummy=1,uint8_t bits=8) override { return selected_->read(c,p,n,dummy,bits); }
  Status readWords(uint16_t c,uint16_t *p,size_t n,uint8_t dummy=1,uint8_t bits=8) override { return selected_->readWords(c,p,n,dummy,bits); }
  Status wait() override { return selected_->wait(); }
  bool busy() const override { return selected_->busy(); }
  bool asynchronous() const override { return selected_->asynchronous(); }
  bool validOutputPin(int pin) const override { return selected_->validOutputPin(pin); }
  TransportInfo info() const override { return selected_->info(); }
  void output(int pin,bool high) override { selected_->output(pin,high); }
  void delayMs(uint32_t ms) override { selected_->delayMs(ms); }
 private:
  Rp2040Pio pio_;
  GpioBus gpio_;
  Transport *selected_;
};
using Rp2040PioSpi = Rp2040BusAuto<SoftwareSpi,1>;
using Rp2040Parallel8 = Rp2040BusAuto<Parallel8,8>;
using Rp2040Parallel16 = Rp2040BusAuto<Parallel16,16>;
}
#endif
