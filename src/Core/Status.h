#pragma once
#include <stdint.h>
namespace nice_gfx {
enum class Status : uint8_t { Ok, InvalidArgument, NotInitialized, Busy, NoMemory, Unsupported, IoError };
}
