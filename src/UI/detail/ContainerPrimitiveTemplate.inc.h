#ifndef NICE_GFX_UI_CONTAINER_TEMPLATE_PAIR_KERNEL
#define NICE_GFX_UI_CONTAINER_TEMPLATE_PAIR_KERNEL 1
#endif
// Included inside View::Impl. Bounded isolated primitive, never a subtree.
// Captures one container primitive, never its descendants or scene occlusion.
#if NICE_GFX_UI_CONTAINER_PRIMITIVE_TEMPLATE
struct ContainerPrimitiveTemplate {
  static constexpr size_t MaxPixels=4480,Bytes=MaxPixels*sizeof(uint16_t);
  struct Key {
    int32_t width=0,height=0,clipX=0,clipY=0,clipWidth=0,clipHeight=0;
    uint16_t background=0,border=0,underlay=0;uint8_t radius=0,borderWidth=0;bool aa=false;
    bool equals(const Key &b)const{return width==b.width&&height==b.height&&clipX==b.clipX&&clipY==b.clipY&&
      clipWidth==b.clipWidth&&clipHeight==b.clipHeight&&background==b.background&&border==b.border&&
      underlay==b.underlay&&radius==b.radius&&borderWidth==b.borderWidth&&aa==b.aa;}
  } key;
  uint16_t *pixels=nullptr;bool ready=false;
  ~ContainerPrimitiveTemplate(){
    if(pixels){
      detail::freeAlignedMemory<64>(pixels);
    }
  }
  bool reserve(){if(pixels)return true;
    pixels=static_cast<uint16_t*>(detail::allocateAlignedMemory<64>(Bytes));
    return pixels!=nullptr;
  }
} containerPrimitiveTemplate;
#endif
Status paintContainerPrimitiveTemplate(int index,Rect clipped,const DirtyRect &area,uint16_t *pixels,size_t stride,uint8_t alpha){
#if NICE_GFX_UI_CONTAINER_PRIMITIVE_TEMPLATE
  // Optional total candidate cost, including refusals, uniform scan and capture.
  // Calls below counts only successful complete primitive blits.
  struct ProfileScope {
    RuntimeStats &stats;uint32_t (*clock)();uint32_t started;
    ~ProfileScope(){if(clock)stats.containerTemplateMicros+=clock()-started;}
  } profile{stats,profilingClock,profilingClock?profilingClock():0};
  const auto reject=[](){
    return Status::Unsupported;
  };
  const auto &p=*rasterFrame.current.page;const auto &values=rasterFrame.current.values;
  const auto &n=p.nodes[index];const auto &value=values[index];const auto *target=stagingNativeTarget?stagingNativeTarget:nativeOverlayTarget;
  if(!alpha||alpha==255||n.kind!=Kind::Container||value.kind!=Kind::Container||n.instrument||value.instrument||
     displayPolicy.monochrome||buildingSurface||foregroundCapture||renderOrder!=PixelOrder::Native||!target||
     index==snapshotPressed||focusStyle(index)||componentFor(index)||!behaviorVisuals.empty()||paintOffsetX||paintOffsetY||
     !canvas.targetsNativeFramebuffer(*target)||p.width>INT16_MAX||p.height>INT16_MAX)return reject();
  for(int at=index;at>=0;at=values[at].parent==NoParent?-1:values[at].parent)
    if((values[at].flags&Disabled)||p.nodes[at].motion)return reject();
  const auto &style=styleFor(p,values,index);
  if((value.flags&Transparent)||style.gradient||area.width<=0||area.height<=0||stride<size_t(area.width)||
     size_t(area.height-1)>(SIZE_MAX-size_t(area.width))/stride)return reject();
  const Rect rect=paintCache->nodes[index].rect,actual=behaviorPaintRect(index,rect);
  if(actual.x!=rect.x||actual.y!=rect.y||actual.width!=rect.width||actual.height!=rect.height||
     rect.width<=0||rect.height<=0||rect.x<0||rect.y<0||int64_t(rect.x)+rect.width>p.width||int64_t(rect.y)+rect.height>p.height||
     uint64_t(rect.width)*rect.height>ContainerPrimitiveTemplate::MaxPixels||clipped.width<=0||clipped.height<=0||
     clipped.x<rect.x||clipped.y<rect.y||int64_t(clipped.x)+clipped.width>int64_t(rect.x)+rect.width||
     int64_t(clipped.y)+clipped.height>int64_t(rect.y)+rect.height||clipped.x<area.x||clipped.y<area.y||
     int64_t(clipped.x)+clipped.width>int64_t(area.x)+area.width||int64_t(clipped.y)+clipped.height>int64_t(area.y)+area.height)return reject();
  // Do not add background pixels that the live rounded painter would omit.
  const Rect covered=backgroundCover(index,clipped);if(covered.width>0&&covered.height>0)return reject();
  const size_t available=size_t(area.height-1)*stride+size_t(area.width),offset=size_t(clipped.y-area.y)*stride+size_t(clipped.x-area.x);
  auto *destination=pixels+offset;const uint16_t background=*destination;
#if NICE_GFX_UI_CONTAINER_TEMPLATE_PAIR_KERNEL
  if(!detail::uniformContainerTemplateNative(destination,available-offset,stride,clipped.width,clipped.height,background))return Status::Unsupported;
#else
  for(int32_t y=0;y<clipped.height;++y)for(int32_t x=0;x<clipped.width;++x)if(destination[size_t(y)*stride+x]!=background){
    return Status::Unsupported;
  }
#endif
  auto &entry=containerPrimitiveTemplate;if(!entry.reserve())return reject();
  const uintptr_t input=reinterpret_cast<uintptr_t>(entry.pixels),output=reinterpret_cast<uintptr_t>(target->pixels);
  if(input>UINTPTR_MAX-ContainerPrimitiveTemplate::Bytes||output>UINTPTR_MAX-target->bytes||
     nativeMemoryOverlap(output,target->bytes,entry.pixels,ContainerPrimitiveTemplate::Bytes)){
    entry.ready=false;return reject();
  }
  const ContainerPrimitiveTemplate::Key key{rect.width,rect.height,clipped.x-rect.x,clipped.y-rect.y,clipped.width,clipped.height,
    style.background,style.borderColor,background,style.radius,style.borderWidth,(value.flags&NoAntialias)==0};
  if(!entry.ready||!entry.key.equals(key)){
    entry.ready=false;std::fill(entry.pixels,entry.pixels+size_t(rect.width)*rect.height,background);
    // Separate canvas: no live attachment, viewport, AA or target state changes.
    TileCanvas capture(int16_t(p.width),int16_t(p.height));
    if(capture.attach(DirtyRect(rect.x,rect.y,rect.width,rect.height),entry.pixels,ContainerPrimitiveTemplate::MaxPixels,size_t(rect.width),PixelOrder::Native)!=Status::Ok)return reject();
    capture.setAntialiasing(key.aa);capture.setViewport(clipped.x,clipped.y,clipped.width,clipped.height,false);
    const int32_t radius=std::min<int32_t>(style.radius,std::min(rect.width,rect.height)/2);
    capture.fillRoundRect(rect.x,rect.y,rect.width,rect.height,radius,style.background);
    for(int b=0;b<style.borderWidth&&2*b<std::min(rect.width,rect.height);++b)
      capture.drawRoundRect(rect.x+b,rect.y+b,rect.width-2*b,rect.height-2*b,std::max<int32_t>(0,radius-b),style.borderColor);
    entry.key=key;entry.ready=true;
    ++stats.containerTemplateBuilds;
  }else{
    ++stats.containerTemplateHits;
  }
  // Exact original outer-alpha rounding after the opaque primitive raster.
#if NICE_GFX_UI_CONTAINER_TEMPLATE_PAIR_KERNEL
  const size_t sourceOffset=size_t(clipped.y-rect.y)*rect.width+size_t(clipped.x-rect.x);
  if(!detail::compositeContainerTemplateNative(destination,available-offset,stride,entry.pixels+sourceOffset,ContainerPrimitiveTemplate::MaxPixels-sourceOffset,rect.width,clipped.width,clipped.height,background,alpha))return Status::Unsupported;
#else
  const rgb565_blend::SolidBackgroundLut lut(background,alpha);
  for(int32_t y=0;y<clipped.height;++y){const auto *source=entry.pixels+size_t(clipped.y-rect.y+y)*rect.width+size_t(clipped.x-rect.x);
    auto *row=destination+size_t(y)*stride;for(int32_t x=0;x<clipped.width;++x)row[x]=lut.lookupNative(source[x]);}
#endif
  ++stats.containerTemplateCalls;
  return finishOverlayPaintState(index,clipped,area,pixels,stride,alpha);
#else
  (void)index;(void)clipped;(void)area;(void)pixels;(void)stride;(void)alpha;return Status::Unsupported;
#endif
}
