// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "SeeedHal.h"
#include "Driver.h"
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file   Driver_UC8151D.h
 * @brief  UC8151D ePaper display driver for Seeed_GFX v2.0
 *
 * Driver for the UltraChip UC8151D ePaper controller, used by the Good Display
 * GDEW0213I5FD 2.13" flexible panel (104x212 native) and GDEW029I6FD
 * 2.9" flexible panel (128x296 native), both 1-bit Black/White.
 *
 * UC8151D shares its command opcodes with UC8179, but the register LAYOUTS
 * differ (BTST/TRES/PSR/CDI take a different number of data bytes). This
 * driver therefore uses panel-size-matched OTP init profiles rather than
 * UC8179's hardcoded LUT path. PSR bit7=0 selects the factory-programmed OTP
 * LUT, so no LUT register writes are needed.
 *
 * Modeled on the profile-based Driver_SSD1680 (not UC8179's hardcoded style).
 * Supports full refresh only for this first cut; partial/fast/gray are
 * conservatively disabled.
 */

#ifndef NGFX_SEEED_SEEED_GFX_DRIVER_UC8151D_H
#define NGFX_SEEED_SEEED_GFX_DRIVER_UC8151D_H


// UC8151D Command Constants
// Opcodes are shared with UC8179; only the register *layouts* differ. The
// GDEW029I6FD reference program is the source of the data byte counts below.
#define NGFX_SEEED_UC8151D_PSR       0x00   // Panel setting (bit7=0 -> LUT from OTP)
#define NGFX_SEEED_UC8151D_POF       0x02   // Power off
#define NGFX_SEEED_UC8151D_PON       0x04   // Power on (then wait BUSY high)
#define NGFX_SEEED_UC8151D_DSLP      0x07   // Deep sleep (data 0xA5)
#define NGFX_SEEED_UC8151D_BTST      0x06   // Booster soft start (3 data bytes)
#define NGFX_SEEED_UC8151D_DTM1      0x10   // Display update - old data
#define NGFX_SEEED_UC8151D_REFRESH   0x12   // Display refresh (then wait BUSY)
#define NGFX_SEEED_UC8151D_DTM2      0x13   // Display update - new data
#define NGFX_SEEED_UC8151D_CDI       0x50   // VCOM and data interval (1 data byte)
#define NGFX_SEEED_UC8151D_TRES      0x61   // Resolution (3 data bytes: HRES, VRES_hi, VRES_lo)
#define NGFX_SEEED_UC8151D_PTLW      0x90   // Partial window
#define NGFX_SEEED_UC8151D_PTLIN     0x91   // Partial in

class Driver_UC8151D : public IDriver {
public:
    /**
     * @param w  Native source width in pixels (104 or 128)
     * @param h  Native gate height in pixels (212 or 296)
     */
    Driver_UC8151D(uint16_t w = 128, uint16_t h = 296);

    // ---- IDriver interface ------------------------------------------------
    const char* name() const override { return "UC8151D"; }
    uint16_t width() const override { return _width; }
    uint16_t height() const override { return _height; }
    // Override so profile lookup keys on the physical panel dimensions, not on
    // the (rotation-swapped) reported dimensions (mirrors SSD1680).
    uint16_t nativeWidth() const override { return _init_width; }
    uint16_t nativeHeight() const override { return _init_height; }
    uint8_t  colorDepth() const override { return 1; }
    bool     supportsDeepSleep() const override { return true; }
    // Conservative first cut: the 2.9 flex HelloWorld uses full refresh only.
    bool     supportsPartialRefresh() const override { return false; }
    bool     supportsFastRefresh() const override { return false; }
    bool     supportsGrayRefresh(uint8_t levels) const override {
        (void)levels;
        return false;
    }
    bool     supportsTemperatureCompensation() const override { return false; }
    uint16_t partialXAlignment() const override { return 8; }

    bool     init(IBus& bus) override;
    void     setRotation(uint8_t rotation) override;
    uint8_t  rotation() const override { return _rotation; }
    void     invertDisplay(bool invert) override;
    void     displayOn() override;
    void     displayOff() override;
    void     setAddrWindow(uint16_t xs, uint16_t ys,
                           uint16_t xe, uint16_t ye) override;
    void     writePixel(uint16_t color) override;
    void     writePixels(const uint16_t* data, size_t len) override;
    void     writeFill(uint16_t color, size_t len) override;
    void     sleep() override;
    void     wake() override;
    void     update() override;
    IBus&    bus() override { return *_bus; }

    // ---- Pin configuration (called by Panel/Board before init) -----------
    void setBusyPin(int pin) override   { _busy_pin = static_cast<int8_t>(pin); }
    void setResetPin(int pin) override  { _reset_pin = static_cast<int8_t>(pin); }

    // ---- ePaper-specific public API --------------------------------------

    /// Push B/W pixel data to new-color RAM (0x13 DTM2). Panel_EPaper performs
    /// its own framebuffer rotation and (if mirrored) bit/byte flip, then
    /// calls this 2-arg override.
    void pushNewColors(const uint8_t* colors, size_t len) override {
        if (!_bus || !colors) return;
        _bus->writeCommand(NGFX_SEEED_UC8151D_DTM2);
        _bus->writeData(colors, len);
    }

    /// Push B/W pixel data to old-color RAM (0x10 DTM1).
    void pushOldColors(const uint8_t* colors, size_t len) override {
        if (!_bus || !colors) return;
        _bus->writeCommand(NGFX_SEEED_UC8151D_DTM1);
        _bus->writeData(colors, len);
    }

    /// Push new-color data with per-row byte+bit reverse (horizontal mirror).
    /// Mirrors the SSD1680/UC8179 flip shape; Panel_EPaper now performs the
    /// flip itself and calls the 2-arg push above, so this is kept for direct
    /// use and API parity.
    void pushNewColorsFlip(uint16_t w, uint16_t h, const uint8_t* colors);

    /// Push old-color data with per-row byte+bit reverse (horizontal mirror).
    void pushOldColorsFlip(uint16_t w, uint16_t h, const uint8_t* colors);

private:
    /// Poll the BUSY pin until the controller is ready (no-op if no pin).
    /// UC8151D BUSY is NGFX_GXEPD2_LOW while busy and NGFX_GXEPD2_HIGH when ready, so readyHigh=true.
    void checkBusy();

    /// Hardware reset via the RST pin only. UC8151D has no software-reset
    /// opcode (0x12 is DISPLAY REFRESH), so unlike SSD1680 we must NOT emit
    /// any command here -- the GDEW029I6FD OTP init begins with BTST.
    void reset();

    uint16_t _init_width  = 128;
    uint16_t _init_height = 296;
    int8_t   _busy_pin    = -1;
    int8_t   _reset_pin   = -1;
};

#endif // NGFX_SEEED_SEEED_GFX_DRIVER_UC8151D_H

} }
