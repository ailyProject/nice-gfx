#pragma once
#include "../Graphics/Gfx.h"
#include "../Graphics/ChartBackdropGrid.h"
#include "detail/IncrementalFrameGeometry.h"
#include "detail/AxisMediaPpa.h"
#include "Compiled.h"
#include "Keys.h"
#include "MenuMotion.h"
#include "MonochromeEffects.h"
#include "Behaviors.h"
#include "SystemBar.h"
#include "Dynamic.h"
#include "Image.h"
#include "Components.h"
#include "Navigation.h"
#include "Interaction.h"
#include "MediaPlayback.h"
#include "JpegDecoder.h"
#define NICE_GFX_UI_JPEG_MEDIA_VERSION 1
#define NICE_GFX_UI_MEDIA_PLAYBACK_VERSION 1
#define NICE_GFX_UI_MEDIA_PLAYBACK_CONFIG_VERSION 1
#define NICE_GFX_UI_SHARED_MEDIA_VERSION 1
#define NICE_GFX_UI_INTERACTION_VERSION 1
#define NICE_GFX_UI_STATS_VERSION 5
#define NICE_GFX_UI_FRAME_GEOMETRY_IN_PLACE_STATS_VERSION 1
#ifndef NICE_GFX_UI_FRAME_GEOMETRY_IN_PLACE
#define NICE_GFX_UI_FRAME_GEOMETRY_IN_PLACE 0
#endif
#define NICE_GFX_UI_OUTLINED_MASK_STATS_VERSION 1
#define NICE_GFX_UI_RESOURCE_CACHE_VERSION 1
#define NICE_GFX_UI_RENDER_CACHE_VERSION 3
#define NICE_GFX_UI_CIRCULAR_CLIP_VERSION 1
#define NICE_GFX_UI_ACTIONS_VERSION 1
#define NICE_GFX_UI_FOCUS_STYLES_VERSION 1
#define NICE_GFX_UI_NAVIGATION_VERSION 5
#define NICE_GFX_UI_SHAPES_VERSION 1
#define NICE_GFX_UI_TEXT_INPUT_VERSION 1
#define NICE_GFX_UI_DYNAMIC_IMAGE_VERSION 1
#define NICE_GFX_UI_PAGE_IMAGE_CLEAR_VERSION 1
#define NICE_GFX_UI_COMPONENTS_VERSION 2
#define NICE_GFX_UI_SCROLL_READING_VERSION 1
#define NICE_GFX_UI_COMPACT_PAGER_VERSION 1
#define NICE_GFX_UI_MOTION_VERSION 1
#define NICE_GFX_UI_CHECKED_MEMORY_VERSION 1
#define NICE_GFX_UI_RENDER_BUDGET_VERSION 1
#define NICE_GFX_UI_MONO_THEME_VERSION 1
#define NICE_GFX_UI_DISSOLVE_VERSION 1
#define NICE_GFX_UI_FOCUSABLE_VERSION 1
#define NICE_GFX_UI_SYSTEM_CHROME_VERSION 1

