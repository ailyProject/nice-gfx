#pragma once
#include <stdint.h>
#if defined(ARDUINO_ARCH_CI13XX) && defined(__riscv)
extern "C" {
#include <ci130x_gpio.h>
}
namespace nice_gfx { namespace detail {
// DATA[mask] updates only the selected GPIO bits, without a port-wide RMW.
inline volatile uint32_t *ci13xxGpioData(uint8_t port,uint32_t mask) {
  const uintptr_t bases[]={uintptr_t(PA),uintptr_t(PB),uintptr_t(PC),uintptr_t(PD)};
  return port<4 ? reinterpret_cast<volatile uint32_t *>(bases[port]+4U*mask) : nullptr;
}
} }
#endif
