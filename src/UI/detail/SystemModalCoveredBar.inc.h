// Included inside View::Impl. This proof is local to the current synchronous
// Native region. It retains no pointers, frame flags, pixel storage or clocks.
#ifndef NICE_GFX_UI_SYSTEM_MODAL_COVERED_BAR
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#define NICE_GFX_UI_SYSTEM_MODAL_COVERED_BAR 1
#else
#define NICE_GFX_UI_SYSTEM_MODAL_COVERED_BAR 0
#endif
#endif

bool systemModalBarCovered(const DirtyRect &area,bool baseApplied,bool backdropFused,int firstPaint){
#if NICE_GFX_UI_SYSTEM_MODAL_COVERED_BAR && NICE_GFX_UI_SYSTEM_MODAL_BASE_LEASE && NICE_GFX_UI_SYSTEM_MODAL_PANEL_CACHE
  if(!baseApplied||!backdropFused||!systemModalBaseFramePainted||systemModalBaseFrameSuppressed||
     systemModalBaseClosingFinal||!project||!rasterFrame.current.page||!paintCache||
     buildingSurface||foregroundCapture||nativeCacheRepairing||systemModalPanelBuilding||
     snapshotTransition||rasterFrame.preview.page||renderingViewport||paintOffsetX||paintOffsetY||
     snapshotBackOffset||snapshotInput||snapshotKeyboard.visible||snapshotPressed>=0||
     softInput.node>=0||suspendedSystemInput||editNode>=0||focusStyleCount||
     (snapshotGroupIndex>=0&&snapshotGroup.indicatorHeight)||nativePrefixLimit>=0||firstPaint>=0||
     renderOrder!=PixelOrder::Native||!nativeOverlayTarget||stagingNativeTarget||
     !canvas.targetsNativeFramebuffer(*nativeOverlayTarget)||behaviorVisuals.size()!=1)return false;
#if NICE_GFX_UI_NATIVE_CACHE_REPAIR && NICE_GFX_UI_NATIVE_PREFIX_CACHE
  if(nativePrefixSource)return false;
#endif
  const auto &p=*rasterFrame.current.page;const auto &values=rasterFrame.current.values;
  const auto root=systemModalBaseFrame.root,bar=systemModalBaseFrame.bar;
  if(!p.width||!p.height||size_t(p.height)>SIZE_MAX/size_t(p.width)/2||
     root>=p.count||bar>=p.count||values.size()!=p.count||paintCache->nodes.size()!=p.count||
     systemModalBaseFrameKinds.size()!=p.count||
     !modalBaseSameRect({area.x,area.y,area.width,area.height},{0,0,p.width,p.height})||
     systemModalPanelFrame.owner!=frameSourceOwner||systemModalPanelFrame.root!=root||
     (systemModalPanelFrame.sharedSource?systemModalPanelFrame.hostPage:systemModalPanelFrame.page)!=systemModalBaseFrame.page)return false;
  auto *base=systemModalBaseSurface(systemModalBaseFrame);
  auto *source=systemModalPanelSurface(systemModalPanelFrame);
  if(!base||base->page!=&p||!source||!source->page||!source->pixels||
     source->page->width!=p.width||source->page->height!=p.height||
     base->geometry.nodes.size()!=p.count)return false;
  const auto &visual=behaviorVisuals.front();const auto &panel=paintCache->nodes[root],&chrome=paintCache->nodes[bar];
  const auto *motion=behavior(p.nodes[root].id);
  if(!motion||!registeredSystemPanel(*motion)||!motion->open||motion->closing||
     visual.root!=root||visual.kind!=BehaviorKind::Modal||visual.motion!=DialogMotion::Spring||visual.reveal!=1024||
     !panel.visible||panel.alpha!=255||panel.rect.x||panel.rect.width!=p.width||panel.rect.height!=p.height||
     !modalBaseSameRect(systemModalPanelFrame.target,panel.rect)||
     !modalBaseSameRect(visual.visual,panel.rect)||!modalBaseSameRect(behaviorPaintRect(root,panel.rect),panel.rect)||
     !chrome.visible||chrome.alpha!=255||chrome.rect.x||chrome.rect.y||chrome.rect.width!=p.width||chrome.rect.height<=0||
     !modalBaseSameRect(chrome.rect,base->geometry.nodes[bar].rect)||!modalBaseContains(chrome.clip,chrome.rect))return false;
  const auto &style=styleFor(p,values,root);
  if(p.nodes[root].kind!=Kind::Container||values[root].kind!=Kind::Container||p.nodes[root].motion||
     p.nodes[root].instrument||values[root].instrument||(values[root].flags&(Transparent|Disabled|Hidden))||
     !(values[root].flags&Clip)||style.radius||style.gradient||style.borderWidth)return false;
  // paintSystemModalPanel copies this opaque ROI after the lower bar layer.
  // A successful early body copy still leaves the complete head for that copy.
  const Rect copied=intersect(panel.rect,{area.x,area.y,area.width,area.height});
  if(!modalBaseContains(panel.clip,copied)||!modalBaseContains(copied,chrome.rect))return false;
  const size_t bytes=size_t(p.width)*p.height*2;size_t footprint=0;
  const uintptr_t target=reinterpret_cast<uintptr_t>(nativeOverlayTarget->pixels),input=reinterpret_cast<uintptr_t>(source->pixels);
  if(nativeOverlayTarget->width!=p.width||nativeOverlayTarget->height!=p.height||
     nativeOverlayTarget->bytes>UINTPTR_MAX-target||input>UINTPTR_MAX-bytes||
     !detail::nativeWireCopyFootprint(nativeOverlayTarget->pixels,nativeOverlayTarget->bytes/2,nativeOverlayTarget->stride,p.width,p.height,footprint)||
     nativeMemoryOverlap(target,nativeOverlayTarget->bytes,source->pixels,bytes))return false;
  const auto barLayer=viewportOverlayDepth(p,values,bar),panelLayer=viewportOverlayDepth(p,values,root);
  if(!barLayer||barLayer>=panelLayer)return false;
  for(uint16_t node=0;node<p.count;++node)if(systemModalBaseKind(node,1)){
    const auto &n=p.nodes[node];const auto &state=values[node];const auto &geometry=paintCache->nodes[node];
    if(n.motion||n.instrument||state.instrument||state.animating||state.animationPending||focusStyle(node)||
       viewportOverlayDepth(p,values,node)!=barLayer)return false;
    if(!geometry.visible||!geometry.alpha)continue;
    if(!modalBaseContains(chrome.rect,intersect(geometry.rect,geometry.clip))||
       !modalBaseSameRect(behaviorPaintRect(node,geometry.rect),geometry.rect))return false;
#if NICE_GFX_UI_SHAPE_INK_DAMAGE
    if(!modalBaseContains(chrome.rect,intersect(geometry.paintRect,geometry.clip)))return false;
#endif
  }
  return true;
#else
  (void)area;(void)baseApplied;(void)backdropFused;(void)firstPaint;return false;
#endif
}