namespace nice_gfx { namespace htmlui {

enum class EventType : uint8_t { PointerDown, PointerMove, PointerUp, PointerCancel, Click, Input, Change, LongPress,
  Scroll, PageChange, BeforeNavigate, RouteChange, NavigationCancel, KeyAction, Focus, Blur, Submit, Cancel, AnimationEnd,
  GestureStart, GestureUpdate, GestureEnd, GestureCancel,
  Timer, TaskStart, TaskTick, TaskEnd, EditStart, EditCommit, EditCancel, Shown, Hidden };
enum class EventPhase : uint8_t { Capture, Target, Bubble };
struct Event {
  EventType type;
  EventPhase phase;
  Id target, currentTarget;
  int32_t x, y, value;
  bool bubbles = true, cancelable = true;
  bool propagationStopped = false, defaultPrevented = false;
  bool immediatePropagationStopped = false;
  int32_t scrollX = 0, scrollY = 0;
  const char *from = nullptr, *to = nullptr;
  RouteAction routeAction = RouteAction::None;
  Id component = 0; // Public authoring component containing the target, if any.
  int32_t optionIndex = -1; // Option intent; Click remains cancelable before commit.
  void stopPropagation() { propagationStopped = true; }
  void stopImmediatePropagation() { immediatePropagationStopped = propagationStopped = true; }
  void preventDefault() { if (cancelable) defaultPrevented = true; }
};
using EventHandler = void (*)(void *context, Event &event);
using RouteGuard = bool (*)(void *context, const char *from, const char *to, RouteAction action);

// Last rejected atlas candidate only; no text contents or resource pointers.
// region: 0=host, 1=registered bar subtree, 2=registered panel subtree.
// reason: 1=unsupported pose/state, 2=non-translation state, 3=geometry,
// 4=protected viewport, 5=background proof, 6=terminal-pose repair guard.
// diffMask bits: text=1, value/switch=2, flags=4, scroll=8, XY=16,
// opacity=32, motion revision=64, dynamic style=128, media=256,
// instrument samples=512, kind/parent/style index=1024.
struct AnimationAtlasRejectInfo {
  uint32_t sequence=0,diffMask=0;
  Id id=0;
  uint16_t page=NoParent,node=NoParent,sourceSlot=NoParent,kind=0;
  uint8_t region=0,sourceHidden=0,liveHidden=0,reason=0,animationEffect=0,hasMotion=0;
  int32_t sourceX=0,sourceY=0,liveX=0,liveY=0,sourceValue=0,liveValue=0;
  uint32_t sourceTextRevision=0,liveTextRevision=0,sourceMotionRevision=0,liveMotionRevision=0;
  uint16_t sourceFlags=0,liveFlags=0;
  uint8_t sourceOpacity=0,liveOpacity=0;
};
#define NICE_GFX_UI_CHART_COLUMN_STATS_VERSION 1
#define NICE_GFX_UI_AXIS_MEDIA_CACHE_STATS_VERSION 1
#define NICE_GFX_UI_AXIS_MEDIA_RUNS_STATS_VERSION 1
#define NICE_GFX_UI_AXIS_MEDIA_FULL_FOOTPRINT_STATS_VERSION 1
#define NICE_GFX_UI_CHART_PHASE_STATS_VERSION 1
#define NICE_GFX_UI_CHART_NODE_PROFILE_STATS_VERSION 1
#ifndef NICE_GFX_UI_CHART_NODE_PROFILE
#define NICE_GFX_UI_CHART_NODE_PROFILE 0
#endif
#define NICE_GFX_UI_OUTLINED_GEOMETRY_CACHE_STATS_VERSION 2
#define NICE_GFX_UI_CHART_GRID_BATCH_STATS_VERSION 1
#define NICE_GFX_UI_EARLY_GEOMETRY_STATS_VERSION 1
#define NICE_GFX_UI_KEYED_PIE_STATS_VERSION 1
#define NICE_GFX_UI_BACKGROUND_COVER_STATS_VERSION 1
#define NICE_GFX_UI_NODE_KIND_PROFILE_STATS_VERSION 1
#ifndef NICE_GFX_UI_THIN_BACKGROUND_COVER
#define NICE_GFX_UI_THIN_BACKGROUND_COVER NICE_GFX_P4_PERFORMANCE_PROFILE
#endif
#ifndef NICE_GFX_UI_BACKGROUND_COVER_PROFILE
#define NICE_GFX_UI_BACKGROUND_COVER_PROFILE 0
#endif
#ifndef NICE_GFX_UI_NODE_KIND_PROFILE
#define NICE_GFX_UI_NODE_KIND_PROFILE 0
#endif
#define NICE_GFX_UI_SHAPE_PATH_PROFILE_STATS_VERSION 1
#ifndef NICE_GFX_UI_SHAPE_PATH_PROFILE
#define NICE_GFX_UI_SHAPE_PATH_PROFILE 0
#endif
enum class ShapePaintPath : uint8_t { Unclassified, NativeSpanOpaque, NativeSpanAlpha, GpuOverlay, TransformedDirectFill, NativeAlphaRow, OutlinedMask, PlainRenderShape, GenericTransformedRows, WireSpanCache, Count };
#define NICE_GFX_UI_ROI_PREFIX_STATS_VERSION 1
#define NICE_GFX_UI_AXIS_MEDIA_PIE_STATS_VERSION 1
#ifndef NICE_GFX_UI_AXIS_MEDIA_PIE
#define NICE_GFX_UI_AXIS_MEDIA_PIE NICE_GFX_P4_PERFORMANCE_PROFILE
#endif
#define NICE_GFX_UI_ROI_DIRTY_PREFIX_STATS_VERSION 2
#define NICE_GFX_UI_ROI_PREFIX_FRAME_MEMO_STATS_VERSION 1
#define NICE_GFX_UI_OUTLINED_FOREGROUND_LUT_STATS_VERSION 1
// BEGIN ROI_PREFIX_TIMING public-contract
#ifndef NICE_GFX_UI_ROI_PREFIX_TIMING
#define NICE_GFX_UI_ROI_PREFIX_TIMING 0
#endif
#define NICE_GFX_UI_ROI_PREFIX_TIMING_STATS_VERSION 1
enum class RoiPrefixTimingPhase : uint8_t { Resolve, LegacyResolve, ContainerPlan, DirtyPlan, MediaWorld, SignatureMatch, ColdMetadata, ColdRaster, PrefixCopy, TransportCopy, PpaCopy, CanvasCopy, Count };
// END ROI_PREFIX_TIMING public-contract
#ifndef NICE_GFX_UI_OPAQUE_CONTAINER_BANDS
#define NICE_GFX_UI_OPAQUE_CONTAINER_BANDS 0
#endif
#define NICE_GFX_UI_OPAQUE_CONTAINER_BANDS_STATS_VERSION 1

struct RuntimeStats {
  // Candidate enumeration only; no extra profiling clocks.
  uint64_t retainedChartHits=0,retainedChartCold=0,retainedChartRejectedWriters=0;
  uint64_t retainedChartSkippedStores=0,retainedChartRestoredStores=0,retainedChartInkStores=0;
  uint64_t retainedCpuOnlyPaintFrames=0,retainedGpuSuppressedOperations=0;
  uint64_t nativeStagePlans=0,nativeStageFrames=0,nativeStageStripes=0,nativeStagePixels=0,nativeStageTransfers=0,nativeStageCpuCopies=0,nativeStageErrors=0,nativeStageClockReads=0,nativeStageRenderMicros=0,nativeStageCopyMicros=0;
  uint64_t nativeStageRejected[8]{},nativeStageCompletedCopyPixels=0,nativeStageCpuCopyPixels=0,nativeStagePlanMicros=0,nativeStageTotalMicros=0,nativeStageFrozenSnapshotAdmissions=0,nativeStageAffinePlans=0;
#if NICE_GFX_UI_NATIVE_STAGE && NICE_GFX_UI_RETAINED_CHART_CPU_CONTEXT
  uint64_t nativeStageValidationBytes=0,nativeStageInitializations=0;
#endif
  uint64_t opaqueContainerQueries=0,opaqueContainerFullNodeCount=0,opaqueContainerBandQueries=0;
  uint64_t opaqueContainerBandCandidates=0,opaqueContainerLinearCandidates=0,opaqueContainerBandFallbacks=0;
  uint64_t opaqueContainerMatches=0,opaqueContainerSelections=0;
  uint64_t frameGeometryInPlaceFrames=0,frameGeometryInPlaceNodeBytes=0,frameGeometryInPlaceInvalidations=0;
  uint64_t incrementalGeometryAttempts=0,incrementalGeometryFrames=0;
  uint64_t incrementalGeometryNodes=0,incrementalGeometryReuses=0,incrementalGeometryFallbacks=0;

  uint64_t outlinedForegroundLutDraws=0,outlinedForegroundLutBuilds=0,outlinedForegroundLutReuses=0,outlinedForegroundLutSmallBypasses=0;
  uint64_t outlinedForegroundLutPixels=0,outlinedForegroundLutProbeWords=0,outlinedForegroundLutBuildMicros=0;

