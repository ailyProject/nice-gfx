#pragma once
#include "../Graphics/Gfx.h"
#include "Compiled.h"
#include "Keys.h"
#include "Dynamic.h"
#include "Image.h"
#include "Components.h"
#include "Navigation.h"
#include "Interaction.h"
#include "MediaPlayback.h"
#define NICE_GFX_UI_MEDIA_PLAYBACK_VERSION 1
#define NICE_GFX_UI_MEDIA_PLAYBACK_CONFIG_VERSION 1
#define NICE_GFX_UI_INTERACTION_VERSION 1
#define NICE_GFX_UI_STATS_VERSION 5
#define NICE_GFX_UI_RESOURCE_CACHE_VERSION 1
#define NICE_GFX_UI_RENDER_CACHE_VERSION 3
#define NICE_GFX_UI_CIRCULAR_CLIP_VERSION 1
#define NICE_GFX_UI_ACTIONS_VERSION 1
#define NICE_GFX_UI_NAVIGATION_VERSION 5
#define NICE_GFX_UI_SHAPES_VERSION 1
#define NICE_GFX_UI_TEXT_INPUT_VERSION 1
#define NICE_GFX_UI_DYNAMIC_IMAGE_VERSION 1
#define NICE_GFX_UI_COMPONENTS_VERSION 1
#define NICE_GFX_UI_MOTION_VERSION 1
#define NICE_GFX_UI_CHECKED_MEMORY_VERSION 1
#define NICE_GFX_UI_RENDER_BUDGET_VERSION 1

namespace nice_gfx { namespace htmlui {

enum class EventType : uint8_t { PointerDown, PointerMove, PointerUp, PointerCancel, Click, Input, Change, LongPress,
  Scroll, PageChange, BeforeNavigate, RouteChange, NavigationCancel, KeyAction, Focus, Blur, Submit, Cancel, AnimationEnd,
  GestureStart, GestureUpdate, GestureEnd, GestureCancel };
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

struct RuntimeStats {
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
  uint64_t geometryReuses=0,textLayoutReuses=0,componentIndexBuilds=0;
  uint64_t tickMicros=0,budgetDeferrals=0;
  uint32_t maxTickMicros=0;
  // Optional profiling only. Node-kind buckets are disjoint; capture/repair
  // include their geometry and painting, so these phase totals overlap.
  uint64_t geometryMicros=0,mediaPaintMicros=0,shapePaintMicros=0,textPaintMicros=0,otherPaintMicros=0;
  uint64_t captureMicros=0,repairMicros=0;
};
struct RenderCacheStats {
  size_t capacityPages=0, readyPages=0;
  uint64_t hits=0, misses=0, warmedTiles=0;
  uint64_t repairedPages=0, repairPixels=0, lastRepairPixels=0;
  uint64_t seededPages=0, seededPixels=0;
  uint64_t mirroredFrames=0;
  uint64_t capturedPages=0;
  size_t compressedPages=0,compressedBytes=0,compressedCapacityBytes=0;
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
  size_t totalBytes() const { return stateBytes+snapshotBytes+geometryBytes+payloadBytes+tileBytes+scratchBytes+cacheBytes; }
};

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
  // Click actions run after veto listeners in Coordinated mode; legacy stays additive.
  bool onDefault(Id id, EventType event, EventHandler handler, void *context = nullptr);
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
  // interaction retains the previous edge-first order.
  bool configureNavigation(const NavigationDesc *config);
  // Optional borrowed RGB565 page surfaces. Call after mount(); keep storage
  // alive until cleared/replaced or destruction. nullptr,0 disables caching.
  // Warming does at most one tile per idle tick and never blocks pointer input.
  // The default remains tiled rendering without full-page allocations.
  // compressedPixels optionally reserves part of this same storage as a
  // bounded RLE565 cold bank. At least two unpacked page slots must remain.
  // The default zero preserves the existing uncompressed cache behavior.
  bool configureRenderCache(uint16_t *storage, size_t pixels, size_t compressedPixels = 0);
  // Opt in to warming a route before its first visit; no navigation/events or
  // immediate raster work. One-shot requests have priority over automatic
  // fills after the current route, and retire when exact pixels are ready.
  // Completed routes remain in the bounded LRU cache; requesting one again
  // refreshes its recency. Duplicate requests succeed. All project routes may
  // be queued without increasing surface storage. Returns false before mount/
  // cache setup or for an unknown route. At most one tile is warmed by each
  // idle tick; pointer contact pauses it. A one-slot cache temporarily yields
  // the live slot to finish a pending request before restoring the live route.
  bool prewarmRenderCache(const char *path);
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
  // Restores animation poses and emits NavigationCancel once. Entry and legacy
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
  // Copied sparse node policies. Call after mount; null/zero restores automatic
  // looping. A scrub source and its Media target must belong to the same page.
  // Reconfiguration is rejected during pointer contact or event dispatch.
  bool configureMediaPlayback(const MediaPlaybackDesc *policies, size_t count);
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
  bool scrollTo(Id id, int32_t x, int32_t y);
  bool setPage(Id id, int32_t index);
  int32_t pageIndex(Id id) const;
  void setEdgeBackEnabled(bool enabled);
  void setSwipeHomeEnabled(bool enabled);
  void configureInput(const InputConfig &config);
  bool hasKeyBinding(Id key, KeyGesture gesture) const;
  // Physical key entry points reveal focus decoration until the next pointer
  // down. Navigation retains the last input mode; mount starts with it hidden.
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
  RuntimeStats runtimeStats() const;
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
