#pragma once
#include "../../../Display/Backends/GxEpd2Hal.h"
#include "../../../Platform/Common/UiMemory.h"
#include <stdlib.h>
#include <string.h>
namespace nice_gfx { namespace seeed_protocol {
inline void pinMode(int pin,uint8_t mode){if(pin>=0)if(auto*h=gxEpd2CurrentHal())h->pinMode(pin,mode);}
inline void digitalWrite(int pin,int value){if(pin>=0)if(auto*h=gxEpd2CurrentHal())h->writePin(pin,value!=0);}
inline int digitalRead(int pin){if(auto*h=gxEpd2CurrentHal())return h->readPin(pin);return 0;}
inline void delay(uint32_t ms){if(auto*h=gxEpd2CurrentHal())h->delayMs(ms);}
inline void delayMicroseconds(uint32_t us){if(auto*h=gxEpd2CurrentHal())h->delayUs(us);}
inline uint32_t millis(){if(auto*h=gxEpd2CurrentHal())return h->micros()/1000;return 0;}
inline void yield(){if(auto*h=gxEpd2CurrentHal())h->yield();}
inline void gfxPinModeOutput(int p){pinMode(p,NGFX_GXEPD2_OUTPUT);}
inline void gfxPinModeInput(int p){pinMode(p,NGFX_GXEPD2_INPUT);}
inline void gfxPinModeInputPullup(int p){pinMode(p,NGFX_GXEPD2_INPUT_PULLUP);}
inline void gfxDigitalWrite(int p,bool high){digitalWrite(p,high);}
inline bool gfxDigitalRead(int p){return digitalRead(p)!=0;}
// Read-free drivers record this wait and the engine checks real BUSY on replay.
// Worker drivers execute the same wait synchronously against actual hardware.
inline bool gfxWaitForPin(int pin,bool readyHigh,uint32_t timeout=30000,uint16_t=1){
  if(pin>=0)if(auto*h=gxEpd2CurrentHal()){h->waitBusy(pin,!readyHigh,timeout*1000UL,timeout);return h->status()==Status::Ok;}
  return true;
}
inline void*protocolAllocate(size_t bytes){auto*p=platform::allocateRetained(bytes);if(!p)if(auto*h=gxEpd2CurrentHal())h->fault(Status::NoMemory);return p;}
inline void protocolFree(void*p){platform::freeUiMemory(p);}
} }
