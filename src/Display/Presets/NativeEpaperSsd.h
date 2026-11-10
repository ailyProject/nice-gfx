#pragma once
#include "../NativeEpaperDisplay.h"
#include "../../Drivers/Display/Epaper/SSD16xxProducts.h"

namespace nice_gfx { namespace epaper_profiles {

struct GDEY0154D67 {
  static const NativeEpaperProfile &profile() {
    static const NativeEpaperProfile p={"GDEY0154D67",NativeEpaperController::SSD1681,
      200,200,200,NativeEpaperEncoding::Mono,5000,120000,true,1,true,
      "https://files.seeedstudio.com/wiki/Other_Display/154-epaper/GDEY0154D67.pdf",
      &native_epaper::ssdProductProtocol()};return p;
  }
};
struct GDEY0213B74 {
  static const NativeEpaperProfile &profile() {
    static const NativeEpaperProfile p={"GDEY0213B74",NativeEpaperController::SSD1680,
      122,250,128,NativeEpaperEncoding::Mono,5000,120000,true,1,true,
      "https://files.seeedstudio.com/wiki/Other_Display/213-epaper/GDEY0213B74.pdf",
      &native_epaper::ssdProductProtocol()};return p;
  }
};
struct GDEY027T91 {
  static const NativeEpaperProfile &profile() {
    static const NativeEpaperProfile p={"GDEY027T91",NativeEpaperController::SSD1680,
      176,264,176,NativeEpaperEncoding::Mono,5000,120000,true,1,true,
      "https://v4.cecdn.yun300.cn/100001_1909185148/GDEY027T91.pdf?download=1",
      &native_epaper::ssdProductProtocol()};return p;
  }
};
struct GDEY0266T90 {
  static const NativeEpaperProfile &profile() {
    static const NativeEpaperProfile p={"GDEY0266T90",NativeEpaperController::SSD1680,
      152,296,152,NativeEpaperEncoding::Mono,5000,120000,true,1,true,
      "https://v4.cecdn.yun300.cn/100001_1909185148/GDEY0266T90.pdf?download=1",
      &native_epaper::ssdProductProtocol()};return p;
  }
};
struct GDEY029T94 {
  static const NativeEpaperProfile &profile() {
    static const NativeEpaperProfile p={"GDEY029T94",NativeEpaperController::SSD1680,
      128,296,128,NativeEpaperEncoding::Mono,5000,120000,true,1,true,
      "https://files.seeedstudio.com/wiki/Other_Display/29-epaper/GDEY029T94.pdf",
      &native_epaper::ssdProductProtocol()};return p;
  }
};
struct GDEY042T81 {
  static const NativeEpaperProfile &profile() {
    static const NativeEpaperProfile p={"GDEY042T81",NativeEpaperController::SSD1683,
      400,300,400,NativeEpaperEncoding::Mono,5000,120000,true,1,true,
      "https://files.seeedstudio.com/wiki/Other_Display/42-epaper/GDEY042T81.pdf",
      &native_epaper::ssdProductProtocol()};return p;
  }
};
struct GDEQ0426T82 {
  static const NativeEpaperProfile &profile() {
    static const NativeEpaperProfile p={"GDEQ0426T82",NativeEpaperController::SSD1677,
      800,480,800,NativeEpaperEncoding::Mono,5000,120000,true,1,true,
      "https://files.seeedstudio.com/wiki/Other_Display/426-epaper/GDEQ0426T82.pdf",
      &native_epaper::gdeq0426Protocol()};return p;
  }
};
struct GDEM133T91 {
  static const NativeEpaperProfile &profile() {
    static const NativeEpaperProfile p={"GDEM133T91",NativeEpaperController::SSD1677,
      960,680,960,NativeEpaperEncoding::Mono,5000,120000,true,1,true,
      "https://v4.cecdn.yun300.cn/100001_1909185148/GDEM133T91.pdf?download=1",
      &native_epaper::gdem133Protocol()};return p;
  }
};
struct GDEH116T91 {
  static const NativeEpaperProfile &profile() {
    static const NativeEpaperProfile p={"GDEH116T91",NativeEpaperController::SSD1677,
      960,640,960,NativeEpaperEncoding::Mono,5000,120000,true,1,true,
      "https://v4.cecdn.yun300.cn/100001_1909185148/GDEH116T91-210119.pdf?download=1",
      &native_epaper::gdem133Protocol()};return p;
  }
};
struct GDEM0397T81P {
  static const NativeEpaperProfile &profile() {
    static const NativeEpaperProfile p={"GDEM0397T81P",NativeEpaperController::SSD1677,
      800,480,800,NativeEpaperEncoding::Mono,5000,120000,true,1,true,
      "https://v4.cecdn.yun300.cn/100001_1909185148/GDEM0397T81P.pdf?download=1",
      &native_epaper::gdem0397pProtocol()};return p;
  }
};

// Discovery entries are deliberately refused by begin(). A matching
// controller and geometry alone do not establish a factory-OTP panel flow.
struct GDEM0397T81 {
  static const NativeEpaperProfile &profile() {
    static const NativeEpaperProfile p={"GDEM0397T81",NativeEpaperController::SSD1677,
      800,480,800,NativeEpaperEncoding::Mono,5000,120000,true,1,false,
      "https://www.good-display.com/product/1167.html",nullptr};return p;
  }
};
struct GDEM102T91 {
  static const NativeEpaperProfile &profile() {
    static const NativeEpaperProfile p={"GDEM102T91",NativeEpaperController::SSD1677,
      960,640,960,NativeEpaperEncoding::Mono,5000,120000,true,1,false,
      "https://v4.cecdn.yun300.cn/100001_1909185148/GDEM102T91.pdf?download=1",
      nullptr};return p;
  }
};

} } // namespace nice_gfx::epaper_profiles

