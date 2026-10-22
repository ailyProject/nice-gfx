// Hand-authored native UI fixture for Graphics_Benchmark (not generated UI).
// The physical page matches the display; all content is clipped above the HUD.
// Requires width >= 240, height >= 240 and contentHeight >= 150 pixels.
#pragma once

#include <UI/View.h>
#include <Fonts/Fixed6x13.h>
#include <stdio.h>
#include <string.h>

namespace benchmark_ui {
using namespace nice_gfx::htmlui;

namespace detail {
enum : Id {
  Root = 1001, Title, Button, Toggle, ToggleLabel, Slider, Progress, Chart,
  List, FirstRow, FadeCard = 1100, FloatCard, PulseCard
};
constexpr uint16_t Background = 0x0843;
constexpr uint16_t Panel = 0x10a5;
constexpr uint16_t Cyan = 0x2e7f;
constexpr uint16_t Green = 0x4ed5;

struct Counters {
  uint32_t events = 0, actions = 0, failures = 0;
  uint32_t clicks = 0, changes = 0, inputs = 0, scrolls = 0;
  uint32_t baseline = 0, inputBaseline = 0, changeBaseline = 0;
  uint32_t nextAction = 0, lastElapsed = 0;
  int32_t beforeValue = 0;
  unsigned mode = 99;
  uint8_t step = 0, transition = 0;
  bool down = false;
};
inline Counters &state() { static Counters result; return result; }

inline bool checked(bool ok) {
  if (!ok) ++state().failures;
  return ok;
}

inline Style style(uint16_t background = Panel, uint16_t color = 0xffff) {
  Style s{};
  s.background = background;
  s.color = color;
  s.borderColor = 0x2969;
  s.trackColor = 0x2949;
  s.fillColor = Cyan;
  s.thumbColor = 0xffff;
  s.pressedColor = 0x1bce;
  s.disabledColor = 0x4208;
  s.radius = 5;
  s.fontSize = 1;
  s.font = &nice_gfx::fonts::Fixed6x13;
  s.trackSize = 5;
  s.thumbWidth = s.thumbHeight = 16;
  s.thumbRadius = 8;
  return s;
}

inline NodeDesc node(Id id, Kind kind, Rect rect, const char *text = "",
                     uint16_t parent = 0) {
  NodeDesc n{};
  n.parent = parent;
  n.id = id;
  n.kind = kind;
  n.rect = rect;
  n.style = style();
  n.text = text;
  n.maximum = 100;
  n.step = 1;
  if (kind == Kind::Text) n.flags = Transparent;
  return n;
}

// Storage is created once, then borrowed immutably by View. Counter resets and
// stage warmups never rebuild the resource or add duplicate event listeners.
struct Fixture {
  NodeDesc widgets[15]{};
  NodeDesc motion[6]{};
  PageDesc pages[2]{};
  ProjectDesc project{};
  InstrumentDesc chart{};
  ChartSeriesDesc series{};
  int32_t samples[24]{};
  uint16_t width = 0, height = 0, contentHeight = 0;
  bool built = false;
};
inline Fixture &fixture() { static Fixture result; return result; }

inline void build(uint16_t width, uint16_t height, uint16_t contentHeight) {
  Fixture &f = fixture();
  const int32_t inner = width - 16;
  const int32_t half = (inner - 8) / 2;
  const int32_t lowerY = 106;
  const int32_t lowerH = contentHeight - lowerY - 6;
  const int32_t listX = 8 + half + 8;
  f.width = width; f.height = height; f.contentHeight = contentHeight;

  f.widgets[0] = node(Root, Kind::Container, {0, 0, width, contentHeight}, "", NoParent);
  f.widgets[0].style = style(Background);
  f.widgets[0].flags = Clip;
  f.widgets[1] = node(Title, Kind::Text, {8, 6, inner, 18}, "UI / dynamic widgets");
  f.widgets[2] = node(Button, Kind::Button, {8, 30, 92, 28}, "CLICK");
  f.widgets[2].style = style(0x1230);
  f.widgets[2].style.textAlign = 1;
  f.widgets[2].animation = {AnimationEffect::Pulse, AnimationEasing::EaseInOut,
    AnimationTrigger::Manual, 700, 0, 0, 4};
  f.widgets[3] = node(Toggle, Kind::Switch, {width - 54, 33, 44, 23});
  f.widgets[3].style.fillColor = Green;
  f.widgets[4] = node(ToggleLabel, Kind::Text, {110, 34, width - 170, 20}, "SWITCH");
  f.widgets[5] = node(Slider, Kind::Range, {8, 64, inner, 24});
  f.widgets[5].value = 30;
  f.widgets[5].flags = Transparent | TouchPanX;
  f.widgets[6] = node(Progress, Kind::Progress, {8, 92, inner, 8});
  f.widgets[6].style.fillColor = Green;
  f.widgets[6].style.trackSize = 8;
  f.widgets[7] = node(Chart, Kind::LineChart, {8, lowerY, half, lowerH});
  for (uint16_t i = 0; i < 24; ++i) f.samples[i] = (i * 17 + 11) % 101;
  f.series = {"load", Cyan, 24, f.samples};
  f.chart.unit = "";
  f.chart.sweep = 270;
  f.chart.thickness = 3;
  f.chart.capacity = 24;
  f.chart.seriesCount = 1;
  f.chart.series = &f.series;
  f.chart.grid = 3;
  f.widgets[7].instrument = &f.chart;
  f.widgets[8] = node(List, Kind::Scroll, {listX, lowerY, inner - half - 8, lowerH});
  f.widgets[8].flags = Clip | TouchPanY | OverscrollContain;
  const char *const rowText[6] = {"01  DMA", "02  CLIP", "03  TEXT", "04  TILE", "05  INPUT", "06  ROUTE"};
  // Six rows exceed the viewport at every supported display size.
  const int32_t rowStep = lowerH / 4 + 10;
  for (uint16_t i = 0; i < 6; ++i) {
    f.widgets[9 + i] = node(FirstRow + i, Kind::Text,
      {listX + 4, lowerY + 4 + i * rowStep, inner - half - 16, rowStep - 2}, rowText[i], 8);
    f.widgets[9 + i].flags = 0;
    f.widgets[9 + i].style = style(i & 1 ? 0x1947 : 0x10c5);
    f.widgets[9 + i].style.paddingLeft = 3;
  }

  f.motion[0] = node(Root, Kind::Container, {0, 0, width, contentHeight}, "", NoParent);
  f.motion[0].style = style(0x1064);
  f.motion[0].flags = Clip;
  f.motion[1] = node(Title, Kind::Text, {8, 6, inner, 18}, "UI / native animation");
  const int32_t cardWidth = (inner - 16) / 3;
  const int32_t cardHeight = contentHeight - 80;
  const char *const labels[3] = {"FADE", "FLOAT", "PULSE"};
  const AnimationEffect effects[3] = {AnimationEffect::FadeIn, AnimationEffect::Float, AnimationEffect::Pulse};
  const uint16_t colors[3] = {0x1230, 0x31a9, 0x2450};
  for (uint16_t i = 0; i < 3; ++i) {
    f.motion[2 + i] = node(FadeCard + i, Kind::Button,
      {8 + i * (cardWidth + 8), 40, cardWidth, cardHeight}, labels[i]);
    f.motion[2 + i].style = style(colors[i]);
    f.motion[2 + i].style.textAlign = 1;
    f.motion[2 + i].animation = {effects[i], AnimationEasing::EaseInOut,
      AnimationTrigger::Enter, 800, 0, 0, 7};
  }
  f.motion[5] = node(Progress, Kind::Progress, {8, contentHeight - 22, inner, 8});
  f.motion[5].style.trackSize = 8;
  f.motion[5].style.fillColor = Green;

  f.pages[0] = {"/widgets", f.widgets, 15, width, height, Background, nullptr, nullptr};
  f.pages[1] = {"/motion", f.motion, 6, width, height, Background, nullptr, nullptr};
  f.project = {f.pages, 2, 0, ResourceVersion,
    {TransitionEffect::None, AnimationEasing::EaseInOut, 240}};
  f.built = true;
}
} // namespace detail

// Counts delivered runtime callbacks, rather than setters (setters are silent).
inline void onEvent(void *, Event &event) {
  detail::Counters &s = detail::state();
  ++s.events;
  if (event.type == EventType::Click) ++s.clicks;
  if (event.type == EventType::Input) ++s.inputs;
  if (event.type == EventType::Change) ++s.changes;
  if (event.type == EventType::Scroll) ++s.scrolls;
}
inline uint32_t events() { return detail::state().events; }
// Completed scripted clicks, switch toggles, slider/list drags and route changes.
inline uint32_t actions() { return detail::state().actions; }
inline uint32_t failures() { return detail::state().failures; }
inline void resetCounters() { detail::state() = detail::Counters{}; }

// Exclude warmup observations without replaying route setup or restarting the
// scene's native animations. The caller restarts its elapsed clock at zero.
inline void beginMeasurement(View &view) {
  const unsigned mode = detail::state().mode;
  view.pointerCancel();
  detail::state() = detail::Counters{};
  detail::state().mode = mode;
  detail::state().nextAction = mode == 2 ? 1000 : 0;
}

inline bool begin(View &view, uint16_t width, uint16_t height, uint16_t contentHeight) {
  using namespace detail;
  resetCounters();
  if (!checked(width >= 240 && height >= 240 && contentHeight >= 150 && contentHeight <= height)) return false;
  Fixture &f = fixture();
  if (!f.built) build(width, height, contentHeight);
  if (!checked(f.width == width && f.height == height && f.contentHeight == contentHeight)) return false;
  if (!checked(view.mount(f.project) == nice_gfx::Status::Ok)) return false;
  view.setEdgeBackEnabled(false);
  view.setSwipeHomeEnabled(false);
  if (!checked(view.configureNavigation(nullptr))) return false;
  const Id ids[] = {Button, Toggle, Slider, Slider, List, 0, Button, FadeCard, FloatCard, PulseCard};
  const EventType types[] = {EventType::Click, EventType::Change, EventType::Input,
    EventType::Change, EventType::Scroll, EventType::RouteChange,
    EventType::AnimationEnd, EventType::AnimationEnd, EventType::AnimationEnd, EventType::AnimationEnd};
  for (size_t i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i)
    if (!checked(view.onOnce(ids[i], types[i], onEvent))) return false;
  return true;
}

inline void pointer(View &view, int32_t x, int32_t y, bool down, uint32_t now) {
  detail::Counters &s = detail::state();
  if (down) {
    if (s.down) view.pointerMove(x, y, now);
    else view.pointerDown(x, y, now);
  } else if (s.down) view.pointerUp(x, y, now);
  s.down = down;
}

namespace detail {
inline void centerPointer(View &view, Id id, bool down, uint32_t now) {
  const Rect r = view.bounds(id);
  pointer(view, r.x + r.width / 2, r.y + r.height / 2, down, now);
}

inline bool playSceneAnimations(View &view, uint32_t now) {
  // An explicit paired page transition commits the destination immediately and
  // suppresses Enter presets. Start the destination's native animations here.
  if (strcmp(view.path(), "/motion") == 0) {
    const Id animated[] = {FadeCard, FloatCard, PulseCard};
    for (Id id : animated)
      if (!checked(view.playAnimation(id, now))) return false;
    return true;
  }
  return checked(view.playAnimation(Button, now));
}

// One small input step per deadline: press/release two controls, drag the
// slider across its range, then scroll the clipped list with real pointer input.
// No keyboard is opened: its bottom dock would cover the benchmark HUD.
inline bool script(View &view, uint32_t elapsedMs, uint32_t now) {
  Counters &s = state();
  if (elapsedMs < s.nextAction) return true;
  s.nextAction = elapsedMs + 80;
  Rect r = view.bounds(Slider);
  switch (s.step) {
    case 0:
      s.baseline = s.clicks;
      centerPointer(view, Button, true, now);
      break;
    case 1:
      centerPointer(view, Button, false, now);
      if (!checked(s.clicks > s.baseline)) return false;
      ++s.actions;
      break;
    case 2:
      s.beforeValue = view.value(Toggle); s.baseline = s.changes;
      centerPointer(view, Toggle, true, now);
      break;
    case 3:
      centerPointer(view, Toggle, false, now);
      if (!checked(view.value(Toggle) != s.beforeValue && s.changes > s.baseline)) return false;
      ++s.actions;
      break;
    case 4:
      // Change compares the released value with the value at pointer-down.
      // Reset silently so repeated left-to-right drags still commit a change.
      if (!checked(view.setValue(Slider, 0))) return false;
      s.inputBaseline = s.inputs; s.changeBaseline = s.changes;
      pointer(view, r.x + 9, r.y + r.height / 2, true, now);
      break;
    case 5: case 6: case 7:
      pointer(view, r.x + 9 + (r.width - 18) * (s.step - 4) / 3,
        r.y + r.height / 2, true, now);
      break;
    case 8:
      pointer(view, r.x + r.width - 9, r.y + r.height / 2, false, now);
      if (!checked(s.inputs > s.inputBaseline && s.changes > s.changeBaseline && view.value(Slider) >= 95)) return false;
      ++s.actions;
      break;
    case 9:
      if (!checked(view.scrollTo(List, 0, 0))) return false;
      s.baseline = s.scrolls;
      r = view.bounds(List);
      pointer(view, r.x + r.width / 2, r.y + r.height - 5, true, now);
      break;
    case 10: case 11: case 12:
      r = view.bounds(List);
      pointer(view, r.x + r.width / 2,
        r.y + r.height - 5 - (r.height - 10) * (s.step - 9) / 3, true, now);
      break;
    case 13:
      r = view.bounds(List);
      pointer(view, r.x + r.width / 2, r.y + 5, false, now);
      if (!checked(s.scrolls > s.baseline)) return false;
      ++s.actions;
      break;
  }
  s.step = uint8_t((s.step + 1) % 14);
  return true;
}
} // namespace detail

// Pass the same monotonic timestamp to update() and tick(), then finish() to
// include completion of every submitted raster frame in FPS.
inline bool update(View &view, unsigned mode, uint32_t elapsedMs, uint32_t frame, uint32_t now) {
  using namespace detail;
  Counters &s = state();
  if (!checked(fixture().built && mode <= 2)) return false;
  if (s.mode != mode || elapsedMs < s.lastElapsed) {
    view.pointerCancel(); s.down = false;
    s.step = 0; s.nextAction = 0; s.transition = 0;
    const TransitionDesc immediate{TransitionEffect::None, AnimationEasing::Linear, 0};
    // Stage boundaries are serviced only after the preceding frame completed.
    if (view.transitioning()) {
      s.lastElapsed = elapsedMs;
      return true;
    }
    if (!checked(view.replace(mode == 2 ? "/motion" : "/widgets", immediate))) return false;
    if (mode != 2 && !checked(view.stopAnimation(Button))) return false;
    if (mode == 2 && !playSceneAnimations(view, now)) return false;
    s.mode = mode;
    s.nextAction = mode == 2 ? 1000 : 0;
  }
  s.lastElapsed = elapsedMs;
  char text[40];
  snprintf(text, sizeof(text), "UI %s f%lu e%lu", mode == 0 ? "DATA" : mode == 1 ? "INPUT" : "MOTION",
    static_cast<unsigned long>(frame), static_cast<unsigned long>(s.events));
  if (!checked(view.setText(Title, text))) return false;
  // The changing readout/progress guarantees a genuine dirty frame even between
  // input deadlines. Data mode also updates the slider, switch and chart.
  if (!checked(view.setValue(Progress, int32_t(frame % 101)))) return false;
  if (mode == 0) {
    if (!checked(view.setValue(Slider, int32_t(frame % 101)))) return false;
    if (!checked(view.setValue(Toggle, int32_t((frame / 15) & 1)))) return false;
    if (!checked(view.appendChartValue(Chart, 0, int32_t((frame * 17 + 13) % 101)))) return false;
  } else if (mode == 1) {
    if (!script(view, elapsedMs, now)) return false;
  } else if (elapsedMs >= s.nextAction && !view.transitioning()) {
    const TransitionEffect effects[] = {TransitionEffect::SlideLeft, TransitionEffect::SlideRight,
      TransitionEffect::Fade, TransitionEffect::SlideUp};
    const TransitionDesc transition{effects[s.transition % 4], AnimationEasing::EaseInOut, 240};
    const bool toWidgets = strcmp(view.path(), "/motion") == 0;
    if (!checked(view.replace(toWidgets ? "/widgets" : "/motion", transition))) return false;
    if (!playSceneAnimations(view, now)) return false;
    ++s.actions; ++s.transition;
    s.nextAction = elapsedMs + 1000;
  }
  return true;
}
} // namespace benchmark_ui
