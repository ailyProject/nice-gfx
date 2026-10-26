#pragma once
// Additional exact panel protocols for the NICE_GFX asynchronous driver HAL.
// Adafruit portions: Copyright Adafruit Industries, MIT (LICENSE-Adafruit.txt).
// BitBank portions: Copyright 2024 BitBank Software, Inc., Larry Bank,
// GPL-3.0-or-later (LICENSE-BitBank.txt).
// Legacy GxEPD portions: Copyright Jean-Marc Zingg, GPLv3 (LICENSE-GxEPD.txt).
// Fixed sources and changes are recorded in upstream-sources.json.
#include "../GxEpd2/NgfxGxEpd2_EPD.h"
#include "ExtraSequences.h"

class NgfxEpaperExtraPanel : public NgfxGxEpd2_EPD {
 public:
  enum Protocol { Ssd1619, Ek79686, Ssd1675, Ssd2683, Spectra400, Spectra1024, Spectra1200, Legacy154Z17, Legacy213Z19 };
  NgfxEpaperExtraPanel(Protocol protocol, uint16_t width, uint16_t height, uint16_t visible,
      int16_t cs, int16_t dc, int16_t rst, int16_t busy, bool color, uint32_t refreshMs)
    : NgfxGxEpd2_EPD(cs,dc,rst,busy,protocol==Ssd1619||protocol==Ssd1675?1:0,
        120000000UL,width,height,static_cast<NgfxGxEpd2::Panel>(0),color,false,false),
      _protocol(protocol),_visible(visible),_refreshMs(refreshMs) {}
  void init(uint32_t bitrate=0) override { init(bitrate,true,10,false); }
  void init(uint32_t bitrate, bool initial, uint16_t reset=10, bool pull=false) override {
    _ready=false; _frameWritten=false;
    NgfxGxEpd2_EPD::init(bitrate,initial,reset,pull);
  }
  void clearScreen(uint8_t value=0xff) override { writeScreenBuffer(value); refresh(false); }
  void writeScreenBuffer(uint8_t value=0xff) override {
    _fill=value;
    writeFrame(nullptr,nullptr,WIDTH,HEIGHT,false,false,false,false);
    _fill=0xff;
  }
  void writeImage(const uint8_t* black,int16_t x,int16_t y,int16_t w,int16_t h,
      bool invert=false,bool mirror=false,bool pgm=false) override {
    if (!fullFrame(x,y,w,h) || !black) { unsupported(); return; }
    writeFrame(black,nullptr,w,h,invert,mirror,pgm,false);
  }
  void writeImage(const uint8_t* black,const uint8_t* color,int16_t x,int16_t y,int16_t w,int16_t h,
      bool invert=false,bool mirror=false,bool pgm=false) {
    if (!fullFrame(x,y,w,h) || (!black&&!color)) { unsupported(); return; }
    writeFrame(black,color,w,h,invert,mirror,pgm,false);
  }
  // SixColor native input is packed high-nibble-first palette indices 0..5.
  // Wire codes are 0,1,2,3,5,6, as in BitBank's u8Colors_spectra table.
  void writeNative(const uint8_t* data,const uint8_t* color,int16_t x,int16_t y,int16_t w,int16_t h,
      bool invert=false,bool mirror=false,bool pgm=false) {
    if (!fullFrame(x,y,w,h) || !data) { unsupported(); return; }
    writeFrame(data,color,w,h,invert,mirror,pgm,spectra());
  }
  void refresh(bool partial=false) override {
    if (partial || !_frameWritten) { unsupported(); return; }
    if (spectra()) {
      // BB repeats the non-extra-init profiles at refresh; keep that ordering.
      if (_protocol==Spectra400 || _protocol==Spectra1024) initializeSequence();
      _ngfxCsSelect(dual()?3:1);
      command(0x12,_protocol==Spectra1200?0x01:0x00);
    } else if (_protocol==Ssd1619 || _protocol==Ssd1675) {
      command(0x22,0xc7); _writeCommand(0x20);
    } else if (_protocol==Ssd2683) {
      command(0x12,0x00);
    } else {
      _writeCommand(0x12);
      if (_protocol==Ek79686) _ngfxDelay(10);
    }
    wait("EpaperExtra refresh",_refreshMs);
    _ngfxCsSelect(1);
    _frameWritten=false;
  }
  void refresh(int16_t,int16_t,int16_t,int16_t) override { unsupported(); }
  void powerOff() override {
    if (!_ready) return;
    if (spectra()) {
      _ngfxCsSelect(dual()?3:1); command(0x02,0x00);
      wait("Spectra power off",500);
    } else if (_protocol==Ssd1619 || _protocol==Ssd1675) {
      if (_rst>=0) { command(0x10,0x01); _ngfxDelay(100); }
      else { _writeCommand(0x12); wait("SSD reset",500); }
    } else if (_protocol==Ssd2683) {
      command(0x50,0x17); _writeCommand(0x02); wait("SSD2683 power off",500);
    } else {
      _writeCommand(0x02); wait("EPD power off",500);
      if (_protocol==Ek79686) command(0x07,0xa5);
    }
    _ngfxCsSelect(1); _ready=false; _frameWritten=false;
  }
  void hibernate() override {
    powerOff();
    if (_rst>=0 && _protocol!=Ssd1619 && _protocol!=Ssd1675 && _protocol!=Ek79686 && !spectra()) command(0x07,0xa5);
  }
 protected:
  Protocol _protocol;
 private:
  uint16_t _visible;
  uint32_t _refreshMs;
  bool _ready=false,_frameWritten=false;
  uint8_t _fill=0xff;
  bool spectra() const { return _protocol>=Spectra400 && _protocol<=Spectra1200; }
  bool dual() const { return _protocol==Spectra1024 || _protocol==Spectra1200; }
  bool fullFrame(int16_t x,int16_t y,int16_t w,int16_t h) const {
    return x==0 && y==0 && (w==int16_t(WIDTH)||w==int16_t(_visible)) && h==int16_t(HEIGHT);
  }
  static uint8_t read(const uint8_t* p,bool pgm) { return pgm?ngfx_gxepd2_read_byte(p):*p; }
  void command(uint8_t cmd,uint8_t value) { _writeCommand(cmd); _writeData(value); }
  void command(uint8_t cmd,const uint8_t* data,uint16_t n) {
    _writeCommand(cmd);
    if (_protocol==Ssd1619) { for (uint16_t i=0;i<n;++i) _writeData(data[i]); }
    else if (n) _writeData(data,n);
  }
  void wait(const char* reason,uint32_t ms) {
    if (_protocol==Ek79686) _waitWhileBusyCommand(0x71,10,uint16_t(ms>65535?65535:ms));
    else _waitWhileBusy(reason,uint16_t(ms>65535?65535:ms));
    if (_protocol==Ek79686) _ngfxDelay(200);
  }
  void resetExact(uint16_t duration) {
    if (_rst>=0) { _ngfxDigitalWrite(_rst,0); _ngfxDelay(duration); _ngfxDigitalWrite(_rst,1); _ngfxDelay(duration); }
  }
  void adafruitSequence(const uint8_t* p) {
    while (*p!=0xfe) {
      const uint8_t cmd=*p++,n=*p++;
      if (cmd==0xff) { wait("Adafruit init",500); _ngfxDelay(n); }
      else { command(cmd,p,n); p+=n; }
    }
  }
  void bitbankSequence(const uint8_t* p) {
    while (*p) {
      const uint8_t n=*p++;
      if (n==0xff) { _ngfxDelay(11); wait("BitBank init",spectra()?35000:5000); }
      else if (n==0xfe) { resetExact(20); wait("BitBank reset",5000); }
      else if (n==0xf7) _ngfxDelay(*p++);
      else if (n==0xfb) _ngfxCsSelect(1);
      else if (n==0xfa) _ngfxCsSelect(2);
      else if (n==0xf9) _ngfxCsSelect(3);
      else { command(p[0],p+1,n-1); p+=n; }
    }
  }
  void ssdAddress() {
    command(0x4e,0x00);
    const uint8_t y[]={uint8_t(_protocol==Ssd1675?0xf9:0),0}; command(0x4f,y,2);
  }
  void initializeSequence() {
    using namespace ngfx_epaper_extra;
    if (_protocol==Spectra400) { _ngfxCsSelect(1); bitbankSequence(spectra400); }
    else if (_protocol==Spectra1024) { _ngfxCsSelect(3); bitbankSequence(spectra1024); }
    else if (_protocol==Spectra1200) bitbankSequence(spectra1200);
  }
  void ensureReady() {
    if (_ready) return;
    using namespace ngfx_epaper_extra;
    if (_protocol==Ssd1619) {
      _reset(); _ngfxDelay(100); wait("SSD1619 wake",500); adafruitSequence(ssd1619);
      const uint8_t gates[]={uint8_t(HEIGHT-1),uint8_t((HEIGHT-1)>>8),0}; command(0x01,gates,3);
      const uint8_t xs[]={0,uint8_t((WIDTH-1)/8)}; command(0x44,xs,2);
      const uint8_t ys[]={0,0,uint8_t(HEIGHT-1),uint8_t((HEIGHT-1)>>8)}; command(0x45,ys,4);
    } else if (_protocol==Ek79686) {
      _reset(); _ngfxDelay(10); adafruitSequence(ek79686); wait("EK79686 wake",500);
    } else if (_protocol==Ssd1675) {
      _reset(); _ngfxDelay(100); wait("SSD1675 wake",500);
      _writeCommand(0x12); wait("SSD1675 soft reset",500);
      command(0x74,0x54); command(0x7e,0x3b);
      // These exact original bytes are retained; no substitution with SSD1675B.
      const uint8_t gates[]={0xfa,0x01,0x00}; command(0x01,gates,3);
      command(0x11,0x03); const uint8_t xs[]={0,0x0f}; command(0x44,xs,2);
      const uint8_t ys[]={0,0,0xf9,0}; command(0x45,ys,4);
      command(0x3c,0x03); command(0x2c,0x70); command(0x03,ssd1675lut[70]);
      command(0x04,ssd1675lut+71,3); command(0x3a,ssd1675lut[74]); command(0x3b,ssd1675lut[75]);
      command(0x32,ssd1675lut,70); ssdAddress(); wait("SSD1675 init",500);
    } else if (_protocol==Ssd2683) {
      bitbankSequence(ssd2683);
    } else if (spectra()) {
      initializeSequence();
    } else {
      resetExact(10);
      if (_protocol==Legacy154Z17) {
        const uint8_t boost[]={0x17,0x17,0x17}; command(0x06,boost,3);
        _writeCommand(0x04); wait("GDEW0154Z17 power on",500);
        const uint8_t panel[]={0x0f,0x0d}; command(0x00,panel,2);
        const uint8_t size[]={0x98,0,0x98}; command(0x61,size,3); command(0x50,0x77);
      } else {
        command(0x00,0x8f); const uint8_t size[]={0x68,0,0xd4}; command(0x61,size,3);
        command(0x50,0x77); _writeCommand(0x04); wait("GDEH0213Z19 power on",500);
      }
    }
    _ready=true;
  }
  uint8_t monoByte(const uint8_t* data,uint32_t offset,bool inv,bool pgm) const {
    uint8_t b=data?read(data+offset,pgm):_fill; return inv?uint8_t(~b):b;
  }
  void plane(uint8_t cmd,const uint8_t* data,uint16_t w,bool inv,bool mirror,bool pgm,bool color) {
    const uint16_t stride=uint16_t((w+7)/8),wire=uint16_t((WIDTH+7)/8);
    _writeCommand(cmd); if (_protocol!=Ssd1619) _startTransfer();
    for (uint16_t y=0;y<HEIGHT;++y) for (uint16_t x=0;x<wire;++x) {
      const uint32_t row=mirror?HEIGHT-1-y:y;
      uint8_t b=color&&!data?0xff:monoByte(data,row*stride+x,inv,pgm);
      if (x>=stride) b=0xff;
      if (x==wire-1 && (_visible&7)) b|=uint8_t(0xff>>(_visible&7));
      if (color && (_protocol==Ssd1619||_protocol==Ek79686)) b=uint8_t(~b);
      // Adafruit marks SSD1619 singleByteTxns=true: retain every CS edge.
      if (_protocol==Ssd1619) _writeData(b); else _transfer(b);
    }
    if (_protocol!=Ssd1619) _endTransfer();
  }
  void packed(const uint8_t* data,uint16_t w,bool inv,bool mirror,bool pgm,bool native) {
    static const uint8_t map[]={0,1,2,3,5,6};
    const uint16_t stride=uint16_t((w+(native?1:7))/(native?2:8));
    const uint16_t step=_protocol==Ssd2683?4:2;
    for (uint8_t pass=0;pass<(dual()?2:1);++pass) {
      _ngfxCsSelect(uint8_t(1u<<pass));
      _writeCommand(0x10);
      if (_protocol==Ssd2683) wait("SSD2683 RAM",5000);
      _startTransfer();
      const uint16_t start=dual()?uint16_t(pass*(WIDTH/2)):0;
      const uint16_t end=dual()?uint16_t(start+WIDTH/2):WIDTH;
      for (uint16_t y=0;y<HEIGHT;++y) for (uint16_t x=start;x<end;x+=step) {
        const uint32_t row=mirror?HEIGHT-1-y:y;
        uint8_t out=0;
        for (uint16_t bit=0;bit<step;++bit) {
          const uint16_t px=uint16_t(x+bit); uint8_t code;
          if (native) {
            const uint8_t b=read(data+row*stride+px/2,pgm);
            const uint8_t index=uint8_t((b>>((px&1)?0:4))&15);
            code=map[index<6?index:0];
          } else {
            const uint8_t b=monoByte(data,row*stride+px/8,inv,pgm);
            const bool white=(b&(0x80>>(px&7)))!=0;
            code=_protocol==Ssd2683?uint8_t(white?1:2):uint8_t(white?1:0);
          }
          out=uint8_t((out<<(step==4?2:4))|code);
        }
        _transfer(out);
      }
      _endTransfer();
    }
    _ngfxCsSelect(1);
    if (_protocol==Ssd2683) { _writeCommand(0x04); wait("SSD2683 power on",5000); }
  }
  void writeFrame(const uint8_t* black,const uint8_t* color,uint16_t w,uint16_t,
      bool inv,bool mirror,bool pgm,bool native) {
    ensureReady();
    if (spectra()||_protocol==Ssd2683) packed(black,w,inv,mirror,pgm,native);
    else {
      if (_protocol==Ssd1619||_protocol==Ssd1675) ssdAddress();
      plane(_protocol==Ssd1619||_protocol==Ssd1675?0x24:0x10,black,w,inv,mirror,pgm,false);
      if (_protocol!=Ssd1675) {
        if (_protocol==Ssd1619) { _ngfxDelay(2); ssdAddress(); }
        plane(_protocol==Ssd1619?0x26:0x13,color,w,false,mirror,pgm,true);
      }
    }
    _frameWritten=true;
  }
};

