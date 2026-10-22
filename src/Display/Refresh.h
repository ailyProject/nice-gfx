#pragma once
#include "Panel.h"
#include "DisplayAccess.h"

namespace nice_gfx {

// Fixed storage. Merge when the enclosing rectangle costs no more than the two
// original rectangles plus mergeSlack pixels. At capacity, merge the incoming
// rectangle with the cheapest existing one; invalidation is never discarded.
template<size_t Capacity = 16>
class DirtyRegions {
  static_assert(Capacity > 0, "DirtyRegions requires at least one slot");
 public:
  explicit DirtyRegions(uint32_t mergeSlack = 32) : slack_(mergeSlack) {}
  uint32_t mergeSlack() const { return slack_; }
  size_t size() const { return count_; }
  bool empty() const { return !count_; }
  const DirtyRect &operator[](size_t index) const { return regions_[index]; }
  void clear() { count_ = 0; }
  void add(DirtyRect r, uint16_t width, uint16_t height) {
    if (r.width <= 0 || r.height <= 0 || !width || !height) return;
    const int64_t right = int64_t(r.x) + r.width, bottom = int64_t(r.y) + r.height;
    if (right <= 0 || bottom <= 0 || r.x >= width || r.y >= height) return;
    r.x = r.x < 0 ? 0 : r.x; r.y = r.y < 0 ? 0 : r.y;
    r.width = int32_t((right < width ? right : width) - r.x);
    r.height = int32_t((bottom < height ? bottom : height) - r.y);
    for (size_t i = 0; i < count_;) {
      const DirtyRect joined = unite(r, regions_[i]);
      if (joined.pixels() <= r.pixels() + regions_[i].pixels() + slack_) {
        r = joined; regions_[i] = regions_[--count_]; i = 0;
      } else ++i;
    }
    if (count_ == Capacity) {
      size_t best = 0;
      uint64_t cost = UINT64_MAX;
      for (size_t i = 0; i < count_; ++i) {
        const uint64_t increase = unite(r, regions_[i]).pixels() - regions_[i].pixels();
        if (increase < cost) { best = i; cost = increase; }
      }
      r = unite(r, regions_[best]); regions_[best] = regions_[--count_];
      // Apply the same coalescing rule after the capacity-forced merge.
      add(r, width, height);
      return;
    }
    regions_[count_++] = r;
  }
 private:
  static DirtyRect unite(const DirtyRect &a, const DirtyRect &b) {
    const int32_t left = a.x < b.x ? a.x : b.x, top = a.y < b.y ? a.y : b.y;
    const int32_t right = a.x + a.width > b.x + b.width ? a.x + a.width : b.x + b.width;
    const int32_t bottom = a.y + a.height > b.y + b.height ? a.y + a.height : b.y + b.height;
    return DirtyRect(left, top, right - left, bottom - top);
  }
  DirtyRect regions_[Capacity];
  size_t count_ = 0;
  uint32_t slack_;
};

struct RefreshStats {
  size_t renderedTiles = 0, submittedTiles = 0;
  uint64_t renderedPixels = 0;
  uint64_t submitMicros = 0;
  uint64_t directBlits = 0, directPixels = 0;
  uint64_t directFills = 0, directFillPixels = 0;
};

// Immutable, nonoverlapping pieces of a complete frame. Accelerated page
// slides borrow clipped cached surfaces and paint exposed solid background
// rectangles without a CPU composition buffer.
struct FrameSlice {
  DirtyRect area;
  PixelView pixels{nullptr,0};
  bool solid = false;
  uint16_t color = 0;
};

// Single-task partial refresh independent of a UI framework. The caller owns
// one or two nonoverlapping RGB565 buffers, the panel and renderer context, and keeps
// all alive through finish()/cancel(). The panel is exclusively borrowed from
// start() until completion. The renderer may invalidate the next frame, but
// must not call other scheduler methods or write outside its supplied tile.
//
// With two buffers, step() renders into the free buffer while the previous tile
// transfers, then polls/waits and submits. With one buffer, it waits first.
// At most one tile is rendered per call. start() defaults to Native RGB565;
// a renderer that composes wire-order words may explicitly use PixelOrder::Wire.
// Busy means more work remains; Ok means this frame has completed. Invalidation
// during a frame is retained for the next start(), including overlapping areas.
template<size_t Capacity = 16>
class RefreshScheduler {
 public:
  using Renderer = Status (*)(void *context, const DirtyRect &area,
                              uint16_t *pixels, size_t stridePixels);
  RefreshScheduler(Panel &panel, uint16_t *first, uint16_t *second,
                   size_t bufferPixels, uint32_t mergeSlack = 32)
      : panel_(&panel), capacity_(bufferPixels), pending_(mergeSlack), work_(mergeSlack) {
    buffers_[0] = first; buffers_[1] = second;
  }
  RefreshScheduler(NICE_GFX &display, uint16_t *first, uint16_t *second,
                   size_t bufferPixels, uint32_t mergeSlack = 32)
      : panel_(detail::DisplayAccess::panel(display)), capacity_(bufferPixels),
        pending_(mergeSlack), work_(mergeSlack) {
    buffers_[0] = first; buffers_[1] = second;
  }
  ~RefreshScheduler() { if (active_) cancel(); }
  RefreshScheduler(const RefreshScheduler &) = delete;
  RefreshScheduler &operator=(const RefreshScheduler &) = delete;

