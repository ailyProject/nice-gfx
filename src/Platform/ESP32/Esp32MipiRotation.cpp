#include "Esp32MipiRotation.h"
#if defined(CONFIG_IDF_TARGET_ESP32P4) && SOC_MIPI_DSI_SUPPORTED && SOC_PPA_SUPPORTED
#include <esp_heap_caps.h>
#include <esp_cache.h>
#include <esp_timer.h>
#include <esp_memory_utils.h>
#include <algorithm>
namespace nice_gfx {
Status Esp32MipiRotation::begin(){
  if(landscape_)return Status::Ok;
  if(rotation_<1||rotation_>3)return Status::Unsupported;
  if(!nativeCopy2d_.configure(NICE_GFX_P4_DMA2D_COPY!=0,NICE_GFX_P4_DMA2D_COPY_ODD_ROI!=0))return Status::Busy;
  const Status s=Esp32Mipi::begin();if(s!=Status::Ok)return s;
  if(!rgbFramebuffer_||!ppaCopy_||!ppaCacheLine_)return fail(Status::Unsupported);
  NativeFramebuffer available;if(Esp32Mipi::mapNativeFramebuffer(available)!=Status::Ok)return fail(Status::Unsupported);
  const size_t bytes=nativeWidth_*nativeHeight_*2;
  if(bytes%ppaCacheLine_||nativeWidth_*2%ppaCacheLine_)return fail(Status::Unsupported);
  scanout_=rgbFramebuffer_;
  landscape_=static_cast<uint16_t*>(heap_caps_aligned_calloc(ppaCacheLine_,1,bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
  if(!landscape_){end();return Status::NoMemory;}
  rgbFramebuffer_=landscape_;
  const Status resized=reshapeRaster(rotation_&1?nativeHeight_:nativeWidth_,rotation_&1?nativeWidth_:nativeHeight_);
  if(resized!=Status::Ok){end();return resized;}
  // The base table was sized for portrait rows; logical landscape has fewer.
  if(mipiDirty_){
    if((rotation_&1)&&nativeWidth_>nativeHeight_){heap_caps_free(mipiDirty_);mipiDirty_=static_cast<MipiDirtySpan*>(heap_caps_malloc(rasterHeight()*sizeof(MipiDirtySpan),MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));}
    if(mipiDirty_)for(size_t y=0;y<rasterHeight();++y)mipiDirty_[y]={uint16_t(rasterWidth()),0};
    mipiDirtyTop_=rasterHeight();mipiDirtyBottom_=0;
  }
  dirtyCount_=0;rotations_=rotationFailures_=0;rotationMicros_=0;
  displayed_=CachedSource{};sourceDeltaBlits_=sourceDeltaPixels_=sourceRepeatSkips_=0;
  sourceComposedFrames_=0;
  landscapeCurrent_=true;mapped_=explicitFrame_=false;frameDamageCount_=0;
  damage({0,0,int32_t(rasterWidth()),int32_t(rasterHeight())});
  return present();
}
void Esp32MipiRotation::end(){
  const Status copied=nativeCopy2d_.end();
  if(copied!=Status::Ok){recordVideoResult(copied);if(nativeCopy2d_.stats().ready||nativeCopy2d_.stats().pending)return;}
  if(blend_){if(ppa_unregister_client(blend_)!=ESP_OK)return;blend_=nullptr;}
  for(auto &source:sources_){if(source.pixels)heap_caps_free(source.pixels);source=CachedSource{};}
  cacheClock_=0;displayed_=CachedSource{};
  if(landscape_)rgbFramebuffer_=scanout_;
  Esp32Mipi::end();
  if(landscape_)heap_caps_free(landscape_);
  landscape_=scanout_=nullptr;dirtyCount_=frameDamageCount_=0;
  landscapeCurrent_=true;mapped_=explicitFrame_=false;
}
void Esp32MipiRotation::damage(const DirtyRect &r){
  if(r.width<=0||r.height<=0)return;
  DirtyRect merged=r;
  for(size_t i=0;i<dirtyCount_;){const auto &d=dirty_[i];
    const int32_t left=std::min(merged.x,d.x),top=std::min(merged.y,d.y);
    const int32_t right=std::max(merged.x+merged.width,d.x+d.width),bottom=std::max(merged.y+merged.height,d.y+d.height);
    // Touching L-shaped strips may surround unchanged UI. Merge only when
    // the bounding copy costs no more than the two regions plus a tiny slack.
    // Full-width adjacent render tiles still coalesce into one rotation.
    if(uint64_t(right-left)*(bottom-top)<=merged.pixels()+d.pixels()+32){
      merged={left,top,right-left,bottom-top};
      dirty_[i]=dirty_[--dirtyCount_];i=0;
    }else ++i;
  }
  if(dirtyCount_==16){dirtyCount_=0;merged={0,0,int32_t(rasterWidth()),int32_t(rasterHeight())};}
  dirty_[dirtyCount_++]=merged;
}
Status Esp32MipiRotation::drawPixelView(int x,int y,PixelView view){
  displayed_=CachedSource{};
  const uintptr_t source=reinterpret_cast<uintptr_t>(view.data),scan=reinterpret_cast<uintptr_t>(scanout_);
  const bool ram=esp_ptr_internal(view.data)||esp_ptr_external_ram(view.data);
  // Immutable cached slide slices can go straight to the physical surface.
  // No caller source is retained. Reconstruct the logical surface lazily if
  // a later native renderer needs it, rather than copying every slice twice.
  if(!mapped_&&!dirtyCount_&&ram&&view.stride<0x4000&&view.height<0x4000&&
     !(source>=scan&&source<scan+nativeWidth_*nativeHeight_*2)){
    const Status s=rotateView(x,y,view);if(s==Status::Ok)landscapeCurrent_=false;return s;
  }
  const Status ready=materialize();if(ready!=Status::Ok)return ready;
  const Status s=Esp32Mipi::drawPixelView(x,y,view);
  if(s==Status::Ok)damage({x,y,int32_t(view.width),int32_t(view.height)});return s;
}
Status Esp32MipiRotation::drawFillRect(int x,int y,size_t w,size_t h,uint16_t color){
  displayed_=CachedSource{};
  const Status ready=materialize();if(ready!=Status::Ok)return ready;
  const Status s=Esp32Mipi::drawFillRect(x,y,w,h,color);
  if(s==Status::Ok)damage({x,y,int32_t(w),int32_t(h)});return s;
}
Status Esp32MipiRotation::beginFrame(const DirtyRect *regions,size_t count){
  const Status s=Esp32Mipi::beginFrame(regions,count);if(s!=Status::Ok)return s;
  explicitFrame_=true;frameDamageCount_=std::min(count,size_t(16));
  if(count>16){frameDamageCount_=1;frameDamage_[0]={0,0,int32_t(rasterWidth()),int32_t(rasterHeight())};}
  else for(size_t i=0;i<frameDamageCount_;++i)frameDamage_[i]=regions[i];
  if(mapped_)for(size_t i=0;i<frameDamageCount_;++i)damage(frameDamage_[i]);return s;
}
Status Esp32MipiRotation::mapNativeFramebuffer(NativeFramebuffer &frame){
  displayed_=CachedSource{};
  const Status s=materialize();if(s!=Status::Ok)return s;
  const Status mapped=Esp32Mipi::mapNativeFramebuffer(frame);
  if(mapped==Status::Ok){mapped_=true;if(explicitFrame_)for(size_t i=0;i<frameDamageCount_;++i)damage(frameDamage_[i]);}
  return mapped;
}
Status Esp32MipiRotation::copySurfaceRows(const NativeFramebuffer &frame,int32_t y,PixelView source){
  const uintptr_t target=reinterpret_cast<uintptr_t>(frame.pixels);const size_t bytes=frame.bytes,owned=nativeWidth_*nativeHeight_*2;
  if(!target||target>UINTPTR_MAX-bytes)return Status::Unsupported;
  const auto overlaps=[&](const void *pixels){const uintptr_t p=reinterpret_cast<uintptr_t>(pixels);return p&&target<p+owned&&p<target+bytes;};
  // Neither the physical scanout nor rotated retained sources may be changed
  // through a copy which intentionally leaves presentation metadata untouched.
  if(overlaps(scanout_)||overlaps(landscape_))return Status::Unsupported;
  for(const auto &entry:sources_)if(overlaps(entry.pixels))return Status::Unsupported;
  return Esp32Mipi::copySurfaceRows(frame,y,source);
}
Status Esp32MipiRotation::present(){
  const Status s=Esp32Mipi::present();if(s!=Status::Ok||explicitFrame_)return s;
  const Status shown=publish();if(shown==Status::Ok)mapped_=false;return shown;
}
Status Esp32MipiRotation::commitFrame(){
  const Status s=Esp32Mipi::commitFrame();explicitFrame_=false;
  const Status shown=s==Status::Ok?publish():s;if(shown==Status::Ok)mapped_=false;return shown;
}
Status Esp32MipiRotation::abortFrame(){
  const Status s=Esp32Mipi::abortFrame();explicitFrame_=false;
  const Status shown=s==Status::Ok?publish():s;if(shown==Status::Ok)mapped_=false;return shown;
}
Status Esp32MipiRotation::rotateView(int x,int y,PixelView view){
  displayed_=CachedSource{};
  if(!landscape_||!scanout_||!ppaCopy_)return Status::NotInitialized;
  const int64_t started=esp_timer_get_time();
    ppa_srm_oper_config_t op={};
    op.in.buffer=view.data;op.in.pic_w=view.stride;op.in.pic_h=view.height;
    const uintptr_t source=reinterpret_cast<uintptr_t>(view.data),base=reinterpret_cast<uintptr_t>(landscape_);
    if(source>=base&&source-base<rasterWidth()*rasterHeight()*2&&view.stride==rasterWidth()&&!((source-base)&1)){
      const size_t pixel=(source-base)/2,sx=pixel%rasterWidth(),sy=pixel/rasterWidth();
      if(view.width<=rasterWidth()-sx&&view.height<=rasterHeight()-sy){op.in.buffer=landscape_;op.in.pic_w=rasterWidth();op.in.pic_h=rasterHeight();op.in.block_offset_x=sx;op.in.block_offset_y=sy;}
    }
    op.in.block_w=view.width;op.in.block_h=view.height;op.in.srm_cm=PPA_SRM_COLOR_MODE_RGB565;
    op.out.buffer=scanout_;op.out.buffer_size=nativeWidth_*nativeHeight_*2;op.out.pic_w=nativeWidth_;op.out.pic_h=nativeHeight_;
    op.out.srm_cm=PPA_SRM_COLOR_MODE_RGB565;
    if(rotation_==1){op.rotation_angle=PPA_SRM_ROTATION_ANGLE_270;op.out.block_offset_x=nativeWidth_-(y+view.height);op.out.block_offset_y=x;}
    else if(rotation_==2){op.rotation_angle=PPA_SRM_ROTATION_ANGLE_180;op.out.block_offset_x=nativeWidth_-(x+view.width);op.out.block_offset_y=nativeHeight_-(y+view.height);}
    else{op.rotation_angle=PPA_SRM_ROTATION_ANGLE_90;op.out.block_offset_x=y;op.out.block_offset_y=nativeHeight_-(x+view.width);}
    op.scale_x=op.scale_y=1;op.byte_swap=view.order==PixelOrder::Wire;op.mode=PPA_TRANS_MODE_BLOCKING;
    const auto error=ppa_do_scale_rotate_mirror(ppaCopy_,&op);
    if(error!=ESP_OK){++rotationFailures_;return recordVideoResult(convert(error));}
    // SRM invalidates output rows itself; discard any speculative CPU reads.
    const size_t offset=size_t(op.out.block_offset_y)*nativeWidth_*2;
    const auto synced=esp_cache_msync(reinterpret_cast<uint8_t*>(scanout_)+offset,size_t(rotation_&1?view.width:view.height)*nativeWidth_*2,ESP_CACHE_MSYNC_FLAG_DIR_M2C);
    if(synced!=ESP_OK){++rotationFailures_;return recordVideoResult(convert(synced));}
    ++rotations_;rotationMicros_+=uint64_t(esp_timer_get_time()-started);return recordVideoResult(Status::Ok);
}
Status Esp32MipiRotation::publish(){
  for(size_t i=0;i<dirtyCount_;++i){const auto &r=dirty_[i];
    const Status s=rotateView(r.x,r.y,PixelView(landscape_+size_t(r.y)*rasterWidth()+r.x,r.width,r.height,rasterWidth(),PixelOrder::Native));
    if(s!=Status::Ok)return s;
  }
  dirtyCount_=0;return recordVideoResult(Status::Ok);
}
Status Esp32MipiRotation::materialize(){
  if(!landscape_||landscapeCurrent_)return Status::Ok;
  ppa_srm_oper_config_t op={};
  op.in.buffer=scanout_;op.in.pic_w=op.in.block_w=nativeWidth_;op.in.pic_h=op.in.block_h=nativeHeight_;op.in.srm_cm=PPA_SRM_COLOR_MODE_RGB565;
  op.out.buffer=landscape_;op.out.buffer_size=nativeWidth_*nativeHeight_*2;op.out.pic_w=rasterWidth();op.out.pic_h=rasterHeight();op.out.srm_cm=PPA_SRM_COLOR_MODE_RGB565;
  op.rotation_angle=rotation_==1?PPA_SRM_ROTATION_ANGLE_90:rotation_==2?PPA_SRM_ROTATION_ANGLE_180:PPA_SRM_ROTATION_ANGLE_270;
  op.scale_x=op.scale_y=1;op.mode=PPA_TRANS_MODE_BLOCKING;
  const auto error=ppa_do_scale_rotate_mirror(ppaCopy_,&op);if(error!=ESP_OK){++rotationFailures_;return recordVideoResult(convert(error));}
  const auto sync=esp_cache_msync(landscape_,op.out.buffer_size,ESP_CACHE_MSYNC_FLAG_DIR_M2C);
  if(sync!=ESP_OK){++rotationFailures_;return recordVideoResult(convert(sync));}
  landscapeCurrent_=true;return recordVideoResult(Status::Ok);
}

Esp32MipiRotation::CachedSource *Esp32MipiRotation::resolve(const FrameSource &source,CachedSource *keep){
  const auto &view=source.pixels;
  if(!source.owner||!source.revision||!view.valid()||uint8_t(view.order)>uint8_t(PixelOrder::Wire)||view.width!=rasterWidth()||view.height!=rasterHeight()||view.stride>=0x4000||view.height>SIZE_MAX/view.stride/2||
     (!esp_ptr_internal(view.data)&&!esp_ptr_external_ram(view.data)))return nullptr;
  const uintptr_t input=reinterpret_cast<uintptr_t>(view.data);const size_t inputBytes=view.stride*view.height*2;
  if(input>UINTPTR_MAX-inputBytes)return nullptr;
  const auto *last=reinterpret_cast<const void*>(input+inputBytes-1);
  if(esp_ptr_internal(view.data)?!esp_ptr_internal(last):!esp_ptr_external_ram(last))return nullptr;
  for(auto &slot:sources_)if(slot.pixels&&slot.source==view.data&&slot.owner==source.owner&&slot.revision==source.revision&&slot.stride==view.stride&&slot.order==view.order){slot.used=++cacheClock_;return &slot;}
  CachedSource *slot=nullptr,*oldest=nullptr,*previous=nullptr;size_t resident=0;
  for(auto &candidate:sources_){if(candidate.pixels)++resident;else if(!slot)slot=&candidate;
    if(&candidate!=keep&&candidate.pixels){if(!oldest||candidate.used<oldest->used)oldest=&candidate;
      if(candidate.source==view.data&&candidate.owner==source.owner&&candidate.stride==view.stride&&candidate.order==view.order&&(!previous||candidate.used>previous->used))previous=&candidate;}}
  const size_t bytes=nativeWidth_*nativeHeight_*2;
  if(slot&&(resident*bytes+bytes>12u*1024u*1024u||heap_caps_get_free_size(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT)<bytes+2u*1024u*1024u))slot=nullptr;
  if(previous)slot=previous;else if(!slot)slot=oldest;if(!slot)return nullptr;
  if(!slot->pixels)slot->pixels=static_cast<uint16_t*>(heap_caps_aligned_alloc(ppaCacheLine_,bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
  if(!slot->pixels)return nullptr;
  bool delta=previous==slot&&slot->revision==source.baseRevision&&source.baseRevision&&source.revision==source.baseRevision+1&&source.damageCount<=16&&(!source.damageCount||source.damage);
  if(delta)for(size_t i=0;i<source.damageCount;++i){const auto &r=source.damage[i];if(r.x<0||r.y<0||r.width<=0||r.height<=0||int64_t(r.x)+r.width>rasterWidth()||int64_t(r.y)+r.height>rasterHeight()){delta=false;break;}}
  slot->source=nullptr;
  ppa_srm_oper_config_t op={};op.in.buffer=view.data;op.in.pic_w=view.stride;op.in.pic_h=view.height;
  op.in.srm_cm=PPA_SRM_COLOR_MODE_RGB565;
  op.out.buffer=slot->pixels;op.out.buffer_size=bytes;op.out.pic_w=nativeWidth_;op.out.pic_h=nativeHeight_;op.out.srm_cm=PPA_SRM_COLOR_MODE_RGB565;
  op.rotation_angle=rotation_==1?PPA_SRM_ROTATION_ANGLE_270:rotation_==2?PPA_SRM_ROTATION_ANGLE_180:PPA_SRM_ROTATION_ANGLE_90;
  op.scale_x=op.scale_y=1;op.byte_swap=view.order==PixelOrder::Wire;op.mode=PPA_TRANS_MODE_BLOCKING;
  const DirtyRect full(0,0,view.width,view.height);const size_t count=delta?source.damageCount:1;
  for(size_t i=0;i<count;++i){const auto &r=delta?source.damage[i]:full;
    op.in.block_offset_x=r.x;op.in.block_offset_y=r.y;op.in.block_w=r.width;op.in.block_h=r.height;
    op.out.block_offset_x=rotation_==1?int(nativeWidth_)-r.y-r.height:rotation_==2?int(nativeWidth_)-r.x-r.width:r.y;
    op.out.block_offset_y=rotation_==1?r.x:rotation_==2?int(nativeHeight_)-r.y-r.height:int(nativeHeight_)-r.x-r.width;
    const size_t offset=size_t(op.out.block_offset_y)*nativeWidth_,rows=rotation_&1?r.width:r.height;
    if(ppa_do_scale_rotate_mirror(ppaCopy_,&op)!=ESP_OK||esp_cache_msync(slot->pixels+offset,rows*nativeWidth_*2,ESP_CACHE_MSYNC_FLAG_DIR_M2C)!=ESP_OK){++rotationFailures_;if(copyStats_)++copyStats_->ppaFailures;return nullptr;}
  }
  slot->source=view.data;slot->owner=source.owner;slot->revision=source.revision;slot->stride=view.stride;slot->order=view.order;slot->used=++cacheClock_;
  return slot;
}
Status Esp32MipiRotation::cacheFrameSource(const FrameSource &source){return landscape_&&resolve(source)?Status::Ok:Status::Unsupported;}
Status Esp32MipiRotation::composeOpaqueFrame(const OpaqueFrameComposition &composition){
  if(!landscape_||!scanout_||!ppaCopy_||!explicitFrame_||mapped_||dirtyCount_||busy()||
     rotation_<1||rotation_>3||!nativeWidth_||!nativeHeight_||
     nativeWidth_>=0x2000||nativeHeight_>=0x4000||nativeHeight_>SIZE_MAX/nativeWidth_/2)
    return Status::Unsupported;
  const size_t width=rasterWidth(),height=rasterHeight(),bytes=nativeWidth_*nativeHeight_*2;
  const auto &area=composition.foregroundArea;
  if(!width||!height||width>=0x4000||height>=0x4000||
     area.x<0||area.y<0||area.width<=0||area.height<=0||composition.sourceX<0||composition.sourceY<0||
     int64_t(area.x)+area.width>int64_t(width)||int64_t(area.y)+area.height>int64_t(height)||
     int64_t(composition.sourceX)+area.width>int64_t(width)||
     int64_t(composition.sourceY)+area.height>int64_t(height))return Status::Unsupported;
  struct Range{uintptr_t address=0;size_t bytes=0;};
  const auto range=[](const void *pixels,size_t count){return Range{reinterpret_cast<uintptr_t>(pixels),count};};
  const auto valid=[](Range r){return r.address&&r.bytes&&r.address<=UINTPTR_MAX-r.bytes;};
  const auto overlaps=[](Range a,Range b){return a.address<b.address+b.bytes&&b.address<a.address+a.bytes;};
  const auto ram=[](Range r){const auto *first=reinterpret_cast<const void*>(r.address),*last=reinterpret_cast<const void*>(r.address+r.bytes-1);
    return (esp_ptr_internal(first)&&esp_ptr_internal(last))||(esp_ptr_external_ram(first)&&esp_ptr_external_ram(last));};
  const Range scan=range(scanout_,bytes),landscape=range(landscape_,bytes);
  if(!valid(scan)||!valid(landscape)||!ram(scan)||!ram(landscape)||overlaps(scan,landscape))return Status::Unsupported;
  const auto picture=[&](const FrameSource &source,Range &input){
    const auto &view=source.pixels;
    if(!source.owner||!source.revision||!view.valid()||uint8_t(view.order)>uint8_t(PixelOrder::Wire)||
       view.width!=width||view.height!=height||view.stride>=0x4000||view.height>SIZE_MAX/view.stride/2)return false;
    input=range(view.data,view.stride*view.height*2);
    if(!valid(input)||(input.address&1)||!ram(input)||overlaps(input,scan)||overlaps(input,landscape))return false;
    for(const auto &cached:sources_)if(cached.pixels){const Range owned=range(cached.pixels,bytes);
      if(!valid(owned)||overlaps(input,owned))return false;}
    return true;
  };
  Range backgroundInput,foregroundInput;
  if(!picture(composition.background,backgroundInput)||!picture(composition.foreground,foregroundInput)||
     overlaps(backgroundInput,foregroundInput))return Status::Unsupported;
  // Cache preparation may allocate or rotate private copies, but cannot write
  // visible output. The second resolve must never recycle the background slot.
  const uint32_t failures=rotationFailures_;
  auto *background=resolve(composition.background);
  if(!background)return recordVideoResult(rotationFailures_!=failures?Status::IoError:Status::Unsupported);
  auto *foreground=resolve(composition.foreground,background);
  if(!foreground)return recordVideoResult(rotationFailures_!=failures?Status::IoError:Status::Unsupported);
  const auto exact=[&](const CachedSource *cached,const FrameSource &source){
    bool owned=false;for(const auto &slot:sources_)if(&slot==cached){owned=true;break;}
    if(!owned||!cached->pixels||cached->source!=source.pixels.data||cached->owner!=source.owner||
       cached->revision!=source.revision||cached->stride!=source.pixels.stride||cached->order!=source.pixels.order)return false;
    const Range memory=range(cached->pixels,bytes);
    return valid(memory)&&ram(memory)&&!overlaps(memory,scan)&&!overlaps(memory,landscape)&&
      !overlaps(memory,backgroundInput)&&!overlaps(memory,foregroundInput);
  };
  if(background==foreground||!exact(background,composition.background)||!exact(foreground,composition.foreground)||
     overlaps(range(background->pixels,bytes),range(foreground->pixels,bytes)))return Status::Unsupported;
  const int32_t right=area.x+area.width,bottom=area.y+area.height;
  const DirtyRect strips[]={{0,0,int32_t(width),area.y},{0,bottom,int32_t(width),int32_t(height)-bottom},
    {0,area.y,area.x,area.height},{right,area.y,int32_t(width)-right,area.height}};
  bool host=false;for(const auto &strip:strips)if(strip.width>0&&strip.height>0){host=true;break;}
  if(host&&composition.backgroundKeep<255&&!blend_){
    ppa_client_config_t config={};config.oper_type=PPA_OPERATION_BLEND;config.max_pending_trans_num=1;
    if(ppa_register_client(&config,&blend_)!=ESP_OK)return Status::Unsupported;
  }
  // Both entries are now fixed exact hits. blendFrame can only resolve this
  // background hit; no later allocation or LRU replacement occurs in the batch.
  // Never describe a partially composed image as an entire presented source.
  displayed_=CachedSource{};landscapeCurrent_=false;
  for(const auto &strip:strips)if(strip.width>0&&strip.height>0){
    const Status status=blendFrame(strip,composition.background,FrameSource{},composition.backgroundKeep,0);
    if(status!=Status::Ok)return recordVideoResult(status==Status::Unsupported?Status::IoError:status);
  }
  const Status copied=copyCachedSource(area,*foreground,composition.sourceX,composition.sourceY);
  if(copied==Status::Ok)++sourceComposedFrames_;
  return recordVideoResult(copied==Status::Unsupported?Status::IoError:copied);
}
bool Esp32MipiRotation::sourcePresented(const FrameSource &s,uint64_t revision)const{
  return displayed_.owner&&s.owner==displayed_.owner&&revision==displayed_.revision&&
    s.pixels.data==displayed_.source&&s.pixels.stride==displayed_.stride&&s.pixels.order==displayed_.order&&
    s.pixels.width==rasterWidth()&&s.pixels.height==rasterHeight();
}
void Esp32MipiRotation::rememberSource(const FrameSource &s){
  displayed_=CachedSource{};
  if(s.owner&&s.revision&&s.pixels.width==rasterWidth()&&s.pixels.height==rasterHeight()){
    displayed_.source=s.pixels.data;displayed_.owner=s.owner;displayed_.revision=s.revision;
    displayed_.stride=s.pixels.stride;displayed_.order=s.pixels.order;
  }
}
Status Esp32MipiRotation::blitFrameSource(const DirtyRect &r,const FrameSource &source,int32_t sx,int32_t sy){
  if(!landscape_||mapped_||dirtyCount_||r.x<0||r.y<0||r.width<=0||r.height<=0||sx<0||sy<0||
     int64_t(r.x)+r.width>rasterWidth()||int64_t(r.y)+r.height>rasterHeight()||
     int64_t(sx)+r.width>rasterWidth()||int64_t(sy)+r.height>rasterHeight())return Status::Unsupported;
  const bool whole=!r.x&&!r.y&&!sx&&!sy&&size_t(r.width)==rasterWidth()&&size_t(r.height)==rasterHeight();
  if(whole&&sourcePresented(source,source.revision)){++sourceRepeatSkips_;return recordVideoResult(Status::Ok);}
  bool delta=whole&&source.baseRevision&&source.revision==source.baseRevision+1&&sourcePresented(source,source.baseRevision)&&
    source.damage&&source.damageCount&&source.damageCount<=16;
  uint64_t changed=0;
  if(delta)for(size_t i=0;i<source.damageCount;++i){const auto &d=source.damage[i];
    if(d.x<0||d.y<0||d.width<=0||d.height<=0||int64_t(d.x)+d.width>rasterWidth()||int64_t(d.y)+d.height>rasterHeight()){delta=false;break;}
    changed+=d.pixels();
  }
  // Many tiny operations or a nearly complete image cost more than one GDMA.
  delta=delta&&changed<r.pixels()/4;
  auto *cached=resolve(source);if(!cached)return Status::Unsupported;
  displayed_=CachedSource{};
  if(delta){
    for(size_t i=0;i<source.damageCount;++i){const auto &d=source.damage[i];const Status s=copyCachedSource(d,*cached,d.x,d.y);if(s!=Status::Ok)return s;}
    ++sourceDeltaBlits_;sourceDeltaPixels_+=changed;
  }else{const Status s=copyCachedSource(r,*cached,sx,sy);if(s!=Status::Ok)return s;}
  if(whole)rememberSource(source);return Status::Ok;
}
Status Esp32MipiRotation::copyCachedSource(const DirtyRect &r,const CachedSource &cached,int32_t sx,int32_t sy){
  ppa_srm_oper_config_t op={};op.in.buffer=cached.pixels;op.in.pic_w=nativeWidth_;op.in.pic_h=nativeHeight_;op.in.srm_cm=PPA_SRM_COLOR_MODE_RGB565;
  op.in.block_offset_x=rotation_==1?int(nativeWidth_)-sy-r.height:rotation_==2?int(nativeWidth_)-sx-r.width:sy;
  op.in.block_offset_y=rotation_==1?sx:rotation_==2?int(nativeHeight_)-sy-r.height:int(nativeHeight_)-sx-r.width;
  op.in.block_w=rotation_&1?r.height:r.width;op.in.block_h=rotation_&1?r.width:r.height;
  op.out.buffer=scanout_;op.out.buffer_size=nativeWidth_*nativeHeight_*2;op.out.pic_w=nativeWidth_;op.out.pic_h=nativeHeight_;op.out.srm_cm=PPA_SRM_COLOR_MODE_RGB565;
  op.out.block_offset_x=rotation_==1?int(nativeWidth_)-r.y-r.height:rotation_==2?int(nativeWidth_)-r.x-r.width:r.y;
  op.out.block_offset_y=rotation_==1?r.x:rotation_==2?int(nativeHeight_)-r.y-r.height:int(nativeHeight_)-r.x-r.width;
  if(!op.in.block_offset_x&&!op.out.block_offset_x&&op.in.block_w==nativeWidth_&&size_t(op.in.block_w)*op.in.block_h>=2048&&
     copyContiguousGdma(scanout_+size_t(op.out.block_offset_y)*nativeWidth_,cached.pixels+size_t(op.in.block_offset_y)*nativeWidth_,size_t(op.in.block_w)*op.in.block_h*2)){
    landscapeCurrent_=false;return recordVideoResult(Status::Ok);
  }
  // Cached pictures already use the physical native orientation. Preserve the
  // established contiguous GDMA path, then offer strided copies to the qualified
  // DMA2D owner. Unsupported means no output was touched; every accepted result
  // is fenced before a caller can release or recycle this cached source.
  if(NICE_GFX_P4_DMA2D_COPY){
    if(!nativeCopy2d_.configure(true,NICE_GFX_P4_DMA2D_COPY_ODD_ROI!=0))return recordVideoResult(Status::IoError);
    const NativeFramebuffer target{scanout_,nativeWidth_,nativeHeight_,nativeWidth_,op.out.buffer_size};
    const PixelView source(cached.pixels,nativeWidth_,nativeHeight_,nativeWidth_,PixelOrder::Native);
    Status copied=nativeCopy2d_.copyIntoNative(target,op.out.block_offset_x,op.out.block_offset_y,source,
      op.out.buffer_size,op.in.block_offset_x,op.in.block_offset_y,op.in.block_w,op.in.block_h);
    if(copied==Status::Busy||nativeCopy2d_.stats().pending){
      const Status drained=nativeCopy2d_.drain();
      if(drained!=Status::Ok)copied=drained;else if(copied==Status::Unsupported)copied=Status::IoError;
    }
    if(copied!=Status::Unsupported){landscapeCurrent_=false;return recordVideoResult(copied);}
  }
  op.rotation_angle=PPA_SRM_ROTATION_ANGLE_0;op.scale_x=op.scale_y=1;op.mode=PPA_TRANS_MODE_BLOCKING;
  const auto error=ppa_do_scale_rotate_mirror(ppaCopy_,&op);if(error!=ESP_OK)return recordVideoResult(convert(error));
  const auto synced=esp_cache_msync(scanout_+size_t(op.out.block_offset_y)*nativeWidth_,size_t(op.in.block_h)*nativeWidth_*2,ESP_CACHE_MSYNC_FLAG_DIR_M2C);
  if(synced!=ESP_OK)return recordVideoResult(convert(synced));landscapeCurrent_=false;return recordVideoResult(Status::Ok);
}
Status Esp32MipiRotation::blendFrame(const DirtyRect &r,const FrameSource &front,const FrameSource &back,uint8_t alpha,uint16_t solid){
  if(!landscape_||mapped_||dirtyCount_||r.x<0||r.y<0||r.width<=0||r.height<=0||int64_t(r.x)+r.width>rasterWidth()||int64_t(r.y)+r.height>rasterHeight())return Status::Unsupported;
  if(alpha==255)return blitFrameSource(r,front,r.x,r.y);
  if(!alpha&&back.pixels.data)return blitFrameSource(r,back,r.x,r.y);
  displayed_=CachedSource{};
  auto *fg=resolve(front);if(!fg)return Status::Unsupported;
  const bool solidBackground=!back.pixels.data;
  auto *bg=solidBackground?nullptr:resolve(back,fg);if(!solidBackground&&!bg)return Status::Unsupported;
  if(!blend_){ppa_client_config_t config={};config.oper_type=PPA_OPERATION_BLEND;config.max_pending_trans_num=1;if(ppa_register_client(&config,&blend_)!=ESP_OK)return Status::Unsupported;}
  const int x=rotation_==1?int(nativeWidth_)-r.y-r.height:rotation_==2?int(nativeWidth_)-r.x-r.width:r.y;
  const int y=rotation_==1?r.x:rotation_==2?int(nativeHeight_)-r.y-r.height:int(nativeHeight_)-r.x-r.width;
  const int w=rotation_&1?r.height:r.width,h=rotation_&1?r.width:r.height;
  ppa_blend_oper_config_t op={};
  op.in_bg.buffer=solidBackground?fg->pixels:bg->pixels;op.in_bg.pic_w=nativeWidth_;op.in_bg.pic_h=nativeHeight_;
  op.in_bg.block_offset_x=x;op.in_bg.block_offset_y=y;op.in_bg.block_w=w;op.in_bg.block_h=h;op.in_bg.blend_cm=PPA_BLEND_COLOR_MODE_RGB565;
  op.bg_alpha_update_mode=PPA_ALPHA_FIX_VALUE;op.bg_alpha_fix_val=255;
  op.in_fg.buffer=fg->pixels;op.in_fg.pic_w=nativeWidth_;op.in_fg.pic_h=nativeHeight_;
  op.in_fg.block_offset_x=x;op.in_fg.block_offset_y=y;op.in_fg.block_w=w;op.in_fg.block_h=h;
  op.in_fg.blend_cm=solidBackground?PPA_BLEND_COLOR_MODE_A8:PPA_BLEND_COLOR_MODE_RGB565;
  op.fg_alpha_update_mode=PPA_ALPHA_FIX_VALUE;op.fg_alpha_fix_val=solidBackground?255-alpha:alpha;
  if(solidBackground){
    // A8 supplies alpha only; FIX replaces it. Map its byte coordinates so
    // every descriptor read remains inside the immutable native RGB picture.
    op.in_fg.pic_w=nativeWidth_*2;op.in_fg.block_offset_x=x*2;
    op.fg_fix_rgb_val.r=uint8_t((solid>>11)<<3);op.fg_fix_rgb_val.g=uint8_t(((solid>>5)&63)<<2);op.fg_fix_rgb_val.b=uint8_t((solid&31)<<3);
  }
  op.out.buffer=scanout_;op.out.buffer_size=nativeWidth_*nativeHeight_*2;op.out.pic_w=nativeWidth_;op.out.pic_h=nativeHeight_;
  op.out.block_offset_x=x;op.out.block_offset_y=y;op.out.blend_cm=PPA_BLEND_COLOR_MODE_RGB565;op.mode=PPA_TRANS_MODE_BLOCKING;
  const auto error=ppa_do_blend(blend_,&op);if(error!=ESP_OK){if(copyStats_)++copyStats_->ppaFailures;return recordVideoResult(convert(error));}
  const auto synced=esp_cache_msync(scanout_+size_t(y)*nativeWidth_,size_t(h)*nativeWidth_*2,ESP_CACHE_MSYNC_FLAG_DIR_M2C);
  if(synced!=ESP_OK)return recordVideoResult(convert(synced));
  landscapeCurrent_=false;return recordVideoResult(Status::Ok);
}
} // namespace nice_gfx
#endif
