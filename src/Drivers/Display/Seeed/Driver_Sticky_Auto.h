// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "SeeedHal.h"
#include "Driver.h"
#include "Driver_SSD1677.h"
#include "Driver_SSD2677.h"
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file   Driver_Sticky_Auto.h
 * @brief  Auto-detect composite driver for the reTerminal Sticky ePaper
 *
 * Sticky units ship with one of two controllers, mixed in production:
 * SSD1677 or SSD2677, both 800x480 monochrome glass on identical wiring.
 * This wrapper owns one of each child driver and picks between them at
 * init() time in two stages:
 *
 *   Stage 1 — the exact probe the Sticky product firmware uses
 *   (seeed_epaper/epaper_panel.c resolve_panel_model):
 *     reset (SSD2677 timing: low 20 ms / high 10 ms)
 *     -> write command 0x70
 *     -> read one byte
 *     -> byte == 0x07 ? SSD2677 : SSD1677
 *   The probe is an SPI read (Sticky shares MISO GPIO12 with the microSD
 *   slot). A byte actively driven to any value other than 0x07 is an
 *   SSD1677 answering, matching the firmware's "anything else" rule.
 *   0x00/0xFF is what an undriven line returns: field units exist whose
 *   panel SDO is not wired to GPIO12 (the probe and even the firmware's
 *   own temperature read come back 0x00), where the firmware's else
 *   branch would mis-resolve SSD2677 units to SSD1677 and leave the panel
 *   blank. Those reads fall through to stage 2.
 *
 *   Stage 2 — BUSY-polarity detection, no SPI read needed. The two
 *   controllers drive BUSY with opposite polarity (firmware
 *   apply_panel_model_config: SSD2677 busy_level=0/ready NGFX_GXEPD2_HIGH, SSD1677
 *   busy_level=1/ready NGFX_GXEPD2_LOW; each child driver matches in busyWait()).
 *   After a reset the chip is busy in its own polarity and settles at its
 *   ready level, so the post-reset BUSY waveform names the chip. Also used
 *   outright when the bus has no MISO at all.
 *
 * Construction touches no hardware; the probe runs only inside init(),
 * after the board has powered the panel and begun the bus — the same
 * ordering as the firmware's seeed_epaper_new_panel(). If the chosen
 * child's init fails anyway (busy timeout against the wrong polarity),
 * init() resets the panel and retries the other child before giving up.
 */

#ifndef NGFX_SEEED_SEEED_GFX_DRIVER_STICKY_AUTO_H
#define NGFX_SEEED_SEEED_GFX_DRIVER_STICKY_AUTO_H




class Driver_Sticky_Auto : public IDriver {
public:
    Driver_Sticky_Auto(uint16_t w = 800, uint16_t h = 480);

    /** Before init(): "Sticky-Auto". Afterwards: the resolved child name
     *  ("SSD1677" or "SSD2677"), mirroring the firmware's resolved model. */
    const char* name() const override;
    uint16_t width() const override;
    uint16_t height() const override;
    uint16_t nativeWidth() const override;
    uint16_t nativeHeight() const override;
    uint8_t colorDepth() const override;

    bool supportsReadback() const override;
    bool supportsPartialRefresh() const override;
    uint16_t partialXAlignment() const override;
    bool supportsFastRefresh() const override;
    bool supportsGrayRefresh(uint8_t levels) const override;
    bool supportsColorfulEPaper() const override;
    bool supportsBWRYEPaper() const override;
    bool supportsDeepSleep() const override;
    bool supportsTemperatureCompensation() const override;

    DriverOperationError lastOperationError() const override;
    void clearOperationError() override;

    bool init(IBus& bus) override;

    void setRotation(uint8_t rotation) override;
    uint8_t rotation() const override;
    void invertDisplay(bool invert) override;
    void displayOn() override;
    void displayOff() override;

    void setAddrWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye) override;
    void writePixel(uint16_t color) override;
    void writePixels(const uint16_t* data, size_t len) override;
    void writeFill(uint16_t color, size_t len) override;

    bool supportsAsyncPixelTransfer() const override;
    bool enableDMA(bool enable = true) override;
    bool writePixelsDMA(const uint16_t* data, size_t len) override;
    bool dmaBusy() override;

    void sleep() override;
    void wake() override;

    // ePaper-specific
    void update() override;
    void updateFast() override;
    void updatePartial() override;
    void wakePartial() override;
    void wakeFast() override;
    void wakeGray() override;
    void updateGray() override;
    void pushNewColors(const uint8_t* data, size_t len) override;
    void pushOldColors(const uint8_t* data, size_t len) override;
    void pushGrayColors(const uint8_t* data, size_t len) override;
    void setTemperature(int8_t temperatureC) override;

    size_t waveformProfileCount() const override;
    const EPaperWaveformProfile* waveformProfileAt(size_t index) const override;
    bool selectWaveformProfile(const char* id) override;
    const EPaperWaveformProfile* waveformProfile() const override;
    EPaperWaveformResult waveformProfileResult() const override;

    IBus& bus() override;
    void setBusyPin(int pin) override;
    void setResetPin(int pin) override;

    /** Chip ID byte read back by the probe, or -1 before init(). */
    int probedChipId() const override { return _probedChipId; }

private:
    /** True when the post-reset BUSY waveform identifies an SSD2677.
     *  Used when the read probe cannot work (no MISO, or the panel SDO is
     *  not wired to the shared MISO and reads come back undriven). */
    bool probeByBusyPolarity();

    /** Chip ID reported by an SSD2677 for probe command 0x70. */
    static constexpr uint8_t kSsd2677ChipId = 0x07;
    /** What an undriven (unwired SDO) read line returns; not a chip
     *  answer, so stage 1 treats both as "probe inconclusive". */
    static constexpr uint8_t kUnwiredLow = 0x00;
    static constexpr uint8_t kUnwiredHigh = 0xFF;

    Driver_SSD1677 _ssd1677;
    Driver_SSD2677 _ssd2677;
    IDriver* _active = nullptr;
    int _probedChipId = -1;
    int8_t _busyPin = -1;
};

#endif // NGFX_SEEED_SEEED_GFX_DRIVER_STICKY_AUTO_H

} }