#define NGFX_EXTRA_PANEL_CLASS(Name, ProtocolName, W, H, V, Color, Refresh) \
class Name : public NgfxEpaperExtraPanel { \
 public: static const uint16_t WIDTH=W, WIDTH_VISIBLE=V, HEIGHT=H; \
 static const uint32_t full_refresh_time=Refresh; \
 static const bool hasPartialUpdate=false,hasFastPartialUpdate=false,hasColor=Color; \
 Name(int16_t cs,int16_t dc,int16_t rst,int16_t busy) \
   :NgfxEpaperExtraPanel(ProtocolName,W,H,V,cs,dc,rst,busy,Color,Refresh){} \
};
NGFX_EXTRA_PANEL_CLASS(NgfxEpaperSSD1619,Ssd1619,400,300,400,true,13000)
NGFX_EXTRA_PANEL_CLASS(NgfxEpaperEK79686,Ek79686,176,264,176,true,13000)
NGFX_EXTRA_PANEL_CLASS(NgfxEpaperSSD1675,Ssd1675,128,250,122,false,1000)
NGFX_EXTRA_PANEL_CLASS(NgfxEpaperSSD2683,Ssd2683,400,300,400,false,5000)
NGFX_EXTRA_PANEL_CLASS(NgfxEpaperSpectra400,Spectra400,400,600,400,true,35000)
NGFX_EXTRA_PANEL_CLASS(NgfxEpaperSpectra1024,Spectra1024,1024,576,1024,true,35000)
NGFX_EXTRA_PANEL_CLASS(NgfxEpaperSpectra1200,Spectra1200,1200,1600,1200,true,35000)
NGFX_EXTRA_PANEL_CLASS(NgfxEpaperGDEW0154Z17,Legacy154Z17,152,152,152,true,17000)
NGFX_EXTRA_PANEL_CLASS(NgfxEpaperGDEH0213Z19,Legacy213Z19,104,212,104,true,17000)
#undef NGFX_EXTRA_PANEL_CLASS
