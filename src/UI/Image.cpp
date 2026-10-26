#include "Image.h"
#if !defined(ARDUINO_ARCH_AVR) && !defined(__AVR__)
#include "detail/ImageStorage.h"
#include <new>
#include <utility>
#include <string.h>
namespace nice_gfx { namespace htmlui {
namespace {
bool validImageView(const ImageView &source, size_t &stride, size_t &packed) {
  if(!source.pixels||!source.width||!source.height||source.width>4096||source.height>4096||
     uint8_t(source.format)>uint8_t(ImageFormat::Mono1Masked)||uint8_t(source.order)>uint8_t(PixelOrder::Wire)||
     source.byteLength>8388608)return false;
  const bool mono=source.format==ImageFormat::Mono1||source.format==ImageFormat::Mono1Masked;
  const size_t planes=source.format==ImageFormat::Mono1Masked?2:1;
  const size_t row=mono?(source.width+7)/8:source.width*(source.format==ImageFormat::RGB565?2:1);
  stride=source.strideBytes?source.strideBytes:row;
  if(stride<row||source.height*planes-1>(SIZE_MAX-row)/stride)return false;
  const size_t required=(source.height*planes-1)*stride+row;
  if(source.byteLength<required||reinterpret_cast<uintptr_t>(source.pixels)>UINTPTR_MAX-required)return false;
  packed=row*source.height*planes;return true;
}
void releaseCopiedImage(void *,const void *pixels) { delete[] static_cast<const uint8_t *>(pixels); }
}
Image::Image():impl_(nullptr) {}
Image::Image(Impl *storage):impl_(storage) {}
Image::~Image() { release(); }
void Image::retain() { if(impl_)impl_->references.fetch_add(1,std::memory_order_relaxed); }
void Image::release() {
  if(impl_&&impl_->references.fetch_sub(1,std::memory_order_acq_rel)==1) {
    if(impl_->deleter)impl_->deleter(impl_->context,impl_->source.pixels);
    delete impl_;
  }
  impl_=nullptr;
}
Image::Image(const Image &other):impl_(other.impl_) { retain(); }
Image &Image::operator=(const Image &other) { if(this!=&other){Image copy(other);std::swap(impl_,copy.impl_);}return *this; }
Image::Image(Image &&other) noexcept:impl_(other.impl_) { other.impl_=nullptr; }
Image &Image::operator=(Image &&other) noexcept { if(this!=&other){release();impl_=other.impl_;other.impl_=nullptr;}return *this; }
bool Image::valid() const { return impl_!=nullptr; }
ImageView Image::view() const { return impl_?impl_->source:ImageView(); }
size_t Image::storageBytes() const { return impl_?impl_->ownedBytes:0; }
Status Image::copy(const ImageView &source,Image &out) {
  size_t stride,bytes;if(!validImageView(source,stride,bytes))return Status::InvalidArgument;
  uint8_t *pixels=new(std::nothrow) uint8_t[bytes];if(!pixels)return Status::NoMemory;
  const size_t planes=source.format==ImageFormat::Mono1Masked?2:1;
  const size_t row=bytes/source.height/planes;
  const auto *input=static_cast<const uint8_t *>(source.pixels);
  for(size_t y=0;y<source.height*planes;++y) {
    if(source.format==ImageFormat::RGB565&&source.order==PixelOrder::Native) {
      for(size_t x=0;x<source.width;++x){uint16_t color;memcpy(&color,input+y*stride+2*x,2);pixels[y*row+2*x]=uint8_t(color>>8);pixels[y*row+2*x+1]=uint8_t(color);}
    } else memcpy(pixels+y*row,input+y*stride,row);
  }
  const ImageView packed(pixels,source.width,source.height,bytes,source.format,PixelOrder::Wire,row);
  Impl *storage=new(std::nothrow) Impl(packed,releaseCopiedImage,nullptr,bytes);
  if(!storage){delete[] pixels;return Status::NoMemory;}
  Image image(storage);out=std::move(image);return Status::Ok;
}
Status Image::adopt(const ImageView &source,ImageRelease callback,void *context,Image &out) {
  size_t stride,bytes;if(!callback||!validImageView(source,stride,bytes))return Status::InvalidArgument;
  ImageView view=source;view.strideBytes=stride;
  Impl *storage=new(std::nothrow) Impl(view,callback,context,source.byteLength);
  if(!storage)return Status::NoMemory;
  Image image(storage);out=std::move(image);return Status::Ok;
}

} }
#endif
