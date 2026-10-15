#pragma once
#include <stdint.h>
namespace nice_gfx {
// Register commands use one lane; pixels use four lanes. All imported GRAM
// controllers have the 4-byte [opcode,0,register,0] header. CH13613 defaults
// remain unchanged; new profiles explicitly enable RAMWRC continuation.
struct QspiProtocol {
  uint8_t commandOpcode = 0x02, pixelOpcode = 0x32, readOpcode = 0x0B;
  uint8_t continuationCommand = 0x3C;
  bool pixelContinuation = false;
  // Required pixels per RAMWR/RAMWRC transaction, from the chip datasheet.
  uint8_t pixelQuantum = 1;
  // SPD2010 requires RAMWR then RAMWRC transactions for a whole frame.
  bool splitPixelTransactions = false;
  uint32_t address(uint16_t command) const { return uint32_t(command & 0xFF) << 8; }
  uint8_t pixelCommand(uint16_t command, uint8_t bits) const {
    return bits ? uint8_t(command) : continuationCommand;
  }
};
} // namespace nice_gfx
