#pragma once
#include "../../Core/Status.h"
#include <stdint.h>

namespace nice_gfx {

// A transaction belongs to one controller. Shared display buses must complete
// outstanding DMA before select(); implementations can do so in select().
class TouchBus {
 public:
  virtual ~TouchBus() {}
  virtual Status begin() = 0;
  virtual Status select() = 0;
  virtual uint8_t transfer(uint8_t value) = 0;
  virtual void deselect() = 0;
  virtual bool pressed() const { return true; } // Optional active-low PENIRQ.
 protected:
  TouchBus() {}
 private:
  TouchBus(const TouchBus &) = delete;
  TouchBus &operator=(const TouchBus &) = delete;
};

} // namespace nice_gfx
