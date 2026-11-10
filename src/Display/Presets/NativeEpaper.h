#pragma once
#include "../NativeEpaperDisplay.h"
#include "../../Drivers/Display/Epaper/NativeMono.h"

namespace nice_gfx { namespace epaper_profiles {
// Exact panel specifications, not aliases inferred from diagonal size.
struct GDEW042T2 {
  static const NativeEpaperProfile &profile(){
    static const NativeEpaperProfile p={"GDEW042T2",NativeEpaperController::UC8176,400,300,400,
      NativeEpaperEncoding::Mono,5000,120000,false,1,true,
      "https://thingpulse.com/wp-content/uploads/2019/07/GDEW042T2-V3.1-Specification_.pdf",
      &native_epaper::ucProtocol()};return p;
  }
};
struct GDEW0154M10 {
  static const NativeEpaperProfile &profile(){
    static const NativeEpaperProfile p={"GDEW0154M10",NativeEpaperController::UC8151D,152,152,152,
      NativeEpaperEncoding::Mono,5000,120000,false,1,true,
      "https://static.chipdip.ru/lib/392/DOC033392437.pdf",
      &native_epaper::ucProtocol()};return p;
  }
};
// The TCON firmware owns the glass calibration and waveform. Its device-info
// response must match these dimensions before any image upload is accepted.
// These are controller-board configurations, not bare parallel-panel drivers.
template<uint16_t Width,uint16_t Height> struct IT8951System {
  static const NativeEpaperProfile &profile(){
    static const NativeEpaperProfile p={"IT8951 4bpp system",NativeEpaperController::IT8951,
      Width,Height,Width,NativeEpaperEncoding::Gray16,1500,120000,false,1,true,
      "https://files.waveshare.com/upload/b/b0/IT8951_I80%2BProgrammingGuide_16bits_20170904_v2.7_common_CXDX.zip",nullptr};return p;
  }
};
} } // namespace nice_gfx::epaper_profiles
using NGFX_EPD_GDEW042T2=nice_gfx::NativeEpaperDisplay<nice_gfx::epaper_profiles::GDEW042T2>;
using NGFX_EPD_GDEW0154M10=nice_gfx::NativeEpaperDisplay<nice_gfx::epaper_profiles::GDEW0154M10>;
using UC8176_400x300_EPD_SPI=NGFX_EPD_GDEW042T2;
using IT8951_800x600_EPD_SPI=nice_gfx::NativeEpaperDisplay<nice_gfx::epaper_profiles::IT8951System<800,600>>;
using IT8951_1448x1072_EPD_SPI=nice_gfx::NativeEpaperDisplay<nice_gfx::epaper_profiles::IT8951System<1448,1072>>;
using IT8951_1872x1404_EPD_SPI=nice_gfx::NativeEpaperDisplay<nice_gfx::epaper_profiles::IT8951System<1872,1404>>;
