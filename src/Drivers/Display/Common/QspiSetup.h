#pragma once
#include <stdint.h>
namespace nice_gfx { namespace controllers {
template<class Config> inline void configureGramQspi(Config &config,uint8_t readOpcode=0x0B,uint8_t mode=0,uint8_t quantum=1,bool split=false) {
  config.quad=true; config.mode=mode;
  config.protocol.commandOpcode=0x02; config.protocol.pixelOpcode=0x32;
  config.protocol.readOpcode=readOpcode; config.protocol.pixelContinuation=true;
  config.protocol.continuationCommand=0x3C;
  config.protocol.pixelQuantum=quantum; config.protocol.splitPixelTransactions=split;
}
} }