  void invalidate(DirtyRect area) {
    if (!panel_ || area.width <= 0 || area.height <= 0) return;
    const int32_t width = panel_->width(), height = panel_->height();
    const int64_t right = int64_t(area.x) + area.width, bottom = int64_t(area.y) + area.height;
    if (right <= 0 || bottom <= 0 || area.x >= width || area.y >= height) return;
    const int32_t ax = panel_->windowAlignmentX(), ay = panel_->windowAlignmentY();
    if (!ax || !ay) return;
    const int32_t left = area.x < 0 ? 0 : area.x, top = area.y < 0 ? 0 : area.y;
    const int32_t endX = int32_t(right < width ? right : width), endY = int32_t(bottom < height ? bottom : height);
    area.x = left - left % ax; area.y = top - top % ay;
    area.width = ((endX + ax - 1) / ax * ax) - area.x;
    area.height = ((endY + ay - 1) / ay * ay) - area.y;
    pending_.add(area, panel_->width(), panel_->height());
  }
  void invalidateAll() {
    if (panel_) invalidate(DirtyRect(0, 0, panel_->width(), panel_->height()));
  }
  // Physical output only: rectangular logical pixels remain the default.
  // The round mask includes a two-pixel margin around the nominal panel circle.
  bool configureCircularClip(bool enabled) {
    if(active_)return false;
    if(circularClip_!=enabled){circularClip_=enabled;invalidateAll();}
    return true;
  }
  bool busy() const { return active_; }
  bool dirty() const { return !pending_.empty(); }
  size_t pendingRegions() const { return pending_.size(); }
  // Read-only snapshot before start(); invalidations remain pending until the
  // normal refresh lifecycle accepts and commits the frame.
  bool pendingRegion(size_t index,DirtyRect &area) const {
    if(index>=pending_.size())return false;area=pending_[index];return true;
  }
  Status status() const { return status_; }
  const RefreshStats &stats() const { return stats_; }
  void setProfilingClock(uint32_t (*clock)()) { profilingClock_ = clock; }

  // Extra pixels worth transferring to avoid another window/submit operation.
  // A measured setupMicros / microsPerPixel is a useful initial estimate.
  // Change only between frames. Pending invalidations are coalesced again;
  // lowering the value cannot split rectangles already merged by an older rule.
  bool configureMergeSlack(uint32_t pixels) {
    if (active_) return false;
    DirtyRegions<Capacity> pending(pixels);
    if (panel_) for (size_t i = 0; i < pending_.size(); ++i)
      pending.add(pending_[i], panel_->width(), panel_->height());
    pending_ = pending;
    work_ = DirtyRegions<Capacity>(pixels);
    return true;
  }
  uint32_t mergeSlack() const { return pending_.mergeSlack(); }