  uint64_t roiMemoFrames=0,roiMemoResolveCalls=0,roiMemoContainerRequests=0,roiMemoContainerEvaluations=0;
  uint64_t roiMemoDirtyRequests=0,roiMemoDirtyEvaluations=0,roiMemoPlanHits=0,roiMemoWorldRequests=0;
  uint64_t roiMemoWorldBuilds=0,roiMemoWorldHits=0,roiMemoMatchEvaluations=0,roiMemoEntryValidations=0,roiMemoResets=0;

  uint64_t roiDirtyPrefixPlans=0,roiDirtyPrefixColdBuilds=0,roiDirtyPrefixColdBuildMicros=0,roiDirtyPrefixColdPixels=0;
  uint64_t roiDirtyPrefixHits=0,roiDirtyPrefixBlitPixels=0,roiDirtyPrefixInvalidations=0,roiDirtyPrefixRejects=0;
  uint64_t roiDirtyPrefixWorldPlans=0,roiDirtyPrefixWorldColdBuilds=0,roiDirtyPrefixWorldHits=0,roiDirtyPrefixWorldBlitPixels=0;
  int32_t roiDirtyPrefixLastX=0,roiDirtyPrefixLastY=0,roiDirtyPrefixLastWidth=0,roiDirtyPrefixLastHeight=0;
  uint32_t roiDirtyPrefixLastDamagePixels=0,roiDirtyPrefixLastScopePixels=0,roiDirtyPrefixLastLimit=0,roiDirtyPrefixLastScopeKind=0;

  uint64_t axisPieDraws=0,axisPieRuns=0,axisPieDirectBlocks=0,axisPieScratchBlocks=0,axisPieAcceptedBlocks=0,axisPieFallbackBlocks=0;

  uint64_t roiPrefixPlans=0,roiPrefixColdBuilds=0,roiPrefixColdBuildMicros=0,roiPrefixColdPixels=0;
  uint64_t roiPrefixHits=0,roiPrefixBlitPixels=0,roiPrefixInvalidations=0,roiPrefixRejects=0,roiPrefixOom=0;

// Mutually exclusive final paint paths. The full node elapsed includes
// construction/preflight/fallback work; do not add these to Shape time again.
  uint64_t shapePathMicros[10]{},shapePathCalls[10]{};

  // Paint invocations per dirty ROI, not distinct nodes. Same elapsed sample
  // as the existing grouped timer; these fields must not be added to it again.
  uint64_t nodeKindPaintMicros[20]{},nodeKindPaintCalls[20]{};
  uint64_t backgroundCoverCalls=0,backgroundCoverCandidates=0,thinBackgroundCoverSkips=0;

  bool uiPieEnabled=false,uiPieGateOpen=false;
  uint64_t uiPieRows=0,uiPieEligibleBlocks=0,uiPieAcceptedBlocks=0,uiPieFallbackBlocks=0,uiPieKeyBlocks=0,uiPiePeelPixels=0,uiPiePathMicros=0;
  uint64_t uiPiePixels=0,uiPieMaskedCycles=0;
  uint32_t uiPieChunks=0,uiPieMaximumMaskedCycles=0,uiPieMaximumEnvelopeCycles=0;
  uint32_t uiPieRejectedContext=0,uiPieRejectedArguments=0,uiPieRejectedUnverified=0;

  uint32_t earlyGeometryFrames=0;

  uint64_t chartGridBatches=0,chartGridPixels=0;

  uint64_t outlinedCacheHits=0,outlinedCacheMisses=0,outlinedCacheRejected=0,outlinedCacheBoundaryRejected=0,outlinedCacheEvictions=0,outlinedCacheAllocationFailures=0;
  uint64_t outlinedCacheFullBuildMicros=0,outlinedCacheRoiBuildMicros=0,outlinedCacheExtractMicros=0;
  uint64_t outlinedCacheColdReuses=0,outlinedCacheSmallRoiBypasses=0;
  uint64_t outlinedCacheFullBuildMaskBytes=0,outlinedCacheRequestedMaskBytes=0,outlinedCacheExtractedBytes=0;
  size_t outlinedCacheBytes=0,outlinedCacheUsedBytes=0,outlinedCachePeakBytes=0,outlinedCacheEntries=0;

  uint64_t chartPhaseMicros[5]{};

  uint64_t axisMediaFullFootprintPaints=0,axisMediaFullFootprintFallbacks=0,axisMediaFullFootprintSubsetPixels=0;

  uint64_t axisMediaRunBuilds=0,axisMediaRunBuildRejected=0,axisMediaRunPaints=0,axisMediaRunSegments=0,axisMediaRunOpaquePixels=0,axisMediaRunBlendPixels=0,axisMediaRunSkippedPixels=0,axisMediaRunMetadataBytes=0;

