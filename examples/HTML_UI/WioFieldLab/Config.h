#pragma once
// Set either option to 0 for a dependency-free live-instrument build.
#ifndef WIO_ENABLE_SD
#define WIO_ENABLE_SD 1
#endif
#ifndef WIO_ENABLE_WIFI
#define WIO_ENABLE_WIFI 1
#endif
#ifndef WIO_ENABLE_IR
#define WIO_ENABLE_IR 1
#endif
// Deliberately labelled test commands. Replace with your receiver's NEC profile.
constexpr uint16_t kIrAddress = 0x00;
constexpr uint8_t kIrCommands[] = {0x45,0x46,0x47};
