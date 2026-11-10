// Included inside View::Impl. Optional retained-source composition is selected
// before any Native target map. Its sources already exist; cold/repair work
// stays on the ordinary renderer for this frame.
#ifndef NICE_GFX_UI_SYSTEM_MODAL_SOURCE_COMPOSITION
#define NICE_GFX_UI_SYSTEM_MODAL_SOURCE_COMPOSITION NICE_GFX_P4_SYSTEM_MODAL_ACCELERATION_DEFAULT
#endif
Status composeSystemModalSourceFrame(nice_gfx::Backend &backend,bool &geometryPrepared){
#if NICE_GFX_UI_SYSTEM_MODAL_SOURCE_COMPOSITION && NICE_GFX_UI_SYSTEM_MODAL_BASE_LEASE && NICE_GFX_UI_SYSTEM_MODAL_PANEL_CACHE
  if(!project||!rasterFrame.current.page||!backend.frameOpen()||backend.busy()||!backend.supportsFrameBlit()||
     refresh->busy()||!refresh->pendingRegions()||refresh->pixelBudget()||renderBudget.maxPixelsPerTick||
     displayPolicy.monochrome||circularKeyboard||snapshotTransition||rasterFrame.preview.page||
     snapshotSurface||previewSurface||snapshotAnimationSurface||snapshotAnimationLayerCount||coldFadeSurface||seedSurface||
     snapshotInput||snapshotKeyboard.visible||snapshotPressed>=0||softInput.node>=0||suspendedSystemInput||editNode>=0||
     focusStyleCount||styledFocus()||snapshotMenuEnabled||snapshotMenuVisible||snapshotMarqueeId||
     buildingSurface||foregroundCapture||nativeCacheRepairing||systemModalPanelBuilding||renderingViewport||
     groupMotion.active||groupMotion.prepared||backPrepared||backOffset||snapshotBackOffset||menuMoving()||
     nativePrefixLimit>=0||behaviorVisuals.size()!=1||systemModalBaseRepairPending||systemModalBaseClosingFinal||
     systemModalBaseFrameSuppressed||systemModalPanelOverlap.active)return Status::Unsupported;
  auto *base=systemModalBaseSurface(systemModalBaseFrame);
  if(!base||base->page!=rasterFrame.current.page||systemModalBaseFrameKinds.size()!=base->page->count||
     !validSystemModalBase(*base,systemModalBaseFrame.root,systemModalBaseFrame.bar,systemModalBaseFrameKinds)||
     (snapshotGroupIndex>=0&&snapshotGroup.indicatorHeight))return Status::Unsupported;
  const auto &p=*rasterFrame.current.page;const auto &values=rasterFrame.current.values;
  const auto root=systemModalBaseFrame.root,bar=systemModalBaseFrame.bar;
  if(!p.width||!p.height||size_t(p.height)>SIZE_MAX/size_t(p.width)/2||root>=p.count||bar>=p.count||
     values.size()!=p.count)return Status::Unsupported;
  const auto &visual=behaviorVisuals.front();
  if(visual.root!=root||visual.kind!=BehaviorKind::Modal||visual.motion!=DialogMotion::Spring||visual.reveal!=1024)
    return Status::Unsupported;
  for(const auto &v:values)if(v.imageOverride||v.animating||v.animationPending)return Status::Unsupported;
  const int32_t ax=backend.windowAlignmentX(),ay=backend.windowAlignmentY();if(ax<=0||ay<=0)return Status::Unsupported;
  for(size_t i=0;i<refresh->pendingRegions();++i){DirtyRect r;
    if(!refresh->pendingRegion(i,r)||r.x<0||r.y<0||r.width<=0||r.height<=0||
       int64_t(r.x)+r.width>p.width||int64_t(r.y)+r.height>p.height||
       r.x%ax||r.y%ay||r.width%ax||r.height%ay)return Status::Unsupported;
  }
  if(!geometryPrepared){prepareFrameGeometry(p,values,rasterFrame.current.geometry);geometryPrepared=true;}
  const auto &geometry=rasterFrame.current.geometry.nodes;if(geometry.size()!=p.count)return Status::Unsupported;
  const auto &panel=geometry[root],&chrome=geometry[bar];const Rect screen{0,0,p.width,p.height};
  const Rect foreground=intersect(panel.rect,screen);
  if(!panel.visible||panel.alpha!=255||panel.rect.x||panel.rect.width!=p.width||panel.rect.height!=p.height||
     panel.rect.y>0||panel.rect.y<=-int32_t(p.height)||!modalBaseSameRect(visual.visual,panel.rect)||
     !modalBaseSameRect(behaviorPaintRect(root,panel.rect),panel.rect)||!modalBaseContains(panel.clip,foreground)||
     !chrome.visible||chrome.alpha!=255||chrome.rect.x||chrome.rect.y||chrome.rect.width!=p.width||chrome.rect.height<=0||
     !modalBaseContains(chrome.clip,chrome.rect)||!modalBaseContains(foreground,chrome.rect))return Status::Unsupported;
  // Null means lookup only: no source allocation/capture/repair and no dummy
  // target. The backend performs final two-source/output alias admission.
  prepareSystemModalPanelFrame(nullptr,true);
  auto *front=systemModalPanelSurface(systemModalPanelFrame);
  if(!front||front==base||systemModalPanelFrame.sharedSource||systemModalPanelFrame.owner!=frameSourceOwner||
     systemModalPanelFrame.page!=systemModalBaseFrame.page||systemModalPanelFrame.root!=root||
     !modalBaseSameRect(systemModalPanelFrame.target,panel.rect)||front->page!=&p||
     !front->pixels||!base->pixels)return Status::Unsupported;
  OpaqueFrameComposition composition;
  composition.background=backendFrameSource(*base);composition.foreground=backendFrameSource(*front);
  composition.foregroundArea={foreground.x,foreground.y,foreground.width,foreground.height};
  composition.sourceX=foreground.x-panel.rect.x;composition.sourceY=foreground.y-panel.rect.y;
  composition.backgroundKeep=uint8_t(255u-visual.dim);
  Status status;
  {RenderTimer timer(*this);status=backend.composeOpaqueFrame(composition);}
  if(status==Status::Ok){
    ++surfaceStats.hits; // The opaque panel source was actually consumed.
    if(foreground.width!=p.width||foreground.height!=p.height){
      ++surfaceStats.hits;systemModalBaseFramePainted=true;++stats.systemModalBaseFrames;
    }
  }
  // Unsupported is a pre-write refusal. A failed accepted composition is an
  // actual error; the caller aborts/drains instead of painting another frame.
  return status;
#else
  (void)backend;(void)geometryPrepared;return Status::Unsupported;
#endif
}