  uint64_t axisMediaCacheHits=0,axisMediaCacheMisses=0,axisMediaCacheEvictions=0,axisMediaCacheRejected=0,axisMediaCacheBytes=0;
  uint64_t chartColumnBatches=0,chartColumnPixels=0;
  uint64_t frames=0, geometryNodes=0, textLayouts=0, candidateNodes=0, paintedNodes=0;
  uint64_t renderedTiles=0, renderedPixels=0;
  uint64_t completedFrames=0;
  uint64_t keyboardLayouts=0, keyboardPaints=0;
  uint64_t lookupQueries=0, lookupComparisons=0, listenerVisits=0;
  uint64_t renderMicros=0;
  uint64_t submitMicros=0;
  uint64_t directBlits=0, directPixels=0;
  uint32_t maxRenderMicros=0;
  // Optional S31 cached full-page fade. Timings include PPA wait + Wire565
  // conversion; hardware channel rounding can differ from CPU by one step.
  uint64_t ppaBlendCalls=0,ppaBlendPixels=0,ppaBlendMicros=0;
  uint32_t ppaBlendFailures=0;
  uint64_t ppaFillCalls=0,ppaFillPixels=0,ppaFillMicros=0;
  uint32_t ppaFillFailures=0;
  uint64_t nativePrefixHits=0,nativePrefixBuilds=0,nativePrefixBuildMicros=0,nativePrefixBlitPixels=0;
  uint64_t mediaOcclusionRequests=0,mediaOcclusionSkippedPixels=0;
  uint64_t keyedGpuBlits=0,shapeGpuBlits=0,textGpuBlits=0;
  uint64_t rgb565NativeCopies=0;
  uint64_t containerAlphaGpuBlits=0,containerAlphaGpuPixels=0;
  uint64_t shapeDamageClips=0,shapeDamageSavedPixels=0;
  uint64_t nativeAffineMediaBlits=0;
  uint64_t axisPpaAttempts=0,axisPpaRejects=0,axisPpaBlits=0,axisPpaPixels=0,axisPpaErrors=0,axisPpaMicros=0;
  uint64_t offscreenGdmaCopies=0,offscreenGdmaPixels=0;
  uint64_t fixedChromeBlits=0,fixedChromeNodesSkipped=0,fixedChromeCacheHits=0,fixedChromeCacheMisses=0,fixedChromeBuilds=0;
  uint64_t fixedChromeRejects=0,fixedChromeBytes=0;
  uint64_t textNativeBlits=0,shapeNativeBlits=0;
  uint64_t buttonTextMaskBlits=0;
  uint64_t animationAtlasStageFrames=0,animationAtlasStagePixels=0,animationAtlasStageMicros=0;
  uint64_t animationAtlasAttempts=0,animationAtlasContextRejects=0,animationAtlasBehaviorRejects=0,animationAtlasViewportRejects=0;
  uint64_t animationAtlasEligibleSources=0,animationAtlasSourceMisses=0,animationAtlasStateRejects=0,animationAtlasProofRejects=0,animationAtlasZeroMotion=0,animationAtlasAcceptedFrames=0;
  AnimationAtlasRejectInfo animationAtlasLastReject;
  uint64_t coldFadeStageRasters=0,coldFadeStageReuses=0,coldFadeStagePartialRasters=0;
  uint64_t coldFadeStageRasterPixels=0,coldFadeStagePartialPixels=0;
  uint64_t coldFadeButtonMaskBlits=0;
  uint64_t nativeCaptureCalls=0,nativeCapturePixels=0,nativeCaptureMicros=0;
  // Wire cache-writeback only, independent of native async/copy counters.
  uint64_t copyWireAttempts=0,copyWireCompleted=0,copyWireRejected=0,copyWireErrors=0;
  uint64_t copyWireDriverCalls=0,copyWireDriverMicros=0,copyWirePixels=0;
  uint64_t copyWireRejectedFootprint=0,copyWireRejectedContext=0,copyWireRejectedSourceCache=0;
  uint64_t copyWireRejectedMemory=0,copyWireRejectedDmaAlignment=0,copyWireRejectedPlatform=0;
  uint64_t cachedViewportFadeFrames=0,cachedViewportFadePixels=0,cachedViewportFadeMicros=0;
  // Resource-only work in the real panel-open operation; not raster time.
  // Pixels counts attempted aligned A8 mask bytes, including row padding.
  uint64_t systemPanelTextPrepareCalls=0,systemPanelTextPrepareMicros=0;
  uint64_t systemPanelTextPrepareNodes=0,systemPanelTextPreparePixels=0;
  uint64_t systemPanelTextPrepareHits=0,systemPanelTextPrepareBuilds=0;
  uint64_t systemPanelTextPrepareRejects=0,systemPanelTextPrepareBudgetStops=0;
  uint64_t containerAlphaInteriorBlits=0,containerAlphaInteriorPixels=0,containerAlphaInteriorMicros=0;
  // Isolated primitive templates; optional time includes rejected attempts.
  uint64_t containerTemplateCalls=0,containerTemplateBuilds=0,containerTemplateHits=0,containerTemplateMicros=0;
  uint64_t systemModalBaseAcquisitions=0,systemModalBaseInvalidations=0,systemModalBaseFrames=0;
  uint64_t systemModalBaseClosingFrames=0;
  uint64_t systemModalBaseCopyCalls=0,systemModalBaseCopyPixels=0,systemModalBaseCopyMicros=0;
  uint64_t systemModalBaseSkippedNodes=0,systemModalBaseWriterPinSkips=0;
  uint32_t systemModalBaseLivePins=0,systemModalBaseFramePins=0;
  uint64_t jpegPrefetchBlits=0,jpegPrefetchHits=0,jpegPrefetchMisses=0,jpegPrefetchRequests=0;
  uint64_t jpegPrefetchDecoded=0,jpegPrefetchFailures=0,jpegPrefetchDecodeMicros=0,jpegPrefetchBytes=0;
  uint64_t jpegPrefetchCopyMicros=0;
  uint64_t shapeResolveMicros=0,shapeCompositeMicros=0,shapeFinishMicros=0;
  uint64_t animationMicros=0,snapshotMicros=0,framePrepareMicros=0,refreshStepMicros=0;
  uint64_t subtreeRangeBuilds=0,subtreeRangeHits=0,subtreeRangeFallbacks=0,subtreeVisitedNodes=0,subtreeFullSkips=0;
  uint64_t textResolveMicros=0,textCompositeMicros=0,textFinishMicros=0;
  // Only the legacy glyph loop in text(); excludes background/decorations and
  // widgets with dedicated text renderers such as InputField and Spinbox.
  uint64_t legacyGlyphInkMicros=0;
  uint64_t textMaskHits=0,textMaskBuilds=0,textMaskRejects=0,textMaskBytes=0;
  uint64_t textMaskRoiPixels=0,textMaskCompositePixels=0,textMaskBoundsClips=0,textMaskEmptySkips=0;
  size_t controlBytes=0;
  bool controlInternal=false;
  // Overlay driver timing includes SDK-owned synchronization and blocking
  // DMA wait. Sync timing/count/bytes concern additional library msync only.
  uint64_t overlayPreflightMicros=0,overlayDriverMicros=0,overlaySyncMicros=0;
  uint64_t overlayPreflightCalls=0,overlaySyncCalls=0,overlaySyncedBytes=0;
  uint64_t shapeTextureHits=0,shapeTextureMisses=0,shapeTextureEvictions=0,shapeTextureBytes=0;
  uint64_t geometryReuses=0,textLayoutReuses=0,componentIndexBuilds=0;
  uint64_t chartFusedBackdropCalls=0,chartFusedBackdropPixels=0,chartFusedBackdropHolePixels=0,chartFusedBackdropRows=0;
  uint64_t chartBackdropMicros=0,chartBackdropCalls=0,chartInstrumentMicros=0,chartInstrumentCalls=0;
  uint64_t tickMicros=0,budgetDeferrals=0;
  uint32_t maxTickMicros=0;
  // Optional profiling only. Node-kind buckets are disjoint; capture/repair
  // include their geometry and painting, so these phase totals overlap.
  uint64_t geometryMicros=0,mediaPaintMicros=0,shapePaintMicros=0,textPaintMicros=0,otherPaintMicros=0;
  uint64_t captureMicros=0,repairMicros=0;
  // Optional RLE cache profiling. Restore time includes stream validation,
  // slot selection and decoding; successful pixels/bytes exclude rejected
  // attempts. Encode time covers bounded idle count/encode/maintenance work.
  uint64_t compressedEncodeMicros=0,compressedRestoreMicros=0;
  uint64_t compressedRestoreCalls=0,compressedRestorePixels=0,compressedRestoreBytes=0;
  // Backdrop is outside the node-kind buckets; pixels/spans count only the
  // surviving dimmed region. The profiling clock is optional as above.
  uint64_t behaviorBackdropMicros=0,behaviorBackdropPixels=0,behaviorBackdropSpans=0;
  uint64_t systemModalCoveredNodes=0;
  uint64_t systemModalClippedPixels=0,rootClearSkippedPixels=0;
  // Optional candidate counters. Default-OFF builds leave all three at zero.
  uint64_t outlinedMaskHits=0,outlinedMaskFallbacks=0,outlinedMaskCoveredPixels=0;
// BEGIN ROI_PREFIX_TIMING statistics
  // Inclusive/self times overlap the existing render/node/cold-build timers.
  // These append-only fields stay zero unless the feature and Profile are ON.
  uint64_t roiPrefixTimingMicros[12]{},roiPrefixTimingSelfMicros[12]{},roiPrefixTimingCalls[12]{};
  uint64_t roiPrefixTimingAccepted=0,roiPrefixTimingRejected=0;
  uint64_t roiPrefixTimingCopyUnsupported[3]{},roiPrefixTimingCopyErrors[3]{};
// END ROI_PREFIX_TIMING statistics
};
struct RenderCacheStats {
  size_t capacityPages=0, readyPages=0;
  // Completed body pixels can await only a small live status-bar repair.
  // readyPages remains the exact full-page count for existing callers.
  size_t bodyReadyPages=0, chromePendingPages=0;
  uint64_t hits=0, misses=0, warmedTiles=0;
  uint64_t repairedPages=0, repairPixels=0, lastRepairPixels=0;
  uint64_t seededPages=0, seededPixels=0;
  uint64_t mirroredFrames=0;
  uint64_t capturedPages=0;
  size_t compressedPages=0,compressedBytes=0,compressedCapacityBytes=0;
  // Compressed records are reported separately from raw resident routes.
  // A body-ready record may need a live status-bar repair after decoding;
  // compressedReadyPages and renderCacheReady() require the exact full key.
  size_t compressedReadyPages=0,compressedBodyReadyPages=0,compressedChromePendingPages=0;
  uint64_t compressedStores=0,compressedRestores=0,compressedRejected=0,compressedEvictions=0;
  // True while eligible cache work is pending. A slot deferred until visual
  // changes become quiet, or an unused capacity slot, is not warming.
  bool warming=false;
};
// Capacity-based retained storage estimate; excludes allocator metadata, driver
// storage and immutable resources in flash. External tile buffers are counted.
struct MemoryStats {
  size_t residentPages=0, residentNodes=0, stateBytes=0, snapshotBytes=0;
  size_t geometryBytes=0, payloadBytes=0, tileBytes=0, scratchBytes=0;
  size_t cacheBytes=0;
  size_t nativeStageBytes=0;
  size_t retainedMaskBytes=0,retainedProtocolBytes=0; // mask is a subset of cacheBytes; protocol belongs to Backend.
  size_t totalBytes() const { return stateBytes+snapshotBytes+geometryBytes+payloadBytes+tileBytes+scratchBytes+cacheBytes+retainedProtocolBytes+nativeStageBytes; }
};

#if NICE_GFX_UI_FRAME_GEOMETRY_EARLY_RESERVE
struct FrameGeometryStorageSnapshot {
  struct OwnedNodes {
    uintptr_t address=0;
    size_t size=0,capacity=0,activeBytes=0,allocatedBytes=0;
    bool owned=false,spanValid=false,internal=false,external=false;
  };
  OwnedNodes current,preview,memo;
  bool memoBorrowsCurrent=false;
  bool internalAllocatorAvailable=false,earlyReserveAvailable=false;
};
#endif

struct RenderBudget {
  // Zero preserves automatic native/PPA/full-frame paths. A positive limit
  // selects bounded tile composition (including cache copies) for each tick.
  size_t maxPixelsPerTick=0;
  // Cooperative deadline: required frame work still makes progress. Optional
  // cache maintenance is deferred after this time, measured with the profiling
  // clock or platform micros(). Preparation/driver calls are not preempted.
  uint32_t maxMicrosPerTick=0;
};

// A native compiled-page runtime. Owns dynamic text and uses immutable render
// snapshots so input and setters remain safe while asynchronous tiles transfer.
// Calls are single-task: invoke from one UI task, not interrupt handlers.
class View {
 public:
  // Owned tiles prefer internal DMA-capable RAM on ESP32 and fall back to the
  // ordinary heap. The borrowed-buffer overload never allocates tile storage.
  explicit View(NICE_GFX &display, size_t tilePixels = 0);
  View(NICE_GFX &display, uint16_t *first, uint16_t *second, size_t tilePixels);
  // Checked construction; output is unchanged on failure. The caller owns a
  // successful result and deletes it normally. With exceptions enabled, all
  // construction allocations report NoMemory. Without exceptions, only the
  // outer View/Impl allocations are checked; STL exhaustion may terminate.
  static Status create(NICE_GFX &display, View *&output, size_t tilePixels = 0) noexcept;
  static Status create(NICE_GFX &display, uint16_t *first, uint16_t *second,
                       size_t tilePixels, View *&output) noexcept;
  ~View();
  View(const View &) = delete;
  View &operator=(const View &) = delete;
  // Prepare the new initial page before replacing the mounted state. With
  // exceptions enabled, allocation failure returns NoMemory and retains the
  // old project, live resources and any in-flight frame. Tiles are not copied.
  // A successful replacement cancels pointer/navigation notifications; user
  // callback exceptions propagate. Other mutators are not checked OOM APIs.
  Status mount(const ProjectDesc &project);
  // Optional two-page immutable metadata cache. Configure before mount; zero
  // disables it. Budget includes page descriptors; ESP32 preserves the stated
  // free internal RAM floor and falls back to borrowed Flash resources on OOM.
  bool configureResourceCache(size_t maxBytes, size_t reserveInternalBytes = 65536);
  bool on(Id id, EventType event, EventHandler handler, void *context = nullptr, bool capture = false);
  // Idempotent registration for generated declarative UI actions. on() remains additive.
  bool onOnce(Id id, EventType event, EventHandler handler, void *context = nullptr, bool capture = false);
  // Click actions run after veto listeners in Coordinated mode; Legacy mode is additive.
  bool onDefault(Id id, EventType event, EventHandler handler, void *context = nullptr);
  bool onUi(Id id, EventType event, EventHandler handler, void *context = nullptr);
  bool onScoped(Id id, EventType event, EventHandler handler, void *context = nullptr);
  bool configureInteraction(InteractionVersion version, const InteractionDesc *policies = nullptr, size_t count = 0);
  InteractionVersion interactionVersion() const;
  bool setGesturePolicy(Id id, const GesturePolicy &policy);
  // Request ownership synchronously from PointerDown/Move. Deepest eligible
  // request wins after propagation; mutate only in GestureStart/Update/End.
  bool claimPointer(Id owner, GestureAxis axis);
  Id gestureOwner() const;
  bool acceptsEvent(const Event &event) const;
  // Borrowed immutable descriptors/strings until replacement or mount. Generated
  // ui.h installs these links so ordinary setters/getters accept public IDs.
  // Replacement is rejected while a raster frame still borrows the links.
  bool configureComponents(const ComponentDesc *components, size_t count);
  bool setExpanded(Id id, bool expanded); // Select, Popup or Confirm panel.
  bool expanded(Id id) const;
  void setRouteGuard(RouteGuard guard, void *context = nullptr);
  // Call after mount(). Invalid resources leave the existing config intact;
  // nullptr clears it. Config and strings remain valid until replaced/cleared.
  // Coordinated interaction: local controls and scrollers precede navigation;
  // only explicit overrideControls edge bindings can preempt them. Legacy
  // interaction uses edge-first order.
  bool configureNavigation(const NavigationDesc *config);
  // Optional project chrome, painted above authored viewport popups. Arrays are
  // copied; every root must be an ordinary container in the mounted project.
  bool configureSystemChrome(const Id *bars, size_t barCount, const Id *panels, size_t panelCount);
  bool configureSystemPanelLinks(const SystemPanelLink *links, size_t count);
  // Optional borrowed RGB565 page surfaces. Call after mount(); keep storage
  // alive and unmodified until a successful clear/replacement or destruction,
  // after pending refreshes drain. It must be independent of writable display
  // framebuffers, tile buffers and mutable asset storage. nullptr,0 disables.
  // Warming does at most one tile per idle tick and never blocks pointer input.
  // The default remains tiled rendering without full-page allocations.
  // compressedPixels optionally reserves part of this same storage as a
  // bounded RLE565 cold bank. At least two unpacked page slots must remain.
  // The default zero uses an uncompressed cache.
  bool configureRenderCache(uint16_t *storage, size_t pixels, size_t compressedPixels = 0);
  // Opt in to warming a route before its first visit; no navigation/events or
  // immediate raster work. One-shot requests have priority over automatic
  // fills after the current route, and retire when body pixels are ready.
  // Live status-bar text/progress may await a small repair at presentation;
  // renderCacheReady/readyPages still report exact full-page readiness.
  // Completed routes remain in the bounded LRU cache; requesting one again
  // refreshes its recency. Duplicate requests succeed. All project routes may
  // be queued without increasing surface storage. Returns false before mount/
  // cache setup or for an unknown route. At most one tile is warmed by each
  // idle tick; pointer contact pauses it. A one-slot cache temporarily yields
  // the live slot to finish a pending request before restoring the live route.
  bool prewarmRenderCache(const char *path);
  // Optional startup-only native shape/text/chrome preparation on the UI
  // owner task after mount(), before input is enabled. Uses an independent
  // bounded scratch framebuffer; never submits LCD pixels or advances clocks.
  // Returns false when busy, unsupported, budget-limited or optional allocation
  // fails. As with other mutators, exception-free STL exhaustion may terminate.
  bool prewarmNativeResources();
  // Optional idle maintenance, independent of animation sampling. Performs at
  // most one bounded cache tile; never navigates or advances animation clocks.
  // Busy refreshes, pointer contact and transitions defer the work. Call only
  // on the View's UI thread, when the application has spare frame time.
  bool stepRenderCache(uint32_t now, size_t maxPixels = 8192);
  bool renderCacheReady(const char *path) const;
  RenderCacheStats renderCacheStats() const;
  // Opt-in physical round-panel transfer clipping, with a two-pixel margin.
  // Logical layout/cache/input stay square. Changes require an idle view and
  // repaint the panel; other displays retain complete rectangular transfers.
  bool configureCircularClip(bool enabled);
  // Up to 64 history entries; push returns false at the limit. Page state is
  // initialized on first visit and retained until released, mount or destruction.
  bool push(const char *path);
  bool replace(const char *path);
  // Override the paired page effect for this navigation (None is immediate).
  bool replace(const char *path, const TransitionDesc &transition);
  bool back();
  // Return to ProjectDesc::initial and clear navigation history. Page data is
  // retained; route guards and the authored reverse transition still apply.
  bool home();
  const char *path() const;
  size_t historySize() const;
  // Authored project transition; pointer input is gated until its final frame.
  // A second programmatic navigation returns false while a transition runs.
  bool transitioning() const;
  bool transitionFramePending() const;
  // True only while a raster frame borrows its snapshot/buffers. busy() also
  // includes animation clocks and gestures waiting for their next frame.
  bool refreshing() const;
  // Cancel an accepted component/page departure before history commits.
  // Restores animation poses and emits NavigationCancel once. Entry and
  // paired transitions have already committed and cannot be canceled here.
  // Interactive edge-back/group drags use their own settle animation and do not
  // replay authored departure/page-transition overrides on the confirmed gesture.
  bool cancelNavigation();
  // Data access and geometry queries below address the active page only.
  // Keep off-page data in application state and refresh it on RouteChange.
  // Text/value setters copy/update state without emitting Input or Change.
  // Input-field setters reject invalid types, capacities or numeric bounds;
  // required fields may be cleared programmatically. Active drafts are retained.
  // Spinbox text is its caption; read/write the committed number with value().
  bool setText(Id id, const char *text);
  // Explicit retained cross-page text updates, including inactive pages. Plain
  // Text/Label/Button only; input fields retain their active-session API.
  bool setPageText(const char *path, Id id, const char *text);
  // Retained, silent progress update on an active or inactive page.
  bool setPageProgress(const char *path, Id id, int32_t value);
  const char *text(Id id) const;
  // animateSwitch=false restores a Switch pose immediately; other controls
  // retain their normal value semantics. Setters remain silent.
  bool setValue(Id id, int32_t value, bool animateSwitch = true);
  int32_t value(Id id) const;
  // Media nodes only, including empty gfx-media placeholders. Ordinary upload
  // copies pixels before returning; Image overload shares immutable ownership.
  // A frame already submitted keeps its old source. Failure leaves state intact.
  Status setImage(Id id, const ImageView &source);
  Status setImage(Id id, const Image &image);
  bool hasImage(Id id, const Image &image) const; // Same immutable source on the active Media node.
  // Copy the descriptor, borrow immutable compiled/flash pixels until all
  // states, snapshots and cache keys release them. Never use mutable RAM here;
  // use setImage/copy/adopt for decoded or downloaded buffers instead.
  Status setMediaResource(Id id, const MediaDesc &resource);
  // S31/P4 uses the hardware JPEG decoder by default. An optional caller codec
  // supports other platforms and deterministic host tests; context outlives View.
  bool configureJpegDecoder(JpegDecodeHandler handler,void *context=nullptr);
  JpegMediaStats jpegMediaStats() const;
  // Copied sparse node policies. Call after mount; null/zero restores automatic
  // looping. A scrub source and its Media target must belong to the same page.
  // Reconfiguration is rejected during pointer contact or event dispatch.
  bool configureMediaPlayback(const MediaPlaybackDesc *policies, size_t count);
  // Copied cross-route media policies; install after mount()/playback policies.
  // Members of one group must share their authored MediaDesc. The first entry
  // holds its target frame; route changes within the group move from the current
  // frame to the new target in either direction, then pause. Fast route changes
  // retarget from the displayed frame. Nonmember entry resets to its target.
  // Invalid tables leave the active configuration/playback unchanged.
  bool configureSharedMedia(const SharedMediaDesc *policies, size_t count);
  // Per-node playback, independent of other nodes sharing the same pixels.
  // Frame numbers are zero-based. Invalid frames/durations leave state intact.
  // Seek pauses; timed playback may run backwards and pauses at its target.
  // Timers begin on the next tick; later commands replace earlier commands.
  bool setMediaFrame(Id id, int32_t frame);
  bool playMediaTo(Id id, int32_t frame, uint32_t durationMs = 500);
  // Constant frame cadence, including spans longer than the fixed-duration API limit.
  bool playMediaToFps(Id id, int32_t frame, uint16_t fps);
  bool playMedia(Id id);
  bool pauseMedia(Id id);
  int32_t mediaFrame(Id id) const; // -1 for an empty/unknown/non-Media target.
  bool mediaPlaying(Id id) const;
  Status clearImage(Id id); // Blank content; preserve layout and interactions.
  // Clear exactly this Media node to a blank override, including an authored
  // or setMediaResource() source. An active page uses clearImage(id). A valid
  // uninitialized hidden page returns Ok without creating page state; there is
  // no existing override to release. Null/unknown paths or non-Media IDs fail.
  // Accepted frames, transitions and cache keys keep their immutable leases
  // until their normal lifecycle ends; this does not force their release.
  Status clearImage(const char *pagePath, Id id);
  Status resetImage(Id id); // Restore the authored fallback (or empty content).
  // Clear RAM Image overrides on an inactive page without resetting its
  // controls or touching accepted raster/transition snapshots. Safe from a
  // RouteChange handler using event.from. Retained pixels keep their source
  // leases until their normal drain/repair/warm lifecycle completes. Authored
  // and setMediaResource() sources stay intact. Null/unknown/current -> false;
  // an existing inactive page with nothing to clear succeeds without work.
  bool clearPageImages(const char *path);
  // One shared editing session. Text/value always return committed content;
  // draft changes emit Input, confirmation emits Change (if changed) and Submit.
  bool beginTextInput(Id id);
  const char *inputDraft() const;
  size_t inputCursor() const; // UTF-8 byte offset, always at a character boundary.
  bool inputText(const char *text);
  bool moveInputCursor(int32_t characters);
  bool backspaceInput();
  bool deleteInput(); // Delete the UTF-8 character after the caret.
  bool clearInput();
  bool keyboardKey(const char *key); // Current key ID, label or action name; "done" honors the field's enterAction.
  bool commitTextInput(bool next=false); // Also honors Auto/Next/Done/Search on the field.
  bool cancelTextInput();
  bool hideTextInput(); // Retains draft until confirmation, cancellation or navigation.
  bool keyboardVisible() const;
  Id inputTarget() const;
  const char *inputError() const;
  Rect keyboardBounds() const;
  Rect keyboardKeyBounds(const char *key) const;
  // Override every field's keyboard theme until resetKeyboardTheme() restores
  // the active field's compiled preference. Safe to change during a DMA frame.
  void setKeyboardTheme(KeyboardTheme theme);
  void resetKeyboardTheme();
  KeyboardTheme keyboardTheme() const;
  // Instrument numbers use integer fixed point: data-decimals=1 means 235 = 23.5.
  // Chart index is chronological. INT32_MIN represents a missing sample.
  bool setChartValue(Id id, uint8_t series, uint16_t index, int32_t value);
  bool appendChartValue(Id id, uint8_t series, int32_t value);
  bool setChartSeries(Id id, uint8_t series, const int32_t *values, size_t count);
  int32_t chartValue(Id id, uint8_t series, uint16_t index) const;
  bool setVisible(Id id, bool visible);
  // show plays a Show/Enter preset; hide delays Hidden until an authored
  // exitAnimation's final raster frame is on the display. setVisible is immediate.
  bool show(Id id);
  bool hide(Id id);
  bool setEnabled(Id id, bool enabled);
  bool setFocusable(Id id, bool focusable);
  bool scrollTo(Id id, int32_t x, int32_t y);
  bool setPage(Id id, int32_t index);
  int32_t pageIndex(Id id) const;
  void setEdgeBackEnabled(bool enabled);
  void setSwipeHomeEnabled(bool enabled);
  void configureInput(const InputConfig &config);
  // Explicit monochrome menu behavior. Configuration is copied, validated and
  // reset by mount(); without configuration, focus and scroll move immediately.
  bool configureMenuMotion(const MenuMotionConfig &config);
  bool setMenuMotionPreset(MenuPreset preset);
  bool setMenuIndicator(MenuIndicator indicator);
  bool setLightMode(bool enabled);
  bool lightMode() const;
  // Opt-in final-scanout dithering for immediate monochrome displays.
  // Configuration is reset by mount(); cached surfaces retain RGB565 pixels.
  bool configureMonochromeEffects(MonochromeEffects effects);
  bool configureBehaviors(const BehaviorDesc *descriptions,size_t count);
  bool startBehavior(Id id);
  bool pauseBehavior(Id id,bool paused);
  bool finishBehavior(Id id,bool commit=true);
  bool behaviorRunning(Id id)const;
  bool setNavigationLimit(uint16_t depth);
  // Borrowed sorted paint descriptors. Rejected while a raster frame borrows them.
  bool configureFocusStyles(const FocusStyleDesc *styles, size_t count);
  bool hasKeyBinding(Id key, KeyGesture gesture) const;
  // Physical key entry points reveal focus decoration until the next pointer
  // down. Navigation retains the last input mode; mount starts with it hidden.
  // Confirm on an overflowing Scroll with no focusable children enters reading:
  // directions advance one glyph row/column; Confirm or Back returns to focus
  // navigation before dismissing a modal. Explicit scroll actions retain a 24px step.
  bool inputButton(Id key, KeyGesture gesture, uint32_t now = 0);
  bool inputAction(InputAction action, Id target = 0, const char *route = nullptr, uint32_t now = 0);
  // Programmatic focus preserves the input mode and still emits Focus/Blur.
  bool focus(Id id);
  Id focused() const;
  uint32_t inputGeneration() const;
  Rect bounds(Id id) const;
  // Current topmost visible node at a pointer coordinate; zero outside a page.
  Id hitTest(int32_t x, int32_t y) const;
  Rect layoutBounds(Id id) const;
  Style style(Id id) const;
  bool setBounds(Id id, Rect bounds);
  bool resetBounds(Id id);
  bool setStyle(Id id, const Style &style);
  bool resetStyle(Id id);
  bool beginUpdate();
  bool endUpdate();
  bool layoutChildren(Id id, LayoutAxis axis, int32_t gap=0, bool resizeContainer=false);
  // Reset an inactive page to compiled defaults on its next visit. A page in
  // history or used by a transition/snapshot cannot be released.
  bool releasePage(const char *path);
  MemoryStats memoryStats() const;
#if NICE_GFX_UI_FRAME_GEOMETRY_EARLY_RESERVE
  FrameGeometryStorageSnapshot frameGeometryStorage() const noexcept;
#endif
  RuntimeStats runtimeStats() const;
  nice_gfx::NativePresentationStats nativePresentationStats() const;
  void resetRuntimeStats();
  // Optional profiling only. The monotonic microsecond clock is borrowed;
  // nullptr disables timing, keeping the default renderer free of clock calls.
  void setProfilingClock(uint32_t (*clock)());
  // Reconfigure only outside a raster frame; existing dirty work is retained.
  bool configureRenderBudget(const RenderBudget &budget);
  RenderBudget renderBudget() const;
  // Optional S31 fade blending. CPU fallback, cache contents, dirty regions
  // and authored timing stay intact. Reject while a raster frame is borrowed
  // or a callback mutation is being processed. Default enabled on all targets.
  bool configurePpaBlend(bool enabled);
  bool ppaBlendEnabled() const;
  // Extra pixels worth repainting to avoid one more window/submit operation.
  // Defaults to 32. Changes require an idle raster frame and retain dirty work.
  bool configureMergeSlack(uint32_t pixels);
  uint32_t mergeSlack() const;
  // Restart on the next tick, or pass an explicit clock shared with tick().
  // stopAnimation restores the authored position and full opacity.
  bool playAnimation(Id id);
  bool playAnimation(Id id, uint32_t now);
  bool stopAnimation(Id id);
  bool animationRunning(Id id) const;
  // Single pointer API: a new down cancels the previous pointer sequence.
  void pointerDown(int32_t x, int32_t y, uint32_t now = 0);
  void pointerMove(int32_t x, int32_t y, uint32_t now = 0);
  void pointerUp(int32_t x, int32_t y, uint32_t now = 0);
  void pointerCancel();
  // Permanently stop held-key repeats without canceling a click.
  void stopPointerRepeat();
  // Retain the hold across brief sampling gaps; stable release still ends it.
  void pausePointerRepeat(bool paused);
  // Ordinary composition does one tile per tick. Automatic accelerated paths
  // can process full dirty regions; configureRenderBudget bounds raster work.
  // Busy is expected until a frame finishes. No hard real-time deadline.
  Status tick(uint32_t now = 0);
  // Drains UI composition/source reads. E-paper optical refresh advances via
  // tick(now) and may remain refreshing() after this call returns.
  Status finish();
  // Logical UI composition/animation work; physical e-paper latency does not
  // lock menus. refreshing() additionally reports a physical display update.
  bool busy() const;
  bool dirty() const;
  void invalidate();
 private:
  struct Impl;
  explicit View(Impl *impl) : impl_(impl) {}
  Impl *impl_;
};

} }
