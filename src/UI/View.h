#pragma once
#include "../Graphics/Gfx.h"
#include "Compiled.h"
#include "Keys.h"
#include "Dynamic.h"
#include "Image.h"
#include "Components.h"
#include "Navigation.h"
#define NICE_GFX_UI_STATS_VERSION 4
#define NICE_GFX_UI_RENDER_CACHE_VERSION 2
#define NICE_GFX_UI_CIRCULAR_CLIP_VERSION 1
#define NICE_GFX_UI_ACTIONS_VERSION 1
#define NICE_GFX_UI_NAVIGATION_VERSION 4
#define NICE_GFX_UI_SHAPES_VERSION 1
#define NICE_GFX_UI_TEXT_INPUT_VERSION 1
#define NICE_GFX_UI_DYNAMIC_IMAGE_VERSION 1
#define NICE_GFX_UI_COMPONENTS_VERSION 1
#define NICE_GFX_UI_MOTION_VERSION 1

namespace nice_gfx { namespace htmlui {

enum class EventType : uint8_t { PointerDown, PointerMove, PointerUp, PointerCancel, Click, Input, Change, LongPress,
  Scroll, PageChange, BeforeNavigate, RouteChange, NavigationCancel, KeyAction, Focus, Blur, Submit, Cancel, AnimationEnd };
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
};
struct RenderCacheStats {
  size_t capacityPages=0, readyPages=0;
  uint64_t hits=0, misses=0, warmedTiles=0;
  uint64_t repairedPages=0, repairPixels=0, lastRepairPixels=0;
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

// A native compiled-page runtime. Owns dynamic text and uses immutable render
// snapshots so input and setters remain safe while asynchronous tiles transfer.
// Calls are single-task: invoke from one UI task, not interrupt handlers.
class View {
 public:
  // Owned tiles prefer internal DMA-capable RAM on ESP32 and fall back to the
  // ordinary heap. The borrowed-buffer overload never allocates tile storage.
  explicit View(NICE_GFX &display, size_t tilePixels = 0);
  View(NICE_GFX &display, uint16_t *first, uint16_t *second, size_t tilePixels);
  ~View();
  View(const View &) = delete;
  View &operator=(const View &) = delete;
  Status mount(const ProjectDesc &project);
  bool on(Id id, EventType event, EventHandler handler, void *context = nullptr, bool capture = false);
  // Idempotent registration for generated declarative UI actions. on() remains additive.
  bool onOnce(Id id, EventType event, EventHandler handler, void *context = nullptr, bool capture = false);
  // Borrowed immutable descriptors/strings until replacement or mount. Generated
  // ui.h installs these links so ordinary setters/getters accept public IDs.
  bool configureComponents(const ComponentDesc *components, size_t count);
  bool setExpanded(Id id, bool expanded); // Select, Popup or Confirm panel.
  bool expanded(Id id) const;
  void setRouteGuard(RouteGuard guard, void *context = nullptr);
  // Call after mount(). Invalid resources leave the existing config intact;
  // nullptr clears it. Config and strings remain valid until replaced/cleared.
  // Explicit edge gestures win over controls; then ranges/scroll/pagers,
  // unbounded gestures/group swipes, and legacy Home/Back, respecting touch-action.
  bool configureNavigation(const NavigationDesc *config);
  // Optional borrowed RGB565 page surfaces. Call after mount(); keep storage
  // alive until cleared/replaced or destruction. nullptr,0 disables caching.
  // Warming does at most one tile per idle tick and never blocks pointer input.
  // The default remains tiled rendering without full-page allocations.
  bool configureRenderCache(uint16_t *storage, size_t pixels);
  // Opt in to warming a route before its first visit; no navigation/events or
  // immediate raster work. Requests have priority over automatic cache fills
  // and are retained until mount/cache reconfiguration. Duplicate requests
  // succeed. Returns false before mount/cache setup, for an unknown route, or
  // when distinct requests already fill the configured page capacity. At most
  // one tile is warmed by a subsequent idle tick; pointer contact pauses it.
  bool prewarmRenderCache(const char *path);
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
  Status clearImage(Id id); // Blank content; preserve layout and interactions.
  Status resetImage(Id id); // Restore the authored fallback (or empty content).
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
  // tick does at most one tile of work. Busy is expected until a frame finishes.
  Status tick(uint32_t now = 0);
  Status finish();
  bool busy() const;
  bool dirty() const;
  void invalidate();
 private:
  struct Impl;
  Impl *impl_;
};

} }
