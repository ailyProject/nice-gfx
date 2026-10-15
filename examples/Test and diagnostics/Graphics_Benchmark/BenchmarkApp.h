// Shared workload used by the Arduino and native ESP-IDF examples.
#pragma once
#include <NICE_GFX.h>
#include <UI/View.h>
#include "BenchmarkStats.h"
#include "BenchmarkScenes.h"
#include "BenchmarkUi.h"
#include <cstdio>

namespace graphics_benchmark {
namespace platform = benchmark_platform;
GFX_Sprite animation(&gfx);
GFX_Sprite raster(&gfx);
nice_gfx::htmlui::View ui(gfx, 2048);

// Optional touch: edit exampleAttachTouch() for your hardware before enabling.
// Automatic pointer scripts work without any touch controller.
constexpr bool kAttachTouch = false;
constexpr uint8_t kRotation = 0;
constexpr uint32_t kWarmupMs = 750, kMeasureMs = 6000, kSummaryMs = 12000;
constexpr int kHudHeight = 78;

using nice_gfx::Status;
using benchmark::Scene;
benchmark::Stats stats;
struct Result {
  double fps = 0, mean = 0, p95 = 0;
  uint32_t frames = 0, errors = 0;
  bool available = false, skipped = false;
};
Result results[benchmark::SceneCount];
Scene scene = benchmark::Fill;
bool ready = false, paused = false, summary = false, warming = true;
bool requestDma = true, dmaEnabled = false, touchDown = false, touchReady = false;
uint16_t touchX = 0, touchY = 0;
uint32_t phaseAt = 0, sceneAt = 0, summaryAt = 0, lastHudAt = 0;
uint32_t measureAtUs = 0, lastCompleteUs = 0, frame = 0;
uint32_t heapMinimum = UINT32_MAX, warmupErrors = 0;
int contentHeight = 0;
uint8_t alignmentX = 1, alignmentY = 1;
bool bufferedPrimitives = false;

uint32_t profileClock() { return uint32_t(platform::micros()); }
bool uiScene() { return scene >= benchmark::Widgets; }
const char *submissionMode() {
  if (uiScene()) return gfx.asynchronous() ? "UI-ASYNC" : "UI-SYNC";
  if (scene == benchmark::Sprite) return dmaEnabled ? "DMA" : "SYNC";
  return bufferedPrimitives ? "BUFFERED" : "DIRECT";
}
bool good(Status status) { return status == Status::Ok || status == Status::Busy; }
uint32_t freeHeap() { return platform::freeHeap(); }
uint32_t freePsram() { return platform::freePsram(); }

Status submitRaster(int x, int y, int width, int height) {
  const auto *data = static_cast<const uint8_t *>(raster.getPointer()) +
                     2 * (size_t(y) * gfx.width() + x);
  const nice_gfx::PixelView view(data, width, height, gfx.width(), nice_gfx::PixelOrder::Wire);
  const Status submitted = gfx.flush(x, y, view, false);
  const Status drained = gfx.wait();
  return submitted == Status::Ok ? drained : submitted;
}

void textLine(const char *text, int y, uint16_t color = GFX_WHITE) {
  NICE_GFX &target = bufferedPrimitives ? static_cast<NICE_GFX &>(raster) : gfx;
  target.setFont(nice_gfx::fonts::Fixed6x13);
  target.setTextSize(1);
  target.setTextDatum(TL_DATUM);
  target.setTextColor(color, GFX_BLACK);
  target.drawString(text, 3, y);
}

void hud() {
  // Only called between drained frames. UI may repaint its page background,
  // so redraw after a full-page render as well as the regular 500ms interval.
  gfx.wait();
  gfx.resetViewport();
  if (bufferedPrimitives) {
    raster.resetViewport();
    raster.fillRect(0, contentHeight, gfx.width(), kHudHeight, GFX_BLACK);
  } else gfx.fillRect(0, contentHeight, gfx.width(), kHudHeight, GFX_BLACK);
  const uint32_t wall = warming ? 0 : uint32_t(platform::micros()) - measureAtUs;
  const uint32_t heap = freeHeap(), psram = freePsram();
  heapMinimum = std::min(heapMinimum, heap);
  char line[100];
  int y = contentHeight;
  std::snprintf(line, sizeof(line), "FPS %6.1f  frames %lu err %lu", stats.fps(wall),
    static_cast<unsigned long>(stats.frames), static_cast<unsigned long>(stats.errors + warmupErrors));
  textLine(line, y); y += 13;
  std::snprintf(line, sizeof(line), "ms avg %.1f p95 %.1f max %.1f", stats.meanMs(), stats.p95Us()/1000.0, stats.maxUs/1000.0);
  textLine(line, y); y += 13;
  std::snprintf(line, sizeof(line), "work draw %.2f io+other %.2f ms", stats.drawMs(), stats.ioMs());
  textLine(line, y); y += 13;
  if (heap == UINT32_MAX) std::snprintf(line, sizeof(line), "heap N/A  PSRAM N/A");
  else if (psram == UINT32_MAX) std::snprintf(line, sizeof(line), "heap %luk min %luk PSRAM N/A", static_cast<unsigned long>(heap/1024), static_cast<unsigned long>(heapMinimum/1024));
  else std::snprintf(line, sizeof(line), "heap %luk min %luk PS %luk", static_cast<unsigned long>(heap/1024), static_cast<unsigned long>(heapMinimum/1024), static_cast<unsigned long>(psram/1024));
  textLine(line, y); y += 13;
  if (uiScene()) {
    const auto r = ui.runtimeStats();
    std::snprintf(line, sizeof(line), "UI tiles %llu act %lu ev %lu", static_cast<unsigned long long>(r.renderedTiles),
      static_cast<unsigned long>(benchmark_ui::actions()), static_cast<unsigned long>(benchmark_ui::events()));
  } else if (scene == benchmark::Sprite) std::snprintf(line, sizeof(line), "RGB565 %dx%d %s", animation.width(), animation.height(), dmaEnabled ? "async submit" : "sync submit");
  else std::snprintf(line, sizeof(line), "%s", benchmark::sceneNames[scene]);
  textLine(line, y, GFX_CYAN); y += 13;
  std::snprintf(line, sizeof(line), "%u/%u %s %lus %s n/p/r/d", unsigned(scene)+1, unsigned(benchmark::SceneCount),
    paused ? "PAUSE" : warming ? "WARM" : "TEST", static_cast<unsigned long>((platform::millis()-phaseAt)/1000), submissionMode());
  textLine(line, y, GFX_YELLOW);
  if (bufferedPrimitives) submitRaster(0, contentHeight, gfx.width(), kHudHeight);
  gfx.wait();
  lastHudAt = platform::millis();
}

void environment() {
  const auto info = gfx.displayInfo();
  char line[420];
  std::snprintf(line, sizeof(line), "BENCH_ENV version=%s controller=0x%lx size=%ux%u content=%u rotation=%u bus_hz=%lu lanes=%u async_capable=%u sprite_async_requested=%u tile_pixels=2048 warmup_ms=%lu measure_ms=%lu primitive_mode=%s alignment=%ux%u",
    info.version, static_cast<unsigned long>(info.controllerId), info.width, info.height, unsigned(contentHeight), info.rotation,
    static_cast<unsigned long>(info.bus.frequency), info.bus.lanes, info.asynchronous, requestDma,
    static_cast<unsigned long>(kWarmupMs), static_cast<unsigned long>(kMeasureMs),
    bufferedPrimitives ? "BUFFERED" : "DIRECT", alignmentX, alignmentY);
  platform::console().println(line);
  platform::console().println("BENCH_CSV,scene,status,mode,wall_us,frames,fps,avg_ms,p95_recent_ms,max_ms,draw_ms,io_other_ms,pixels,ui_tiles,ui_bytes,actions,events,observed_errors,heap_min,psram_free,warmup_errors");
}

void showSummary() {
  summary = true; summaryAt = platform::millis();
  gfx.wait(); gfx.resetViewport(); gfx.fillScreen(GFX_BLACK);
  if (bufferedPrimitives) { raster.resetViewport(); raster.fillSprite(GFX_BLACK); }
  textLine("NICE_GFX BENCHMARK", 3, GFX_CYAN);
  textLine("Scene          FPS avg/p95 ms", 20);
  char line[100];
  for (unsigned i = 0; i < benchmark::SceneCount; ++i) {
    const Result &r = results[i];
    if (r.skipped) std::snprintf(line, sizeof(line), "%u %-12.12s SKIP", i+1, benchmark::sceneNames[i]);
    else if (!r.available) std::snprintf(line, sizeof(line), "%u %-12.12s --", i+1, benchmark::sceneNames[i]);
    else std::snprintf(line, sizeof(line), "%u %-12.12s %5.1f %4.1f/%4.1f", i+1, benchmark::sceneNames[i], r.fps, r.mean, r.p95);
    textLine(line, 38+int(i)*15, r.errors ? GFX_ORANGE : GFX_WHITE);
  }
  textLine("P95: recent <=256 frames", 180, GFX_YELLOW);
  textLine("r:run  d:Sprite DMA  n:run", 196);
  textLine("Auto repeat after 12s", 212);
  if (bufferedPrimitives) submitRaster(0, 0, gfx.width(), gfx.height());
  gfx.wait();
}

void beginScene(Scene next) {
  gfx.wait(); gfx.resetViewport(); animation.deleteSprite();
  scene = next; summary = false; warming = true; paused = false; frame = 0;
  touchDown = false; stats = benchmark::Stats{}; warmupErrors = 0; heapMinimum = freeHeap();
  gfx.deInitDMA(); dmaEnabled = scene == benchmark::Sprite && requestDma && gfx.initDMA();
  gfx.fillScreen(GFX_BLACK);
  if (bufferedPrimitives) { raster.resetViewport(); raster.fillSprite(GFX_BLACK); }
  bool ok = true;
  if (uiScene()) {
    ui.setProfilingClock(profileClock);
    ok = benchmark_ui::begin(ui, gfx.width(), gfx.height(), contentHeight);
    if (ok) ok = ui.finish() == Status::Ok;
  } else {
    textLine(benchmark::sceneNames[scene], 4, GFX_CYAN);
    if (bufferedPrimitives) ok = submitRaster(0, 0, gfx.width(), 24) == Status::Ok;
    if (scene == benchmark::Sprite) ok = benchmark::createAnimation(animation, gfx.width(), contentHeight-24) && ok;
  }
  gfx.wait();
  if (!ok) {
    results[scene] = Result{}; results[scene].skipped = true;
    platform::console().print("BENCH_SKIP scene="); platform::console().print(benchmark::sceneNames[scene]);
    platform::console().println(" reason=fixture_or_buffer_unavailable");
    if (unsigned(scene)+1 < benchmark::SceneCount) beginScene(Scene(unsigned(scene)+1));
    else showSummary();
    return;
  }
  ui.resetRuntimeStats(); benchmark_ui::resetCounters();
  sceneAt = phaseAt = platform::millis();
  hud();
}

void finishScene() {
  const uint32_t wall = uint32_t(platform::micros()) - measureAtUs;
  Result &r = results[scene];
  r.available = true; r.skipped = false; r.fps = stats.fps(wall);
  r.mean = stats.meanMs(); r.p95 = stats.p95Us()/1000.0;
  r.frames = stats.frames; r.errors = stats.errors + warmupErrors;
  const auto u = ui.runtimeStats();
  const size_t bytes = uiScene() ? ui.memoryStats().totalBytes() : 0;
  char line[480];
  std::snprintf(line, sizeof(line), "BENCH_CSV,%s,%s,%s,%lu,%lu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%llu,%llu,%lu,%lu,%lu,%lu,%lu,%lu,%lu",
    benchmark::sceneNames[scene], r.errors ? "ERROR" : stats.frames ? "OK" : "NO_FRAMES", submissionMode(),
    static_cast<unsigned long>(wall), static_cast<unsigned long>(stats.frames), r.fps, r.mean, r.p95,
    stats.maxUs/1000.0, stats.drawMs(), stats.ioMs(), static_cast<unsigned long long>(stats.pixels),
    static_cast<unsigned long long>(uiScene() ? u.renderedTiles : 0), static_cast<unsigned long>(bytes),
    static_cast<unsigned long>(uiScene() ? benchmark_ui::actions() : 0), static_cast<unsigned long>(uiScene() ? benchmark_ui::events() : 0),
    static_cast<unsigned long>(stats.errors), static_cast<unsigned long>(heapMinimum), static_cast<unsigned long>(freePsram()), static_cast<unsigned long>(warmupErrors));
  platform::console().println(line);
  if (unsigned(scene)+1 < benchmark::SceneCount) beginScene(Scene(unsigned(scene)+1));
  else showSummary();
}

void touch(uint32_t now) {
  if (!touchReady || !uiScene()) return;
  uint16_t x = 0, y = 0;
  const bool down = gfx.getTouch(&x, &y) && y < contentHeight;
  // Scripted input and physical input must not fight over one pointer.
  if (scene != benchmark::Input) {
    if (down) { touchX = x; touchY = y; }
    if (down || touchDown) benchmark_ui::pointer(ui, touchX, touchY, down, now);
    touchDown = down;
  }
}

void controls() {
  while (platform::console().available()) {
    const int c = platform::console().read();
    if (c == 'r' || c == 'R') { for (auto &r : results) r = Result{}; beginScene(benchmark::Fill); }
    else if (c == 'n' || c == 'N') {
      if (summary) beginScene(benchmark::Fill);
      else {
        platform::console().print("BENCH_ABORT scene="); platform::console().println(benchmark::sceneNames[scene]);
        results[scene] = Result{};
        if (unsigned(scene)+1 < benchmark::SceneCount) beginScene(Scene(unsigned(scene)+1)); else showSummary();
      }
    } else if (c == 'p' || c == 'P') {
      if (!summary) { if (paused) beginScene(scene); else { paused = true; hud(); } }
    } else if (c == 'd' || c == 'D') {
      requestDma = !requestDma;
      platform::console().println(requestDma ? "BENCH_MODE request=DMA" : "BENCH_MODE request=SYNC");
      // This toggle controls the Sprite flush. UI owns its submission policy.
      for (auto &r : results) r = Result{};
      beginScene(benchmark::Fill); environment();
    }
  }
}

void setup() {
  platform::console().begin(115200);
  platform::describe();
  const Status status = gfx.beginChecked();
  if (status != Status::Ok) { platform::console().print("BENCH_FATAL display status="); platform::console().println(unsigned(status)); return; }
  gfx.setRotation(kRotation);
  contentHeight = gfx.height() - kHudHeight;
  if (gfx.width() < 240 || gfx.height() < 240) {
    platform::console().println("BENCH_FATAL requires at least 240x240; change panel/rotation"); return;
  }
  if (const auto *panel = nice_gfx::detail::DisplayAccess::panel(gfx)) {
    alignmentX = panel->windowAlignmentX(); alignmentY = panel->windowAlignmentY();
  }
  bufferedPrimitives = alignmentX > 1 || alignmentY > 1;
  if (bufferedPrimitives) {
    // A controller such as CH13613 cannot accept a one-pixel drawing window.
    // Keep exact primitive pixels in one RGB565 canvas, then submit aligned
    // rectangles. Allocate before initDMA(), so large canvases can use PSRAM.
    if (24 % alignmentY || contentHeight % alignmentY || kHudHeight % alignmentY ||
        gfx.width() % alignmentX || gfx.height() % alignmentY) {
      platform::console().println("BENCH_FATAL content layout does not fit panel alignment"); return;
    }
    raster.setColorDepth(16);
    if (!raster.createSprite(gfx.width(), gfx.height())) {
      platform::console().println("BENCH_FATAL aligned primitives require an RGB565 canvas"); return;
    }
    raster.fillSprite(GFX_BLACK);
  }
  const int backlight = exampleBacklightPin();
  if (backlight >= 0) platform::output(backlight, true);
  if (kAttachTouch) touchReady = exampleAttachTouch(gfx) == Status::Ok;
  platform::console().println("NICE_GFX graphics benchmark; n=next p=pause/resume r=restart d=Sprite DMA/sync");
  beginScene(benchmark::Fill); environment(); ready = true;
}

void loop() {
  if (!ready) { platform::delay(20); return; }
  controls();
  if (summary) {
    if (uint32_t(platform::millis()-summaryAt) >= kSummaryMs) { for (auto &r : results) r = Result{}; beginScene(benchmark::Fill); }
    platform::delay(1); return;
  }
  if (paused) { platform::delay(10); return; }
  const uint32_t now = platform::millis();
  if (warming && uint32_t(now-phaseAt) >= kWarmupMs) {
    stats = benchmark::Stats{};
    if (uiScene()) benchmark_ui::beginMeasurement(ui);
    ui.resetRuntimeStats();
    warming = false; phaseAt = sceneAt = now; frame = 0; heapMinimum = freeHeap();
    measureAtUs = lastCompleteUs = uint32_t(platform::micros());
  }
  if (!warming && uint32_t(now-phaseAt) >= kMeasureMs) { finishScene(); return; }

  const auto before = ui.runtimeStats();
  const uint32_t failures = benchmark_ui::failures();
  const uint32_t started = uint32_t(platform::micros());
  uint32_t draw = 0, io = 0;
  uint64_t pixels = 0;
  bool ok = true, complete = true;
  if (uiScene()) {
    touch(now);
    ok = benchmark_ui::update(ui, unsigned(scene)-unsigned(benchmark::Widgets), now-sceneAt, frame, now);
    const Status step = ui.tick(now);
    ok = good(step) && ok;
    if (ui.refreshing()) ok = ui.finish() == Status::Ok && ok;
    ok = gfx.wait() == Status::Ok && ok;
    const auto after = ui.runtimeStats();
    complete = after.completedFrames > before.completedFrames;
    pixels = after.renderedPixels-before.renderedPixels + after.directPixels-before.directPixels;
    draw = uint32_t(after.renderMicros-before.renderMicros);
    const uint32_t work = uint32_t(platform::micros())-started;
    io = work > draw ? work-draw : 0; // Includes setters, input, submit, wait and bookkeeping.
  } else if (scene == benchmark::Sprite) {
    benchmark::drawAnimation(animation, now-sceneAt);
    const uint32_t rendered = uint32_t(platform::micros()); draw = rendered-started;
    // 16bpp Sprite storage is already MSB-first. Keep it alive through wait().
    const nice_gfx::PixelView view(animation.getPointer(), animation.width(), animation.height(), animation.width(), nice_gfx::PixelOrder::Wire);
    const int x = (gfx.width()-animation.width())/2;
    const int y = 24+(contentHeight-24-animation.height())/2;
    const Status submitted = gfx.flush(x-x%alignmentX, y-y%alignmentY, view, dmaEnabled);
    const Status drained = gfx.wait();
    ok = submitted == Status::Ok && drained == Status::Ok;
    pixels = uint64_t(animation.width()) * animation.height();
    io = uint32_t(platform::micros())-rendered;
  } else if (bufferedPrimitives) {
    raster.setViewport(0, 24, gfx.width(), contentHeight-24);
    benchmark::drawScene(raster, scene, gfx.width(), contentHeight-24, frame);
    raster.resetViewport();
    const uint32_t rendered = uint32_t(platform::micros()); draw = rendered-started;
    ok = submitRaster(0, 24, gfx.width(), contentHeight-24) == Status::Ok;
    pixels = uint64_t(gfx.width()) * (contentHeight-24);
    io = uint32_t(platform::micros())-rendered;
  } else {
    gfx.setViewport(0, 24, gfx.width(), contentHeight-24);
    pixels = benchmark::drawScene(gfx, scene, gfx.width(), contentHeight-24, frame);
    draw = uint32_t(platform::micros())-started;
    // Direct draw timings contain synchronous bus work; this is tail wait only.
    const Status observed = gfx.lastError(), drained = gfx.wait();
    ok = observed == Status::Ok && drained == Status::Ok;
    io = uint32_t(platform::micros())-started-draw;
    gfx.resetViewport();
  }
  const uint32_t ended = uint32_t(platform::micros());
  ok = ok && benchmark_ui::failures() == failures;
  if (!ok) { if (warming) ++warmupErrors; else ++stats.errors; }
  if (complete && ok && !warming) {
    stats.add(ended-lastCompleteUs, ended-started, draw, io, pixels);
    lastCompleteUs = ended;
  }
  ++frame;
  heapMinimum = std::min(heapMinimum, freeHeap());
  // Page transitions can repaint the HUD background; restore between frames.
  if (uint32_t(platform::millis()-lastHudAt) >= 500 || (uiScene() && pixels >= uint64_t(gfx.width())*gfx.height())) hud();
  platform::yield();
}

} // namespace graphics_benchmark
