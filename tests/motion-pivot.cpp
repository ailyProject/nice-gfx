#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "UI/View.h"
#include "UI/MotionGeometry.h"
#include "UI/Animation.h"
#include "UI/Shapes.h"
#include "Graphics/Icon.h"

using namespace nice_gfx;
using namespace nice_gfx::htmlui;

namespace {
constexpr int Width=112, Height=96, Guard=32;
constexpr uint16_t Background=0x1234, Canary=0xa55a, ImageColor=0xf81f;
int checks=0;
void require(bool condition,const char *message) {
  ++checks;
  if(!condition){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}

// Capture real Panel RGB565 transfers. Reject invalid windows or overlong
// writes, and guard the destination so clipping errors cannot hide in a mock.
struct MemoryTransport : Transport {
  std::vector<uint16_t> storage=std::vector<uint16_t>(Guard+Width*Height+Guard,Canary);
  int left=0,top=0,right=0,bottom=0,cursor=0;
  size_t transferred=0;
  bool accelerated=false;
  MemoryTransport(){std::fill(storage.begin()+Guard,storage.end()-Guard,Background);}
  Status begin() override{return Status::Ok;}
  void end() override{}
  Status command(uint8_t command,const uint8_t *data,size_t bytes) override {
    if(command==0x2a||command==0x2b){
      require(bytes==4,"panel window command has invalid length");
      const int a=int(data[0])*256+data[1],b=int(data[2])*256+data[3];
      require(a>=0&&b>=a&&b<(command==0x2a?Width:Height),"out-of-screen panel window");
      if(command==0x2a){left=a;right=b;}else{top=a;bottom=b;}cursor=0;
    }
    return Status::Ok;
  }
  void push(uint16_t color){
    const int x=left+cursor%(right-left+1),y=top+cursor/(right-left+1);
    require(y<=bottom,"panel transfer exceeds its window");
    storage[Guard+y*Width+x]=color;++cursor;++transferred;
  }
  Status pixels(uint8_t,PixelView view,bool) override {
    for(size_t y=0;y<view.height;++y)for(size_t x=0;x<view.width;++x){
      const uint8_t *p=view.data+2*(y*view.stride+x);uint16_t color;
      if(view.order==PixelOrder::Wire)color=uint16_t(p[0]<<8|p[1]);else std::memcpy(&color,p,2);
      push(color);
    }
    return Status::Ok;
  }
  Status fill(uint8_t,uint16_t color,size_t count) override{while(count--)push(color);return Status::Ok;}
  Status wait() override{return Status::Ok;}
  bool busy() const override{return false;}
  bool asynchronous() const override{return false;}
  bool supportsFrameBlit() const override{return accelerated;}
  void output(int,bool) override{}
  void delayMs(uint32_t) override{}
  uint16_t at(int x,int y) const{return storage[Guard+y*Width+x];}
  std::vector<uint16_t> pixels() const{return {storage.begin()+Guard,storage.end()-Guard};}
  void safe() const{
    require(std::all_of(storage.begin(),storage.begin()+Guard,[](uint16_t c){return c==Canary;}),"leading framebuffer guard overwritten");
    require(std::all_of(storage.end()-Guard,storage.end(),[](uint16_t c){return c==Canary;}),"trailing framebuffer guard overwritten");
  }
};
const PanelConfig panelConfig={Width,Height,0,nullptr,0,{0,0,0,0},{0,0,0,0},{0,0,0,0},1};
struct MemoryDisplay : NICE_GFX{explicit MemoryDisplay(Panel &p):NICE_GFX(p){}};

struct Fixture {
  MemoryTransport transport;
  Panel panel{transport,panelConfig};
  MemoryDisplay display{panel};
  std::vector<uint16_t> tile1,tile2;
  View view;
  std::vector<NodeDesc> nodes;
  PageDesc page{};
  ProjectDesc project{};
  explicit Fixture(NodeDesc image,int tileLines=8):
    tile1(Guard+Width*tileLines+Guard,Canary),tile2(Guard+Width*tileLines+Guard,Canary),
    view(display,tile1.data()+Guard,tile2.data()+Guard,Width*tileLines),nodes{image}{
    require(display.beginChecked()==Status::Ok,"memory display failed to initialize");
    page={"/",nodes.data(),uint16_t(nodes.size()),Width,Height,Background,nullptr,nullptr};
    project={&page,1,0,ResourceVersion,{TransitionEffect::None,AnimationEasing::Linear,0}};
    require(view.mount(project)==Status::Ok,"test project failed to mount");
  }
  void flush(uint32_t now=1){
    for(int n=0;n<3000;++n){
      const Status status=view.tick(now);
      require(status==Status::Ok||status==Status::Busy,"runtime refresh failed");
      if(!view.refreshing()&&!view.dirty()){safe();return;}
    }
    require(false,"runtime refresh failed to settle");
  }
  void safe() const{
    transport.safe();
    for(const auto *tile:{&tile1,&tile2}){
      require(std::all_of(tile->begin(),tile->begin()+Guard,[](uint16_t c){return c==Canary;}),"leading tile guard overwritten");
      require(std::all_of(tile->end()-Guard,tile->end(),[](uint16_t c){return c==Canary;}),"trailing tile guard overwritten");
    }
  }
};


bool same(Rect a,Rect b){return a.x==b.x&&a.y==b.y&&a.width==b.width&&a.height==b.height;}
NodeDesc leaf(Kind kind){NodeDesc n{};n.parent=NoParent;n.id=1;n.kind=kind;n.rect={45,36,16,24};n.flags=Transparent;n.style.color=ImageColor;
  n.animation={AnimationEffect::None,AnimationEasing::Linear,AnimationTrigger::Manual,1000,0,1,0};return n;}
void defaultsAndValidation(){
  const MotionKeyframe frames[]={{0,0,0,1024,0,1024,255},{1024,0,0,1024,90,1024,255}};
  MotionDesc m={frames,2,0};const auto a=leaf(Kind::Media).animation;
  require(m.pivotX==50&&m.pivotY==50,"old C++ aggregates must retain center defaults");
  for(int16_t p:{-1000,0,50,100,1000}){m.pivotX=p;m.pivotY=p;require(validMotion(a,&m),"valid external pivot rejected");}
  m.pivotX=1001;require(!validMotion(a,&m),"out-of-range pivot accepted");m.pivotX=0;m.pivotY=-1001;require(!validMotion(a,&m),"invalid negative pivot accepted");
  const MotionKeyframe neutral[]={{0,0,0,1024,0,1024,255},{1024,0,0,1024,0,1024,255}};
  m={neutral,2,0,50,50};NodeDesc n=leaf(Kind::Media);n.motion=&m;Fixture invalid(n);
  m.pivotX=1001;require(invalid.view.mount(invalid.project)==Status::InvalidArgument,"mount accepted invalid pivot");
  m.pivotX=0;invalid.nodes[0].kind=Kind::Container;require(invalid.view.mount(invalid.project)==Status::InvalidArgument,"container accepted a nondefault transform pivot");
  const Rect box={-5,7,33,22};
  for(uint16_t scale:{128,512,1024,4096})for(int16_t angle:{-360,-93,0,23,90,180,359}){
    const motion_geometry::Inverse inv(scale,angle);const int32_t c=motion_geometry::sine(angle+90)*4096/scale,s=motion_geometry::sine(angle)*4096/scale;
    for(int y=-10;y<60;++y)for(int x=-40;x<70;++x){
      int64_t u,v;inv.point(box,x,y,u,v);const int64_t dx=int64_t(x)*2+1-(int64_t(box.x)*2+box.width),dy=int64_t(y)*2+1-(int64_t(box.y)*2+box.height);
      require(u==(int64_t(box.width)<<15)+motion_geometry::floorDivide(dx*c+dy*s,2)&&v==(int64_t(box.height)<<15)+motion_geometry::floorDivide(-dx*s+dy*c,2),"default inverse changed existing integer samples");
    }
    require(same(motion_geometry::bounds(box,scale,angle),motion_geometry::bounds(box,scale,angle,50,50,true)),"shape default bounds changed");
  }
}
void geometricBounds(){
  const Rect boxes[]={{-32768,-32768,65535,65535},{-3,5,17,29},{80,70,1,1}};
  for(Rect box:boxes)for(bool shape:{false,true})for(int16_t px:{-1000,-37,0,37,50,100,1000})for(int16_t py:{-1000,0,50,100,1000})
    for(uint16_t scale:{128,512,1024,4096})for(int16_t angle:{-90,0,23,90,180,359}){
      Rect b=motion_geometry::bounds(box,scale,angle,px,py,shape);const motion_geometry::Inverse inv(scale,angle,px,py,shape);
      const long double ax=shape?((box.width-1)*px/100.L+.5L):box.width*px/100.L,ay=shape?((box.height-1)*py/100.L+.5L):box.height*py/100.L;
      const long double c=inv.xx/65536.L,s=inv.xy/65536.L,d=c*c+s*s;
      for(int corner=0;corner<4;++corner){const long double u=(corner&1?box.width:0)-ax,v=(corner&2?box.height:0)-ay;
        const long double x=box.x+ax+(c*u-s*v)/d,y=box.y+ay+(s*u+c*v)/d;
        require(x>=b.x-1e-6L&&x<=int64_t(b.x)+b.width+1e-6L&&y>=b.y-1e-6L&&y<=int64_t(b.y)+b.height+1e-6L,"transformed corner escaped dirty bounds");}
    }
}
void actualLeafRenderingAndHits(){
  const uint8_t bitmap[]={255,255,255,255,255,255,255,255};const MonoIcon icon={bitmap,8,8,8,1};
  for(int mode:{0,1,2})for(int16_t px:{-100,0,37,50,100,150})for(int16_t py:{0,50,100})for(uint16_t scale:{512,1024,1536})for(int16_t angle:{-90,23,90,180}){
    NodeDesc n=leaf(mode==2?Kind::Icon:Kind::Media);n.icon=mode==2?&icon:nullptr;
    if(mode==1){n.style.paddingLeft=2;n.style.paddingRight=3;n.style.paddingTop=1;n.style.paddingBottom=4;}
    std::vector<uint8_t> solid((n.rect.width-n.style.paddingLeft-n.style.paddingRight)*(n.rect.height-n.style.paddingTop-n.style.paddingBottom)*2);for(size_t j=0;j<solid.size();j+=2){solid[j]=0xf8;solid[j+1]=0x1f;}
    const MediaDesc sizedMedia={solid.data(),uint16_t(n.rect.width-n.style.paddingLeft-n.style.paddingRight),uint16_t(n.rect.height-n.style.paddingTop-n.style.paddingBottom),1,1,2};n.media=mode==2?nullptr:&sizedMedia;
    const MotionKeyframe f[]={{0,0,0,scale,angle,1024,255},{1024,0,0,scale,angle,1024,255}};const MotionDesc motion={f,2,0,px,py};n.motion=&motion;
    Fixture tiled(n,3),whole(n,Height);require(tiled.view.playAnimation(1,1)&&whole.view.playAnimation(1,1),"pivot animation did not play");tiled.flush();whole.flush();
    require(tiled.transport.pixels()==whole.transport.pixels(),"pivot changes with tile height");
    const auto b=tiled.view.bounds(1);require(same(b,motion_geometry::bounds(n.rect,scale,angle,px,py)),"public bounds lost configured pivot");
    const long double ax=n.rect.width*px/100.L,ay=n.rect.height*py/100.L;
    const long double c=motion_geometry::sine(angle+90)*4096/scale/65536.L,s=motion_geometry::sine(angle)*4096/scale/65536.L;
    int hitX=-1,hitY=-1;
    for(int y=0;y<Height;++y)for(int x=0;x<Width;++x){
      const long double dx=x+.5L-n.rect.x-ax,dy=y+.5L-n.rect.y-ay,u=ax+c*dx+s*dy,v=ay-s*dx+c*dy;
      const int left=n.style.paddingLeft,top=n.style.paddingTop,right=mode==2?8:n.rect.width-n.style.paddingRight,bottom=mode==2?8:n.rect.height-n.style.paddingBottom;
      const bool inside=u>=left&&u<right&&v>=top&&v<bottom;
      if(tiled.transport.at(x,y)!=(inside?ImageColor:Background)){int64_t iu,iv;motion_geometry::Inverse(scale,angle,px,py).point(n.rect,x,y,iu,iv);std::fprintf(stderr,"bounds=%d,%d %dx%d fixed=%lld,%lld\n",b.x,b.y,b.width,b.height,(long long)iu,(long long)iv);std::fprintf(stderr,"mode=%d pivot=%d,%d scale=%d angle=%d xy=%d,%d uv=%.12f,%.12f got=%x expected=%d\n",mode,px,py,scale,angle,x,y,double(u),double(v),tiled.transport.at(x,y),inside);require(false,"real Media/Icon rendering differs from independent pivot specification");}
      if(inside){require(x>=b.x&&x<int64_t(b.x)+b.width&&y>=b.y&&y<int64_t(b.y)+b.height,"paint escaped dirty bounds");hitX=x;hitY=y;}
    }
    int clicks=0;tiled.view.on(1,EventType::Click,[](void *p,Event &){++*static_cast<int*>(p);},&clicks);
    if(hitX>=0){tiled.view.pointerDown(hitX,hitY,2);tiled.view.pointerUp(hitX,hitY,40);require(clicks==1,"pointer hit did not follow configured pivot");}
    tiled.view.pointerDown(0,0,50);tiled.view.pointerUp(0,0,90);
  }
}
void patternedMedia(){
  const uint16_t colors[]={0xf800,0x07e0,0x001f,0xffff,0xffe0,0xf81f,0x07ff,0x4208,0x8410,0xfc00,0x03f0,0x8010};
  uint8_t pixels[24];for(int i=0;i<12;++i){pixels[i*2]=uint8_t(colors[i]>>8);pixels[i*2+1]=uint8_t(colors[i]);}
  const MediaDesc media={pixels,4,3,1,1,2};
  uint16_t palette[256]={};const GradientDesc unusedGradient={palette,0,0,0,0};
  for(int16_t angle:{23,90,180}){
    const MotionKeyframe frames[]={{0,0,0,1024,angle,1024,255},{1024,0,0,1024,angle,1024,255}};
    const MotionDesc motion={frames,2,0,0,100};NodeDesc n=leaf(Kind::Media);n.media=&media;n.motion=&motion;
    Fixture optimized(n,3),whole(n,Height);n.style.gradient=&unusedGradient;Fixture generic(n,5);
    require(optimized.view.playAnimation(1,1)&&whole.view.playAnimation(1,1)&&generic.view.playAnimation(1,1),"patterned media did not start");optimized.flush();whole.flush();generic.flush();
    require(optimized.transport.pixels()==generic.transport.pixels(),"optimized media source coordinates differ from generic sampling");
    require(optimized.transport.pixels()==whole.transport.pixels(),"patterned media source coordinates depend on tile height");
    if(angle==90)for(int y=0;y<Height;++y)for(int x=0;x<Width;++x){
      // A clockwise quarter-turn about the bottom-left box edge. The 4x3
      // source expands to 16x24, making this independent texel map exact.
      const double u=y+.5-60,v=24-(x+.5-45);uint16_t expected=Background;
      if(u>=0&&u<16&&v>=0&&v<24)expected=colors[int(v)/8*4+int(u)/4];
      require(optimized.transport.at(x,y)==expected,"off-center media changed texel orientation or anchoring");
    }
  }
}
void shapeEndpointAndDirtyCleanup(){
  const ShapePoint points[]={{50,100},{20,0},{80,0}};const ShapeDesc shape={ShapeType::Polygon,ImageColor,0,0,ShapeFill,3,100,100,points};
  const Rect r={50,20,11,31};
  for(uint16_t scale:{128,512,1024,4096})for(int16_t angle:{-90,0,37,90,180}){
    const shape_raster::TransformedShape transformed(shape,r,r,scale,angle,0,50,100);
    require(std::abs(transformed.points[0].x-55)<.0001f&&std::abs(transformed.points[0].y-50)<.0001f,"bottom endpoint moved away from configured pivot");
  }
  for(Kind kind:{Kind::Media,Kind::Shape}){
    const uint8_t p[]={0xf8,0x1f};const MediaDesc media={p,1,1,1,1,2};NodeDesc n=leaf(kind);n.rect=r;n.media=kind==Kind::Media?&media:nullptr;n.shape=kind==Kind::Shape?&shape:nullptr;
    const MotionKeyframe frames[]={{0,0,0,1024,0,1024,255},{1024,0,0,1024,180,1024,255}};const MotionDesc motion={frames,2,0,50,100};n.motion=&motion;
    Fixture animated(n,3);require(animated.view.playAnimation(1,1),"endpoint animation did not start");animated.flush(1);animated.flush(501);animated.flush(1001);
    const MotionKeyframe finalFrames[]={{0,0,0,1024,180,1024,255},{1024,0,0,1024,180,1024,255}};const MotionDesc finalMotion={finalFrames,2,0,50,100};n.motion=&finalMotion;
    Fixture clean(n,Height);require(clean.view.playAnimation(1,1),"reference endpoint animation did not start");clean.flush(1);
    require(animated.transport.pixels()==clean.transport.pixels(),"animation left stale pixels outside new pivot bounds");
  }
}
} // namespace
int main(){defaultsAndValidation();geometricBounds();actualLeafRenderingAndHits();patternedMedia();shapeEndpointAndDirtyCleanup();std::printf("{\"ok\":true,\"checks\":%d,\"realView\":true,\"pivotBounds\":true,\"dirtyCleanup\":true}\n",checks);}
