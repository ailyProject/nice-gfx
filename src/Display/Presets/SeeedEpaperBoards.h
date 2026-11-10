#pragma once
#include "../Backends/NativeEpaperPlatformIo.h"
namespace nice_gfx {
// ESP32-S3 board GPIOs from the board schematics. XIAO Breakout /
// Driver and nRF EN boards use their core's D-pin aliases; wire those explicitly.
enum class SeeedEpaperBoard:uint8_t{EE02,EE03,EE04,EE05,E1001,E1002,E1003,E1004,Sticky};
inline NativeEpaperConfig seeedEpaperConfig(SeeedEpaperBoard board){
  NativeEpaperConfig c;c.busClockHz=4000000;
  c.pins.clock=7;c.pins.mosi=9;c.pins.cs=44;c.pins.dc=10;c.pins.reset=38;c.pins.busy=4;
  switch(board){
    case SeeedEpaperBoard::EE02:c.pins.extraCs[0]=41;break;
    case SeeedEpaperBoard::EE03:c.pins.dc=-1;c.pins.miso=8;break;
    case SeeedEpaperBoard::E1001:case SeeedEpaperBoard::E1002:
    case SeeedEpaperBoard::E1003:case SeeedEpaperBoard::E1004:
      c.pins.cs=10;c.pins.dc=11;c.pins.reset=12;c.pins.busy=13;c.pins.miso=8;
      if(board==SeeedEpaperBoard::E1003){c.pins.dc=-1;c.pins.reset=-1;}
      if(board==SeeedEpaperBoard::E1004){c.pins.reset=38;c.pins.extraCs[0]=2;}break;
    case SeeedEpaperBoard::Sticky:
      c.pins.clock=13;c.pins.mosi=14;c.pins.miso=12;c.pins.cs=15;c.pins.dc=16;c.pins.reset=17;c.pins.busy=18;break;
    default:break;
  }
  return c;
}
// Pass this object's address as config.io. It must outlive the display.
// Power nets are enabled only at begin(), never during global construction.
class SeeedEpaperBoardIo:public NativeEpaperPlatformIo{
 public:
  explicit SeeedEpaperBoardIo(SeeedEpaperBoard board):board_(board){}
  Status begin(const NativeEpaperProfile&p,const NativeEpaperConfig&c)override{
    Status s=NativeEpaperPlatformIo::begin(p,c);if(s!=Status::Ok)return s;
    auto output=[&](int pin,bool high){if(s==Status::Ok)s=setOutput(pin,high);};
    switch(board_){
      case SeeedEpaperBoard::EE02:case SeeedEpaperBoard::EE03:case SeeedEpaperBoard::EE04:case SeeedEpaperBoard::EE05:output(43,true);break;
      case SeeedEpaperBoard::E1001:case SeeedEpaperBoard::E1002:output(14,true);output(16,true);break;
      case SeeedEpaperBoard::E1003:output(14,true);output(39,true);output(21,true);output(11,true);break;
      case SeeedEpaperBoard::E1004:output(14,true);output(16,true);output(12,true);break;
      case SeeedEpaperBoard::Sticky:output(8,true);output(10,true);output(47,true);break;
    }
    // The backend supplies the nonblocking power-settle interval.
    if(s!=Status::Ok)end();return s;
  }
 private:SeeedEpaperBoard board_;
};
}
