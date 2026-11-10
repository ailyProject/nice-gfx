#pragma once
#include "Transport.h"

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

} // namespace nice_gfx
