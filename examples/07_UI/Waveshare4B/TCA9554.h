#pragma once
#include <Drivers/IO/TCA9554.h>

// Compatibility names for the example; the chip protocol is shared by board presets.
namespace waveshare4b {
using Tca9554=nice_gfx::Tca9554;
using Tca9554CommandBus=nice_gfx::Tca9554CommandBus;
}
