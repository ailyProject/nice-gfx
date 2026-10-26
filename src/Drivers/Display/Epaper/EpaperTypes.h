#pragma once
#include <stddef.h>
#include <stdint.h>
namespace nice_gfx {
enum class EpaperController:uint8_t{SSD1680,SSD1681};
struct EpaperProfile {
  const char *name;
  EpaperController controller;
  uint16_t width,height;
  const uint8_t *fullLut;
  size_t lutBytes;
  uint32_t minRefreshMs,busyTimeoutMs;
  uint16_t idleSettleMs;
  bool temperatureLoad;
  const uint8_t *partialLut=nullptr;
  uint32_t partialMinRefreshMs=500;
  constexpr EpaperProfile(const char *label,EpaperController chip,uint16_t w,uint16_t h,
      const uint8_t *full,size_t bytes,uint32_t minimum,uint32_t timeout,uint16_t settle,bool temperature,
      const uint8_t *partial=nullptr,uint32_t partialMinimum=500)
      :name(label),controller(chip),width(w),height(h),fullLut(full),lutBytes(bytes),
       minRefreshMs(minimum),busyTimeoutMs(timeout),idleSettleMs(settle),temperatureLoad(temperature),
       partialLut(partial),partialMinRefreshMs(partialMinimum){}
};
struct EpaperRefreshOptions {
  bool partial=true;
  uint16_t maxPartialUpdates=10;
  uint8_t partialAreaPercent=25;
  constexpr EpaperRefreshOptions(bool enabled=true,uint16_t limit=10,uint8_t percent=25)
      :partial(enabled),maxPartialUpdates(limit),partialAreaPercent(percent){}
};
struct EpaperWindow {
  uint16_t x=0,y=0,width=0,height=0;
  EpaperWindow(uint16_t left=0,uint16_t top=0,uint16_t w=0,uint16_t h=0):x(left),y(top),width(w),height(h){}
};
}
