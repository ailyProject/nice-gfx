#pragma once
#include "../Drivers/Display/U8g2/u8x8.h"
#include "../Core/Status.h"
#include <stddef.h>
#include <stdint.h>
#include <limits.h>

namespace nice_gfx {

enum class U8g2Interface : uint8_t {
  Spi4, Spi3, I2c, Parallel6800, Parallel8080, Ks0108, Sed1520, Serial, Custom
};

// The upstream low-level buffer writer is part of the display contract. A
// horizontal tile row contains eight scanlines, each tile_width bytes long.
struct U8g2Profile {
  const char *name;
  ngfx_u8x8_msg_cb display, cad, byte;
  U8g2Interface interface;
  bool slowRefresh, refreshCommand;
  uint8_t nativeBitsPerPixel;
  bool fullFrameOnly, horizontalTiles, inputOneIsBlack;
  uint32_t requiredPins;
  constexpr U8g2Profile(const char *n, ngfx_u8x8_msg_cb d, ngfx_u8x8_msg_cb c,
      ngfx_u8x8_msg_cb b, U8g2Interface i, bool slow=false, bool refresh=false,
      uint8_t nativeDepth=1, bool full=false, bool horizontal=false, uint32_t pins=0,
      bool blackInput=false)
      : name(n), display(d), cad(c), byte(b), interface(i), slowRefresh(slow),
        refreshCommand(refresh), nativeBitsPerPixel(nativeDepth), fullFrameOnly(full),
        horizontalTiles(horizontal), inputOneIsBlack(blackInput), requiredPins(pins) {}
};

struct U8g2Pins {
  int clock=-1, mosi=-1, scl=-1, sda=-1, cs=-1, dc=-1, reset=-1;
  int enable=-1, cs1=-1, cs2=-1, tx=-1;
  int data[8]={-1,-1,-1,-1,-1,-1,-1,-1};
};
class U8g2Io;
struct U8g2Config {
  U8g2Pins pins;
  uint8_t i2cAddress=0; // Seven-bit; zero retains the upstream module default.
  uint32_t busClockHz=0;
  // One tile row at a time. SSD1606's upstream multi-clear INIT peaks at
  // 18,331 encoded bytes; allocation grows on demand and is freed after INIT.
  size_t maxTapeBytes=32768UL;
  uint32_t pollBudgetMicros=2000;
  uint16_t maxDataBytesPerPoll=512;
  U8g2Io *io=nullptr;
};

// Platform I/O is independent of display/CAD drivers. BYTE Unsupported selects
// the upstream software protocol; any other error is sticky until recover().
// A hardware override must preserve START/END, D/C and the current I2C address.
class U8g2Io {
 public:
  virtual ~U8g2Io() {}
  virtual Status begin(const U8g2Profile &, const U8g2Config &,
                       const ngfx_u8x8_display_info_t &) { return Status::Ok; }
  virtual void end() {}
  virtual bool validPin(int pin) const { return pin>=0; }
  virtual int physicalPin(int pin) const { return pin; }
  virtual Status configure(int, bool) { return Status::Unsupported; }
  // openDrain=true means high must release the line, never drive it high.
  virtual Status write(int, bool, bool=false) { return Status::Unsupported; }
  virtual Status read(int, bool &) { return Status::Unsupported; }
  virtual void delayNs(uint32_t) {}
  virtual uint32_t nowMicros() const { return 0; }
  // Limit one unsplittable software BYTE callback; poll also checks wall time
  // and its aggregate byte budget between callbacks.
  virtual size_t maxBurstBytes() const { return 16; }
  // SEND must consume/copy its source before returning (no borrowed DMA view).
  virtual Status byte(ngfx_u8x8_t *, uint8_t, uint8_t, void *) { return Status::Unsupported; }
};

} // namespace nice_gfx
