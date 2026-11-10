  bool chartFusedPpaFrameAllowed()const{
#if NICE_GFX_UI_RETAINED_CHART_CPU_CONTEXT
    if(retainedActive&&ppaFade.cpuOnlyNativePaint())return nativeOverlayTarget&&canvas.chartBackdropMatchesFencedFrame(*nativeOverlayTarget);
#endif
#if NICE_GFX_UI_CHART_FUSED_PPA_FRAME && defined(CONFIG_IDF_TARGET_ESP32P4)
    return nativeOverlayTarget&&canvas.chartBackdropMatchesFencedFrame(*nativeOverlayTarget);
#else
    return false;
#endif
  }
  bool paintChartBackdropGrid(int index,Rect clipped,Rect box,uint16_t background,const Style &style){
#if NICE_GFX_UI_CHART_FUSED_BACKDROP_GRID
    const auto &page=*rasterFrame.current.page;const auto &values=rasterFrame.current.values;
    const auto &node=page.nodes[index];const auto &value=values[index];
    if(node.kind!=Kind::LineChart||value.kind!=Kind::LineChart||!node.instrument||!value.instrument||
       value.dynamic||style.borderWidth||style.gradient||style.paddingTop||style.paddingRight||style.paddingBottom||style.paddingLeft||
       paintCache->nodes[index].alpha!=255||paintCache->nodes[index].children||
       (ppaBlendEnabled&&rgb565_ppa::Context::available()&&!chartFusedPpaFrameAllowed())||!nativeOverlayTarget||stagingNativeTarget||
       renderOrder!=PixelOrder::Native||displayPolicy.monochrome||buildingSurface||foregroundCapture||renderingViewport||
       snapshotTransition||transition.active||rasterFrame.preview.page||snapshotSurface||snapshotAnimationSurface||
       snapshotKeyboard.visible||snapshotInput||snapshotMenuEnabled||snapshotMarqueeId||styledFocus()||snapshotPressed>=0||
       groupMotion.active||groupMotion.prepared||viewportOverlayLevels||nativePrefixLimit>=0||behaviorVisualMoving())return false;
#if NICE_GFX_UI_NATIVE_CACHE_REPAIR
    if(nativeCacheRepairing)return false;
#endif
    const auto &a=*node.instrument;const auto *options=a.chartStyle;
    if(!options||a.horizontal||options->fillMode||options->axes||options->legend||options->referenceCount)return false;
    for(int at=index;at>=0;at=values[at].parent==NoParent?-1:values[at].parent){const auto &v=values[at];
      if(v.dynamic||v.animating||v.animationPending||(v.flags&Disabled))return false;
      if(page.nodes[at].motion){const auto &pose=motionPose(page,values,at);if(pose.scale!=1024||pose.rotation%360||pose.reveal!=1024)return false;}
    }
    nice_gfx::ChartBackdropGridPlan plan;
    const unsigned radius=unsigned(std::min<int32_t>(style.radius,std::min(box.width,box.height)/2));
    if(!canvas.prepareChartBackdropGrid(box.x,box.y,box.width,box.height,radius,a.grid,options->gridX,options->minorGrid,
       background,style.trackColor,chartBlend(style.trackColor,style.background,35),plan))return false;
    // This is the original rounded() query, once, including its counters.
    // Only the background excludes its hole; H/V grid retain their old writes.
    const Rect cover=backgroundCover(index,clipped);nice_gfx::ChartBackdropGridResult result;
    retainedChartBackdropBegin(index);
    canvas.paintChartBackdropGrid(plan,DirtyRect(cover.x,cover.y,cover.width,cover.height),result);
    retainedChartBackdropEnd();
    ++stats.chartFusedBackdropCalls;stats.chartFusedBackdropPixels+=result.pixels;
    stats.chartFusedBackdropHolePixels+=result.holes;stats.chartFusedBackdropRows+=result.rows;
    return true;
#else
    (void)index;(void)clipped;(void)box;(void)background;(void)style;return false;
#endif
  }
