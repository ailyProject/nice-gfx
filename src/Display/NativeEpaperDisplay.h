#pragma once
#include "Display.h"
#include "Backends/NativeEpaperPanel.h"
#include "Backends/NativeEpaperPlatformIo.h"

namespace nice_gfx {
namespace detail {
template<class Profile> struct NativeEpaperStorage {
  NativeEpaperPlatformIo io;
  NativeEpaperPanel panel;
  explicit NativeEpaperStorage(const NativeEpaperConfig &config)
    :panel(Profile::profile(),config,config.io?*config.io:io){}
};
}
template<class Profile>
class NativeEpaperDisplay : private detail::NativeEpaperStorage<Profile>,public NICE_GFX {
  using Storage=detail::NativeEpaperStorage<Profile>;
 public:
  explicit NativeEpaperDisplay(const NativeEpaperConfig &config=NativeEpaperConfig())
    :Storage(config),NICE_GFX(Storage::panel){}
  NativeEpaperDisplay(int cs,int dc,int reset,int busy,int clock,int mosi,uint32_t hz=4000000)
    :NativeEpaperDisplay(spiConfig(cs,dc,reset,busy,clock,mosi,hz)){}
  bool begin(){return beginChecked()==Status::Ok;}
  NativeEpaperPanel &nativeEpaperPanel(){return Storage::panel;}
  const NativeEpaperPanel &nativeEpaperPanel() const {return Storage::panel;}
 private:
  static NativeEpaperConfig spiConfig(int cs,int dc,int reset,int busy,int clock,int mosi,uint32_t hz){
    NativeEpaperConfig c;c.pins.cs=cs;c.pins.dc=dc;c.pins.reset=reset;c.pins.busy=busy;
    c.pins.clock=clock;c.pins.mosi=mosi;c.busClockHz=hz;return c;
  }
};
} // namespace nice_gfx