  // Limit each ordinary raster tile without resizing caller-owned storage.
  // A bounded scheduler uses its renderer instead of draining direct surfaces.
  bool configurePixelBudget(size_t pixels) {
    if(active_||(pixels&&panel_&&pixels<size_t(panel_->windowAlignmentX())*panel_->windowAlignmentY()))return false;
    pixelBudget_=pixels;return true;
  }
  size_t pixelBudget() const {return pixelBudget_;}

  // An optional immutable surface is borrowed until the refresh completes or
  // cancel() drains accepted DMA. Only capable rectangular backends use it.
  Status start(Renderer renderer, void *context = nullptr, PixelOrder order = PixelOrder::Native,
               const PixelView *surface = nullptr, const FrameSlice *slices = nullptr, size_t sliceCount = 0) {
    if (active_) return Status::Busy;
    if (!panel_ || !panel_->ready()) return status_ = Status::NotInitialized;
    if (!renderer || !validBuffers()) {panel_->abortFrame();return status_ = Status::InvalidArgument;}
    stats_ = RefreshStats();
    if (pending_.empty()) return status_ = panel_->frameOpen()?panel_->abortFrame():panel_->finish();
    work_ = pending_; pending_.clear();
    renderer_ = renderer; context_ = context;
    order_ = order;
    screenWidth_ = panel_->width(); screenHeight_ = panel_->height();
    rotation_ = panel_->rotation();
    direct_=(surface||slices) && !pixelBudget_ && !circularClip_ && panel_->supportsFrameBlit();
    if(direct_){
      directCount_=0;
      if(surface){
        if(slices||!surface->valid()||surface->width!=screenWidth_||surface->height!=screenHeight_)return fail(Status::InvalidArgument);
        directSlices_[0].area=DirtyRect(0,0,screenWidth_,screenHeight_);directSlices_[0].pixels=*surface;directCount_=1;
      }else{
        if(!sliceCount||sliceCount>6)return fail(Status::InvalidArgument);
        uint64_t covered=0;
        for(size_t i=0;i<sliceCount;++i){
          const auto &s=slices[i];const auto &r=s.area;
          if(r.x<0||r.y<0||r.width<=0||r.height<=0||int64_t(r.x)+r.width>screenWidth_||int64_t(r.y)+r.height>screenHeight_||
             (!s.solid&&(!s.pixels.valid()||s.pixels.width!=size_t(r.width)||s.pixels.height!=size_t(r.height))))return fail(Status::InvalidArgument);
          for(size_t j=0;j<i;++j){const auto &a=directSlices_[j].area;
            if(r.x<a.x+a.width&&r.x+r.width>a.x&&r.y<a.y+a.height&&r.y+r.height>a.y)return fail(Status::InvalidArgument);
          }
          directSlices_[i]=s;covered+=r.pixels();
        }
        if(covered!=uint64_t(screenWidth_)*screenHeight_)return fail(Status::InvalidArgument);
        directCount_=sliceCount;
      }
      // A slide can split an otherwise aligned panel at an odd column/row.
      // Check every actual dirty/slice intersection before submitting any
      // pixels, so an unsupported window or strided transaction falls back
      // as a whole frame rather than failing after a partially drawn slide.
      const int32_t ax=panel_->windowAlignmentX(),ay=panel_->windowAlignmentY();
      for(size_t i=0;direct_&&i<work_.size();++i)for(size_t j=0;direct_&&j<directCount_;++j){
        const auto &dirty=work_[i];const auto &slice=directSlices_[j];const auto &a=slice.area;
        const int32_t left=dirty.x>a.x?dirty.x:a.x,top=dirty.y>a.y?dirty.y:a.y;
        const int32_t right=dirty.x+dirty.width<a.x+a.width?dirty.x+dirty.width:a.x+a.width;
        const int32_t bottom=dirty.y+dirty.height<a.y+a.height?dirty.y+dirty.height:a.y+a.height;
        if(right<=left||bottom<=top)continue;
        if(left%ax||top%ay||(right-left)%ax||(bottom-top)%ay)direct_=false;
        else if(!slice.solid){
          const auto &v=slice.pixels;
          const PixelView pixels(v.data+2*(size_t(top-a.y)*v.stride+size_t(left-a.x)),
                                 size_t(right-left),size_t(bottom-top),v.stride,v.order);
          if(!panel_->canFrameBlit(pixels))direct_=false;
        }
      }
      if(!direct_)clearDirect();
    }
    status_=panel_->beginFrame(&work_[0],work_.size());
    if(status_!=Status::Ok)return fail(status_);
    region_ = 0; directIndex_=0; column_ = row_ = 0;
    prepared_ = inFlight_ = -1; active_ = true;
    return status_ = Status::Ok;
  }
  Status step(bool waitForTransfer = false) {
    if (!active_) return status_;
    if (!panel_->ready()) return fail(Status::NotInitialized);
    if (panel_->width() != screenWidth_ || panel_->height() != screenHeight_ ||
        panel_->rotation() != rotation_) {
      invalidateAll(); return fail(Status::InvalidArgument);
    }
    if(direct_){
      for(;;){
      if(inFlight_>=0){
        if(!waitForTransfer&&panel_->busy())return Status::Busy;
        const Status s=panel_->finish();inFlight_=-1;if(s!=Status::Ok)return fail(s);
      }
      DirtyRect r;const FrameSlice *source=nullptr;
      while(region_<work_.size()){
        const auto &dirty=work_[region_];const auto &slice=directSlices_[directIndex_];const auto &a=slice.area;
        const int32_t left=dirty.x>a.x?dirty.x:a.x,top=dirty.y>a.y?dirty.y:a.y;
        const int32_t right=dirty.x+dirty.width<a.x+a.width?dirty.x+dirty.width:a.x+a.width;
        const int32_t bottom=dirty.y+dirty.height<a.y+a.height?dirty.y+dirty.height:a.y+a.height;
        if(++directIndex_==directCount_){directIndex_=0;++region_;}
        if(right>left&&bottom>top){r=DirtyRect(left,top,right-left,bottom-top);source=&slice;break;}
      }
      if(!source)return completeFrame();
      inFlight_=0;const uint32_t submittedAt=profilingClock_?profilingClock_():0;
      Status s;
      if(source->solid){
        s=panel_->window(r.x,r.y,r.x+r.width-1,r.y+r.height-1);
        if(s==Status::Ok)s=panel_->fill(source->color,uint32_t(r.pixels()));
      }else{
        const auto &v=source->pixels;
        const PixelView pixels(v.data+2*(size_t(r.y-source->area.y)*v.stride+size_t(r.x-source->area.x)),size_t(r.width),size_t(r.height),v.stride,v.order);
        s=panel_->flush(r.x,r.y,pixels,true);
      }
      if(profilingClock_)stats_.submitMicros+=uint32_t(profilingClock_()-submittedAt);
      if(s!=Status::Ok)return fail(s);
      ++stats_.submittedTiles;stats_.directPixels+=r.pixels();
      if(source->solid){++stats_.directFills;stats_.directFillPixels+=r.pixels();}
      else ++stats_.directBlits;
      // Commit the final synchronous slice in this call. CPU RGB has already
      // consumed its source; another application/RTOS tick adds only latency.
      if(!panel_->asynchronous()){
        const Status finished=panel_->finish();inFlight_=-1;
        if(finished!=Status::Ok)return fail(finished);
        if(region_==work_.size())return completeFrame();
        // Synchronous direct sources have no outstanding transfer or renderer
        // work. Drain the remaining prevalidated intersections before returning
        // to the application loop; only the final one may commit this frame.
        continue;
      }
      return Status::Busy;
      }
    }
    // A single-buffer renderer must wait before overwriting pixels that DMA
    // still owns. Two buffers keep rendering overlapped with the transfer.
    if (!buffers_[1] && inFlight_ >= 0) {
      if (!waitForTransfer && panel_->busy()) return Status::Busy;
      Status s = panel_->finish(); inFlight_ = -1;
      if (s != Status::Ok) return fail(s);
    }
    while (prepared_ < 0 && region_ < work_.size()) {
      prepared_ = inFlight_ == 0 ? 1 : 0;
      const DirtyRect &r = work_[region_];
      const size_t ax = panel_->windowAlignmentX(), ay = panel_->windowAlignmentY();
      const size_t remainingWidth = size_t(r.width - column_);
      // Reserve at least one complete aligned row group. Partial columns must
      // all use the same height, including the narrower final column.
      const size_t workCapacity=pixelBudget_&&pixelBudget_<capacity_?pixelBudget_:capacity_;
      const size_t maxWidth = workCapacity / ay;
      size_t w = remainingWidth < maxWidth ? remainingWidth : maxWidth;
      w -= w % ax;
      const size_t remainingHeight = size_t(r.height - row_);
      size_t h = size_t(r.width) <= maxWidth ? workCapacity / w : ay;
      h -= h % ay;
      if (h > remainingHeight) h = remainingHeight;
      preparedArea_ = DirtyRect(r.x + column_, r.y + row_, int32_t(w), int32_t(h));
      if(!circularClip_||clipCircleBand(preparedArea_)) {
        Status s = renderer_(context_, preparedArea_, buffers_[prepared_], size_t(preparedArea_.width));
        if (s != Status::Ok) return fail(s);
        ++stats_.renderedTiles; stats_.renderedPixels += preparedArea_.pixels();
      } else prepared_=-1;
      // Advance by the original rectangular work tile, never its cropped
      // width. Skipped corners cannot leave rows/columns unfinished.
      column_ += int32_t(w);
      if (column_ == r.width) { column_ = 0; row_ += int32_t(h); }
      if (row_ == r.height) { ++region_; row_ = column_ = 0; }
    }
    if (inFlight_ >= 0) {
      if (!waitForTransfer && panel_->busy()) return Status::Busy;
      Status s = panel_->finish(); inFlight_ = -1;
      if (s != Status::Ok) return fail(s);
    }
    if (prepared_ >= 0) {
      const DirtyRect &r = preparedArea_;
      // Claim the buffer before submission: even a failed submit may have
      // accepted descriptors. fail() drains them before making buffers free.
      inFlight_ = prepared_; prepared_ = -1;
      const uint32_t submittedAt=profilingClock_?profilingClock_():0;
      Status s = panel_->flush(r.x, r.y,
          PixelView(buffers_[inFlight_], size_t(r.width), size_t(r.height), 0, order_), true);
      if(profilingClock_)stats_.submitMicros+=uint32_t(profilingClock_()-submittedAt);
      if (s != Status::Ok) return fail(s);
      ++stats_.submittedTiles;
    }
    if (region_ == work_.size() && inFlight_ < 0) {
      return completeFrame();
    }
    return Status::Busy;
  }
  Status finish() {
    while (active_) {
      Status s = step(true);
      if (s != Status::Busy) return s;
    }
    return status_;
  }
  // Cancel drains accepted transfers and retains the entire frame as dirty.
  Status cancel() {
    if (!active_) return status_;
    const Status s = panel_->abortFrame();
    requeue(); active_ = false; prepared_ = inFlight_ = -1;clearDirect();
    return status_ = s;
  }
 private:
  template<class UInt> static uint32_t integerRoot(UInt value) {
    UInt result=0,bit=UInt(1)<<(sizeof(UInt)*8-2);
    while(bit>value)bit>>=2;
    while(bit){if(value>=result+bit){value-=result+bit;result=(result>>1)+bit;}else result>>=1;bit>>=2;}
    return uint32_t(result);
  }
  bool clipCircleBand(DirtyRect &r) const {
    // Doubled pixel-center coordinates avoid half-pixel ambiguity on even
    // displays. The band's closest row has its widest visible horizontal span.
    const int32_t centerX=int32_t(screenWidth_)-1,centerY=int32_t(screenHeight_)-1;
    const int32_t nearest=centerY/2<r.y?r.y:centerY/2>=r.y+r.height?r.y+r.height-1:centerY/2;
    const int32_t delta=2*nearest-centerY;
    const uint32_t distance=uint32_t(delta<0?-delta:delta),radius=uint32_t(screenWidth_<screenHeight_?screenWidth_:screenHeight_)+4;
    if(distance>radius)return false;
    // Ready panels are bounded by INT16_MAX, so radius <= 32771 and its
    // square fits uint32_t even with the circular clip margin.
    const uint32_t extent=integerRoot(uint32_t(radius*radius-distance*distance));
    // ceil((centerX-extent)/2), including negative numerators, then floor of
    // the nonnegative right edge. Clamp to the original dirty rectangle.
    const int32_t lower=centerX-int32_t(extent),upper=centerX+int32_t(extent);
    const int32_t left=lower>=0?(lower+1)/2:lower/2,right=upper/2+1;
    // Hardware addressing may round an unaligned endpoint independently of
    // the raster buffer. Expand on its grid before rendering, so the window,
    // packed row stride and transmitted pixel count describe the same image.
    const int32_t alignment=panel_->windowAlignmentX();
    const int32_t alignedLeft=left>=0?left-left%alignment:left;
    const int32_t alignedRight=(right+alignment-1)/alignment*alignment;
    const int32_t begin=r.x>alignedLeft?r.x:alignedLeft,end=r.x+r.width<alignedRight?r.x+r.width:alignedRight;
    if(end<=begin)return false;r.x=begin;r.width=end-begin;return true;
  }
  bool validBuffers() const {
    if (!buffers_[0] || !capacity_ || capacity_ > SIZE_MAX / sizeof(uint16_t))
      return false;
    if (capacity_ < size_t(panel_->windowAlignmentX()) * panel_->windowAlignmentY()) return false;
    if (pixelBudget_ && pixelBudget_ < size_t(panel_->windowAlignmentX()) * panel_->windowAlignmentY()) return false;
    const uintptr_t a = reinterpret_cast<uintptr_t>(buffers_[0]);
    const uintptr_t b = reinterpret_cast<uintptr_t>(buffers_[1]);
    const size_t bytes = capacity_ * sizeof(uint16_t);
    if ((a % alignof(uint16_t)) || a > UINTPTR_MAX - bytes) return false;
    if (!buffers_[1]) return true;
    if ((b % alignof(uint16_t)) || b > UINTPTR_MAX - bytes) return false;
    return a < b ? a + bytes <= b : b + bytes <= a;
  }
  void requeue() {
    for (size_t i = 0; i < work_.size(); ++i)
      pending_.add(work_[i], panel_->width(), panel_->height());
    work_.clear();
  }
  Status fail(Status error) {
    panel_->abortFrame(); // Drain accepted transfers without publishing partial pixels.
    requeue(); active_ = false; prepared_ = inFlight_ = -1;clearDirect();
    return status_ = error;
  }
  Status completeFrame() {
    const uint32_t submittedAt=profilingClock_?profilingClock_():0;
    const Status s=panel_->commitFrame();
    if(profilingClock_)stats_.submitMicros+=uint32_t(profilingClock_()-submittedAt);
    if(s!=Status::Ok)return fail(s);
    active_=false;prepared_=inFlight_=-1;clearDirect();work_.clear();return status_=Status::Ok;
  }
  Panel *panel_;
  uint16_t *buffers_[2];
  size_t capacity_;
  size_t pixelBudget_=0;
  DirtyRegions<Capacity> pending_, work_;
  Renderer renderer_ = nullptr;
  uint32_t (*profilingClock_)() = nullptr;
  void *context_ = nullptr;
  DirtyRect preparedArea_;
  RefreshStats stats_;
  size_t region_ = 0;
  int32_t column_ = 0, row_ = 0;
  uint16_t screenWidth_ = 0, screenHeight_ = 0;
  uint8_t rotation_ = 0;
  int8_t prepared_ = -1, inFlight_ = -1;
  bool active_ = false,circularClip_ = false;
  bool direct_ = false;
  FrameSlice directSlices_[6];
  size_t directCount_=0,directIndex_=0;
  void clearDirect(){direct_=false;directCount_=directIndex_=0;for(auto &s:directSlices_)s=FrameSlice{};}
  Status status_ = Status::Ok;
  PixelOrder order_ = PixelOrder::Native;
};

} // namespace nice_gfx

using nice_gfx::DirtyRect;
using nice_gfx::RefreshScheduler;
