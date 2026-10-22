#include "Bus.h"
#include <new>

namespace nice_gfx {
// Amortize the Seeed core's synchronous ZeroDMA setup over more pixels. This
// is stack staging only; no persistent heap or full framebuffer is required.
#if defined(SEEED_WIO_TERMINAL)
static constexpr size_t streamBufferBytes = 1536;
#else
static constexpr size_t streamBufferBytes = transferBufferBytes;
#endif
static inline void delayMicrosecondsBounded(uint32_t us) {
  // AVR narrows the argument and multiplies it internally. Keep each call
  // within its safe range while preserving long, explicitly requested holds.
  while (us>1000) { delayMicroseconds(1000); us-=1000; }
  delayMicroseconds(us);
}
Status StreamBus::send(uint16_t c, uint8_t bits, const uint8_t *p, size_t n) {
  if (!begun_) return Status::NotInitialized;
  if (n && !p) return Status::InvalidArgument;
  Status status = start(c, bits, false);
  if (status != Status::Ok) return status;
  if (n) writeBlock(p, n, false);
  finish(); return Status::Ok;
}
Status StreamBus::image(uint16_t c, uint8_t bits, PixelView view) {
  if (!begun_) return Status::NotInitialized;
  if (!view.valid()) return Status::InvalidArgument;
  Status status = start(c, bits, false);
  if (status != Status::Ok) return status;
  uint8_t buffer[streamBufferBytes];
  const size_t bpp = wirePixelBytes(format_), capacity = sizeof(buffer) / bpp;
  const bool contiguous=view.stride==view.width;
  const size_t rows=contiguous ? 1 : view.height, columns=contiguous ? view.width*view.height : view.width;
  if (view.order == PixelOrder::Wire && format_ == PixelFormat::RGB565) {
    // The backend owns any hardware transfer limit. Already encoded pixels
    // need no conversion buffer and must not inherit its small chunk size.
    for (size_t row = 0; row < rows; ++row)
      writeBlock(view.data + 2 * row * view.stride, columns * 2, true);
    finish(); return Status::Ok;
  }
  for (size_t row = 0; row < rows; ++row) {
    const uint8_t *source = view.data + 2 * row * view.stride;
    size_t remaining = columns;
    while (remaining) {
      size_t count = remaining < capacity ? remaining : capacity;
      encodePixels(buffer, source, count, view.order, format_);
      writeBlock(buffer, count * bpp, true);
      source += count * 2; remaining -= count;
    }
  }
  finish(); return Status::Ok;
}
Status StreamBus::solid(uint16_t c, uint8_t bits, uint16_t color, size_t count) {
  if (!begun_) return Status::NotInitialized;
  if (!count) return Status::Ok;
  Status status = start(c, bits, false);
  if (status != Status::Ok) return status;
  uint8_t buffer[streamBufferBytes]; const size_t bpp = wirePixelBytes(format_);
  encodePixels(buffer, reinterpret_cast<const uint8_t *>(&color), 1, PixelOrder::Native, format_);
  if (writeStrobe_.number >= 0 && format_ == PixelFormat::RGB565) {
    // The padded SPI bridge retains the shifted colour after its first write.
    // Keep CS asserted until every extra WR pulse has latched that same colour.
    writeBlock(buffer,bpp,true);
    while (--count) { writeStrobe_.write(false); writeStrobe_.write(true); }
    finish(); return Status::Ok;
  }
  const size_t capacity = sizeof(buffer) / bpp;
  const size_t buffered = count < capacity ? count : capacity;
  // Small primitives must not initialize an entire bulk-transfer buffer.
  for (size_t i = bpp; i < buffered * bpp; i += bpp) memcpy(buffer+i, buffer, bpp);
  while (count) {
    size_t n = count < sizeof(buffer)/bpp ? count : sizeof(buffer)/bpp;
    writeBlock(buffer, n*bpp, true); count -= n;
  }
  finish(); return Status::Ok;
}
Status StreamBus::read(uint16_t c, uint8_t *p, size_t n, uint8_t dummy, uint8_t bits) {
  if (!begun_) return Status::NotInitialized;
  if ((n && !p) || (bits != 0 && bits != 8 && bits != 16)) return Status::InvalidArgument;
  Status status = start(c, bits, true);
  if (status != Status::Ok) return status;
  uint8_t discard;
  while (dummy-- && status == Status::Ok) status = readBlock(&discard, 1);
  if (status == Status::Ok && n) status = readBlock(p, n);
  finish(); return status;
}
static bool samePin(int first,int second) {
  return FastPin::gpioNumber(first)==FastPin::gpioNumber(second);
}
static bool spiPinsValid(const SpiConfig &c,bool hardware=false) {
  const int pins[] = {c.pins.clock,c.pins.mosi,c.pins.cs,c.pins.dc,c.pins.miso,c.writeStrobePin};
  if (!c.frequency || !c.readFrequency || c.mode > 3 || c.pins.clock < 0 || c.pins.mosi < 0 || c.pins.dc < 0)
    return false;
  if (c.writeStrobePin >= 0 && !c.paddedCommands) return false;
  for (unsigned i=0; i<6; ++i) {
    if (pins[i] < 0) continue;
    bool flashRoute=false;
#if defined(ESP8266)
    flashRoute=hardware && c.pins.clock==6 && c.pins.mosi==8 && (i==0 || i==1 || (i==4 && pins[i]==7));
#else
    (void)hardware;
#endif
    if (!flashRoute && !(i==4 ? FastPin::validInput(pins[i]) : FastPin::valid(pins[i]))) return false;
    for (unsigned j=0; j<i; ++j) if (samePin(pins[i],pins[j]) && !(i==4 && j==1)) return false;
  }
  return true;
}
static bool unusedSpiPin(const SpiPins &p, int pin) {
  return FastPin::valid(pin) && !samePin(pin,p.clock) && !samePin(pin,p.mosi) && !samePin(pin,p.miso)
      && !samePin(pin,p.cs) && !samePin(pin,p.dc);
}
static TransportInfo spiInfo(const SpiConfig &c) {
  TransportInfo i; i.frequency=c.frequency; i.clock=c.pins.clock; i.cs=c.pins.cs;
  i.dc=c.pins.dc; i.write=c.writeStrobePin;
  i.data[0]=c.pins.mosi; i.data[1]=c.pins.miso; i.lanes=1; return i;
}
struct ArduinoSpiOwner { const void *key=nullptr,*object=nullptr; ArduinoSpi *owner=nullptr; };
static ArduinoSpiOwner arduinoSpiOwners[8];
Status ArduinoSpi::begin() {
  if (begun_) return Status::Ok;
  if (!spiPinsValid(config_,true)) return Status::InvalidArgument;
  const auto &p=config_.pins;
  const void *key=&spi_;
#if defined(ARDUINO_ARCH_STM32)
  const void *mosi=pinmap_peripheral(digitalPinToPinName(p.mosi),PinMap_SPI_MOSI);
  const void *clock=pinmap_peripheral(digitalPinToPinName(p.clock),PinMap_SPI_SCLK);
  if (!mosi || mosi!=clock || (p.miso>=0 && p.miso!=p.mosi && mosi!=pinmap_peripheral(digitalPinToPinName(p.miso),PinMap_SPI_MISO)))
    return Status::InvalidArgument;
  key=mosi;
#elif defined(ESP8266)
  key=reinterpret_cast<const void *>(uintptr_t(1));
#elif defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
  // Arduino-Pico's setSCK/setTX/setRX panic on invalid routes. Validate the
  // selected standard peripheral before invoking those APIs.
  const int peripheral=&spi_==&SPI ? 0 : (&spi_==&SPI1 ? 1 : -1);
  if(peripheral<0) return Status::Unsupported;
  if((p.clock & 3)!=2 || ((p.clock >> 3)&1)!=peripheral ||
     (p.mosi & 3)!=3 || ((p.mosi >> 3)&1)!=peripheral ||
     (p.miso>=0 && p.miso!=p.mosi && ((p.miso & 3)!=0 || ((p.miso >> 3)&1)!=peripheral)))
    return Status::InvalidArgument;
  key=reinterpret_cast<const void *>(uintptr_t(1+((p.clock >> 3)&1)));
#endif
  int slot=-1;
  for (int i=0;i<8;++i) {
    if (arduinoSpiOwners[i].owner && (arduinoSpiOwners[i].key==key || arduinoSpiOwners[i].object==&spi_)) return Status::Busy;
    if (!arduinoSpiOwners[i].owner && slot<0) slot=i;
  }
  if (slot<0) return Status::Busy;
  Status routed=beginPeripheral(); if(routed!=Status::Ok) return routed;
  cs_.attach(p.cs,true); dc_.attach(p.dc,true);
  writeStrobe_.attach(config_.writeStrobePin,true);
  arduinoSpiOwners[slot].key=key; arduinoSpiOwners[slot].object=&spi_; arduinoSpiOwners[slot].owner=this; ownerSlot_=int8_t(slot);
  begun_=true; return Status::Ok;
}
Status ArduinoSpi::beginPeripheral() {
  const auto &p=config_.pins;
  const int input=samePin(p.miso,p.mosi) ? -1 : p.miso;
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_CI13XX)
  if (!spi_.begin(p.clock,input,p.mosi,-1)) return Status::InvalidArgument;
#elif defined(ARDUINO_ARCH_STM32)
  spi_.setMOSI(p.mosi); if (input>=0) spi_.setMISO(input); else spi_.setMISO(NC); spi_.setSCLK(p.clock); spi_.begin();
#elif defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
  spi_.end();
  if (!spi_.setSCK(p.clock) || !spi_.setTX(p.mosi) || !spi_.setRX(input>=0 ? input : uint8_t(-1)))
    return Status::InvalidArgument;
  spi_.begin();
#elif defined(ARDUINO_ARCH_MBED)
  // MbedSPI binds physical PinNames at construction and has no remapping API.
  // Validate the selected board peripheral using its variant's actual mapping.
  const auto clock=digitalPinToPinName(p.clock),mosi=digitalPinToPinName(p.mosi);
  const auto miso=input>=0 ? digitalPinToPinName(input) : NC;
  bool route=&spi_==&SPI && clock==SPI_SCK && mosi==SPI_MOSI && (input<0 || miso==SPI_MISO);
#if defined(SPI_HOWMANY) && SPI_HOWMANY > 1
  route=route || (&spi_==&SPI1 && clock==SPI_SCK1 && mosi==SPI_MOSI1 && (input<0 || miso==SPI_MISO1));
#endif
  if (!route) return Status::Unsupported;
  spi_.begin();
#elif defined(ESP8266)
  // ESP8266 HSPI has only these two hardware routes; reject unsupported routes.
  if (p.clock == 14 && p.mosi == 13 && (input < 0 || input == 12)) spi_.pins(14,12,13,15);
  else if (p.clock == 6 && p.mosi == 8 && (input < 0 || input == 7)) spi_.pins(6,7,8,0);
  else return Status::Unsupported;
  spi_.begin();
#elif defined(ARDUINO_ARCH_SAMD)
  // SAMD peripherals have fixed variant routes. In particular Wio Terminal's
  // LCD is SPI3 (SERCOM7), SD is SPI2, and the header is SPI. Never silently
  // accept the header pins for an explicitly selected onboard peripheral.
  bool route=&spi_==&SPI && p.clock==SCK && p.mosi==MOSI && (input<0 || input==MISO);
#if SPI_INTERFACES_COUNT > 1
  route=route || (&spi_==&SPI1 && p.clock==PIN_SPI1_SCK && p.mosi==PIN_SPI1_MOSI && (input<0 || input==PIN_SPI1_MISO));
#endif
#if SPI_INTERFACES_COUNT > 2
  route=route || (&spi_==&SPI2 && p.clock==PIN_SPI2_SCK && p.mosi==PIN_SPI2_MOSI && (input<0 || input==PIN_SPI2_MISO));
#endif
#if SPI_INTERFACES_COUNT > 3
  route=route || (&spi_==&SPI3 && p.clock==PIN_SPI3_SCK && p.mosi==PIN_SPI3_MOSI && (input<0 || input==PIN_SPI3_MISO));
#endif
  if (!route) return Status::Unsupported;
  spi_.begin();
#else
  // Cores without a pin remapping API must use the board's physical SPI route.
  if (p.clock != SCK || p.mosi != MOSI || (input >= 0 && input != MISO)) return Status::Unsupported;
  spi_.begin();
#endif
  return Status::Ok;
}
void ArduinoSpi::end() {
  if (begun_) { cs_.write(true); spi_.end(); begun_=false; }
  if (ownerSlot_>=0) { arduinoSpiOwners[ownerSlot_].owner=nullptr; arduinoSpiOwners[ownerSlot_].key=nullptr; arduinoSpiOwners[ownerSlot_].object=nullptr; ownerSlot_=-1; }
}
bool ArduinoSpi::validOutputPin(int pin) const { return unusedSpiPin(config_.pins,pin) && !samePin(pin,config_.writeStrobePin); }
TransportInfo ArduinoSpi::info() const { return spiInfo(config_); }
Status ArduinoSpi::start(uint16_t c, uint8_t bits, bool reading) {
  if (reading && config_.pins.miso < 0) return Status::Unsupported;
  // Arduino's mode constants are core-specific (STM32 mode 3 is not literal 3).
  const uint8_t modes[]={SPI_MODE0,SPI_MODE1,SPI_MODE2,SPI_MODE3};
  spi_.beginTransaction(SPISettings(reading ? config_.readFrequency : config_.frequency,MSBFIRST,modes[config_.mode]));
  cs_.write(false);
  if (bits) {
    dc_.write(false);
    if (bits == 16 || config_.paddedCommands) spi_.transfer(uint8_t(c >> 8));
    spi_.transfer(uint8_t(c));
  }
  dc_.write(true);
  if(reading && samePin(config_.pins.miso,config_.pins.mosi)) {
    spi_.endTransaction(); spi_.end();
    sdaClock_.attach(config_.pins.clock,(config_.mode & 2)!=0);
    pinMode(config_.pins.mosi,INPUT); sdaReading_=true;
  }
  return Status::Ok;
}
void ArduinoSpi::finish() {
  cs_.write(true);
  if(sdaReading_) {
    pinMode(config_.pins.mosi,OUTPUT); sdaReading_=false;
    if(beginPeripheral()!=Status::Ok) begun_=false;
  } else spi_.endTransaction();
}
void ArduinoSpi::writeBlock(const uint8_t *p, size_t n, bool pixels) {
  if (!pixels && config_.paddedCommands) {
    uint8_t padded[transferBufferBytes];
    while (n) {
      size_t count=n < sizeof(padded)/2 ? n : sizeof(padded)/2;
      for(size_t i=0;i<count;++i) { padded[2*i]=0; padded[2*i+1]=p[i]; }
      writeBlock(padded,count*2,true); p+=count; n-=count;
    }
    return;
  }
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP8266) || defined(ARDUINO_ARCH_CI13XX)
  spi_.writeBytes(p,n);
