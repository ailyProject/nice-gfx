#include "FrameProcessing.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <random>
#include <vector>

using namespace mosaico_camera;
static size_t checks = 0;
static void require(bool condition, const char *message) {
  ++checks;
  if (!condition) { std::fprintf(stderr, "FAIL: %s (check %zu)\n", message, checks); std::exit(1); }
}
static uint16_t referencePack(unsigned r, unsigned g, unsigned b) {
  return uint16_t((r / 8) * 2048 + (g / 4) * 32 + b / 8);
}
static SourceFrame makeSource(const std::vector<uint8_t> &data, size_t width, size_t height,
                              size_t stride, PixelFormat format) {
  return {data.data(), data.size(), width, height, stride, format};
}
static void testKnownPixels() {
  const uint16_t colors[] = {0xf800, 0x07e0, 0x001f, 0xffff, 0x0000, 0xffe0};
  const uint8_t rgb[][3] = {{255,0,0},{0,255,0},{0,0,255},{255,255,255},{0,0,0},{255,255,0}};
  for (PixelFormat format : {PixelFormat::RGB565LE, PixelFormat::RGB565BE, PixelFormat::RGB888, PixelFormat::BGR888}) {
    std::vector<uint8_t> bytes(6 * pixelBytes(format) + 7, 0xa5);
    for (size_t x=0; x<6; ++x) {
      if (format==PixelFormat::RGB565LE) { bytes[2*x]=uint8_t(colors[x]); bytes[2*x+1]=uint8_t(colors[x]>>8); }
      else if (format==PixelFormat::RGB565BE) { bytes[2*x]=uint8_t(colors[x]>>8); bytes[2*x+1]=uint8_t(colors[x]); }
      else for(size_t c=0;c<3;++c) bytes[3*x+c]=rgb[x][format==PixelFormat::RGB888?c:2-c];
    }
    auto source=makeSource(bytes,6,1,bytes.size(),format);
    require(validSource(source),"known pixel source valid");
    for(size_t x=0;x<6;++x)require(sourcePixel(source,x,0)==colors[x],"RGB format/byte order");
  }
  for(unsigned intensity=0;intensity<256;++intensity) {
    std::vector<uint8_t> bytes{uint8_t(intensity)};
    auto source=makeSource(bytes,1,1,1,PixelFormat::Gray8);
    require(sourcePixel(source,0,0)==referencePack(intensity,intensity,intensity),"gray conversion");
  }
  // BT.601 neutral chroma has exact legal-range black and white endpoints.
  for(PixelFormat format:{PixelFormat::UYVY,PixelFormat::YUYV}) {
    std::vector<uint8_t> bytes=format==PixelFormat::UYVY?std::vector<uint8_t>{128,16,128,235}:std::vector<uint8_t>{16,128,235,128};
    auto source=makeSource(bytes,2,1,4,format);
    require(sourcePixel(source,0,0)==0,"YUV black");require(sourcePixel(source,1,0)==0xffff,"YUV white");
  }
}
static void testValidation() {
  std::vector<uint8_t> bytes(32,0xa5);const auto valid=makeSource(bytes,3,2,8,PixelFormat::RGB565LE);
  require(validSource(valid),"padded row source valid");
  SourceFrame source=valid;source.bytes=14;require(validSource(source),"last row needs visible bytes only");
  source.bytes=13;require(!validSource(source),"short last row rejected");
  source=valid;source.data=nullptr;require(!validSource(source),"null source rejected");
  source=valid;source.width=0;require(!validSource(source),"zero source width rejected");
  source=valid;source.height=8193;require(!validSource(source),"excess height rejected");
  source=valid;source.stride=5;require(!validSource(source),"short stride rejected");
  source=valid;source.height=3;source.stride=(std::numeric_limits<size_t>::max)();require(!validSource(source),"stride overflow rejected");
  source=valid;source.format=PixelFormat::UYVY;require(!validSource(source),"odd YUV width rejected");
  std::array<uint16_t,20> guarded{};guarded.fill(0xbeef);
  require(!convertFrame(valid,guarded.data()+1,4,4,15),"short output rejected");
  require(std::all_of(guarded.begin(),guarded.end(),[](uint16_t p){return p==0xbeef;}),"failed conversion leaves output intact");
  require(!convertFrame(valid,guarded.data(),1025,1,20),"excess output dimensions rejected");
  require(!convertFrame(valid,nullptr,3,2,6),"null output rejected");
  source=valid;source.format=static_cast<PixelFormat>(999);require(!validSource(source),"unknown pixel format rejected");
}
static void testContainAndGuards() {
  std::mt19937 random(0x4d4f5341);
  for(size_t iteration=0;iteration<1200;++iteration) {
    const size_t sw=1+random()%79,sh=1+random()%67,dw=1+random()%83,dh=1+random()%71;
    const size_t stride=sw*2+random()%13;
    std::vector<uint8_t> bytes((sh-1)*stride+sw*2,0xed);
    std::vector<uint16_t> sourceColors(sw*sh);
    for(size_t y=0;y<sh;++y)for(size_t x=0;x<sw;++x) {
      const uint16_t value=uint16_t(1+(y*sw+x)%65534);sourceColors[y*sw+x]=value;
      bytes[y*stride+2*x]=uint8_t(value);bytes[y*stride+2*x+1]=uint8_t(value>>8);
    }
    const auto original=bytes;
    const auto source=makeSource(bytes,sw,sh,stride,PixelFormat::RGB565LE);
    std::vector<uint16_t> output(dw*dh+2,0xbeef);
    require(convertFrame(source,output.data()+1,dw,dh,dw*dh),"random padded conversion accepted");
    require(output.front()==0xbeef&&output.back()==0xbeef,"output guards retained");
    require(bytes==original,"source and padding retained");
    const long double scale=std::min(static_cast<long double>(dw)/sw,static_cast<long double>(dh)/sh);
    // Avoid a floating-point floor at an exactly integral ratio.
    const size_t vw=std::max<size_t>(1,static_cast<size_t>(std::floor(sw*scale+1e-12L)));
    const size_t vh=std::max<size_t>(1,static_cast<size_t>(std::floor(sh*scale+1e-12L)));
    const size_t left=(dw-vw)/2,top=(dh-vh)/2;
    for(size_t y=0;y<dh;++y)for(size_t x=0;x<dw;++x) {
      uint16_t expected=0;
      if(x>=left&&x<left+vw&&y>=top&&y<top+vh) {
        const size_t sx=static_cast<size_t>(std::floor(static_cast<long double>(x-left)*sw/vw));
        const size_t sy=static_cast<size_t>(std::floor(static_cast<long double>(y-top)*sh/vh));
        expected=sourceColors[sy*sw+sx];
      }
      require(output[1+y*dw+x]==expected,"aspect fit/letterbox/nearest-neighbor oracle");
    }
  }
}
static void testDetection() {
  constexpr size_t width=32,height=24;
  std::vector<uint16_t> pixels(width*height,0);
  const uint16_t color[]={0xf800,0x07e0,0x001f,0xffe0};
  const std::array<std::array<size_t,4>,4> boxes{{{{1,2,4,5}},{{8,6,3,7}},{{16,0,7,4}},{{25,16,7,8}}}};
  for(size_t k=0;k<4;++k){const auto &b=boxes[k];for(size_t y=b[1];y<b[1]+b[3];++y)for(size_t x=b[0];x<b[0]+b[2];++x)pixels[y*width+x]=color[k];}
  Detection detected[kDetectionCount];detectColors(pixels.data(),width,height,detected,1,1);
  for(size_t k=0;k<4;++k){const auto &b=boxes[k];require(detected[k].valid,"color rectangle found");require(detected[k].x==b[0]&&detected[k].y==b[1]&&detected[k].width==b[2]&&detected[k].height==b[3],"exact full sampling bbox");require(detected[k].samples==b[2]*b[3],"samples are counts");}
  detectColors(pixels.data(),width,height,detected,1,1000);for(const auto &d:detected)require(!d.valid,"minimum samples filtering");
  pixels.assign(width*height,0);pixels[0]=0xf800;pixels.back()=0xf800;detectColors(pixels.data(),width,height,detected,1,2);
  require(detected[0].valid&&detected[0].width==width&&detected[0].height==height&&detected[0].samples==2,"same-color objects deliberately merge");
  detectColors(nullptr,width,height,detected);for(const auto &d:detected)require(!d.valid&&!d.samples,"invalid input clears detections");
  detectColors(pixels.data(),width,height,detected,0);for(const auto &d:detected)require(!d.valid,"zero sampling step rejected");
  std::mt19937 random(0x43414d);
  for(size_t iteration=0;iteration<800;++iteration) {
    size_t w=1+random()%91,h=1+random()%83,step=1+random()%13;
    std::vector<uint16_t> frame(w*h);for(auto &pixel:frame)pixel=random()%6<4?color[random()%4]:0;
    detectColors(frame.data(),w,h,detected,step,1);
    for(const auto &d:detected)if(d.valid)require(d.width&&d.height&&d.x+d.width<=w&&d.y+d.height<=h,"sample cell expansion clips to frame");
  }
  for(uint16_t neutral:{uint16_t(0),uint16_t(0xffff),uint16_t(0x8410)})require(colorClass(neutral)==-1,"neutral colors not recognized");
}
int main(){testKnownPixels();testValidation();testContainAndGuards();testDetection();std::printf("PASS: %zu checks, 1200 contain/stride oracle cases, 800 detection bounds cases\n",checks);}
