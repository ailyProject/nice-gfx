#pragma once
#include <algorithm>

namespace fieldlab {
uint32_t profileMicros(){return micros();}
struct PerfProbe {
  bool active=false,measuring=false;
  uint32_t phaseAt=0,startUs=0,lastUs=0,frames=0,errors=0,maxFrame=0,maxLoop=0;
  uint32_t intervals[256]={};size_t count=0,cursor=0;
  RuntimeStats before{},last{};
} perf;

void startPerf(){
  perf=PerfProbe{};perf.active=true;perf.phaseAt=millis();
  ui.setProfilingClock(profileMicros);
  Serial.print("WIO_PERF_BEGIN route=");Serial.println(ui.path());
}
void samplePerf(uint32_t loopUs){
  if(!perf.active)return;
  const uint32_t now=millis();const auto stats=ui.runtimeStats();
  if(!perf.measuring){
    if(uint32_t(now-perf.phaseAt)<750)return;
    perf.measuring=true;perf.phaseAt=now;perf.startUs=perf.lastUs=micros();perf.before=perf.last=stats;
  }
  perf.maxLoop=std::max(perf.maxLoop,loopUs);
  if(stats.completedFrames!=perf.last.completedFrames){
    uint32_t completed=micros(),interval=completed-perf.lastUs;perf.lastUs=completed;
    perf.frames+=uint32_t(stats.completedFrames-perf.last.completedFrames);
    perf.intervals[perf.cursor]=interval;perf.cursor=(perf.cursor+1)%256;
    perf.count=std::min(perf.count+1,size_t(256));perf.maxFrame=std::max(perf.maxFrame,interval);
  }
  perf.last=stats;
  if(uint32_t(now-perf.phaseAt)<6000)return;
  const uint32_t wall=micros()-perf.startUs;
  std::sort(perf.intervals,perf.intervals+perf.count);
  const uint32_t p95=perf.count?perf.intervals[(perf.count*95+99)/100-1]:0;
  const auto mem=ui.memoryStats();
  Serial.print("WIO_PERF route=");Serial.print(ui.path());
  Serial.print(" frames=");Serial.print(perf.frames);Serial.print(" wall_us=");Serial.print(wall);
  Serial.print(" fps=");Serial.print(double(perf.frames)*1e6/wall,3);
  Serial.print(" p95_us=");Serial.print(p95);Serial.print(" max_frame_us=");Serial.print(perf.maxFrame);
  Serial.print(" render_us=");Serial.print(uint32_t(stats.renderMicros-perf.before.renderMicros));
  Serial.print(" submit_us=");Serial.print(uint32_t(stats.submitMicros-perf.before.submitMicros));
  Serial.print(" pixels=");Serial.print(uint32_t(stats.renderedPixels-perf.before.renderedPixels));
  Serial.print(" tiles=");Serial.print(uint32_t(stats.renderedTiles-perf.before.renderedTiles));
  Serial.print(" max_loop_us=");Serial.print(perf.maxLoop);
  Serial.print(" ui_bytes=");Serial.print(mem.totalBytes());
  Serial.print(" tile_pixels=");Serial.print(kAppTilePixels);
  const auto heap=mallinfo();
  Serial.print(" libc_used=");Serial.print(heap.uordblks);
  Serial.print(" libc_free=");Serial.print(heap.fordblks);
#if WIO_ENABLE_WIFI
  Serial.print(" rtos_free=");Serial.print(xPortGetFreeHeapSize());
  Serial.print(" stack_free=");Serial.print(uxTaskGetStackHighWaterMark(nullptr)*sizeof(StackType_t));
#endif
  Serial.print(" errors=");Serial.println(perf.errors);
  perf.active=false;ui.setProfilingClock(nullptr);
}
}
