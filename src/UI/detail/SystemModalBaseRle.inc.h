// Included inside View::Impl after RenderCacheCompressed.inc.h. The stream is
// looked up, proved and consumed in one synchronous mapped render call. Never
// retain this local plan, its cursor or its pointers across ticks/refreshes.
#ifndef NICE_GFX_UI_SYSTEM_MODAL_RLE_DIM
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#define NICE_GFX_UI_SYSTEM_MODAL_RLE_DIM 1
#else
#define NICE_GFX_UI_SYSTEM_MODAL_RLE_DIM 0
#endif
#endif
#ifndef NICE_GFX_UI_SYSTEM_MODAL_RLE_FILL
#define NICE_GFX_UI_SYSTEM_MODAL_RLE_FILL 0
#endif
struct SystemModalBaseRlePlan {
  const uint16_t *encoded=nullptr;size_t words=0;
  Rect textDamage[16];size_t textDamageCount=0;
  rle565::ValidatedSeekCursor localValidatedCursor;
  rle565::ValidatedSeekCursor lowerValidatedCursor;
  Rect parallelSeedRoi{};
  bool lowerValidated=false;
};
static bool modalRleSameGeometry(const Geometry &a,const Geometry &b){
  if(!modalBaseSameRect(a.rect,b.rect)||!modalBaseSameRect(a.clip,b.clip)||a.alpha!=b.alpha||
     a.visible!=b.visible||a.inputVisible!=b.inputVisible||a.children!=b.children)return false;
#if NICE_GFX_UI_SHAPE_INK_DAMAGE
  if(!modalBaseSameRect(a.paintRect,b.paintRect))return false;
#endif
  return true;
}
static bool modalRleValidLines(const FrameCache &cache,const Geometry &g){
  return !g.lineCount||(size_t(g.lineFirst)<=cache.lines.size()&&size_t(g.lineCount)<=cache.lines.size()-size_t(g.lineFirst));
}
static bool modalRleSameLines(const FrameCache &a,const Geometry &x,const FrameCache &b,const Geometry &y){
  if(x.lineCount!=y.lineCount||x.textY!=y.textY||x.ellipsisCount!=y.ellipsisCount||
     !modalRleValidLines(a,x)||!modalRleValidLines(b,y))return false;
  for(size_t line=0;line<x.lineCount;++line){const auto &left=a.lines[size_t(x.lineFirst)+line],&right=b.lines[size_t(y.lineFirst)+line];
    if(left.begin!=right.begin||left.end!=right.end||left.width!=right.width)return false;
  }
  return true;
}
bool modalRleOnlyTextChanged(const State &old,const State &current,Kind kind)const{
  if((kind!=Kind::Text&&kind!=Kind::Label)||old.parent!=current.parent||old.kind!=current.kind||
     old.styleIndex!=current.styleIndex||old.dynamic!=current.dynamic||old.implicitInput!=current.implicitInput||
     old.mediaAt!=current.mediaAt||old.mediaPending!=current.mediaPending||old.mediaPaused!=current.mediaPaused||
     old.animationAt!=current.animationAt||old.animating!=current.animating||old.animationPending!=current.animationPending||
     old.focusOverride!=current.focusOverride||old.instrument!=current.instrument||
     old.switchAt!=current.switchAt||old.switchFrom!=current.switchFrom||old.switchTo!=current.switchTo||
     old.switchPending!=current.switchPending||old.motionSlot!=current.motionSlot||!sameSurfaceGeometry(old,current))return false;
  State normalized=old;normalized.text=current.text;
  return sameSurfaceNode(normalized,current);
}
bool prepareSystemModalBaseRlePlan(const Surface &base,const NativeFramebuffer &target,SystemModalBaseRlePlan &plan,
                                   const Rect *parallelRoi=nullptr){
  plan=SystemModalBaseRlePlan{};
#if NICE_GFX_UI_SYSTEM_MODAL_RLE_DIM
  if(!project||cacheOrder!=PixelOrder::Wire||renderOrder!=PixelOrder::Native||!compressedStorage||!compressedCapacity||
     compressedCapacity>SIZE_MAX/2||systemModalBaseFrameSuppressed||systemModalBaseClosingFinal||
     buildingSurface||foregroundCapture||nativeCacheRepairing||snapshotTransition||rasterFrame.preview.page||
     !canvas.targetsNativeFramebuffer(target)||systemModalBaseSurface(systemModalBaseFrame)!=&base)return false;
  const auto &p=*base.page;const auto root=systemModalBaseFrame.root,bar=systemModalBaseFrame.bar;
  if(!p.width||!p.height||size_t(p.height)>SIZE_MAX/size_t(p.width)/2||
     !validSystemModalBase(base,root,bar,systemModalBaseFrameKinds))return false;
  const size_t pixels=size_t(p.width)*p.height,bankBytes=compressedCapacity*2;
  const uintptr_t bank=reinterpret_cast<uintptr_t>(compressedStorage),output=reinterpret_cast<uintptr_t>(target.pixels);
  size_t targetWords=0;
  if(bank%alignof(uint16_t)||bank>UINTPTR_MAX-bankBytes||output>UINTPTR_MAX-target.bytes||
     !detail::nativeWireCopyFootprint(target.pixels,target.bytes/2,target.stride,p.width,p.height,targetWords)||
     target.width!=p.width||target.height!=p.height||nativeMemoryOverlap(output,target.bytes,compressedStorage,bankBytes))return false;
  const auto *record=compressedSurfaceFor(p);
  if(!record||record->key.size()!=p.count||record->geometry.nodes.size()!=p.count||
     !record->words||(record->words&1u)||record->offset>compressedCapacity||record->words>compressedCapacity-record->offset||
     !(record->key[root].flags&Hidden))return false;
  const Rect screen{0,0,p.width,p.height};const Rect chrome=base.geometry.nodes[bar].rect;
  for(uint16_t i=0;i<p.count;++i){const auto kind=systemModalBaseFrameKinds[i];
    const auto &old=record->geometry.nodes[i],&current=base.geometry.nodes[i];
    if(!modalRleValidLines(record->geometry,old)||!modalRleValidLines(base.geometry,current))return false;
    if(kind==2){if(old.visible||current.visible)return false;continue;}
    if(!modalRleSameGeometry(old,current))return false;
    if(kind==1){
      // Only the existing bounded status-text/progress exception is allowed.
      // Its geometry stays fixed and every old/new pixel is inside the bar,
      // which paintSystemModalBase removes from every copied ROI.
      if(!modalBaseSameNode(record->key[i],base.key[i],1,p.nodes[i].kind)||
         (old.visible&&old.alpha&&!modalBaseContains(chrome,intersect(old.rect,old.clip))))return false;
      continue;
    }
    if(modalBaseSameNode(record->key[i],base.key[i],0,p.nodes[i].kind)){
      if(!modalRleSameLines(record->geometry,old,base.geometry,current))return false;
    }else{
      // Compare every node against the original record, never just the last
      // backend damage list. All accumulated telemetry edits remain covered.
      if(p.nodes[i].motion||!modalRleOnlyTextChanged(record->key[i],base.key[i],p.nodes[i].kind))return false;
      const Rect dirty=intersect(intersect(current.rect,current.clip),screen);
      if(current.visible&&current.alpha&&dirty.width>0&&dirty.height>0){
        bool duplicate=false;for(size_t at=0;at<plan.textDamageCount;++at)if(modalBaseSameRect(plan.textDamage[at],dirty)){duplicate=true;break;}
        if(!duplicate){if(plan.textDamageCount==16)return false;plan.textDamage[plan.textDamageCount++]=dirty;}
      }
    }
    if(componentCount&&p.nodes[i].kind==Kind::Button){
      const auto before=componentPaint(p,record->key,p.nodes[i].id),currentPaint=componentPaint(p,base.key,p.nodes[i].id);
      if(before.selected!=currentPaint.selected||before.expanded!=currentPaint.expanded)return false;
    }
  }
  const uint16_t *encoded=compressedStorage+record->offset;
  // Recheck the actual bank on every draw. A previous successful read cannot
  // establish structural validity after a backend has mapped that storage.
#if NICE_GFX_UI_SYSTEM_MODAL_RLE_VALIDATED_SEEK
  const auto &panel=paintCache->nodes[root].rect;
  const size_t seek=size_t(std::clamp<int64_t>(int64_t(panel.y)+panel.height,0,p.height))*p.width;
  const bool parallelCandidate=parallelRoi&&parallelRoi->x==0&&parallelRoi->y>=0&&
    parallelRoi->width==p.width&&parallelRoi->height>=128&&
    int64_t(parallelRoi->y)+parallelRoi->height<=p.height&&
    target.stride==p.width&&!(p.width%32)&&!(output&63u);
  const size_t lowerSeek=parallelCandidate?
    (size_t(parallelRoi->y)+size_t(parallelRoi->height/2+parallelRoi->height%2))*p.width:0;
  const size_t roiBegin=parallelCandidate?size_t(parallelRoi->y)*p.width:0;
  if(parallelCandidate&&seek<=roiBegin){
    if(!rle565::validateAndSeek(encoded,record->words,pixels,seek,plan.localValidatedCursor,
                              lowerSeek,&plan.lowerValidatedCursor))return false;
    plan.parallelSeedRoi=*parallelRoi;plan.lowerValidated=true;
  }else if(!rle565::validateAndSeek(encoded,record->words,pixels,seek,plan.localValidatedCursor))return false;
#else
  if(!rle565::validate(encoded,record->words,pixels))return false;
#endif
  record->verifiedPixels=pixels;
  plan.encoded=encoded;plan.words=record->words;return true;
#else
  (void)base;(void)target;(void)parallelRoi;return false;
#endif
}
// Both passes visit only eligible RLE pixels. A dirty text rectangle is
// repaired from the current raw source after the optional complete ROI fill.
// Weighted cancellation produces the only possible majority candidate. The
// second pass counts it exactly before the 60-percent threshold is applied.
template<bool CountCandidate>
#if defined(__GNUC__)
__attribute__((always_inline))
#endif
static inline size_t countSystemModalRleColor(const SystemModalBaseRlePlan &plan,size_t width,const Rect &roi,
                                           uint16_t &candidate,size_t &weight,size_t &eligible){
  size_t streamIndex=0,position=0,runRemaining=0;uint16_t wireColor=0;
  weight=eligible=0;
  const int32_t right=roi.x+roi.width,bottom=roi.y+roi.height;
  for(int32_t y=roi.y;y<bottom;++y){int32_t x=roi.x;
    while(x<right){int32_t coveredEnd=x,clearEnd=right;
      for(size_t at=0;at<plan.textDamageCount;++at){const auto &box=plan.textDamage[at];
        if(y<box.y||y>=box.y+box.height)continue;
        const int32_t end=box.x+box.width;
        if(box.x<=x){if(end>x&&end>coveredEnd)coveredEnd=std::min(end,right);}
        else if(box.x<clearEnd)clearEnd=box.x;
      }
      if(coveredEnd>x){x=coveredEnd;continue;}
      size_t skip=size_t(y)*width+size_t(x)-position;
      while(skip){
        if(!runRemaining){runRemaining=plan.encoded[streamIndex];wireColor=plan.encoded[streamIndex+1];streamIndex+=2;}
        const size_t count=std::min(skip,runRemaining);
        skip-=count;runRemaining-=count;position+=count;
      }
      size_t remaining=size_t(clearEnd-x);
      while(remaining){
        if(!runRemaining){runRemaining=plan.encoded[streamIndex];wireColor=plan.encoded[streamIndex+1];streamIndex+=2;}
        const size_t count=std::min(remaining,runRemaining);eligible+=count;
        if(CountCandidate){if(wireColor==candidate)weight+=count;}
        else if(!weight){candidate=wireColor;weight=count;}
        else if(wireColor==candidate)weight+=count;
        else if(count<=weight)weight-=count;
        else {candidate=wireColor;weight=count-weight;}
        remaining-=count;runRemaining-=count;position+=count;
      }
      x=clearEnd;
    }
  }
  return streamIndex;
}
// The mapped target and the complete immutable stream have already passed
// their footprint/alias checks. Each span is positive and bounded by the ROI.
#if defined(__GNUC__)
__attribute__((always_inline))
#endif
static inline void writeSystemModalNativeRun(uint16_t *output,size_t count,uint16_t color){
  if(count==1){*output=color;return;}
#if defined(__GNUC__)
  typedef uint32_t ModalRunWord __attribute__((__may_alias__));
  if(reinterpret_cast<uintptr_t>(output)&3u){*output++=color;--count;}
  auto *pairs=reinterpret_cast<ModalRunWord*>(output);
  const uint32_t pair=uint32_t(color)|(uint32_t(color)<<16);
  while(count>=16){
    pairs[0]=pair;pairs[1]=pair;pairs[2]=pair;pairs[3]=pair;
    pairs[4]=pair;pairs[5]=pair;pairs[6]=pair;pairs[7]=pair;
    pairs+=8;count-=16;
  }
  while(count>=2){*pairs++=pair;count-=2;}
  if(count)*reinterpret_cast<uint16_t*>(pairs)=color;
#else
  while(count--)*output++=color;
#endif
}
// The caller prevalidated the complete immutable stream and all source/target
// footprints. This function never publishes or advances any UI/animation clock.
// Raw telemetry damage and RLE spans form a disjoint per-row union.
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("O3")))
#endif
Status copyDimSystemModalRleRows(const Surface &base,const NativeFramebuffer &target,const Rect &roi,
                              const SystemModalBaseRlePlan &plan,uint8_t dim){
#if NICE_GFX_UI_SYSTEM_MODAL_RLE_DIM
  const uint32_t restoreStarted=profilingClock?profilingClock():0;
  if(!plan.encoded||!plan.words||!base.page||cacheOrder!=PixelOrder::Wire||!base.pixels||
     plan.textDamageCount>16||roi.x<0||roi.y<0||roi.width<=0||roi.height<=0||
     int64_t(roi.x)+roi.width>base.page->width||int64_t(roi.y)+roi.height>base.page->height)return Status::Unsupported;
  const size_t width=base.page->width,height=base.page->height;size_t targetWords=0,sourceWords=0;
  if(!width||!height||height>SIZE_MAX/width/2||target.width!=width||target.height!=height||
     !detail::nativeWireCopyFootprint(target.pixels,target.bytes/2,target.stride,width,height,targetWords)||
     !detail::nativeWireCopyFootprint(base.pixels,width*height,width,width,height,sourceWords))return Status::Unsupported;
  const uintptr_t outputAddress=reinterpret_cast<uintptr_t>(target.pixels),encodedAddress=reinterpret_cast<uintptr_t>(plan.encoded),bankAddress=reinterpret_cast<uintptr_t>(compressedStorage);
  if(!compressedStorage||!compressedCapacity||compressedCapacity>SIZE_MAX/2||bankAddress>UINTPTR_MAX-compressedCapacity*2||
     outputAddress>UINTPTR_MAX-target.bytes||encodedAddress<bankAddress||encodedAddress%alignof(uint16_t)||
     encodedAddress-bankAddress>compressedCapacity*2||plan.words>(compressedCapacity*2-(encodedAddress-bankAddress))/2||
     nativeMemoryOverlap(outputAddress,target.bytes,compressedStorage,compressedCapacity*2)||
     nativeMemoryOverlap(outputAddress,target.bytes,base.pixels,width*height*2))return Status::Unsupported;
  for(size_t at=0;at<plan.textDamageCount;++at){const auto &box=plan.textDamage[at];
    if(box.x<0||box.y<0||box.width<=0||box.height<=0||int64_t(box.x)+box.width>int64_t(width)||int64_t(box.y)+box.height>int64_t(height))return Status::Unsupported;
  }
  const detail::BackdropFloorLut lut(dim);
  bool filledDominant=false;uint16_t dominantWireColor=0;size_t selectionWords=0;
#if NICE_GFX_UI_SYSTEM_MODAL_RLE_FILL
#if defined(NICE_GFX_UI_SYSTEM_MODAL_RLE_TEST_FILL)
  const bool fillAvailable=true;
#else
  const bool fillAvailable=rgb565_ppa::Context::available();
#endif
  if(fillAvailable&&ppaBlendEnabled&&renderOrder==PixelOrder::Native&&roi.x==0&&size_t(roi.width)==width&&
     size_t(roi.width)*size_t(roi.height)>=16384&&nativeOverlayTarget==&target&&canvas.targetsNativeFramebuffer(target)&&
     !buildingSurface&&!foregroundCapture&&!nativeCacheRepairing&&!snapshotTransition&&!rasterFrame.preview.page&&
     !renderingViewport&&!stagingNativeTarget&&!buildingColdFadeStage){
    size_t weight=0,eligible=0;
    selectionWords+=countSystemModalRleColor<false>(plan,width,roi,dominantWireColor,weight,eligible);
    selectionWords+=countSystemModalRleColor<true>(plan,width,roi,dominantWireColor,weight,eligible);
    if(eligible&&uint64_t(weight)*5>=uint64_t(eligible)*3){
      const uint16_t native=rgb565_blend::fromWire(dominantWireColor);
      const uint16_t color=uint16_t(lut.red[native>>11]|lut.green[(native>>5)&63]|lut.blue[native&31]);
      const DirtyRect fillRect{roi.x,roi.y,roi.width,roi.height};
      const uint32_t fillStarted=profilingClock?profilingClock():0;
#if defined(NICE_GFX_UI_SYSTEM_MODAL_RLE_TEST_FILL)
      const Status fillStatus=NICE_GFX_UI_SYSTEM_MODAL_RLE_TEST_FILL(target,fillRect,color);
#else
      const Status fillStatus=ppaFade.fillIntoNative(target,fillRect,color);
#endif
      if(fillStatus==Status::Ok){
        ++stats.ppaFillCalls;stats.ppaFillPixels+=uint64_t(roi.width)*roi.height;
        if(profilingClock)stats.ppaFillMicros+=profilingClock()-fillStarted;
        filledDominant=true;
      }else if(fillStatus!=Status::Unsupported){++stats.ppaFillFailures;return fillStatus;}
    }
  }
#endif
  size_t streamWords=0,drawnRaw=0;
#ifdef NICE_GFX_UI_SYSTEM_MODAL_RLE_TEST_OBSERVER
  size_t drawnRuns=0;
#endif
#if NICE_GFX_UI_SYSTEM_MODAL_RLE_WORKER
  if(!filledDominant){
    const detail::SystemModalRleRowsInput input{plan.encoded,base.pixels,target.pixels,width,target.stride,
      roi,plan.textDamage,plan.textDamageCount,&lut};
    detail::SystemModalRleRowsResult rows;
    // The independent device sample at 96 host rows favored serial. Keep
    // fewer than 128 rows on UI1 and publish only the larger eligible ROIs.
    const auto *lowerSeed=plan.lowerValidated&&modalBaseSameRect(plan.parallelSeedRoi,roi)?
      &plan.lowerValidatedCursor:nullptr;
    const bool parallel=roi.height>=128&&detail::tryDrawSystemModalRleParallel(input,false,rows,&plan.localValidatedCursor,lowerSeed);
    if(!parallel)rows=detail::drawSystemModalRleRows(input,&plan.localValidatedCursor);
    streamWords=rows.cursorWords;drawnRaw=rows.rawPixels;
#ifdef NICE_GFX_UI_SYSTEM_MODAL_RLE_TEST_OBSERVER
    drawnRuns=rows.drawnRuns;
#endif
  }else
#endif
  {
    auto cursor=plan.localValidatedCursor;
    if(size_t(roi.y)*width+size_t(roi.x)<cursor.position)cursor={};
    size_t streamIndex=cursor.index,position=cursor.position,runRemaining=cursor.remaining;
    const size_t initialIndex=streamIndex;
    uint16_t wireColor=cursor.color,previousWireColor=0,nativeColor=0;
    bool colorCached=false;
    const int32_t right=roi.x+roi.width,bottom=roi.y+roi.height;
    for(int32_t y=roi.y;y<bottom;++y){int32_t x=roi.x;
      while(x<right){int32_t coveredEnd=x,clearEnd=right;
        for(size_t at=0;at<plan.textDamageCount;++at){const auto &box=plan.textDamage[at];
          if(y<box.y||y>=box.y+box.height)continue;
          const int32_t end=box.x+box.width;
          if(box.x<=x){if(end>x&&end>coveredEnd)coveredEnd=std::min(end,right);}
          else if(box.x<clearEnd)clearEnd=box.x;
        }
        if(coveredEnd>x){
          const size_t out=size_t(y)*target.stride+size_t(x),in=size_t(y)*width+size_t(x),count=size_t(coveredEnd-x);
          if(!detail::copyDimSystemModalRows(target.pixels+out,target.bytes/2-out,target.stride,
               base.pixels+in,width*height-in,width,count,1,PixelOrder::Wire,dim))return Status::InvalidArgument;
          drawnRaw+=count;
          x=coveredEnd;continue;
        }
        size_t remaining=size_t(clearEnd-x);auto *output=target.pixels+size_t(y)*target.stride+size_t(x);
        // Rows and their clear spans are visited strictly forwards. A raw text
        // span leaves this cursor behind, so the next clear span skips it here.
        // Full-stream validation proves every reload and the final pixel count.
        size_t skip=size_t(y)*width+size_t(x)-position;
        while(skip){
          if(!runRemaining){runRemaining=plan.encoded[streamIndex];wireColor=plan.encoded[streamIndex+1];streamIndex+=2;}
          const size_t count=std::min(skip,runRemaining);
          skip-=count;runRemaining-=count;position+=count;
        }
        while(remaining){
          if(!runRemaining){runRemaining=plan.encoded[streamIndex];wireColor=plan.encoded[streamIndex+1];streamIndex+=2;}
          const size_t count=std::min(remaining,runRemaining);
          if(!filledDominant||wireColor!=dominantWireColor){
            if(!colorCached||wireColor!=previousWireColor){
              const uint16_t value=rgb565_blend::fromWire(wireColor);
              nativeColor=uint16_t(lut.red[value>>11]|lut.green[(value>>5)&63]|lut.blue[value&31]);
              previousWireColor=wireColor;colorCached=true;
            }
            writeSystemModalNativeRun(output,count,nativeColor);
          }
  #ifdef NICE_GFX_UI_SYSTEM_MODAL_RLE_TEST_OBSERVER
          ++drawnRuns;
  #endif
          output+=count;remaining-=count;runRemaining-=count;position+=count;
        }
        x=clearEnd;
      }
    }
    streamWords=streamIndex-initialIndex;
  }
  // Reuse the existing compressed-restoration counters for a successfully
  // consumed stream, whether restored to a raw slot or directly to scanout.
  // Count both color-selection passes and all actual CPU draw cursors, including prefix
  // runs read while skipping. Validation/seek and its preloaded cursor are
  // outside this restore timer and byte count. Raw text damage does
  // not contribute to restored pixels. This timer includes that raw overlay
  // and remains nested inside the modal-base copy/render timers.
  const size_t drawnRle=size_t(roi.width)*size_t(roi.height)-drawnRaw;
  ++stats.compressedRestoreCalls;
  stats.compressedRestorePixels+=drawnRle;
  stats.compressedRestoreBytes+=(uint64_t(streamWords)+selectionWords)*sizeof(uint16_t);
  if(profilingClock)stats.compressedRestoreMicros+=profilingClock()-restoreStarted;
#ifdef NICE_GFX_UI_SYSTEM_MODAL_RLE_TEST_OBSERVER
  NICE_GFX_UI_SYSTEM_MODAL_RLE_TEST_OBSERVER(drawnRle,drawnRaw,drawnRuns,plan.textDamageCount);
#endif
  return Status::Ok;
#else
  (void)base;(void)target;(void)roi;(void)plan;(void)dim;return Status::Unsupported;
#endif
}
