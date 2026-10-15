#pragma once
// Bare RGB controller: panel timing and physical data lines configure the chip.
// This profile requires no SPI/I2C/MIPI command initialization.
#include "../../../Display/VideoControllerTypes.h"
namespace nice_gfx { namespace video {
static constexpr VideoInitProgram EK9716B_RGB_Program;
} }
