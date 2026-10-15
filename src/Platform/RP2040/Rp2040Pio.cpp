#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
#include "Rp2040Pio.h"
#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
#include <hardware/pio_instructions.h>
namespace nice_gfx {
Rp2040PioPort::Rp2040PioPort(const SpiConfig &config)
    : spiConfig_(config),parallelConfig_(ParallelPins(-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1)) {}
Rp2040PioPort::Rp2040PioPort(const ParallelConfig &config,uint8_t width)
    : width_(width),spiConfig_(SpiPins(-1,-1,-1,-1,-1)),parallelConfig_(config) {}
Status Rp2040PioPort::begin() {
  if (begun_) return Status::Ok;
  if (width_!=1 && width_!=8 && width_!=16) return Status::InvalidArgument;
  int pins[20],n=0,base,clock,cs,dc;
  uint32_t frequency;
  if (width_==1) {
    const auto &p=spiConfig_.pins; base=p.mosi; clock=p.clock; cs=p.cs; dc=p.dc;
    pins[n++]=base; pins[n++]=clock; pins[n++]=dc;
    if (p.miso>=0 && p.miso!=p.mosi) pins[n++]=p.miso;
    if (cs>=0) pins[n++]=cs;
    frequency=spiConfig_.frequency;
    if (spiConfig_.mode>3 || !spiConfig_.readFrequency) return Status::InvalidArgument;
  } else {
    const auto &p=parallelConfig_.pins; base=p.data[0]; clock=p.write; cs=p.cs; dc=p.dc;
    for (uint8_t i=0;i<width_;++i) {
      if (p.data[i]!=base+i) return Status::InvalidArgument;
      pins[n++]=p.data[i];
    }
    pins[n++]=clock; pins[n++]=dc;
    if (p.read>=0) pins[n++]=p.read;
    if (cs>=0) pins[n++]=cs;
    frequency=parallelConfig_.frequency;
  }
  if (!frequency) return Status::InvalidArgument;
  for (int i=0;i<n;++i) {
    if (!FastPin::valid(pins[i])) return Status::InvalidArgument;
    for (int j=0;j<i;++j) if (pins[i]==pins[j]) return Status::InvalidArgument;
  }
  // RP2350B PIO instances address either GPIO 0..31 or 16..47. Control
  // pins driven by SIO do not constrain that window; OUT and side-set do.
  const int gpioFirst = base < clock ? base : clock;
  const int dataLast = base + (width_ == 1 ? 1 : width_) - 1;
  const int gpioLast = dataLast > clock ? dataLast : clock;
  if (gpioLast >= 32 && gpioFirst < 16) return Status::Unsupported;
  const bool idle=width_==1 ? (spiConfig_.mode & 2)!=0 : true;
  if (width_==1) {
    const bool phase=(spiConfig_.mode & 1)!=0;
    instructions_[0]=pio_encode_pull(false,true) | pio_encode_sideset(1,idle);
    instructions_[1]=pio_encode_set(pio_x,7) | pio_encode_sideset(1,idle);
    instructions_[2]=pio_encode_out(pio_pins,1) | pio_encode_sideset(1,phase ? !idle : idle);
    instructions_[3]=pio_encode_jmp_x_dec(2) | pio_encode_sideset(1,phase ? idle : !idle);
    instructions_[4]=pio_encode_nop() | pio_encode_sideset(1,idle);
    program_.length=5;
  } else {
    instructions_[0]=pio_encode_pull(false,true) | pio_encode_sideset(1,1);
    instructions_[1]=pio_encode_out(pio_pins,width_) | pio_encode_sideset(1,0);
    instructions_[2]=pio_encode_nop() | pio_encode_sideset(1,1);
    program_.length=3;
  }
  program_.instructions=instructions_; program_.origin=-1;
#if defined(PICO_PIO_VERSION) && PICO_PIO_VERSION > 0
  uint state;
  if (!pio_claim_free_sm_and_add_program_for_gpio_range(&program_,&pio_,&state,&offset_,
      uint(gpioFirst),uint(gpioLast-gpioFirst+1),true)) return Status::Busy;
  sm_=int(state);
#else
  PIO choices[]={pio0,pio1};
  for (PIO candidate:choices) {
    if (!pio_can_add_program(candidate,&program_)) continue;
    int state=pio_claim_unused_sm(candidate,false);
    if (state>=0) { pio_=candidate; sm_=state; break; }
  }
  if (sm_<0) return Status::Busy;
  offset_=pio_add_program(pio_,&program_);
#endif
  cs_.attach(cs,true); dc_.attach(dc,true); clock_.attach(clock,idle);
  if (width_!=1) rd_.attach(parallelConfig_.pins.read,true);
  else if (spiConfig_.pins.miso>=0 && spiConfig_.pins.miso!=spiConfig_.pins.mosi) pinMode(spiConfig_.pins.miso,INPUT);
  for (uint8_t i=0;i<(width_==1 ? 1 : width_);++i) pio_gpio_init(pio_,base+i);
  pio_gpio_init(pio_,clock);
  pio_sm_set_consecutive_pindirs(pio_,sm_,base,width_==1 ? 1 : width_,true);
  pio_sm_set_consecutive_pindirs(pio_,sm_,clock,1,true);
  pio_sm_config config=pio_get_default_sm_config();
  sm_config_set_wrap(&config,offset_,offset_+program_.length-1);
  sm_config_set_out_pins(&config,base,width_==1 ? 1 : width_);
  sm_config_set_sideset(&config,1,false,false); sm_config_set_sideset_pins(&config,clock);
  sm_config_set_out_shift(&config,width_!=1,false,32); sm_config_set_fifo_join(&config,PIO_FIFO_JOIN_TX);
  const uint64_t clock256=uint64_t(clock_get_hz(clk_sys))*256;
  uint64_t denominator=uint64_t(frequency)*(width_==1 ? 2 : 3);
  uint64_t divider256=(clock256+denominator-1)/denominator;
  if (width_!=1) {
    // WR is low for one instruction, high for two. Bound the shortest phase by
    // half the requested period and by an explicit board-specific hold time.
    denominator=uint64_t(frequency)*2;
    uint64_t minimum=(clock256+denominator-1)/denominator;
    if (parallelConfig_.writeDelayUs) {
      const uint64_t hold=(clock256*parallelConfig_.writeDelayUs+999999)/1000000;
      if (hold>minimum) minimum=hold;
    }
    if (divider256<minimum) divider256=minimum;
  }
  // Round upward to the PIO divider's 1/256 tick, never to a shorter phase.
  if (divider256<256) divider256=256;
  if (divider256>0xFFFFFF) { end(); return Status::Unsupported; }
  sm_config_set_clkdiv(&config,float(divider256)/256.0f);
  pio_sm_init(pio_,sm_,offset_,&config); pio_sm_set_enabled(pio_,sm_,true);
  begun_=true; return Status::Ok;
}
void Rp2040PioPort::end() {
  if (begun_) { drain(); cs_.write(true); pio_sm_set_enabled(pio_,sm_,false); }
  if (sm_>=0) { pio_remove_program(pio_,&program_,offset_); pio_sm_unclaim(pio_,sm_); }
  sm_=-1; pio_=nullptr; begun_=false;
}
bool Rp2040PioPort::drained() const { return sm_<0 || (pio_sm_is_tx_fifo_empty(pio_,sm_) && pio_sm_get_pc(pio_,sm_)==offset_); }
void Rp2040PioPort::drain() const { while (!drained()) tight_loop_contents(); }
void Rp2040PioPort::word(uint16_t value) { pio_sm_put_blocking(pio_,sm_,width_==1 ? uint32_t(uint8_t(value)) << 24 : value); }
Status Rp2040PioPort::start(uint16_t command,uint8_t bits,bool reading) {
  if (reading && (width_==1 ? spiConfig_.pins.miso : parallelConfig_.pins.read)<0) return Status::Unsupported;
  drain(); cs_.write(false);
  if (bits) {
    dc_.write(false);
    if ((bits==16 && width_!=16) || (width_==1 && spiConfig_.paddedCommands)) word(command >> 8);
    word(width_==16 ? command : uint8_t(command)); drain();
  }
  dc_.write(true); if (reading) readMode(true); return Status::Ok;
}
void Rp2040PioPort::finish() {
  if (reading_) readMode(false); else drain();
  cs_.write(true);
}
void Rp2040PioPort::writeBlock(const uint8_t *p,size_t n,bool pixelData) {
  if (width_==1 && spiConfig_.paddedCommands && !pixelData) {
    while (n--) { word(0); word(*p++); } return;
  }
  if (width_==16 && pixelData && format_==PixelFormat::RGB565) {
    while (n>=2) { word(uint16_t(p[0]) << 8 | p[1]); p+=2; n-=2; }
  }
  while (n--) word(*p++);
}
void Rp2040PioPort::readMode(bool reading) {
  if (reading) {
    drain(); pio_sm_set_enabled(pio_,sm_,false);
    if (width_==1) {
      gpio_set_function(spiConfig_.pins.clock,GPIO_FUNC_SIO);
      clock_.write((spiConfig_.mode & 2)!=0);
      if(spiConfig_.pins.miso==spiConfig_.pins.mosi) {
        gpio_set_function(spiConfig_.pins.mosi,GPIO_FUNC_SIO); pinMode(spiConfig_.pins.mosi,INPUT);
      }
    } else for (uint8_t i=0;i<width_;++i) {
      gpio_set_function(parallelConfig_.pins.data[i],GPIO_FUNC_SIO); pinMode(parallelConfig_.pins.data[i],INPUT);
    }
  } else {
    if (width_==1) {
      pio_gpio_init(pio_,spiConfig_.pins.clock);
      if(spiConfig_.pins.miso==spiConfig_.pins.mosi) pio_gpio_init(pio_,spiConfig_.pins.mosi);
    }
    else for (uint8_t i=0;i<width_;++i) pio_gpio_init(pio_,parallelConfig_.pins.data[i]);
    pio_sm_set_enabled(pio_,sm_,true);
  }
  reading_=reading;
}
uint8_t Rp2040PioPort::readByte() {
  uint8_t result=0;
  if (width_==1) {
    bool idle=(spiConfig_.mode & 2)!=0,phase=(spiConfig_.mode & 1)!=0;
    uint32_t us=spiConfig_.readFrequency<500000 ? (500000+spiConfig_.readFrequency-1)/spiConfig_.readFrequency : 0;
    for (uint8_t bit=0x80;bit;bit>>=1) {
      clock_.write(!idle); if (us) delayMicroseconds(us);
      if (phase) clock_.write(idle);
      if (digitalRead(spiConfig_.pins.miso)) result |= bit;
      if (us) delayMicroseconds(us);
      if (!phase) clock_.write(idle);
    }
  } else {
    rd_.write(false); if (parallelConfig_.readDelayUs) delayMicroseconds(parallelConfig_.readDelayUs);
#if NUM_BANK0_GPIOS > 32
    result=uint8_t(gpio_get_all64() >> parallelConfig_.pins.data[0]);
#else
    result=uint8_t(gpio_get_all() >> parallelConfig_.pins.data[0]);
#endif
    rd_.write(true);
  }
  return result;
}
Status Rp2040PioPort::readBlock(uint8_t *p,size_t n) { while(n--) *p++=readByte(); return Status::Ok; }
Status Rp2040PioPort::register16(uint16_t command,uint16_t value,uint8_t bits) {
  if (!begun_) return Status::NotInitialized;
  if (bits!=8 && bits!=16) return Status::InvalidArgument;
  if (width_!=16) return Transport::register16(command,value,bits);
  Status status=start(command,bits,false); if (status==Status::Ok) { word(value); finish(); } return status;
}
Status Rp2040PioPort::readWords(uint16_t command,uint16_t *p,size_t n,uint8_t dummy,uint8_t bits) {
  if (!begun_) return Status::NotInitialized;
  if (width_!=16) return Status::Unsupported;
  if ((n && !p) || (bits!=0 && bits!=8 && bits!=16)) return Status::InvalidArgument;
  Status status=start(command,bits,true); if (status!=Status::Ok) return status;
  while(n || dummy) {
    rd_.write(false); if (parallelConfig_.readDelayUs) delayMicroseconds(parallelConfig_.readDelayUs);
#if NUM_BANK0_GPIOS > 32
    uint16_t value=uint16_t(gpio_get_all64() >> parallelConfig_.pins.data[0]);
#else
    uint16_t value=uint16_t(gpio_get_all() >> parallelConfig_.pins.data[0]);
#endif
    rd_.write(true);
    if (dummy) --dummy; else { *p++=value; --n; }
  }
  finish(); return Status::Ok;
}
TransportInfo Rp2040PioPort::info() const {
  TransportInfo i; i.lanes=width_;
  if (width_==1) {
    i.clock=spiConfig_.pins.clock; i.cs=spiConfig_.pins.cs; i.dc=spiConfig_.pins.dc;
    i.data[0]=spiConfig_.pins.mosi; i.data[1]=spiConfig_.pins.miso; i.frequency=spiConfig_.frequency;
  } else {
    i.write=parallelConfig_.pins.write; i.read=parallelConfig_.pins.read; i.cs=parallelConfig_.pins.cs;
    i.dc=parallelConfig_.pins.dc; i.frequency=parallelConfig_.frequency;
    for(uint8_t n=0;n<width_;++n) i.data[n]=parallelConfig_.pins.data[n];
  }
  return i;
}
bool Rp2040PioPort::validOutputPin(int pin) const {
  if (!FastPin::valid(pin)) return false;
  TransportInfo i=info(); if (pin==i.clock || pin==i.cs || pin==i.dc || pin==i.read || pin==i.write) return false;
  for (int data:i.data) if (pin==data) return false;
  return true;
}
Status Rp2040Pio::begin() {
  if (begun_) return Status::Ok;
  Status status=Rp2040PioPort::begin(); if (status!=Status::Ok) return status;
  channel_=dma_claim_unused_channel(false);
  if (channel_<0) { Rp2040PioPort::end(); return Status::Busy; }
  status=allocateDmaBuffers();
  if (status!=Status::Ok) { end(); return status; }
  if (width_==16) { expanded_=new(std::nothrow) uint16_t[1536]; if (!expanded_) { end(); return Status::NoMemory; } }
  return Status::Ok;
}
void Rp2040Pio::end() {
  releaseDmaBuffers();
  if (channel_>=0) { dma_channel_abort(channel_); dma_channel_unclaim(channel_); channel_=-1; }
  delete[] expanded_; expanded_=nullptr; Rp2040PioPort::end();
}
Status Rp2040Pio::dmaWrite(const uint8_t *data,size_t bytes) {
  dma_channel_config config=dma_channel_get_default_config(channel_);
  channel_config_set_read_increment(&config,true); channel_config_set_write_increment(&config,false);
  channel_config_set_dreq(&config,pio_get_dreq(pio_,sm_,true));
  size_t count=bytes; const void *source=data;
  if (width_==16) {
    channel_config_set_transfer_data_size(&config,DMA_SIZE_16);
    if (format_==PixelFormat::RGB565) { count=bytes/2; channel_config_set_bswap(&config,true); }
    else {
      if (bytes>1536) return Status::InvalidArgument;
      for(size_t i=0;i<bytes;++i) expanded_[i]=data[i]; source=expanded_;
    }
  } else channel_config_set_transfer_data_size(&config,DMA_SIZE_8);
  dma_channel_configure(channel_,&config,&pio_->txf[sm_],source,count,true);
  started_=millis(); active_=true; return Status::Ok;
}
Status Rp2040Pio::dmaPoll() {
  if (!active_) return Status::Ok;
  if (dma_channel_is_busy(channel_) || !drained()) {
    if (uint32_t(millis()-started_)<5000) return Status::Busy;
    dma_channel_abort(channel_); drain(); active_=false; return Status::IoError;
  }
  active_=false; return Status::Ok;
}
Status Rp2040Pio::solidFast(uint16_t command,uint8_t bits,uint16_t color,size_t count) {
  if (format_!=PixelFormat::RGB565) {
    if (!bits) return BufferedDmaBus<Rp2040PioPort>::dataFill(color,count);
    return bits==16 ? BufferedDmaBus<Rp2040PioPort>::fill16(command,color,count)
                    : BufferedDmaBus<Rp2040PioPort>::fill(uint8_t(command),color,count);
  }
  if (!begun_) return Status::NotInitialized;
  if (!count) return Status::Ok;
  Status status=start(command,bits,false); if (status!=Status::Ok) return status;
  // Repeat the pixel in DMA hardware. An 8-bit bus wraps its source over the two
  // wire bytes; a 16-bit bus repeatedly reads one native word. CPU work is O(1).
  alignas(4) uint16_t pattern=width_==16 ? color : uint16_t(color << 8 | color >> 8);
  dma_channel_config config=dma_channel_get_default_config(channel_);
  channel_config_set_write_increment(&config,false);
  channel_config_set_read_increment(&config,width_!=16);
  channel_config_set_transfer_data_size(&config,width_==16 ? DMA_SIZE_16 : DMA_SIZE_8);
  channel_config_set_dreq(&config,pio_get_dreq(pio_,sm_,true));
  if (width_!=16) channel_config_set_ring(&config,false,1);
  const size_t capacity=width_==16 ? UINT32_MAX : UINT32_MAX/2;
  while (count && status==Status::Ok) {
    size_t n=count < capacity ? count : capacity;
    dma_channel_configure(channel_,&config,&pio_->txf[sm_],&pattern,width_==16 ? n : n*2,true);
    started_=millis(); active_=true;
    do { status=dmaPoll(); } while (status==Status::Busy);
    count-=n;
  }
  finish(); return status;
}
}
#endif

#endif
