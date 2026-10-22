#pragma once
#include "../VideoDisplay.h"
#if defined(CONFIG_IDF_TARGET_ESP32S31) && SOC_LCD_RGB_SUPPORTED
namespace nice_gfx { namespace detail {
struct RgbDiagnosticsStorage {
  VideoCopyStats copyStats_;
  VideoScanStats scanStats_;
};
} }
#endif
#if defined(CONFIG_IDF_TARGET_ESP32S3) && SOC_LCD_RGB_SUPPORTED
#include "../../Platform/ESP32/BoardI2c.h"
#include "../../Drivers/IO/TCA9554.h"
#include "../../Drivers/Display/Video/ST7701_RGB.h"
namespace nice_gfx { namespace detail {
// Constructed before the RGB display so commandContext stays valid through shutdown.
struct RgbCommandStorage {
  BoardI2c bus_;
  Tca9554CommandBus commands_;
  RgbCommandStorage(int sda,int scl):bus_(sda,scl),commands_(bus_,0x20,0,1,2){}
  Status prepareCommands() {
    Status s=commands_.begin();if(s!=Status::Ok)return s;
    if((s=commands_.level(0x80,true))!=Status::Ok ||
       (s=commands_.direction(0x80,false))!=Status::Ok ||
       (s=commands_.level(0x80,false))!=Status::Ok)return s;
    platform::delayMs(20);
    if((s=commands_.level(0x80,true))!=Status::Ok)return s;
    platform::delayMs(200);
    if((s=commands_.level(0x60,false))!=Status::Ok ||
       (s=commands_.direction(0x60,false))!=Status::Ok)return s;
    platform::delayMs(200);
    if((s=commands_.level(0x20,true))!=Status::Ok)return s;
    platform::delayMs(200);
    return commands_.direction(0x40,true);
  }
};
} } // namespace nice_gfx::detail
#endif
