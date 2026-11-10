#pragma once
#include "SystemModalRleRows.h"
#include "../../Platform/ESP32/SystemModalRowWorker.h"

namespace nice_gfx { namespace htmlui { namespace detail {

struct SystemModalRleRowsWork {
  SystemModalRleRowsInput input;
  SystemModalRleRowsResult result;
  rle565::ValidatedSeekCursor seed;
};
inline void systemModalRleRowsEntry(void *context)noexcept{
  auto &work=*static_cast<SystemModalRleRowsWork*>(context);
  work.result=drawSystemModalRleRows(work.input,&work.seed);
}

// Called only by the UI owner after every original preflight and full-stream
// validation. A false result means no worker owns the request and no output
// was written, so the original serial implementation remains available.
// Both cursors are trusted results for this same immutable stream. The owner
// binds the lower cursor to its actual ROI before this midpoint selection.
inline bool tryDrawSystemModalRleParallel(const SystemModalRleRowsInput &input,bool filledDominant,
                                          SystemModalRleRowsResult &result,
                                          const rle565::ValidatedSeekCursor *seed=nullptr,
                                          const rle565::ValidatedSeekCursor *lowerSeed=nullptr){
#if NICE_GFX_DETAIL_SYSTEM_MODAL_ROW_WORKER
  if(filledDominant||!input.encoded||!input.rawWire||!input.output||!input.lut||
     input.roi.x||input.roi.width<=0||size_t(input.roi.width)!=input.width||
     input.outputStride!=input.width||(input.width%32)||
     (reinterpret_cast<uintptr_t>(input.output)&63u)||input.roi.height<32||
     uint64_t(input.roi.width)*uint64_t(input.roi.height)<32768)return false;
  // Whole rows and a 64-byte-aligned full stride isolate the two writers.
  const int32_t upperRows=input.roi.height/2+input.roi.height%2;
  const auto upperSeed=seed?*seed:rle565::ValidatedSeekCursor{};
  const size_t lowerPosition=size_t(input.roi.y+upperRows)*input.width;
  const auto workerSeed=lowerSeed&&lowerSeed->position==lowerPosition?*lowerSeed:upperSeed;
  auto upper=input;upper.roi.height=upperRows;
  SystemModalRleRowsWork lower{input,{},workerSeed};
  lower.input.roi.y+=upperRows;lower.input.roi.height-=upperRows;
  auto ticket=platform::system_modal_row_worker::tryBegin(systemModalRleRowsEntry,&lower);
  if(!ticket)return false;
  const auto first=drawSystemModalRleRows(upper,&upperSeed);
  ticket.join();
  result.cursorWords=first.cursorWords+lower.result.cursorWords;
  result.rawPixels=first.rawPixels+lower.result.rawPixels;
  result.rlePixels=first.rlePixels+lower.result.rlePixels;
  result.drawnRuns=first.drawnRuns+lower.result.drawnRuns;
  return true;
#else
  (void)input;(void)filledDominant;(void)result;(void)seed;(void)lowerSeed;return false;
#endif
}

}}}
