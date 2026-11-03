# CI13XX platform support

CI130X uses the common [Arduino bus bridge](../Arduino/Bus.cpp) for display transactions, pin routing, pixel encoding and fallback transfers. The ChipIntelli-specific implementations are organized here:

| File | Responsibility |
| --- | --- |
| [Gpio.h](Gpio.h) | GPIO `DATA[mask]` register addressing, preserving unrelated port bits. Used by the Arduino SPI fast path and [FastPin](../Arduino/FastPin.h). |
| [SpiWrite.h](SpiWrite.h) | MODE0/MSB-first bulk-write algorithm, wrapping cycle timing and idle-low yields every 64 bytes. Its IO interface also supports host waveform tests. |
| [ArduinoSpiFast.h](ArduinoSpiFast.h) | Connects that algorithm to CI13XX GPIO, `mcycle`, SDK clock information and FreeRTOS. The Arduino bridge invokes it inside the core SPI transaction. |

The default `NICE_GFX_CI13XX_FAST_SPI=1` enables the fast writer on CI13XX RISC-V targets for MODE0, write-only transfers in a normal running task with an enabled cycle counter. Other cases retain the core transfer path. Setting this macro to `0` through the library's global C++ build flags disables only the bulk fast path; FastPin's masked GPIO writes remain available.

The current arduino-ci130x core exposes GPIO software SPI with a maximum requested frequency of 4 MHz. This directory does not introduce an independent hardware SPI controller or DMA backend. Initialization, peripheral ownership and transaction locking remain with the Arduino core.

See [CI130X usage and hardware results](../../../docs/CI130X.md) and [host behavior/waveform tests](../../../tests/ci13xx/README.md).
