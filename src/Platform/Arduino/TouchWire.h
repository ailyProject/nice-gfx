#pragma once
#include "I2cWire.h"
#include "../../Drivers/Touch/TouchI2c.h"
namespace nice_gfx {
class TouchWire : public I2cWire {
 public:
  explicit TouchWire(TwoWire &wire,int reset=-1,int irq=-1,bool combinedReads=false)
    :I2cWire(wire,combinedReads),reset_(reset),irq_(irq){}
  Status begin(uint8_t address) override {
    if(reset_<-1||irq_<-1||(reset_>=0&&reset_==irq_))return Status::InvalidArgument;
    if(irq_>=0)pinMode(irq_,INPUT_PULLUP);
    return I2cWire::begin(address);
  }
  Status reset(uint16_t activeMs, uint16_t settleMs, int select = -1) override {
    if (reset_ < 0) return Status::Ok; // Board reset/expander managed by sketch.
    pinMode(reset_, OUTPUT); digitalWrite(reset_, LOW);
    if (select >= 0 && irq_ >= 0) { pinMode(irq_, OUTPUT); digitalWrite(irq_, select ? HIGH : LOW); }
    delay(activeMs); digitalWrite(reset_, HIGH);
    if (select >= 0 && irq_ >= 0) { delay(6); digitalWrite(irq_, LOW); delay(50); pinMode(irq_, INPUT_PULLUP); }
    delay(settleMs); return Status::Ok;
  }

 private:int reset_,irq_;
};
}
