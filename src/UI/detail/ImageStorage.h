#pragma once
#include "../Image.h"
#include <atomic>
namespace nice_gfx { namespace htmlui {
struct Image::Impl {
  std::atomic<size_t> references;
  ImageView source;
  MediaDesc media;
  ImageRelease deleter;
  void *context;
  size_t ownedBytes;
  bool flash;
  Impl(const ImageView &view,ImageRelease callback,void *user,size_t bytes,bool compiled=false)
    : references(1),source(view),media{static_cast<const uint8_t *>(view.pixels),uint16_t(view.width),uint16_t(view.height),1,1,uint8_t(view.format==ImageFormat::RGB565?2:view.format==ImageFormat::Mono1?MediaMono1:view.format==ImageFormat::Mono1Masked?MediaMono1Masked:1)},deleter(callback),context(user),ownedBytes(bytes),flash(compiled) {}
};
} }
