#pragma once
#include "../../../Display/ControllerProfile.h"
namespace nice_gfx { namespace controllers {
inline void dcsRotate(PanelSequence &s,uint8_t rotation) {
  s.rotation=rotation & 3;
  static const uint8_t values[]={0x00,0x60,0xC0,0xA0};
  s.writecommand(0x36); s.writedata(values[rotation & 3] | s.colorOrder);
  s._width=(rotation & 1) ? s._init_height : s._init_width;
  s._height=(rotation & 1) ? s._init_width : s._init_height;
  s.colstart=s.offsetX; s.rowstart=s.offsetY;
}
inline void fixedRotate(PanelSequence &s,uint8_t) {
  s.rotation=0;
  s._width=s._init_width; s._height=s._init_height;
  s.colstart=s.offsetX; s.rowstart=s.offsetY;
  s.writecommand(0x36); s.writedata(s.colorOrder);
}
inline void ptRotate(PanelSequence &s,uint8_t) {
  s.rotation=0; s._width=s._init_width; s._height=s._init_height;
  s.colstart=s.offsetX; s.rowstart=s.offsetY; s.writecommand(0x6A);
  s.writecommand(s.colorOrder ? 0xB3 : 0xB2);
}


} }
