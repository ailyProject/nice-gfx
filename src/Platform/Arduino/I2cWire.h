#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "../../Core/I2c.h"
namespace nice_gfx {
// Arduino transport only; chip protocols and board pin assignments live elsewhere.
class I2cWire : public I2cBus {
 public:
  explicit I2cWire(TwoWire &wire,bool combinedReads=false):wire_(wire),combinedReads_(combinedReads){}
  Status begin(uint8_t address) override {return address>=8&&address<=0x77?Status::Ok:Status::InvalidArgument;}
  Status transfer(uint8_t address, const uint8_t *tx, size_t nt,
                  uint8_t *rx, size_t nr, bool stop = true) override {
    if (address < 8 || address > 0x77 || (nt && !tx) || (nr && !rx) || nr > 255)
      return Status::InvalidArgument;
    if (nt) {
      wire_.beginTransmission(address);
      const size_t written = wire_.write(tx, nt);
      // GT1151 register reads may opt into Wire's repeated-start transaction.
      // Keep writes/ACKs and short writes terminated; Wire owns its bus lock.
      const bool readStop = stop && !(combinedReads_ && nt == 2);
      const uint8_t error = wire_.endTransmission(nr && written == nt ? readStop : true);
      if (written != nt || error) return Status::IoError;
    }
    if (nr) {
      const size_t received = wire_.requestFrom(address, uint8_t(nr));
      if (received != nr) { while (wire_.available()) wire_.read(); return Status::IoError; }
      for (size_t i = 0; i < nr; ++i) {
        const int value = wire_.read();
        if (value < 0) return Status::IoError;
        rx[i] = uint8_t(value);
      }
    }
    return Status::Ok;
  }

  void delayMs(uint16_t ms) override {delay(ms);}
  void delayUs(uint16_t us) override {delayMicroseconds(us);}
 protected:
  TwoWire &wire_;bool combinedReads_;
};
}
