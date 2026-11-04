#pragma once
#include "Compiled.h"

#define NICE_GFX_UI_CONTROLS_VERSION 1

namespace nice_gfx { namespace htmlui {
enum class ControlKind : uint8_t { Roller, DatePicker, TimePicker, Knob, RangeSlider, QrCode };

// Borrowed immutable resources, configured after mount. Dates use YYYYMMDD;
// times use seconds since midnight. No NodeDesc or resource ABI extension.
struct ControlDesc {
  Id id;
  ControlKind kind;
  const char *const *options;
  uint16_t optionCount;

  uint8_t rows, flags; // bit 0: wrap roller, bit 1: show seconds

  int32_t upper;
  int16_t start;
  uint16_t sweep;
  uint8_t thickness, ecc, maxVersion;
};

namespace controls {
inline int days(int year,int month) {
  if(month==2)return 28+(year%4==0&&(year%100!=0||year%400==0));

  return month==4||month==6||month==9||month==11?30:31;
}

inline bool date(int32_t value) {
  const int year=value/10000,month=value/100%100,day=value%100;

  return year>=1&&year<=9999&&month>=1&&month<=12&&day>=1&&day<=days(year,month);
}

inline int32_t clamp(int64_t value,int32_t low,int32_t high) {
  return int32_t(value<low?low:value>high?high:value);
}

inline int32_t snap(const NodeDesc &node,int64_t value) {
  value=clamp(value,node.minimum,node.maximum);

  const int32_t high=int32_t(int64_t(node.maximum)-(int64_t(node.maximum)-node.minimum)%node.step);

  return clamp(int64_t(node.minimum)+(value-node.minimum+node.step/2)/node.step*node.step,node.minimum,high);
}

inline int columns(const ControlDesc &desc) {
  return desc.kind==ControlKind::DatePicker?3:desc.kind==ControlKind::TimePicker?(desc.flags&2?3:2):desc.kind==ControlKind::RangeSlider?2:1;
}

inline int32_t shift(const ControlDesc &desc,const NodeDesc &node,int32_t value,int part,int delta) {
  if(desc.kind==ControlKind::Roller&&(desc.flags&1)) {
    const int64_t next=(int64_t(value)+delta)%desc.optionCount;

    return int32_t(next<0?next+desc.optionCount:next);
  }

  if(desc.kind==ControlKind::DatePicker) {
    int fields[]={value/10000,value/100%100,value%100};
    fields[part]+=delta;
    fields[0]=clamp(fields[0],1,9999);
    fields[1]=clamp(fields[1],1,12);
    fields[2]=clamp(fields[2],1,days(fields[0],fields[1]));

    return clamp(fields[0]*10000+fields[1]*100+fields[2],node.minimum,node.maximum);
  }

  if(desc.kind==ControlKind::TimePicker) {
    int fields[]={value/3600,value/60%60,value%60};
    fields[part]=clamp(fields[part]+delta,0,part==0?23:59);

    return fields[0]*3600+fields[1]*60+fields[2];
  }

  return snap(node,int64_t(value)+int64_t(delta)*node.step);
}
} // namespace controls
} }
