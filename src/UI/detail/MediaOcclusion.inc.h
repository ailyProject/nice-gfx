// Included inside View::Impl. Only media-node damage is reduced. Every other
// state/geometry mutation still contributes its original old/new damage, so
// a moving, fading, hidden or edited foreground cannot retain stale pixels.
#ifndef NICE_GFX_UI_MEDIA_OCCLUSION
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#define NICE_GFX_UI_MEDIA_OCCLUSION 1
#else
#define NICE_GFX_UI_MEDIA_OCCLUSION 0
#endif
#endif
bool invalidateVisibleMedia(int index,Rect area){
#if NICE_GFX_UI_MEDIA_OCCLUSION && (NICE_GFX_UI_SHAPE_OVERLAY_PPA || NICE_GFX_UI_SHAPE_NATIVE_SPANS || NICE_GFX_UI_SHAPE_WIRE_SPANS)
  if(page().nodes[index].kind!=Kind::Media||area.width<=0||area.height<=0||uint64_t(area.width)*area.height<16384||
     (transition.active&&transition.phase!=TransitionPhase::Components)||groupMotion.active||backPrepared||backAnimation.active||
     softInput.visible||(input&&keyInput)||(focusStyleCount&&focusNode>=0)||viewportOverlayLevels||menuMoving()||behaviorVisualMoving()||buildingSurface)return false;
  Rect covers[4]{};size_t coverCount=0;
  for(uint16_t i=uint16_t(index+1);i<page().count;++i){
    const auto &n=page().nodes[i];const auto &v=state()[i];
    const bool shape=n.kind==Kind::Shape&&v.kind==Kind::Shape&&n.shape&&(n.shape->flags&ShapeFill);
    const bool container=n.kind==Kind::Container&&v.kind==Kind::Container&&!(v.flags&Transparent)&&
      !(down&&!canceled&&activeControl==i&&(v.flags&ActiveTransparent));
    if((!shape&&!container)||
       !visible(page(),state(),i)||opacity(page(),state(),i)!=255||componentFor(i,true)||focusStyle(i))continue;
    bool ordinary=true;
    for(int ancestor=i;ancestor>=0;ancestor=parent(ancestor)){
      if(state()[ancestor].flags&Disabled){ordinary=false;break;}
      if(page().nodes[ancestor].motion){const auto &m=motionPose(page(),state(),ancestor);if(m.scale!=1024||m.rotation%360||m.reveal!=1024){ordinary=false;break;}}
    }
    if(!ordinary)continue;
    const Rect box=rect(i);if(box.width<=0||box.height<=0||uint64_t(box.width)*box.height<4096)continue;
    Rect cover;const auto &style=styleFor(page(),state(),i);
    if(shape){if(!shapeOverlayCache.opaqueInterior(*n.shape,box,style.radius,cover))continue;}
    else{
      if(style.gradient)continue;
      const int32_t radius=std::min<int32_t>(style.radius,std::min(box.width,box.height)/2);
      // Stay strictly inside the rounded fill, including its antialiased edge.
      cover=radius?Rect{box.x+radius+1,box.y+1,box.width-2*radius-2,box.height-2}:box;
    }
    cover=intersect(intersect(cover,clip(page(),state(),i)),area);
    if(cover.width<=0||cover.height<=0||uint64_t(cover.width)*cover.height<4096)continue;
    size_t insert=0;while(insert<coverCount&&uint64_t(covers[insert].width)*covers[insert].height>=uint64_t(cover.width)*cover.height)++insert;
    if(insert>=4)continue;const size_t last=std::min(coverCount,size_t(3));for(size_t move=last;move>insert;--move)covers[move]=covers[move-1];covers[insert]=cover;coverCount=std::min(coverCount+1,size_t(4));
  }
  if(!coverCount)return false;
  Rect pieces[16]{area};size_t count=1;
  for(size_t cover=0;cover<coverCount;++cover){Rect next[16];size_t size=0;
    const auto add=[&](Rect r){if(r.width<=0||r.height<=0)return true;if(size==16)return false;next[size++]=r;return true;};
    for(size_t i=0;i<count;++i){const Rect r=pieces[i],inside=intersect(r,covers[cover]);
      if(inside.width<=0||inside.height<=0){if(!add(r))return false;continue;}
      const int32_t right=r.x+r.width,bottom=r.y+r.height;
      if(!add({r.x,r.y,r.width,inside.y-r.y})||!add({r.x,inside.y+inside.height,r.width,bottom-inside.y-inside.height})||
         !add({r.x,inside.y,inside.x-r.x,inside.height})||!add({inside.x+inside.width,inside.y,right-inside.x-inside.width,inside.height}))return false;
    }
    count=size;for(size_t i=0;i<count;++i)pieces[i]=next[i];
  }
  uint64_t visiblePixels=0;for(size_t i=0;i<count;++i){visiblePixels+=uint64_t(pieces[i].width)*pieces[i].height;invalidatePageRect(pieces[i]);}
  ++stats.mediaOcclusionRequests;stats.mediaOcclusionSkippedPixels+=uint64_t(area.width)*area.height-visiblePixels;return true;
#else
  (void)index;(void)area;return false;
#endif
}
