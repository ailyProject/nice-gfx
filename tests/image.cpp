#include <UI/Image.h>
#include <cassert>
#include <cstring>
using namespace nice_gfx;
using namespace nice_gfx::htmlui;
static int released=0;
static void releaseImage(void *,const void *) { ++released; }
int main() {
  uint16_t native[]={0x1234,0xabcd,0,0x5678,0xef01};
  Image image;
  assert(Image::copy(ImageView(native,2,2,sizeof(native),ImageFormat::RGB565,PixelOrder::Native,6),image)==Status::Ok);
  const uint8_t expected[]={0x12,0x34,0xab,0xcd,0x56,0x78,0xef,0x01};
  assert(image.storageBytes()==sizeof(expected));
  assert(std::memcmp(image.view().pixels,expected,sizeof(expected))==0);
  const auto previous=image.view().pixels;
  assert(Image::copy(ImageView(native,1025,1,sizeof(native)),image)==Status::InvalidArgument);
  assert(image.view().pixels==previous);
  Image adopted;
  assert(Image::adopt(ImageView(expected,2,2,sizeof(expected),ImageFormat::RGB565,PixelOrder::Wire),releaseImage,nullptr,adopted)==Status::Ok);
  { Image shared=adopted; Image moved=static_cast<Image&&>(shared); adopted=Image();assert(released==0);assert(moved.valid());assert(!shared.valid()); }
  assert(released==1);
}
