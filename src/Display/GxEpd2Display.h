#pragma once
#include "Display.h"
#include "GxEpd2Types.h"
#include "Backends/GxEpd2Panel.h"
#include "Backends/GxEpd2PlatformIo.h"
namespace nice_gfx { namespace detail {
template<class Tag>struct GxEpd2DisplayStorage{
  GxEpd2PlatformIo io;GxEpd2Panel panel;
  explicit GxEpd2DisplayStorage(const GxEpd2Config&c):panel(Tag::profile(),c,c.io?*c.io:io){}
};
}
template<class Tag>class GxEpd2Display:private detail::GxEpd2DisplayStorage<Tag>,public NICE_GFX{
  using Storage=detail::GxEpd2DisplayStorage<Tag>;
 public:
  explicit GxEpd2Display(const GxEpd2Config&c=GxEpd2Config()):Storage(c),NICE_GFX(Storage::panel){}
  bool begin(){return beginChecked()==Status::Ok;}
  GxEpd2Panel&gxEpd2Panel(){return Storage::panel;}
  const GxEpd2Panel&gxEpd2Panel()const{return Storage::panel;}
};
}
