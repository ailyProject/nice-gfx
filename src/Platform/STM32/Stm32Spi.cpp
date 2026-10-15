#if defined(ARDUINO_ARCH_STM32)
#include "Stm32Spi.h"
#if defined(ARDUINO_ARCH_STM32) && !defined(USE_HALV2_DRIVER)
namespace nice_gfx {
#if defined(GFX_RUNTIME_STM32_DMA_AVAILABLE)
static Stm32Spi *stmDmaOwners[3]={};
#endif
Status Stm32Spi::begin() {
  if (this->begun_) return Status::Ok;
  Status status=ArduinoSpi::begin(); if (status!=Status::Ok) return status;
#if defined(GFX_RUNTIME_STM32_DMA_AVAILABLE)
  SPI_TypeDef *instance=spi_.getHandle()->Instance;
  int selected=-1;
  if (instance==SPI1) selected=0;
#if defined(SPI2)
  else if (instance==SPI2) selected=1;
#endif
#if defined(SPI3)
  else if (instance==SPI3) selected=2;
#endif
  if (selected < 0) return Status::Ok; // Core SPI remains available on unhandled ports.
  if (stmDmaOwners[selected]) { ArduinoSpi::end(); return Status::Busy; }
  // Do not replace a handle linked by the application's SPI MSP callback.
  if (spi_.getHandle()->hdmatx || spi_.getHandle()->hdmarx) return Status::Ok;
#if defined(STM32F2xx) || defined(STM32F4xx) || defined(STM32F7xx)
  __HAL_RCC_DMA1_CLK_ENABLE(); __HAL_RCC_DMA2_CLK_ENABLE();
  if (selected==0) { dma_.Instance=DMA2_Stream3; dma_.Init.Channel=DMA_CHANNEL_3; }
  else if (selected==1) { dma_.Instance=DMA1_Stream4; dma_.Init.Channel=DMA_CHANNEL_0; }
  else { dma_.Instance=DMA1_Stream5; dma_.Init.Channel=DMA_CHANNEL_0; }
  // EN alone does not establish ownership: applications often initialize their
  // channels once and leave them disabled between transfers. Only claim reset
  // resources, including the stream's reset FIFO configuration (FS is read-only).
  if (dma_.Instance->CR || dma_.Instance->NDTR || dma_.Instance->PAR ||
      dma_.Instance->M0AR || dma_.Instance->M1AR ||
      (dma_.Instance->FCR & ~DMA_SxFCR_FS) != (0x21u & ~DMA_SxFCR_FS)) return Status::Ok;
  dma_.Init.FIFOMode=DMA_FIFOMODE_DISABLE;
  dma_.Init.FIFOThreshold=DMA_FIFO_THRESHOLD_FULL;
  dma_.Init.MemBurst=DMA_MBURST_SINGLE; dma_.Init.PeriphBurst=DMA_PBURST_SINGLE;
#else
  if (selected==2) return Status::Ok;
  __HAL_RCC_DMA1_CLK_ENABLE();
  dma_.Instance=DMA1_Channel3;
  if (selected==1) {
#if defined(DMA1_Channel5)
    dma_.Instance=DMA1_Channel5;
#else
    return Status::Ok;
#endif
  }
  if (dma_.Instance->CCR || dma_.Instance->CNDTR || dma_.Instance->CPAR || dma_.Instance->CMAR) return Status::Ok;
#if defined(STM32F0xx) && defined(SYSCFG_CFGR1_SPI2_DMA_RMP)
  if (selected==1 && (SYSCFG->CFGR1 & SYSCFG_CFGR1_SPI2_DMA_RMP)) return Status::Ok;
#endif
#if defined(STM32F3xx) && defined(SYSCFG_CFGR3_SPI1_TX_DMA_RMP)
  if (selected==0 && (SYSCFG->CFGR3 & SYSCFG_CFGR3_SPI1_TX_DMA_RMP)) return Status::Ok;
#endif
#if defined(STM32F0xx) && defined(DMA1_CSELR_DEFAULT)
  // F0 embeds CSELR in DMA1 rather than exposing the L0/L4 DMA1_CSELR view.
  if (DMA1->CSELR & (uint32_t(0xF) << (selected==0 ? 8 : 16))) return Status::Ok;
#elif defined(DMA1_CSELR)
  // Preserve even an idle route
  // established by the application; HAL Init/DeInit may otherwise overwrite it.
  if (DMA1_CSELR->CSELR & (uint32_t(0xF) << (selected==0 ? 8 : 16))) return Status::Ok;
#endif
#if defined(DMAMUX1)
  // G0/G4 and L4+/R/S/P/Q route each DMA channel through DMAMUX.
#if defined(__HAL_RCC_DMAMUX1_CLK_ENABLE)
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
#endif
  if (selected==0) {
    if (DMAMUX1_Channel2->CCR) return Status::Ok;
    dma_.Init.Request=DMA_REQUEST_SPI1_TX;
  } else {
#if defined(DMAMUX1_Channel4) && defined(DMA_REQUEST_SPI2_TX)
    if (DMAMUX1_Channel4->CCR) return Status::Ok;
    dma_.Init.Request=DMA_REQUEST_SPI2_TX;
#else
    return Status::Ok;
#endif
  }
#elif defined(STM32L0xx)
  // RM0377 DMA request table: SPI1 uses selector 1, SPI2 selector 2.
  dma_.Init.Request=selected==0 ? DMA_REQUEST_1 : DMA_REQUEST_2;
#elif defined(STM32L4xx)
  dma_.Init.Request=DMA_REQUEST_1;
#endif
#endif
  dma_.Init.Direction=DMA_MEMORY_TO_PERIPH; dma_.Init.PeriphInc=DMA_PINC_DISABLE;
  dma_.Init.MemInc=DMA_MINC_ENABLE; dma_.Init.PeriphDataAlignment=DMA_PDATAALIGN_BYTE;
  dma_.Init.MemDataAlignment=DMA_MDATAALIGN_BYTE; dma_.Init.Mode=DMA_NORMAL; dma_.Init.Priority=DMA_PRIORITY_HIGH;
  if (HAL_DMA_Init(&dma_)!=HAL_OK) { HAL_DMA_DeInit(&dma_); return Status::Ok; }
  status=allocateDmaBuffers();
  if (status!=Status::Ok) { HAL_DMA_DeInit(&dma_); return Status::Ok; }
  slot_=selected; stmDmaOwners[slot_]=this;
#endif
  return Status::Ok;
}
void Stm32Spi::end() {
  releaseDmaBuffers();
#if defined(GFX_RUNTIME_STM32_DMA_AVAILABLE)
  if (slot_>=0) { HAL_DMA_DeInit(&dma_); stmDmaOwners[slot_]=nullptr; slot_=-1; }
#endif
  ArduinoSpi::end();
}
Status Stm32Spi::dmaWrite(const uint8_t *data,size_t bytes) {
#if defined(GFX_RUNTIME_STM32_DMA_AVAILABLE)
  if (!bytes || bytes>65535) return Status::InvalidArgument;
  SPI_HandleTypeDef *handle=spi_.getHandle();
#if defined(STM32F7xx)
  // AN4667: F7 DMA reaches DTCM through AHBS. Cache maintenance is needed for
  // cacheable SRAM/AXI sources (unlike H7, F7 DTCM must not be rejected here).
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
  uintptr_t address=reinterpret_cast<uintptr_t>(data);
  uintptr_t aligned=address & ~uintptr_t(31);
  SCB_CleanDCache_by_Addr(reinterpret_cast<uint32_t *>(aligned),int32_t((address+bytes-aligned+31)&~uintptr_t(31)));
#endif
#endif
  if (HAL_DMA_Start(&dma_,uint32_t(reinterpret_cast<uintptr_t>(data)),uint32_t(reinterpret_cast<uintptr_t>(&handle->Instance->DR)),uint32_t(bytes))!=HAL_OK)
    return Status::IoError;
  __HAL_SPI_ENABLE(handle); SET_BIT(handle->Instance->CR2,SPI_CR2_TXDMAEN);
  started_=millis(); active_=true; return Status::Ok;
#else
  (void)data; (void)bytes; return Status::Unsupported;
#endif
}
bool Stm32Spi::dmaDirect(const uint8_t *data) const {
  uintptr_t address=reinterpret_cast<uintptr_t>(data);
#if defined(STM32F7xx)
  return address>=0x00010000u; // ITCM RAM itself is not visible to DMA.
#else
  // CCM on F4 is CPU-only; copy its pixels into ordinary SRAM staging buffers.
  return address<0x10000000u || address>=0x10010000u;
#endif
}
Status Stm32Spi::dmaPoll() {
#if defined(GFX_RUNTIME_STM32_DMA_AVAILABLE)
  if (!active_) return Status::Ok;
  auto *handle=spi_.getHandle();
  bool error=__HAL_DMA_GET_FLAG(&dma_,__HAL_DMA_GET_TE_FLAG_INDEX(&dma_))!=RESET;
#if defined(STM32F2xx) || defined(STM32F4xx) || defined(STM32F7xx)
  error=error || __HAL_DMA_GET_FLAG(&dma_,__HAL_DMA_GET_DME_FLAG_INDEX(&dma_))!=RESET ||
      __HAL_DMA_GET_FLAG(&dma_,__HAL_DMA_GET_FE_FLAG_INDEX(&dma_))!=RESET;
#endif
  bool complete=__HAL_DMA_GET_FLAG(&dma_,__HAL_DMA_GET_TC_FLAG_INDEX(&dma_))!=RESET;
  if (!error && uint32_t(millis()-started_) < 5000) {
    if (!complete || !__HAL_SPI_GET_FLAG(handle,SPI_FLAG_TXE) || __HAL_SPI_GET_FLAG(handle,SPI_FLAG_BSY)) return Status::Busy;
  } else error=true;
  CLEAR_BIT(handle->Instance->CR2,SPI_CR2_TXDMAEN);
  if (!error && HAL_DMA_PollForTransfer(&dma_,HAL_DMA_FULL_TRANSFER,0)!=HAL_OK) error=true;
  if (error) { HAL_DMA_Abort(&dma_); __HAL_SPI_DISABLE(handle); }
  // Newer channel-DMA series have an RX FIFO. Drain all received bytes before
  // clearing OVR so that the next display read cannot consume stale pixels.
  while (__HAL_SPI_GET_FLAG(handle,SPI_FLAG_RXNE)) {
    volatile uint8_t value=*reinterpret_cast<volatile uint8_t *>(&handle->Instance->DR);
    (void)value;
  }
  __HAL_SPI_CLEAR_OVRFLAG(handle);
  active_=false; return error ? Status::IoError : Status::Ok;
#else
  return Status::Unsupported;
#endif
}
}
#endif

#endif
