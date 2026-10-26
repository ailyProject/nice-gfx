// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "SeeedHal.h"
#include "Driver.h"
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file   Driver_UC8179.h
 * @brief  UC8179 ePaper display driver for Seeed_GFX v2.0
 *
 * Widely used on Seeed XIAO ePaper displays (1.54", 2.13", 2.9", 4.2", 7.5", etc.)
 * Supports full update, fast update, partial update, and 4-level grayscale.
 *
 * Adapted from TFT_Drivers/UC8179_Defines.h, UC8179_Init.h, UC8179_Rotation.h
 */

#ifndef NGFX_SEEED_SEEED_GFX_DRIVER_UC8179_H
#define NGFX_SEEED_SEEED_GFX_DRIVER_UC8179_H



// UC8179 register commands
#define NGFX_SEEED_UC8179_PNLSET      0x00   // Panel setting
#define NGFX_SEEED_UC8179_POWERON     0x04   // Power on
#define NGFX_SEEED_UC8179_POWEROFF    0x02   // Power off
#define NGFX_SEEED_UC8179_DISPOFF     0x03   // Display off
#define NGFX_SEEED_UC8179_DISPON      0x04   // Display on (alias for power on)
#define NGFX_SEEED_UC8179_SLPIN       0x07   // Deep sleep
#define NGFX_SEEED_UC8179_DRFOUTPUT   0x01   // Driver output control
#define NGFX_SEEED_UC8179_BOOSTER     0x06   // Booster soft start
#define NGFX_SEEED_UC8179_DTM1        0x10   // Display update - old data
#define NGFX_SEEED_UC8179_DTM2        0x13   // Display update - new data
#define NGFX_SEEED_UC8179_DISPLAYREFRESH 0x12 // Display refresh
#define NGFX_SEEED_UC8179_LUT_VCOM    0x20   // LUT for VCOM
#define NGFX_SEEED_UC8179_LUT_WW      0x21   // LUT for white-to-white
#define NGFX_SEEED_UC8179_LUT_KW      0x22   // LUT for black-to-white
#define NGFX_SEEED_UC8179_LUT_WK      0x23   // LUT for white-to-black
#define NGFX_SEEED_UC8179_LUT_KK      0x24   // LUT for black-to-black
#define NGFX_SEEED_UC8179_PLL         0x30   // PLL control
#define NGFX_SEEED_UC8179_VCMDC       0x82   // VCM DC setting
#define NGFX_SEEED_UC8179_TEMPSENSOR  0x15   // Temperature sensor
#define NGFX_SEEED_UC8179_TCONSET     0x60   // TCON setting
#define NGFX_SEEED_UC8179_VDCS        0x50   // VCOM and data interval
#define NGFX_SEEED_UC8179_TRES        0x61   // Resolution
#define NGFX_SEEED_UC8179_REV         0xE0   // Border waveform control
#define NGFX_SEEED_UC8179_REVE        0xE5   // Border waveform control extended
#define NGFX_SEEED_UC8179_CDI         0x50   // CDI setting
#define NGFX_SEEED_UC8179_GSST        0x52   // Gate scan start
#define NGFX_SEEED_UC8179_GLD         0xE3   // Gate line delay
#define NGFX_SEEED_UC8179_PTLIN       0x91   // Partial display in
#define NGFX_SEEED_UC8179_PTLOUT      0x92   // Partial display out
#define NGFX_SEEED_UC8179_PTLW        0x90   // Partial window

#define NGFX_SEEED_UC8179_PNLSET_NORMAL     0x1F   // 0b11111 - normal scan
#define NGFX_SEEED_UC8179_PNLSET_LANDSCAPE  0x1B   // 0b11011 - landscape
#define NGFX_SEEED_UC8179_PNLSET_INVERTED   0x13   // 0b10011 - inverted portrait
#define NGFX_SEEED_UC8179_PNLSET_INVLAND    0x17   // 0b10111 - inverted landscape

class Driver_UC8179 : public IDriver {
public:
    ~Driver_UC8179() override { protocolFree(_oldCache); }
    /**
     * @param w  Display width in pixels (default 800 for 7.5" panel)
     * @param h  Display height in pixels (default 480 for 7.5" panel)
     */
    Driver_UC8179(uint16_t w = 800, uint16_t h = 480);

    // IDriver interface

    const char* name() const override { return "UC8179"; }
    bool supportsPartialRefresh() const override {
        return _init_width == 800 && _init_height == 480;
    }
    bool supportsFastRefresh() const override {
        return _init_width == 800 && _init_height == 480;
    }
    bool supportsGrayRefresh(uint8_t levels) const override {
        return levels == 4 && _init_width == 800 && _init_height == 480;
    }
    uint16_t width() const override { return _width; }
    uint16_t height() const override { return _height; }
    uint8_t colorDepth() const override { return 1; }
    bool supportsDeepSleep() const override { return true; }

    bool init(IBus& bus) override;
    void setRotation(uint8_t rotation) override;
    uint8_t rotation() const override { return _rotation; }
    void invertDisplay(bool invert) override;
    void displayOn() override;
    void displayOff() override;
    void setAddrWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye) override;
    void writePixel(uint16_t color) override;
    void writePixels(const uint16_t* data, size_t len) override;
    void writeFill(uint16_t color, size_t len) override;
    void sleep() override;
    void wake() override;
    IBus& bus() override { return *_bus; }

    // ePaper update methods

    /** Trigger a full display update (refresh) */
    void update() override;

    /** Trigger a display update (same as update for this driver) */
    void updateGray() override;

    /** Trigger a partial display update */
    void updatePartial() override;

    // Init sequences (can be called directly for mode switching)

    /** Full initialization sequence */
    void initFull();

    /** Fast (1-bit) initialization sequence with LUT */
    void initFast();

    /** 4-level grayscale initialization sequence */
    void initGray();

    /** Partial update initialization sequence */
    void initPartial();

    // Wakeup sequences (reset + init)

    /** Hardware reset + fast init */
    void wakeupFull();

    /** Hardware reset + fast init with LUT */
    void wakeupFast();
    void wakeFast() override { wakeupFast(); }

    /** Hardware reset + gray init */
    void wakeGray() override;
    void wakeupGray() { wakeGray(); }

    /** Hardware reset + partial init */
    void wakeupPartial();
    void wakePartial() override { wakeupPartial(); }

    // Color data push (new/old image planes)

    /** Push new (next) image data to register 0x13 */
    void pushNewColors(const uint8_t* colors, size_t len) override;

    /** Push old (previous) image data to register 0x10 */
    void pushOldColors(const uint8_t* colors, size_t len) override;

    /** Push new image data with horizontal-bit-flip per byte */
    void pushNewColorsFlip(const uint8_t* colors, size_t len);

    /** Push old image data with horizontal-bit-flip per byte */
    void pushOldColorsFlip(const uint8_t* colors, size_t len);

    /** Push 4-level grayscale data (2-bit per pixel) to new/old planes */
    void pushGrayColors(const uint8_t* colors, size_t len) override;
    void pushNewGrayColors(const uint8_t* colors, size_t len) {
        pushGrayColors(colors, len);
    }

    /** Push flipped grayscale data (monochrome data with gray LUT) */
    void pushNewGrayColorsFlip(const uint8_t* colors, size_t len);

    // OTP control

    /** Enable/disable internal OTP LUT usage */
    void useInternalOTP(bool enable) { _use_otp_lut = enable; }

    /** Check if internal OTP LUT is being used */
    bool isUsingInternalOTP() const { return _use_otp_lut; }

    // Pin configuration

    /** Set the busy pin (used for CHECK_BUSY polling) */
    void setBusyPin(int pin) override { _busy_pin = pin; }

    /** Set the reset pin (used for hardware reset) */
    void setResetPin(int pin) override { _reset_pin = pin; }

    // Data plane selection

    /** Select the data plane for writePixel / writePixels / writeFill
     *  @param plane  Use NGFX_SEEED_UC8179_DTM1 (0x10) for old data, NGFX_SEEED_UC8179_DTM2 (0x13) for new data
     */
    void selectWritePlane(uint8_t plane) { _write_plane = plane; }

    /** Get the currently selected write plane */
    uint8_t writePlane() const { return _write_plane; }

private:
    uint16_t _init_width;
    uint16_t _init_height;
    int      _busy_pin;
    int      _reset_pin;
    bool     _use_otp_lut;
    bool     _has_checked_otp;
    uint8_t  _write_plane;
    bool     _invert_display;             // invertDisplay() toggle (user; off = no inversion)
    bool     _partialActive;              // between wakeupPartial() and updatePartial()
    uint8_t* _oldCache;                   // last displayed 1bpp frame (diff "old" plane)
    uint16_t _winX0, _winY0, _winX1, _winY1; // last partial window (setAddrWindow)

    // Internal helpers

    /** Poll the busy pin until the display is ready */
    void checkBusy();

    /** Hardware reset via reset pin */
    void reset();

    /** Write the normal (1-bit) LUT waveform tables */
    void writeLUT();

    /** Write the 4-level grayscale LUT waveform tables */
    void writeLUTGray();

    /** Probe whether the panel supports internal OTP LUT */
    void probeOtpSupport();

    /** Initialize using internal OTP for grayscale mode */
    void initGrayOTP();

    /** Invert-aware data write; inverts each byte only while _invert_display
     *  is set (an explicit user request). */
    void streamData(const uint8_t* data, size_t len);

    /** Full 1bpp frame size in bytes. */
    size_t frameBytes() const { return (size_t)(_init_width / 8) * _init_height; }

    /** Lazily allocate and white-initialize the old-frame cache. */
    void ensureOldCache();

    /** Stream the current partial window's old plane (DTM1) from the cache. */
    void pushOldWindow();

    /** Stitch a pushed partial window into the old-frame cache. */
    void patchOldCache(const uint8_t* data, size_t len);
};

#endif // NGFX_SEEED_SEEED_GFX_DRIVER_UC8179_H

} }
