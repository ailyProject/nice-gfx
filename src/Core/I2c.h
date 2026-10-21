#pragma once
#include "Status.h"
#include <stddef.h>
#include <stdint.h>
namespace nice_gfx {
class I2cBus {
 public:
  virtual ~I2cBus() {}
  virtual Status begin(uint8_t address) = 0;
  virtual Status transfer(uint8_t address, const uint8_t *tx, size_t txSize,
                          uint8_t *rx, size_t rxSize, bool stop = true) = 0;
  virtual Status reset(uint16_t, uint16_t, int = -1) { return Status::Ok; }
  virtual void delayMs(uint16_t) {}
  virtual void delayUs(uint16_t) {}
};

}
