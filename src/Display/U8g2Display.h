#pragma once
#include "Display.h"
#include "U8g2Types.h"
#include "Backends/U8g2Panel.h"
#include "Backends/U8g2PlatformIo.h"

namespace nice_gfx { namespace detail {
template<class Tag>struct U8g2DisplayStorage {
  U8g2PlatformIo gpio;U8g2Panel panel;
  explicit U8g2DisplayStorage(const U8g2Config &c):panel(Tag::profile(),c,c.io?*c.io:gpio){}
};
} // namespace detail

// All catalog aliases use the same config constructor. The buffer suffixes
// 1/2/F collapse to one private full-frame renderer with incremental polling.
// HW/SW/2ND names retain upstream provenance; default I/O is portable software.
// Select an explicit U8g2SpiIo/U8g2I2cIo in config.io for hardware acceleration.
template<class Tag>class U8g2Display:private detail::U8g2DisplayStorage<Tag>,public NICE_GFX {
  using Storage=detail::U8g2DisplayStorage<Tag>;
 public:
  explicit U8g2Display(const U8g2Config &config=U8g2Config()):Storage(config),NICE_GFX(Storage::panel){}
  bool begin(){return beginChecked()==Status::Ok;}
  U8g2Panel &u8g2Panel(){return Storage::panel;}
  const U8g2Panel &u8g2Panel()const{return Storage::panel;}
};
} // namespace nice_gfx
