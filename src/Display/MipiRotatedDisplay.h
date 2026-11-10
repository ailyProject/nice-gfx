#pragma once
#include "VideoDisplay.h"
#include "../Platform/ESP32/Esp32MipiRotation.h"
#if defined(CONFIG_IDF_TARGET_ESP32P4) && SOC_MIPI_DSI_SUPPORTED && SOC_PPA_SUPPORTED
namespace nice_gfx { namespace detail {
template<uint16_t Width,uint16_t Height,uint8_t Rotation>
class MipiRotatedPanel:public VideoPanel<Width,Height>{
 public:
  MipiRotatedPanel(Transport &bus,int reset=-1,PanelOptions options=PanelOptions())
    :VideoPanel<Width,Height>(bus,reset,options){}
  // The transport owns the fixed memory transform; the controller sees only
  // its native video timing. Zero is used during Panel::begin initialization.
  Status setRotation(uint8_t r)override{return r==0||r==Rotation?Status::Ok:Status::Unsupported;}
  uint8_t rotation()const override{return Rotation;}
};
}
template<VideoController Controller,uint16_t NativeWidth,uint16_t NativeHeight,uint8_t Rotation=1>
class MipiRotatedDisplay:public OwnedDisplay<Esp32MipiRotation,detail::MipiRotatedPanel<(Rotation&1)?NativeHeight:NativeWidth,(Rotation&1)?NativeWidth:NativeHeight,Rotation>>{
  static_assert(Rotation>=1&&Rotation<=3,"MIPI PPA rotation uses 1, 2 or 3");
  using Base=OwnedDisplay<Esp32MipiRotation,detail::MipiRotatedPanel<(Rotation&1)?NativeHeight:NativeWidth,(Rotation&1)?NativeWidth:NativeHeight,Rotation>>;
  static MipiConfig config(MipiConfig value){value.controller=Controller;return value;}
 public:
  explicit MipiRotatedDisplay(const MipiConfig &value)
    :Base(detail::videoOptions((Rotation&1)?NativeHeight:NativeWidth,(Rotation&1)?NativeWidth:NativeHeight),-1,config(value),Rotation){}
};
} // namespace nice_gfx
#endif