#elif defined(ARDUINO_ARCH_STM32)
  // STM32's public size_t overload forwards to spi_transfer(uint16_t len).
  // Keep full-frame direct writes below that limit without splitting pixels.
  while (n) {
    const size_t count=n < 65534U ? n : 65534U;
    spi_.transfer(p,nullptr,count); p+=count; n-=count;
  }
#elif defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
  spi_.transfer(p,nullptr,n);
#elif defined(SEEED_WIO_TERMINAL)
  // DMA setup costs more than polling short command parameters/pixel spans.
  if (n <= 16) {
    while (n--) spi_.transfer(*p++);
    return;
  }
  // Seeed SAMD's const TX / null RX overload uses ZeroDMA synchronously;
  // unlike the standard in-place API it leaves immutable flash data intact.
  spi_.transfer(p,nullptr,n);
#else
  // The standard SPI API is in-place; use bounded staging to preserve input.
  uint8_t buffer[transferBufferBytes];
  while (n) { size_t count=n < sizeof(buffer) ? n : sizeof(buffer); memcpy(buffer,p,count);
    spi_.transfer(buffer,count); p+=count; n-=count; }
#endif
}
Status ArduinoSpi::readBlock(uint8_t *p, size_t n) {
  if(sdaReading_) {
    const bool idle=(config_.mode & 2)!=0,phase=(config_.mode & 1)!=0;
    const uint32_t us=config_.readFrequency<500000 ? (500000+config_.readFrequency-1)/config_.readFrequency : 0;
    while(n--) {
      uint8_t value=0;
      for(uint8_t bit=0x80;bit;bit>>=1) {
        if(phase) sdaClock_.write(!idle);
        if(us) delayMicrosecondsBounded(us);
        sdaClock_.write(phase ? idle : !idle);
        if(digitalRead(config_.pins.mosi)) value |= bit;
        if(us) delayMicrosecondsBounded(us);
        if(!phase) sdaClock_.write(idle);
      }
      *p++=value;
    }
    return Status::Ok;
  }
  memset(p,0,n); spi_.transfer(p,n); return Status::Ok;
}
Status SoftwareSpi::begin() {
  if (begun_) return Status::Ok;
  if (!spiPinsValid(config_)) return Status::InvalidArgument;
  const auto &p=config_.pins;
  clock_.attach(p.clock,(config_.mode & 2) != 0); mosi_.attach(p.mosi);
  cs_.attach(p.cs,true); dc_.attach(p.dc,true);
  writeStrobe_.attach(config_.writeStrobePin,true);
  if (p.miso >= 0 && !samePin(p.miso,p.mosi)) pinMode(p.miso,INPUT);
  halfPeriodUs_=config_.frequency < 500000 ? (500000 + config_.frequency-1)/config_.frequency : 0;
  begun_=true; return Status::Ok;
}
bool SoftwareSpi::validOutputPin(int pin) const { return unusedSpiPin(config_.pins,pin) && !samePin(pin,config_.writeStrobePin); }
TransportInfo SoftwareSpi::info() const { return spiInfo(config_); }
uint8_t SoftwareSpi::exchange(uint8_t value, bool read) {
  uint8_t result=0; const bool idle=(config_.mode & 2)!=0, phase=(config_.mode & 1)!=0;
  for (uint8_t bit=0x80; bit; bit >>= 1) {
    if (phase) clock_.write(!idle);
    if(!sdaReading_) mosi_.write((value & bit)!=0);
    if (halfPeriodUs_) delayMicrosecondsBounded(halfPeriodUs_);
    if (!phase) clock_.write(!idle); else clock_.write(idle);
    if (read && digitalRead(config_.pins.miso)) result |= bit;
    if (halfPeriodUs_) delayMicrosecondsBounded(halfPeriodUs_);
    if (!phase) clock_.write(idle);
  }
  return result;
}
Status SoftwareSpi::start(uint16_t c, uint8_t bits, bool reading) {
  if (reading && config_.pins.miso < 0) return Status::Unsupported;
  uint32_t hz=reading ? config_.readFrequency : config_.frequency;
  halfPeriodUs_=hz < 500000 ? (500000+hz-1)/hz : 0;
  cs_.write(false);
  if (bits) { dc_.write(false); if (bits==16 || config_.paddedCommands) exchange(uint8_t(c >> 8),false); exchange(uint8_t(c),false); }
  dc_.write(true);
  if(reading && samePin(config_.pins.miso,config_.pins.mosi)) { pinMode(config_.pins.mosi,INPUT); sdaReading_=true; }
  return Status::Ok;
}
void SoftwareSpi::finish() {
  cs_.write(true); if(sdaReading_) { pinMode(config_.pins.mosi,OUTPUT); sdaReading_=false; }
}
void SoftwareSpi::writeBlock(const uint8_t *p, size_t n, bool pixels) {
  while (n--) { if (!pixels && config_.paddedCommands) exchange(0,false); exchange(*p++,false); }
}
Status SoftwareSpi::readBlock(uint8_t *p, size_t n) { while (n--) *p++=exchange(0,true); return Status::Ok; }

