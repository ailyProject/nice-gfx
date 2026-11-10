#pragma once
#include "../../../Core/Status.h"
#include <stddef.h>
#include <stdint.h>

namespace nice_gfx {

// Protocol families are not interchangeable panel profiles. Some
// identifiers (ED2208, T133A01, GDEB0709E01, StickyAuto) name products rather
// than verified control ICs; those entries remain disabled without a protocol.
// A profile must name its glass and documented initialization/waveform source.
enum class NativeEpaperController : uint8_t {
  SSD1680, SSD1681, SSD1683, SSD1677, UC8151D, UC8179, JD79686B,
  IT8951, ED2208, T133A01, GDEB0709E01, JD79660, JD79676, JD79667,
  StickyAuto, UC8176, UC8159
};
enum class NativeEpaperEncoding : uint8_t { Mono, FourColor, SixColor, Gray16 };

struct NativeEpaperPins {
  int clock=-1, mosi=-1, miso=-1, cs=-1, dc=-1, reset=-1, busy=-1;
  int extraCs[3]={-1,-1,-1};
};
class NativeEpaperIo;
struct NativeEpaperProtocol;
struct NativeEpaperConfig {
  NativeEpaperPins pins;
  NativeEpaperIo *io=nullptr;
  uint32_t busClockHz=4000000;
  uint16_t resetDurationMs=10;
  uint16_t powerOnDelayMs=100;
  int vcomMillivolts=-1;
  uint32_t pollBudgetMicros=2000;
  uint16_t maxDataBytesPerPoll=1024;
  bool preferPsram=true;
};

struct NativeEpaperProfile {
  const char *name;
  NativeEpaperController controller;
  uint16_t width, height, storageWidth;
  NativeEpaperEncoding encoding;
  uint32_t minRefreshMs=5000, busyTimeoutMs=120000;
  bool busyHigh=true;
  uint8_t chipCount=1;
  // False means essential public panel documentation is still missing.
  // begin() must refuse it rather than applying a look-alike panel sequence.
  bool documented=false;
  const char *source=nullptr;
  const NativeEpaperProtocol *protocol=nullptr;
  uint32_t maxClockHz=10000000;
  uint8_t bitsPerPixel() const {
    return encoding==NativeEpaperEncoding::Mono?1:
      encoding==NativeEpaperEncoding::FourColor?2:4;
  }
  size_t rowBytes() const {const uint8_t per=8/bitsPerPixel();return storageWidth/per+(storageWidth%per!=0);}
  size_t framebufferBytes() const {return rowBytes()*height;}
};

// Small immutable controller operations. Each initialization step is followed
// by a cooperative BUSY wait when its bit is set; no command tape is allocated.
// Multi-chip profiles run initialization and uploads independently per chip.
struct NativeEpaperProtocol {
  uint8_t initSteps=0, planes=1;
  uint32_t waitAfterInit=0;
  Status (*initialize)(NativeEpaperIo&,const NativeEpaperProfile&,uint8_t,uint8_t)=nullptr;
  Status (*startPlane)(NativeEpaperIo&,const NativeEpaperProfile&,uint8_t,uint8_t)=nullptr;
  Status (*refresh)(NativeEpaperIo&,const NativeEpaperProfile&,uint8_t)=nullptr;
  Status (*powerOff)(NativeEpaperIo&,const NativeEpaperProfile&,uint8_t)=nullptr;
  Status (*sleep)(NativeEpaperIo&,const NativeEpaperProfile&,uint8_t)=nullptr;
  uint8_t zeroPlanes=0;
  void (*encode)(const uint8_t*,uint8_t*,size_t)=nullptr;
  uint8_t encodedBytesPerByte=1;
};

// SPI reads/writes consume caller buffers synchronously. Scheduling and BUSY
// waits belong to the backend; implementations must never poll BUSY here.
class NativeEpaperIo {
 public:
  virtual ~NativeEpaperIo() {}
  virtual Status begin(const NativeEpaperProfile&,const NativeEpaperConfig&)=0;
  virtual void end()=0;
  virtual Status reset(bool high)=0;
  virtual Status busy(bool &level)=0;
  virtual Status command(uint8_t,const uint8_t*,size_t,uint8_t chip=0)=0;
  virtual Status data(const uint8_t*,size_t,uint8_t chip=0)=0;
  // IT8951 uses framed 16-bit SPI and a read dummy word. No 8-bit D/C bus.
  // Busy means retry the same operation/arguments; no caller pointer is
  // retained. A partial packet may keep CS asserted across cooperative polls.
  virtual Status writeWords(uint16_t,const uint16_t*,size_t){return Status::Unsupported;}
  virtual Status readWords(uint16_t*,size_t){return Status::Unsupported;}
  virtual uint32_t nowMicros() const {return 0;}
};

} // namespace nice_gfx
