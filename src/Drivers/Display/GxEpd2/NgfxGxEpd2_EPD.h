#pragma once
// Based on GxEPD2_EPD by Jean-Marc Zingg, ZinggJM/GxEPD2, GNU GPL v3.
// Upstream de82887e77a78528ea386e68bba2cb0291d2c319; see LICENSE.
// Modified for per-instance HAL recording, cooperative playback and read workers.
// Independent protocol HAL for the privately namespaced GPLv3 GxEPD2 drivers.
// The surrounding renderer/UI is NICE_GFX; no Adafruit_GFX renderer is used.
#include "NgfxGxEpd2.h"
#include <inttypes.h>
#include <stdio.h>
#include "../../../Display/Backends/GxEpd2Hal.h"
class NgfxGxEpd2_EPD{
 public:
  const uint16_t WIDTH,HEIGHT;
  const NgfxGxEpd2::Panel panel;
  const bool hasColor,hasPartialUpdate,hasFastPartialUpdate;
  NgfxGxEpd2_EPD(int16_t cs,int16_t dc,int16_t rst,int16_t busy,int16_t level,uint32_t timeout,
    uint16_t w,uint16_t h,NgfxGxEpd2::Panel p,bool color,bool partial,bool fast)
    :WIDTH(w),HEIGHT(h),panel(p),hasColor(color),hasPartialUpdate(partial),hasFastPartialUpdate(fast),
     _cs(cs),_dc(dc),_rst(rst),_busy(busy),_busy_level(level),_busy_timeout(timeout),_pSPIx(&_ngfxSpi){}
  virtual ~NgfxGxEpd2_EPD(){}
  virtual void init(uint32_t=0){init(0,true,10,false);}
  virtual void init(uint32_t,bool initial,uint16_t reset=10,bool pull=false){
    _initial_write=initial;_initial_refresh=initial;_pulldown_rst_mode=pull;_reset_duration=reset;
    _power_is_on=false;_using_partial_mode=false;_hibernating=false;_init_display_done=false;
    if(_cs>=0){_ngfxDigitalWrite(_cs,NGFX_GXEPD2_HIGH);_ngfxPinMode(_cs,NGFX_GXEPD2_OUTPUT);_ngfxDigitalWrite(_cs,NGFX_GXEPD2_HIGH);}
    _reset();_pSPIx->begin();
    if(_rst>=0){_ngfxDigitalWrite(_rst,NGFX_GXEPD2_HIGH);_ngfxPinMode(_rst,NGFX_GXEPD2_OUTPUT);_ngfxDigitalWrite(_rst,NGFX_GXEPD2_HIGH);}
    if(_cs>=0){_ngfxDigitalWrite(_cs,NGFX_GXEPD2_HIGH);_ngfxPinMode(_cs,NGFX_GXEPD2_OUTPUT);_ngfxDigitalWrite(_cs,NGFX_GXEPD2_HIGH);}
    if(_dc>=0){_ngfxDigitalWrite(_dc,NGFX_GXEPD2_HIGH);_ngfxPinMode(_dc,NGFX_GXEPD2_OUTPUT);_ngfxDigitalWrite(_dc,NGFX_GXEPD2_HIGH);}
    if(_busy>=0)_ngfxPinMode(_busy,NGFX_GXEPD2_INPUT);
  }
  virtual void end(){_pSPIx->end();_ngfxPinMode(_cs,NGFX_GXEPD2_INPUT);_ngfxPinMode(_dc,NGFX_GXEPD2_INPUT);_ngfxPinMode(_rst,NGFX_GXEPD2_INPUT);}
  virtual void clearScreen(uint8_t){unsupported();}
  virtual void writeScreenBuffer(uint8_t){unsupported();}
  virtual void writeImage(const uint8_t*,int16_t,int16_t,int16_t,int16_t,bool=false,bool=false,bool=false){unsupported();}
  virtual void writeImageForFullRefresh(const uint8_t*p,int16_t x,int16_t y,int16_t w,int16_t h,bool inv=false,bool mirror=false,bool pgm=false){writeImage(p,x,y,w,h,inv,mirror,pgm);}
  virtual void writeImageToPrevious(const uint8_t*,int16_t,int16_t,int16_t,int16_t,bool=false,bool=false,bool=false){}
  virtual void writeImagePart(const uint8_t*,int16_t,int16_t,int16_t,int16_t,int16_t,int16_t,int16_t,int16_t,bool=false,bool=false,bool=false){unsupported();}
  virtual void writeScreenBufferAgain(uint8_t value=0xff){writeScreenBuffer(value);}
  virtual void writeImageAgain(const uint8_t*p,int16_t x,int16_t y,int16_t w,int16_t h,bool inv=false,bool mirror=false,bool pgm=false){writeImage(p,x,y,w,h,inv,mirror,pgm);}
  virtual void writeImagePartAgain(const uint8_t*p,int16_t xp,int16_t yp,int16_t wb,int16_t hb,int16_t x,int16_t y,int16_t w,int16_t h,bool inv=false,bool mirror=false,bool pgm=false){writeImagePart(p,xp,yp,wb,hb,x,y,w,h,inv,mirror,pgm);}
  virtual void writeDemoBitmap(const uint8_t*,const uint8_t*,int16_t,int16_t,int16_t,int16_t,int16_t=0,bool=false,bool=false){unsupported();}
  virtual void drawDemoBitmap(const uint8_t*,const uint8_t*,int16_t,int16_t,int16_t,int16_t,int16_t=0,bool=false,bool=false){unsupported();}
  virtual void refresh(bool=false){unsupported();}
  virtual void refresh(int16_t,int16_t,int16_t,int16_t){unsupported();}
  virtual void powerOff(){unsupported();}
  virtual void hibernate(){unsupported();}
  virtual void setPaged(){}
  virtual void selectFastFullUpdate(bool){}
  virtual void drawNativeColors(){unsupported();}
  void enableRead(int16_t clock,int16_t mosi){_sck=clock;_mosi=mosi;}
  void selectSPI(NgfxGxEpd2Spi &spi,NgfxGxEpd2SpiSettings settings){_pSPIx=&spi;_spi_settings=settings;}
  void bindHal(nice_gfx::GxEpd2Hal*,int vcom=-1){_ngfxVcomMillivolts=vcom;}
  bool busyActiveLevel()const{return _busy_level!=0;}
  static uint16_t gx_uint16_min(uint16_t a,uint16_t b){return a<b?a:b;}
  static uint16_t gx_uint16_max(uint16_t a,uint16_t b){return a>b?a:b;}
 protected:
  static void unsupported(){if(auto*h=nice_gfx::gxEpd2CurrentHal())h->fault(nice_gfx::Status::Unsupported);}
  static void _ngfxPinMode(int pin,uint8_t mode){if(pin>=0)if(auto*h=nice_gfx::gxEpd2CurrentHal())h->pinMode(pin,mode);}
  static void _ngfxDigitalWrite(int pin,int level){if(pin>=0)if(auto*h=nice_gfx::gxEpd2CurrentHal())h->writePin(pin,level!=0);}
  static int _ngfxDigitalRead(int pin){if(auto*h=nice_gfx::gxEpd2CurrentHal())return h->readPin(pin);return 0;}
  static void _ngfxDelay(uint32_t ms){if(auto*h=nice_gfx::gxEpd2CurrentHal())h->delayMs(ms);}
  static void _ngfxDelayMicroseconds(uint32_t us){if(auto*h=nice_gfx::gxEpd2CurrentHal())h->delayUs(us);}
  static uint32_t _ngfxMicros(){if(auto*h=nice_gfx::gxEpd2CurrentHal())return h->micros();return 0;}
  static void _ngfxYield(){if(auto*h=nice_gfx::gxEpd2CurrentHal())h->yield();}
  static int _ngfxDefaultClock(){if(auto*h=nice_gfx::gxEpd2CurrentHal())return h->clockPin();return -1;}
  static int _ngfxDefaultMosi(){if(auto*h=nice_gfx::gxEpd2CurrentHal())return h->mosiPin();return -1;}
  static int _ngfxDefaultMiso(){if(auto*h=nice_gfx::gxEpd2CurrentHal())return h->misoPin();return -1;}
  void _ngfxCsSelect(uint8_t mask){_ngfxCsMask=mask;}
  void _select(bool active){if(_ngfxCsMask!=1){if(auto*h=nice_gfx::gxEpd2CurrentHal())h->selectCs(_ngfxCsMask,active);}
    else _ngfxDigitalWrite(_cs,active?NGFX_GXEPD2_LOW:NGFX_GXEPD2_HIGH);}
  void _reset(){if(_rst<0)return;
    if(_pulldown_rst_mode){_ngfxDigitalWrite(_rst,0);_ngfxPinMode(_rst,NGFX_GXEPD2_OUTPUT);_ngfxDigitalWrite(_rst,0);_ngfxDelay(_reset_duration);
      _ngfxPinMode(_rst,NGFX_GXEPD2_INPUT_PULLUP);_ngfxDelay(_reset_duration>10?_reset_duration:10);}
    else{_ngfxDigitalWrite(_rst,1);_ngfxPinMode(_rst,NGFX_GXEPD2_OUTPUT);_ngfxDigitalWrite(_rst,1);_ngfxDelay(10);
      _ngfxDigitalWrite(_rst,0);_ngfxDelay(_reset_duration);_ngfxDigitalWrite(_rst,1);_ngfxDelay(_reset_duration>10?_reset_duration:10);}
    _hibernating=false;
  }
  void _waitWhileBusy(const char * = nullptr,uint16_t fallback=5000){if(auto*h=nice_gfx::gxEpd2CurrentHal())h->waitBusy(_busy,_busy_level!=0,_busy_timeout,fallback);}
  void _waitWhileBusyCommand(uint8_t command,uint16_t interval=10,uint16_t fallback=5000){if(auto*h=nice_gfx::gxEpd2CurrentHal())h->waitBusyCommand(_busy,_busy_level!=0,_busy_timeout,fallback,command,_cs,_dc,interval);}
  void _writeCommand(uint8_t c){_pSPIx->beginTransaction(_spi_settings);_ngfxDigitalWrite(_dc,0);_select(true);
    _pSPIx->write(c);_select(false);_ngfxDigitalWrite(_dc,1);_pSPIx->endTransaction();}
  void _writeData(uint8_t value){_pSPIx->beginTransaction(_spi_settings);
    if(auto*h=nice_gfx::gxEpd2CurrentHal())h->spiByteTransaction(_cs,_ngfxCsMask,value);_pSPIx->endTransaction();}
  void _writeData(const uint8_t*p,uint16_t n){_startTransfer();for(uint16_t i=0;i<n;++i)_transfer(p[i]);_endTransfer();}
  void _writeDataPGM(const uint8_t*p,uint16_t n,int16_t fill=0){_startTransfer();for(uint16_t i=0;i<n;++i)_transfer(ngfx_gxepd2_read_byte(p+i));
    while(fill-->0)_transfer(0);_endTransfer();}
  void _writeDataPGM_sCS(const uint8_t*p,uint16_t n,int16_t fill=0){_pSPIx->beginTransaction(_spi_settings);
    for(uint16_t i=0;i<n;++i){_select(true);_transfer(ngfx_gxepd2_read_byte(p+i));_select(false);}
    while(fill-->0){_select(true);_transfer(0);_select(false);}_pSPIx->endTransaction();}
  void _writeCommandData(const uint8_t*p,uint8_t n){if(!n)return;_pSPIx->beginTransaction(_spi_settings);_ngfxDigitalWrite(_dc,0);_select(true);_transfer(*p++);
    _ngfxDigitalWrite(_dc,1);for(uint8_t i=1;i<n;++i)_transfer(*p++);_endTransfer();}
  void _writeCommandDataPGM(const uint8_t*p,uint8_t n){if(!n)return;_pSPIx->beginTransaction(_spi_settings);_ngfxDigitalWrite(_dc,0);_select(true);_transfer(ngfx_gxepd2_read_byte(p++));
    _ngfxDigitalWrite(_dc,1);for(uint8_t i=1;i<n;++i)_transfer(ngfx_gxepd2_read_byte(p++));_endTransfer();}
  void _startTransfer(){_pSPIx->beginTransaction(_spi_settings);_select(true);}
  void _transfer(uint8_t value){_pSPIx->write(value);}
  void _endTransfer(){_select(false);_pSPIx->endTransaction();}
  uint8_t _readData(){if(_sck<0||_mosi<0){unsupported();return 0;}
    _pSPIx->end();_ngfxDigitalWrite(_sck,1);_ngfxPinMode(_sck,NGFX_GXEPD2_OUTPUT);_ngfxDigitalWrite(_sck,1);
    _ngfxPinMode(_mosi,NGFX_GXEPD2_INPUT);_select(true);uint8_t value=0;
    for(unsigned bit=0;bit<8;++bit){_ngfxDigitalWrite(_sck,0);value=uint8_t((value<<1)|(_ngfxDigitalRead(_mosi)!=0));_ngfxDigitalWrite(_sck,1);}
    _select(false);_pSPIx->begin();return value;}
  void _readData(uint8_t*p,uint16_t n){
    if(_sck<0||_mosi<0){unsupported();return;}_pSPIx->end();_ngfxDigitalWrite(_sck,0);_ngfxPinMode(_sck,NGFX_GXEPD2_OUTPUT);_ngfxDigitalWrite(_sck,0);
    _ngfxPinMode(_mosi,NGFX_GXEPD2_INPUT);_select(true);
    for(uint16_t j=0;j<n;++j){uint8_t value=0;for(unsigned bit=0;bit<8;++bit){_ngfxDigitalWrite(_sck,1);value=uint8_t((value<<1)|(_ngfxDigitalRead(_mosi)!=0));_ngfxDigitalWrite(_sck,0);}p[j]=value;}
    _select(false);_pSPIx->begin();
  }
  int16_t _cs,_dc,_rst,_busy,_busy_level;
  int16_t _sck=-1,_mosi=-1;
  uint32_t _busy_timeout;
  bool _diag_enabled=false,_pulldown_rst_mode=false;
  NgfxGxEpd2Spi *_pSPIx;
  NgfxGxEpd2SpiSettings _spi_settings;
  bool _initial_write=true,_initial_refresh=true,_power_is_on=false,_using_partial_mode=false,_hibernating=false,_init_display_done=false;
  uint16_t _reset_duration=10;
  int _ngfxVcomMillivolts=-1;
  uint8_t _ngfxCsMask=1;
  static NgfxGxEpd2Spi _ngfxSpi;
  static NgfxGxEpd2Serial _ngfxSerial;
};
