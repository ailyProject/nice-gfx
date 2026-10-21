#pragma once
#include "../Arduino/Bus.h"
#include "../Common/BufferedDmaBus.h"
#if defined(ARDUINO_ARCH_STM32) && !defined(USE_HALV2_DRIVER)
#if defined(STM32F0xx) || defined(STM32F1xx) || defined(STM32F2xx) || defined(STM32F3xx) || defined(STM32F4xx) || defined(STM32F7xx) || defined(STM32G0xx) || defined(STM32G4xx) || defined(STM32L0xx) || defined(STM32L1xx) || defined(STM32L4xx)
#define GFX_RUNTIME_STM32_DMA_AVAILABLE 1
#endif
namespace nice_gfx {
struct Stm32SpiStorage { SPIClass ownedSpi; };
// Uses the Arduino core's real SPI handle. DMA is polled, so the library neither
// installs global IRQ handlers nor steals an application's interrupt callbacks.
// DMA is optional: an already configured channel or request route is left alone
// and the instance continues through the core's synchronous bulk SPI backend.
class Stm32Spi : private Stm32SpiStorage, public BufferedDmaBus<ArduinoSpi> {
 public:
  explicit Stm32Spi(SpiPins pins) : BufferedDmaBus<ArduinoSpi>(ownedSpi,pins),spi_(ownedSpi) {}
  explicit Stm32Spi(const SpiConfig &config) : BufferedDmaBus<ArduinoSpi>(ownedSpi,config),spi_(ownedSpi) {}
  Stm32Spi(SPIClass &spi,SpiPins pins) : BufferedDmaBus<ArduinoSpi>(spi,pins), spi_(spi) {}
  Stm32Spi(SPIClass &spi,const SpiConfig &config) : BufferedDmaBus<ArduinoSpi>(spi,config), spi_(spi) {}
  ~Stm32Spi() override { end(); }
  Status begin() override;
  void end() override;
 protected:
  Status dmaWrite(const uint8_t *data,size_t bytes) override;
  Status dmaPoll() override;
  bool dmaDirect(const uint8_t *data) const override;
 private:
  SPIClass &spi_;
#if defined(GFX_RUNTIME_STM32_DMA_AVAILABLE)
  DMA_HandleTypeDef dma_={};
  uint32_t started_=0;
  int slot_=-1;
  bool active_=false;
#endif
};
}
#endif