Status ParallelBus::begin() {
  if (begun_) return Status::Ok;
  if (!config_.frequency) return Status::InvalidArgument;
  const auto &p=config_.pins;
  int pins[20]; for (uint8_t i=0; i<width_; ++i) pins[i]=p.data[i];
  pins[width_]=p.write; pins[width_+1]=p.dc; pins[width_+2]=p.cs; pins[width_+3]=p.read;
  for (uint8_t i=0; i<width_+4; ++i) {
    if (pins[i] < 0 && i >= width_+2) continue;
    if (!FastPin::valid(pins[i])) return Status::InvalidArgument;
    for (uint8_t j=0; j<i; ++j) if (samePin(pins[i],pins[j])) return Status::InvalidArgument;
  }
  writeDelayUs_=config_.writeDelayUs;
  halfCycles_=0;
  if (writeDelayUs_) {
    // An explicit hold time cannot shorten the frequency's half-period.
    const uint32_t frequencyDelayUs=uint32_t((uint64_t(500000)+config_.frequency-1)/config_.frequency);
    if (writeDelayUs_<frequencyDelayUs) writeDelayUs_=frequencyDelayUs;
  } else {
    uint32_t cpuHz=0;
#if defined(ARDUINO_ARCH_STM32)
    cpuHz=SystemCoreClock;
#elif defined(F_CPU)
    cpuHz=F_CPU;
#endif
    if (cpuHz) {
      const uint64_t denominator=uint64_t(config_.frequency)*2;
      halfCycles_=uint32_t((uint64_t(cpuHz)+denominator-1)/denominator);
      if (!halfCycles_) halfCycles_=1;
    } else {
      writeDelayUs_=uint32_t((uint64_t(500000)+config_.frequency-1)/config_.frequency);
      if (!writeDelayUs_) writeDelayUs_=1;
    }
  }
  for (uint8_t i=0; i<width_; ++i) data_[i].attach(p.data[i]);
  direct_=true; for(uint8_t i=0;i<width_;++i) if(!data_[i].set) direct_=false;
  wr_.attach(p.write,true); rd_.attach(p.read,true); cs_.attach(p.cs,true); dc_.attach(p.dc,true);
  // Group by physical register. Nibble LUTs retain arbitrary pin routing while
  // reducing each pixel to 2/4 lookups and one set/clear per GPIO bank.
  groups_=0;
  for (uint8_t i=0; i<width_; ++i) {
    if (!data_[i].set) continue;
    bool previous=false;
    for (uint8_t j=0; j<i; ++j) if (data_[j].set==data_[i].set) previous=true;
    if (!previous) ++groups_;
  }
  if (groups_) {
    group_=new(std::nothrow) Group[groups_]();
    if (!group_) { groups_=0; return Status::NoMemory; }
    uint8_t used=0;
    for (uint8_t i=0; i<width_; ++i) {
      if (!data_[i].set) continue;
      uint8_t g=0; while (g<used && group_[g].set != data_[i].set) ++g;
      if (g==used) { ++used; group_[g].set=data_[i].set; group_[g].clear=data_[i].clear; group_[g].combined=data_[i].combined; }
      group_[g].mask |= data_[i].mask;
      for (uint8_t value=0; value<16; ++value)
        if (value & (1 << (i & 3))) group_[g].table[i/4][value] |= data_[i].mask;
    }
  }
  begun_=true; return Status::Ok;
}
void ParallelBus::end() {
  if (begun_) { wr_.write(true); rd_.write(true); cs_.write(true); begun_=false; }
  delete[] group_; group_=nullptr; groups_=0;
}
bool ParallelBus::validOutputPin(int pin) const {
  const auto &p=config_.pins;
  if (!FastPin::valid(pin) || samePin(pin,p.write) || samePin(pin,p.read) || samePin(pin,p.cs) || samePin(pin,p.dc)) return false;
  for (uint8_t i=0; i<width_; ++i) if (samePin(pin,p.data[i])) return false;
  return true;
}
TransportInfo ParallelBus::info() const {
  TransportInfo i; i.write=config_.pins.write; i.read=config_.pins.read; i.cs=config_.pins.cs;
  i.dc=config_.pins.dc; i.lanes=width_; i.frequency=config_.frequency;
  for (uint8_t n=0; n<width_; ++n) i.data[n]=config_.pins.data[n];
  return i;
}
void ParallelBus::word(uint16_t value) {
  for (uint8_t i=0; i<groups_; ++i) {
    auto &g=group_[i];
    uint32_t bits=g.table[0][value & 15] | g.table[1][(value >> 4) & 15];
    if (width_==16) bits |= g.table[2][(value >> 8) & 15] | g.table[3][value >> 12];
    if (g.combined) *g.set = bits | ((g.mask & ~bits) << 16);
    else { *g.clear=g.mask & ~bits; *g.set=bits; }
  }
  if (!direct_) for (uint8_t i=0; i<width_; ++i) if (!data_[i].set) data_[i].write((value & (1u << i))!=0);
  writePulse();
}
void ParallelBus::writePulse() {
  // These cached lower bounds apply to both phases. Loop/branch overhead makes
  // the GPIO rate conservative, never shorter than the configured half-period.
  wr_.write(false);
  waitWritePhase();
  wr_.write(true);
  waitWritePhase();
}
void ParallelBus::waitWritePhase() {
  if (writeDelayUs_) {
    delayMicrosecondsBounded(writeDelayUs_);
  }
  else for (uint32_t n=halfCycles_; n; --n) {
#if defined(__GNUC__)
    __asm__ __volatile__("nop");
#endif
  }
}
void ParallelBus::direction(bool out) {
  for (uint8_t i=0; i<width_; ++i) pinMode(config_.pins.data[i],out ? OUTPUT : INPUT);
}
Status ParallelBus::start(uint16_t c, uint8_t bits, bool reading) {
  if (reading && config_.pins.read < 0) return Status::Unsupported;
  cs_.write(false);
  if (bits) {
    dc_.write(false);
    if (bits==16 && width_==8) word(c >> 8);
    word(width_==16 ? c : uint8_t(c));
  }
  dc_.write(true); reading_=reading;
  if (reading) direction(false);
  return Status::Ok;
}
void ParallelBus::finish() {
  rd_.write(true); cs_.write(true);
  if (reading_) { direction(true); reading_=false; }
}
void ParallelBus::writeBlock(const uint8_t *p, size_t n, bool pixelData) {
  if (width_==16 && pixelData && format_==PixelFormat::RGB565) {
    while (n>=2) { word(uint16_t(p[0]) << 8 | p[1]); p+=2; n-=2; }
  }
  while (n--) word(*p++);
}
Status ParallelBus::readBlock(uint8_t *p, size_t n) {
  while (n--) {
    rd_.write(false); if (config_.readDelayUs) delayMicrosecondsBounded(config_.readDelayUs);
    uint8_t value=0;
    for (uint8_t i=0; i<8; ++i) if (digitalRead(config_.pins.data[i])) value |= 1 << i;
    rd_.write(true); *p++=value;
  }
  return Status::Ok;
}
Status ParallelBus::register16(uint16_t command, uint16_t value, uint8_t bits) {
  if (!begun_) return Status::NotInitialized;
  if (bits!=8 && bits!=16) return Status::InvalidArgument;
  if (width_==8) return Transport::register16(command,value,bits);
  Status status=start(command,bits,false);
  if (status==Status::Ok) { word(value); finish(); }
  return status;
}
Status ParallelBus::readWords(uint16_t command, uint16_t *data, size_t count, uint8_t dummy, uint8_t bits) {
  if (!begun_) return Status::NotInitialized;
  if (width_!=16) return Status::Unsupported;
  if ((count && !data) || (bits!=0 && bits!=8 && bits!=16)) return Status::InvalidArgument;
  Status status=start(command,bits,true);
  if (status!=Status::Ok) return status;
  while (dummy || count) {
    rd_.write(false); if (config_.readDelayUs) delayMicrosecondsBounded(config_.readDelayUs);
    uint16_t value=0;
    for (uint8_t i=0; i<16; ++i) if (digitalRead(config_.pins.data[i])) value |= uint16_t(1) << i;
    rd_.write(true);
    if (dummy) --dummy;
    else { *data++=value; --count; }
  }
  finish(); return Status::Ok;
}
Status ParallelBus::solidFast(uint16_t command,uint8_t bits,uint16_t color,size_t count) {
  if(width_!=16 || format_!=PixelFormat::RGB565) {
    if(!bits) return StreamBus::dataFill(color,count);
    return bits==16 ? StreamBus::fill16(command,color,count) : StreamBus::fill(uint8_t(command),color,count);
  }
  if(!begun_) return Status::NotInitialized;
  if(!count) return Status::Ok;
  Status status=start(command,bits,false); if(status!=Status::Ok) return status;
  word(color);
  while(--count) writePulse();
  finish(); return Status::Ok;
}
}
