#pragma once
#include "../NativeEpaperDisplay.h"
#include "../../Drivers/Display/Epaper/NativeMono.h"

namespace nice_gfx { namespace epaper_profiles {

// Exact panel identities with documented factory-OTP full updates. These
// profiles are source-qualified; they have not been validated on hardware.
struct GDEW0213T5D {
  static const NativeEpaperProfile &profile() {
    static const NativeEpaperProfile p={"GDEW0213T5D",NativeEpaperController::UC8151D,
      104,212,104,NativeEpaperEncoding::Mono,5000,120000,false,1,true,
      "https://www.laskakit.cz/user/related_files/gdew0213t5d-1.pdf",
      &native_epaper::ucProtocol()};
    return p;
  }
};

struct GDEW029T5D {
  static const NativeEpaperProfile &profile() {
    static const NativeEpaperProfile p={"GDEW029T5D",NativeEpaperController::UC8151D,
      128,296,128,NativeEpaperEncoding::Mono,5000,120000,false,1,true,
      "https://www.laskakit.cz/user/related_files/gdew029t5d.pdf",
      &native_epaper::ucProtocol()};
    return p;
  }
};

struct GDEY075T7 {
  static const NativeEpaperProfile &profile() {
    static const NativeEpaperProfile p={"GDEY075T7",NativeEpaperController::UC8179,
      800,480,800,NativeEpaperEncoding::Mono,5000,120000,false,1,true,
      "https://files.seeedstudio.com/wiki/Other_Display/750-epaper/GDEY075T7.pdf",
      &native_epaper::ucProtocol()};
    return p;
  }
};

struct GDEY0583T81 {
  static const NativeEpaperProfile &profile() {
    static const NativeEpaperProfile p={"GDEY0583T81",NativeEpaperController::UC8179,
      648,480,648,NativeEpaperEncoding::Mono,5000,120000,false,1,true,
      "https://files.seeedstudio.com/wiki/Other_Display/583-epaper/GDEY0583T81.pdf",
      &native_epaper::ucProtocol()};
    return p;
  }
};

} } // namespace nice_gfx::epaper_profiles

using NGFX_EPD_GDEW0213T5D=nice_gfx::NativeEpaperDisplay<nice_gfx::epaper_profiles::GDEW0213T5D>;
using NGFX_EPD_GDEW029T5D=nice_gfx::NativeEpaperDisplay<nice_gfx::epaper_profiles::GDEW029T5D>;
using NGFX_EPD_GDEY075T7=nice_gfx::NativeEpaperDisplay<nice_gfx::epaper_profiles::GDEY075T7>;
using NGFX_EPD_GDEY0583T81=nice_gfx::NativeEpaperDisplay<nice_gfx::epaper_profiles::GDEY0583T81>;
