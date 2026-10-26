#pragma once
#include <stdint.h>

namespace nice_gfx {
struct SpiPins {
  int clock, mosi, miso, cs, dc;
  SpiPins(int clk, int out, int in, int select, int dataCommand)
      : clock(clk), mosi(out), miso(in), cs(select), dc(dataCommand) {}
};
struct SpiConfig {
  SpiPins pins;
  uint32_t frequency = 40000000, readFrequency = 10000000;
  uint8_t mode = 0;
  bool paddedCommands = false;
  // Buffered monochrome controllers consume only command/data bytes. Platforms
  // may omit RGB DMA workers/staging for this transport preference.
  bool byteOnly = false;
  int writeStrobePin = -1;
  explicit SpiConfig(SpiPins p) : pins(p) {}
};
struct ParallelPins {
  int data[16];
  int write, read, cs, dc;
  ParallelPins(int wr, int rd, int select, int dataCommand,
      int d0, int d1, int d2, int d3, int d4, int d5, int d6, int d7,
      int d8=-1, int d9=-1, int d10=-1, int d11=-1, int d12=-1, int d13=-1, int d14=-1, int d15=-1)
      : data{d0,d1,d2,d3,d4,d5,d6,d7,d8,d9,d10,d11,d12,d13,d14,d15},
        write(wr), read(rd), cs(select), dc(dataCommand) {}
};
struct ParallelConfig {
  ParallelPins pins;
  uint32_t frequency = 20000000;
  uint16_t writeDelayUs = 0, readDelayUs = 1;
  explicit ParallelConfig(ParallelPins p) : pins(p) {}
};
}
