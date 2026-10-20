#pragma once
#include "Hardware.h"
#include "UI/Input.h"
inline void beginButtons() {
  for (size_t i=0;i<kUiButtonCount;++i) { const auto &b=uiButtonPins()[i]; if(b.pin>=0)pinMode(b.pin,b.mode); }
}
inline void pollButtons(nice_gfx::htmlui::View &ui,nice_gfx::htmlui::TouchInput &touch,uint32_t now) {
  using namespace nice_gfx::htmlui;
  static KeyInput keys[] = {KeyInput(30,800,450,120),KeyInput(30,800,450,120),KeyInput(30,800,450,120),KeyInput(30,800,450,120),KeyInput(30,800,450,120),KeyInput(30,800,450,120),KeyInput(30,800,450,120),KeyInput(30,800,450,120)};
  static uint32_t epoch=0;
  if(epoch!=ui.inputGeneration()||ui.transitioning()) { for(auto &key:keys)key.cancel();epoch=ui.inputGeneration(); }
  for(size_t i=0;i<kUiButtonCount;++i) {
    const auto &b=uiButtonPins()[i]; if(b.pin<0)continue;
    const auto event=keys[i].update(digitalRead(b.pin)==(b.activeLow?LOW:HIGH),now,ui.hasKeyBinding(b.id,KeyGesture::Long),ui.hasKeyBinding(b.id,KeyGesture::Repeat));
    if(event!=KeyGesture::None&&ui.hasKeyBinding(b.id,event)) {
      touch.cancel(ui);ui.inputButton(b.id,event,now);
      if(epoch!=ui.inputGeneration()) { for(auto &key:keys)key.cancel();epoch=ui.inputGeneration();break; }
    }
  }
}
