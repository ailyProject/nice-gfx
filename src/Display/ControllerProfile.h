#pragma once
#ifndef GFX_RUNTIME_CONTROLLER_H
#define GFX_RUNTIME_CONTROLLER_H
#include "Transport.h"

namespace nice_gfx {
enum class Protocol : uint8_t {
  Dcs, Dcs16, Ili9225, Ili9331, Jbt6k71, Ssd1283, Hx8347, Hx8357a,
  Seps525, Ssd1331, Ssd1351, Ssd1963
};
enum class ColorOrder : uint8_t { Default, RGB, BGR };
enum class PixelReadFormat : uint8_t { RGB888, RGB565, BGR565, RGB777 };
enum class St7735Tab : uint8_t {
  Green=0, Red=1, Black=2, Green2=3, Green3=4, Green128=5,
  Green160x80=6, Red160x80=7, Robot=8, B=11
};
struct PanelOptions {
  uint16_t width = 0, height = 0; // Zero selects controller defaults.
  ColorOrder colorOrder = ColorOrder::Default;
  St7735Tab tab = St7735Tab::Black;
  int16_t xOffset = 0, yOffset = 0;
  bool overrideOffsets = false;
  int16_t xOffsets[8] = {}, yOffsets[8] = {};
  // Wire encoding belongs to the selected initialization profile, so bus and
  // controller pixel formats cannot be changed independently.
  int8_t inversion = -1; // -1 retains the controller sequence; 0 normal, 1 inverted.
  int writeStrobePin = -1; // Optional WR latch on SPI-to-parallel bridge modules.
  int8_t spiMode = -1; // -1 selects the controller's mode; otherwise 0..3.
  uint32_t spiReadFrequency = 10000000;
  uint32_t parallelFrequency = 20000000; // Maximum write-cycle rate.
  uint16_t parallelWriteDelayUs = 0, parallelReadDelayUs = 1;
};

// Controller initialization programs use this bounded command accumulator.
// No bus call is made per parameter byte; a whole register payload is emitted
// together. Errors are sticky for the entire program.
class PanelSequence {
 public:
  PanelSequence(Transport &bus, uint16_t w, uint16_t h, uint8_t order, uint8_t tab,
                int16_t x = 0, int16_t y = 0, bool registerWords = false)
      : _init_width(w), _init_height(h), _width(w), _height(h),
        tabcolor(tab), colorOrder(order), offsetX(x), offsetY(y), bus_(bus), registerWords_(registerWords) {}
  uint16_t _init_width, _init_height, _width, _height;
  int16_t colstart = 0, rowstart = 0;
  uint8_t rotation = 0, tabcolor, colorOrder;
  int16_t offsetX, offsetY;
  Status result = Status::Ok;
  void writecommand(uint16_t value) { finish(); command_=value; bits_=value>255?16:8; pending_=true; dataWord_=false; }
  void writedata(uint32_t value) {
    if(result!=Status::Ok) return;
    if(!pending_) { result=Status::InvalidArgument; return; }
    if (used_ == sizeof(buffer_)) { result = Status::InvalidArgument; return; }
    buffer_[used_++] = uint8_t(value);
  }
  void commandMode() { commandMode_ = true; }
  void dataMode() { commandMode_ = false; }
  void raw8(uint8_t value) { if (commandMode_) writecommand(value); else writedata(value); }
  void raw16(uint16_t value) {
    if (commandMode_) { writecommand(value); bits_=16; }
    else { writedata(uint8_t(value>>8)); writedata(uint8_t(value)); dataWord_=true; }
  }
  void writeRegister8(uint16_t command, uint8_t value) {
    writecommand(command); bits_=16; writedata(value);
  }
  void writeRegister16(uint16_t command, uint16_t value) {
    writecommand(command); bits_=16; writedata(uint8_t(value>>8)); writedata(uint8_t(value)); dataWord_=true;
  }
  void qspiWriteCommand(uint8_t command, const uint8_t *data, size_t n) {
    writecommand(command); for (size_t i=0;i<n;++i) writedata(data[i]); finish();
  }
  void commandList(const uint8_t *p) {
    uint8_t commands=*p++;
    while (commands--) {
      writecommand(*p++); uint8_t count=*p++; bool wait=(count&128)!=0; count&=127;
      while(count--) writedata(*p++);
      if(wait) { uint16_t ms=*p++; delay(ms==255?500:ms); }
    }
  }
  void delay(uint32_t ms) { finish(); if(result==Status::Ok) bus_.delayMs(ms); }
  void beginPanelWrite() {}
  void endPanelWrite() { finish(); }
  Status finish() {
    if (pending_ && result==Status::Ok) {
      if((dataWord_||registerWords_)&&used_==2) result=bus_.register16(command_,uint16_t(buffer_[0]<<8|buffer_[1]),bits_);
      else result = bits_==16 ? bus_.command16(command_,buffer_,used_) : bus_.command(uint8_t(command_),buffer_,used_);
    }
    used_=0; pending_=false; return result;
  }
 private:
  Transport &bus_;
  uint16_t command_=0;
  uint8_t bits_=8;
  bool pending_=false, commandMode_=true, dataWord_=false, registerWords_=false;
  size_t used_=0;
  uint8_t buffer_[256];
};

struct ControllerProfile {
  const char *name;
  uint16_t width,height;
  uint32_t id;
  Protocol protocol;
  PixelFormat pixelFormat;
  uint16_t ramWrite,ramRead,softReset;
  uint8_t rotations,defaultColorOrder,rgbOrder,bgrOrder;
  void (*initialize)(PanelSequence &);
  void (*rotate)(PanelSequence &,uint8_t);
  PixelReadFormat readFormat;
  uint8_t readDummy;
  uint16_t indexCommand;
};
} // namespace nice_gfx
#endif