using NGFX_EPD_GDEY0154D67=nice_gfx::NativeEpaperDisplay<nice_gfx::epaper_profiles::GDEY0154D67>;
using NGFX_EPD_GDEY0213B74=nice_gfx::NativeEpaperDisplay<nice_gfx::epaper_profiles::GDEY0213B74>;
using NGFX_EPD_GDEY0266T90=nice_gfx::NativeEpaperDisplay<nice_gfx::epaper_profiles::GDEY0266T90>;
using NGFX_EPD_GDEY027T91=nice_gfx::NativeEpaperDisplay<nice_gfx::epaper_profiles::GDEY027T91>;
using NGFX_EPD_GDEY029T94=nice_gfx::NativeEpaperDisplay<nice_gfx::epaper_profiles::GDEY029T94>;
using NGFX_EPD_GDEY042T81=nice_gfx::NativeEpaperDisplay<nice_gfx::epaper_profiles::GDEY042T81>;
using NGFX_EPD_GDEQ0426T82=nice_gfx::NativeEpaperDisplay<nice_gfx::epaper_profiles::GDEQ0426T82>;
using NGFX_EPD_GDEM0397T81=nice_gfx::NativeEpaperDisplay<nice_gfx::epaper_profiles::GDEM0397T81>;
using NGFX_EPD_GDEM0397T81P=nice_gfx::NativeEpaperDisplay<nice_gfx::epaper_profiles::GDEM0397T81P>;
using NGFX_EPD_GDEM102T91=nice_gfx::NativeEpaperDisplay<nice_gfx::epaper_profiles::GDEM102T91>;
using NGFX_EPD_GDEM133T91=nice_gfx::NativeEpaperDisplay<nice_gfx::epaper_profiles::GDEM133T91>;
using NGFX_EPD_GDEH116T91=nice_gfx::NativeEpaperDisplay<nice_gfx::epaper_profiles::GDEH116T91>;
