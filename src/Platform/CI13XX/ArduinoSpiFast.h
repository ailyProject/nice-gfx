#pragma once
#if defined(ARDUINO_ARCH_CI13XX) && defined(__riscv)
#include <Arduino.h>
#include "../Common/BusTypes.h"
#include "Gpio.h"
#include "SpiWrite.h"
extern "C" {
#include <platform_config.h>
#include <ci130x_core_misc.h>
#include <FreeRTOS.h>
#include <task.h>
}
#ifndef NICE_GFX_CI13XX_FAST_SPI
#define NICE_GFX_CI13XX_FAST_SPI 1
#endif

#if NICE_GFX_CI13XX_FAST_SPI
namespace nice_gfx { namespace detail {
inline uint32_t ci13xxCycles() {
  uint32_t value;__asm__ volatile("csrr %0, 0xB00":"=r"(value));return value;
}
struct Ci13xxWriteIo {
  volatile uint32_t *clock,*mosi,*combined;
  uint32_t clockMask,mosiMask;
  uint32_t cycles()const{return ci13xxCycles();}
  void low(bool high)const{
    const uint32_t value=high?mosiMask:0;
    if(combined)*combined=value;
    else{*clock=0;*mosi=value;}
  }
  void high()const{*clock=clockMask;}
  void idleLow()const{*clock=0;}
  void yield()const{taskYIELD();}
};
inline bool ci13xxFastWrite(const SpiConfig &config,const uint8_t *bytes,size_t count) {
  // SPI.begin() has created the core mutex and claimed its GPIO route. In a
  // normal running task beginTransaction() waits for and owns that mutex until
  // finish(). Other contexts retain the core's checked transfer path.
  if(config.mode!=0||config.pins.miso>=0||check_curr_trap()!=0||
     xTaskGetSchedulerState()!=taskSCHEDULER_RUNNING)return false;
  uint32_t inhibited;__asm__ volatile("csrr %0, 0x320":"=r"(inhibited));
  if(inhibited&1U)return false;
  const uint32_t cpuHz=get_ipcore_clk();
  if(!cpuHz||!config.frequency)return false;
  const auto &clock=g_APinDescription[config.pins.clock];
  const auto &mosi=g_APinDescription[config.pins.mosi];
  if(clock.port>=4||mosi.port>=4||clock.bit>=8||mosi.bit>=8)return false;
  const uint32_t clockMask=1U<<clock.bit,mosiMask=1U<<mosi.bit;
  Ci13xxWriteIo io={ci13xxGpioData(clock.port,clockMask),
    ci13xxGpioData(mosi.port,mosiMask),
    clock.port==mosi.port?ci13xxGpioData(clock.port,clockMask|mosiMask):nullptr,
    clockMask,mosiMask};
  // The existing begin() cap keeps frequency <= 4 MHz. Use the SDK's actual
  // recorded CPU clock, rather than the sketch's nominal F_CPU constant.
  const uint32_t halfCycles=(cpuHz-1U)/(2U*config.frequency)+1U;
  ci13xxWriteMode0(io,bytes,count,halfCycles);
  return true;
}
} }
#endif
#endif
