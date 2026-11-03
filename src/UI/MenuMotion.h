#pragma once
#include <stdint.h>
#define NICE_GFX_UI_MENU_MOTION_VERSION 2

namespace nice_gfx { namespace htmlui {

enum class MenuMotionMode : uint8_t { Instant, Smooth, Spring };
enum class MenuIndicator : uint8_t { Outline, Invert, Corners, Underline };
enum class MenuScroll : uint8_t { Edge, Center };
enum class MenuFit : uint8_t { Bounds, Text };
enum class MenuPreset : uint8_t { Custom, Astra, Miao, Snappy, Gentle, Bouncy, Reduced };
struct MenuDynamics { float frequencyHz=0, dampingRatio=0.75f; };
struct MenuMarqueeConfig { bool enabled=false;uint16_t dwellMs=700,speed=24,pauseMs=500; };
// Explicit opt-in for immediate monochrome displays. Logical focus changes
// immediately; only its indicator and focus-driven scroll offsets follow.
// With no explicit configuration the legacy indicator and scrolling remain.
// Explicit Instant applies the chosen indicator/scroll policy without tweening.
struct MenuMotionConfig {
  MenuMotionMode mode = MenuMotionMode::Instant;
  MenuIndicator indicator = MenuIndicator::Outline;
  MenuScroll scroll = MenuScroll::Edge;
  uint16_t responseMs = 160; // 40..5000; response time, not a fixed duration.
  uint8_t radius = 3;
  MenuFit fit=MenuFit::Bounds;uint8_t textPadding=4;
  MenuPreset preset=MenuPreset::Custom;
  MenuDynamics camera{},position{},size{},decoration{},overlay{};
  bool scrollbar=false,optionBar=false,tileCaption=false,previousNext=false,compactRows=false;
  MenuMarqueeConfig marquee{};
  bool ellipsis=false;
  float speed=1;uint16_t maxDeltaMs=64;uint8_t maxFps=60;
};
inline bool validMenuDynamics(const MenuDynamics &d) {
  return (d.frequencyHz==0||(d.frequencyHz>=0.25f&&d.frequencyHz<=30))&&d.dampingRatio>=0.1f&&d.dampingRatio<=2;
}
inline MenuMotionConfig resolveMenuPreset(MenuMotionConfig c) {
  if(c.preset==MenuPreset::Custom)return c;
  if(c.preset==MenuPreset::Reduced){c.mode=MenuMotionMode::Instant;c.marquee.enabled=false;return c;}
  if(c.preset==MenuPreset::Bouncy){c.mode=MenuMotionMode::Spring;MenuDynamics *channels[]={&c.camera,&c.position,&c.size,&c.decoration,&c.overlay};for(auto *d:channels)if(d->frequencyHz==0){d->frequencyHz=3.0f;d->dampingRatio=0.32f;}return c;}
  const float frequencies[4][5]={{3.3f,4.2f,5.2f,3.8f,3.4f},{4.5f,5.0f,6.0f,4.5f,4.0f},{7.0f,8.0f,9.0f,7.0f,6.0f},{2.0f,2.4f,3.0f,2.2f,2.0f}};
  const float damping[4]={0.58f,0.82f,0.9f,1.0f};const unsigned preset=unsigned(c.preset)-1;
  MenuDynamics *channels[]={&c.camera,&c.position,&c.size,&c.decoration,&c.overlay};
  for(unsigned i=0;i<5;++i)if(channels[i]->frequencyHz==0){channels[i]->frequencyHz=frequencies[preset][i];channels[i]->dampingRatio=damping[preset];}
  return c;
}
inline bool validMenuMotion(const MenuMotionConfig &c) {
  return uint8_t(c.mode)<=uint8_t(MenuMotionMode::Spring)&&
    uint8_t(c.indicator)<=uint8_t(MenuIndicator::Underline)&&uint8_t(c.fit)<=uint8_t(MenuFit::Text)&&uint8_t(c.preset)<=uint8_t(MenuPreset::Reduced)&&c.textPadding<=32&&
    validMenuDynamics(c.camera)&&validMenuDynamics(c.position)&&validMenuDynamics(c.size)&&validMenuDynamics(c.decoration)&&validMenuDynamics(c.overlay)&&
    c.speed>=0.25f&&c.speed<=4&&c.maxDeltaMs>=8&&c.maxDeltaMs<=100&&c.maxFps>=1&&c.maxFps<=60&&
    c.marquee.speed>=1&&c.marquee.speed<=120&&c.marquee.dwellMs<=60000&&c.marquee.pauseMs<=60000&&
    uint8_t(c.scroll)<=uint8_t(MenuScroll::Center)&&c.responseMs>=40&&c.responseMs<=5000;
}

} }
