#pragma once
#include "SizedPanel.h"
#include "../../Drivers/Display/ST7789.h"
#include "../../Drivers/Display/JD9853A.h"
#include "../../Drivers/Display/ST7789P.h"

namespace nice_gfx {

using ST7789V2_170x320 = SizedPanel<ST7789,170,320>;
using ST7789V2_240x280 = SizedPanel<ST7789,240,280>;
using ST7789T3_240x320 = SizedPanel<ST7789,240,320>;

} // namespace nice_gfx
