#pragma once
#include "../GxEpd2Types.h"
#include <string.h>

#define NGFX_GXEPD2_LOW 0
#define NGFX_GXEPD2_HIGH 1
#define NGFX_GXEPD2_INPUT 0
#define NGFX_GXEPD2_OUTPUT 1
#define NGFX_GXEPD2_INPUT_PULLUP 2
#define NGFX_GXEPD2_MSBFIRST 1
#define NGFX_GXEPD2_LSBFIRST 0
#define NGFX_GXEPD2_DEC 10
#define NGFX_GXEPD2_SPI_MODE0 0
#define NGFX_GXEPD2_SPI_MODE1 1
#define NGFX_GXEPD2_SPI_MODE2 2
#define NGFX_GXEPD2_SPI_MODE3 3
#define NGFX_GXEPD2_HEX 16
#if defined(__AVR__)
#include <avr/pgmspace.h>
#define NGFX_GXEPD2_PROGMEM PROGMEM
inline uint8_t ngfx_gxepd2_read_byte(const void *p){return pgm_read_byte(p);}
#else
#define NGFX_GXEPD2_PROGMEM
inline uint8_t ngfx_gxepd2_read_byte(const void*p){return *static_cast<const uint8_t*>(p);}
#endif

namespace nice_gfx {
class GxEpd2Hal{
 public:
  virtual ~GxEpd2Hal(){}
  virtual void pinMode(int,uint8_t)=0;
  virtual void writePin(int,bool)=0;
  virtual int readPin(int)=0;
  virtual void delayMs(uint32_t)=0;
  virtual void delayUs(uint32_t)=0;
  virtual uint32_t micros()=0;
  virtual void yield()=0;
  virtual void spiEnable(bool)=0;
  virtual void spiSettings(uint32_t,uint8_t)=0;
  virtual void spiWrite(uint8_t)=0;
  virtual uint8_t spiTransfer(uint8_t)=0;
  virtual void spiFlush()=0;
  virtual void spiByteTransaction(int cs,uint8_t mask,uint8_t value){
    if(mask==1)writePin(cs,false);else selectCs(mask,true);spiWrite(value);spiFlush();
    if(mask==1)writePin(cs,true);else selectCs(mask,false);
  }
  virtual void waitBusy(int,bool,uint32_t,uint32_t)=0;
  virtual void waitBusyCommand(int,bool,uint32_t,uint32_t,uint8_t,int,int,uint16_t){fault(Status::Unsupported);}
  virtual void fault(Status)=0;
  virtual Status status()const{return Status::Ok;}
  virtual int clockPin()const=0;
  virtual int mosiPin()const=0;
  virtual int misoPin()const=0;
  virtual int extraCsPin(unsigned)const{return -1;}
  virtual void selectCs(uint8_t,bool)=0;
};
GxEpd2Hal *gxEpd2CurrentHal();
GxEpd2Hal *gxEpd2SetCurrentHal(GxEpd2Hal*);
class GxEpd2HalScope{
 public:explicit GxEpd2HalScope(GxEpd2Hal*p):previous_(gxEpd2SetCurrentHal(p)){}
  ~GxEpd2HalScope(){gxEpd2SetCurrentHal(previous_);}
 private:GxEpd2Hal *previous_;
};
} // namespace nice_gfx

struct NgfxGxEpd2SpiSettings{
  uint32_t clock;uint8_t order,mode;
  NgfxGxEpd2SpiSettings(uint32_t hz=4000000,uint8_t o=1,uint8_t m=0):clock(hz),order(o),mode(m){}
};
class NgfxGxEpd2Spi{
 public:
  void begin(int=-1,int=-1,int=-1,int=-1){if(auto*h=nice_gfx::gxEpd2CurrentHal())h->spiEnable(true);}
  void end(){if(auto*h=nice_gfx::gxEpd2CurrentHal())h->spiEnable(false);}
  void beginTransaction(const NgfxGxEpd2SpiSettings&s){if(auto*h=nice_gfx::gxEpd2CurrentHal())h->spiSettings(s.clock,s.mode);}
  void endTransaction(){if(auto*h=nice_gfx::gxEpd2CurrentHal())h->spiFlush();}
  void write(uint8_t value){if(auto*h=nice_gfx::gxEpd2CurrentHal())h->spiWrite(value);}
  uint8_t transfer(uint8_t value){if(auto*h=nice_gfx::gxEpd2CurrentHal())return h->spiTransfer(value);return 0;}
};
class NgfxGxEpd2DiagnosticString{
 public:template<class...Args>NgfxGxEpd2DiagnosticString(Args&&...){}
  const char*c_str()const{return "GxEPD2";}
  friend NgfxGxEpd2DiagnosticString operator+(const NgfxGxEpd2DiagnosticString&a,const NgfxGxEpd2DiagnosticString&){return a;}
};
class NgfxGxEpd2Serial{
 public:template<class...Args>void begin(Args&&...){}
  template<class...Args>void print(Args&&...){}
  void println(const char *text){if(text&&strstr(text,"Busy Timeout"))if(auto*h=nice_gfx::gxEpd2CurrentHal())h->fault(nice_gfx::Status::IoError);}
  template<size_t N>void println(const char(&text)[N]){println(static_cast<const char*>(text));}
  template<class...Args>void println(Args&&...){}
};
