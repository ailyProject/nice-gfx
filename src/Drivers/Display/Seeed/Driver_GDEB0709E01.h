// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "SeeedHal.h"
#include "Driver.h"
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file   Driver_GDEB0709E01.h
 * @brief  GDEB0709E01 dual-controller color ePaper driver API
 *
 * 1200x1600 4-bit six-color (E Ink Spectra 6) ePaper driver with
 * master/slave chip selects.
 *
 * Panel: Good Display GDEB0709E01, 7.09" Spectra 6, 1200x1600, 282dpi.
 * Drive ICs (vendor statement 2026-08): NT61522 (PVT61522) x1 + EK73601 x1.
 * Architecturally identical to the 13.3" T133A01 (reTerminal E1004):
 * dual COG / dual CS, each row split into left/right halves pushed to the
 * two ICs, waveforms baked into COG OTP (no host LUT download),
 * CCSET(0xE0)=0x01 selects the OTP waveform group.
 *
 * Value-set decision (2026-08-28): this driver keeps the T133A01 register
 * values byte for byte. They are empirically verified on Seeed hardware
 * (a colleague flashed this 7.09" panel successfully with the 13.3" board
 * and code), and the vendor explicitly endorsed "reference the 13.3" E6".
 * The Good Display example (GDEB0709E01-ESP32, driver file GDEP133C02.c)
 * carries four divergent values that were only verified on the vendor's
 * own 5V carrier board; they are documented in Driver_GDEB0709E01.cpp as
 * the first fallback if light-up fails on a new power topology.
 */

#ifndef NGFX_SEEED_SEEED_GFX_DRIVER_GDEB0709E01_H
#define NGFX_SEEED_SEEED_GFX_DRIVER_GDEB0709E01_H


class Driver_GDEB0709E01 : public IDriver {
public:
    Driver_GDEB0709E01(uint16_t w = 1200, uint16_t h = 1600, int8_t busyPin = -1);

    const char* name() const override { return "GDEB0709E01"; }
    uint16_t width() const override { return _width; }
    uint16_t height() const override { return _height; }
    uint8_t colorDepth() const override { return 4; }
    bool supportsColorfulEPaper() const override { return true; }
    bool supportsDeepSleep() const override { return true; }
    bool supportsTemperatureCompensation() const override { return true; }

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

    // ePaper-specific
    void setBusyPin(int pin) override { _busyPin = static_cast<int8_t>(pin); }
    void setResetPin(int pin) override { _resetPin = static_cast<int8_t>(pin); }
    void update() override;
    void pushColors(const uint8_t* data, uint16_t w, uint16_t h);
    void pushNewColors(const uint8_t* data, size_t len) override {
        (void)len; pushColors(data, _init_width, _init_height);
    }
    void pushOldColors(const uint8_t* data, size_t len) override {
        (void)len; pushOldColors(data, _init_width, _init_height);
    }
    void pushColorsFlip(const uint8_t* data, uint16_t w, uint16_t h);
    void pushOldColors(const uint8_t* data, uint16_t w, uint16_t h);
    void pushOldColorsFlip(const uint8_t* data, uint16_t w, uint16_t h);
    void setTemp(uint8_t temp);
    void setTemperature(int8_t temp) override { setTemp(static_cast<uint8_t>(temp)); }

private:
    void busyWait();
    uint8_t colorGet(uint8_t color);
    void writeCommandData(ChipSelectTarget target, uint8_t cmd,
                          const uint8_t* data, size_t len);
    uint16_t _init_width, _init_height;
    int8_t _busyPin;
    int8_t _resetPin = -1;
};

#endif // NGFX_SEEED_SEEED_GFX_DRIVER_GDEB0709E01_H

} }
