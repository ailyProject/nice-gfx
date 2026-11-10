#pragma once
#include "../NativeEpaperDisplay.h"
#include "../../Drivers/Display/Epaper/UC8159.h"

namespace nice_gfx { namespace epaper_profiles {

// Exact factory-flash panels; host LUTs and substitute glass are unsupported.
// Source-qualified, with no physical display validation performed.
struct GDEW075T8 {
  static const NativeEpaperProfile &profile() {
    static const NativeEpaperProfile p={"GDEW075T8",NativeEpaperController::UC8159,
      640,384,640,NativeEpaperEncoding::Mono,5000,120000,false,1,true,
      "https://www.e-paper-display.com/GDEW075T8%20V3.1%20Specificationbcd8.pdf?method=picker&flag=all&id=baad6eef-df65-41e4-8197-dea4d3a38aba&fileId=486&v=2.zip",
      &native_epaper::uc8159Protocol()};
    return p;
  }
};

struct GDEW0583T7 {
  static const NativeEpaperProfile &profile() {
    static const NativeEpaperProfile p={"GDEW0583T7",NativeEpaperController::UC8159,
      600,448,600,NativeEpaperEncoding::Mono,5000,120000,false,1,true,
      "https://www.e-paper-display.com/GDEW0583T7%20V2.1%20Specification4d76.pdf?fileId=577&flag=all&id=8cb02b5f-d08e-4c3c-89d3-dd8cb1c0932d&method=picker&v=2.zip",
      &native_epaper::uc8159Protocol()};
    return p;
  }
};

} } // namespace nice_gfx::epaper_profiles

using NGFX_EPD_GDEW075T8=nice_gfx::NativeEpaperDisplay<nice_gfx::epaper_profiles::GDEW075T8>;
using NGFX_EPD_GDEW0583T7=nice_gfx::NativeEpaperDisplay<nice_gfx::epaper_profiles::GDEW0583T7>;
