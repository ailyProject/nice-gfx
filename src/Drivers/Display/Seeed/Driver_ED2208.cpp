// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "Driver_ED2208.h"
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file   Driver_ED2208.cpp
 * @brief  ED2208 ePaper display driver implementation
 *
 * Variable-resolution 4bpp indexed six-color ePaper driver used by
 * GDEP040E01 (400x600) and GDEP073E01 (800x480). The 4-bit value is a
 * palette/transfer code for Black, White, Red, Yellow, Green, or Blue;
 * it is not a 16-level grayscale format.
 * Ported from TFT_Drivers/ED2208_Defines.h and ED2208_Init.h
 */

Driver_ED2208::Driver_ED2208(uint16_t w, uint16_t h, int8_t busyPin)
    : _init_width(w), _init_height(h), _busyPin(busyPin)
{
    _width = w;
    _height = h;
}

void Driver_ED2208::busyWait(uint32_t timeoutMs) {
    if (_busyPin < 0) return;
    // The original Seeed_GFX CHECK_BUSY() always waits 10 ms before its
    // first sample. Without this guard, a stale READY-high level immediately
    // after PON/DRF/POF can be mistaken for command completion before ED2208
    // has had time to pull BUSY low.
    delay(10);
    (void)waitForReadyPin(_busyPin, true, timeoutMs, 10);
}

uint8_t Driver_ED2208::colorGet(uint8_t color) {
    switch (color) {
        case 0x0F: return 0x00;
        case 0x00: return 0x01;
        case 0x0D: return 0x05;
        case 0x02: return 0x06;
        case 0x0B: return 0x02;
        case 0x06: return 0x03;
        default:   return 0x00;
    }
}

bool Driver_ED2208::init(IBus& bus) {
    _bus = &bus;
    gfxPinModeInput(_busyPin);
    // GDEP040E01 uses the dual reset sequence from its own reference code.
    // GDEP073E01 must retain Seeed_GFX's original single 20/10 ms reset.
    // Do not wait for BUSY here: ED2208 may keep BUSY low after reset until
    // the CMDH/PWRR setup has been sent. The original driver only waits
    // after PON.
    if (_init_width == 400 && _init_height == 600) {
        hardwareReset(30, 30, 2);
    } else {
        hardwareReset(20, 10, 1);
    }

    if (applyWaveformProfile(EPaperWaveformMode::Full, _busyPin, true)) {
        return lastOperationError() == DriverOperationError::None;
    }
    if (lastOperationError() != DriverOperationError::None) return false;

    _bus->writeCommand(0xAA); // CMDH
    _bus->writeData(0x49);
    _bus->writeData(0x55);
    _bus->writeData(0x20);
    _bus->writeData(0x08);
    _bus->writeData(0x09);
    _bus->writeData(0x18);

    _bus->writeCommand(0x01); // PWRR
    _bus->writeData(0x3F);
    _bus->writeData(0x00);
    _bus->writeData(0x32);
    _bus->writeData(0x2A);
    _bus->writeData(0x0E);
    _bus->writeData(0x2A);

    _bus->writeCommand(0x00); // PSR
    _bus->writeData(0x5F);
    _bus->writeData(0x69);

    _bus->writeCommand(0x03); // POFS
    _bus->writeData(0x00);
    _bus->writeData(0x54);
    _bus->writeData(0x00);
    _bus->writeData(0x44);

    _bus->writeCommand(0x05); // BTST1
    _bus->writeData(0x40);
    _bus->writeData(0x1F);
    _bus->writeData(0x1F);
    _bus->writeData(0x2C);

    _bus->writeCommand(0x06); // BTST2
    _bus->writeData(0x6F);
    _bus->writeData(0x1F);
    _bus->writeData(0x16);
    _bus->writeData(0x25);

    _bus->writeCommand(0x08); // BTST3
    _bus->writeData(0x6F);
    _bus->writeData(0x1F);
    _bus->writeData(0x1F);
    _bus->writeData(0x22);

    _bus->writeCommand(0x13); // IPC
    _bus->writeData(0x00);
    _bus->writeData(0x04);

    _bus->writeCommand(0x30); // PLL
    _bus->writeData(0x02);

    _bus->writeCommand(0x41); // TSE
    _bus->writeData(0x00);

    _bus->writeCommand(0x50); // CDI
    _bus->writeData(0x3F);

    _bus->writeCommand(0x60); // TCON
    _bus->writeData(0x02);
    _bus->writeData(0x00);

    _bus->writeCommand(0x61); // TRES
    _bus->writeData((_init_width >> 8) & 0xFF);
    _bus->writeData(_init_width & 0xFF);
    _bus->writeData((_init_height >> 8) & 0xFF);
    _bus->writeData(_init_height & 0xFF);

    _bus->writeCommand(0x82); // VDCS
    _bus->writeData(0x1E);

    _bus->writeCommand(0x84); // T_VDCS
    _bus->writeData(0x01);

    _bus->writeCommand(0x86); // AGID
    _bus->writeData(0x00);

    _bus->writeCommand(0xE3); // PWS
    _bus->writeData(0x2F);

    _bus->writeCommand(0xE0); // CCSET
    _bus->writeData(0x00);

    _bus->writeCommand(0xE6); // TSSET
    _bus->writeData(0x00);

    _bus->writeCommand(0x04); // Power on
    busyWait();

    return lastOperationError() == DriverOperationError::None;
}

void Driver_ED2208::setRotation(uint8_t rotation) {
    _rotation = rotation % 4;
    if (_rotation & 1U) {
        _width = _init_height;
        _height = _init_width;
    } else {
        _width = _init_width;
        _height = _init_height;
    }
}

void Driver_ED2208::invertDisplay(bool invert) {
    (void)invert;
}

void Driver_ED2208::displayOn() {
    _bus->writeCommand(0x04);
    busyWait();
}

void Driver_ED2208::displayOff() {
    _bus->writeCommand(0x02);
    _bus->writeData(0x00);
    delay(1);
    busyWait();
}

void Driver_ED2208::setAddrWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye) {
    (void)xs; (void)ys; (void)xe; (void)ye;
}

void Driver_ED2208::writePixel(uint16_t color) {
    (void)color;
}

void Driver_ED2208::writePixels(const uint16_t* data, size_t len) {
    (void)data; (void)len;
}

void Driver_ED2208::writeFill(uint16_t color, size_t len) {
    (void)color; (void)len;
}

void Driver_ED2208::sleep() {
    _bus->writeCommand(0x02);
    _bus->writeData(0x00);
    delay(1);
    busyWait();
    if (lastOperationError() != DriverOperationError::None) return;

    // GDEP073E01 / reTerminal E1002 follows the original Seeed_GFX
    // POF -> PON lifecycle.  Entering deep sleep here would discard the
    // controller setup, while its wake sequence deliberately sends only
    // PON (0x04); the next refresh would then time out on BUSY or stop at a
    // yellow intermediate frame.  GDEP040E01 can use deep sleep because its
    // wake path resets the controller and replays the complete profile.
    if (_init_width == 400 && _init_height == 600) {
        _bus->writeCommand(0x07);
        _bus->writeData(0xA5);
    }
}

void Driver_ED2208::wake() {
    if (_init_width == 400 && _init_height == 600) {
        // GDEP040E01's profile deliberately defers PON until update(), after
        // image RAM has been written, so restore its full setup here.
        (void)init(*_bus);
    } else {
        // Match GDEP073E01's original EPD_WAKEUP(): its configuration
        // survives normal POF and only PON + BUSY wait is required.
        // Re-running init() here reset the controller before every transfer.
        displayOn();
    }
}

void Driver_ED2208::update() {
    if (_init_width == 400 && _init_height == 600) {
        // GDEP040E01 requires power-on after image data and a second BTST2
        // value immediately before display refresh.
        _bus->writeCommand(0x04);
        busyWait();
        if (lastOperationError() != DriverOperationError::None) return;
        _bus->writeCommand(0x06);
        const uint8_t booster[] = {0x6F, 0x1F, 0x17, 0x27};
        _bus->writeData(booster, sizeof(booster));
    }
    _bus->writeCommand(0x12);
    _bus->writeData(0x00);
    delay(1);
    busyWait();
}

void Driver_ED2208::pushColors(const uint8_t* data, uint16_t w, uint16_t h) {
    uint16_t bytes_per_row = w / 2;
    // Build a row buffer to send in one SPI transaction (CS stays low per row)
    uint8_t* rowBuf = (uint8_t*)protocolAllocate(bytes_per_row);
    if (!rowBuf) return;

    _bus->writeCommand(0x10);  // Data write command
    for (uint16_t row = 0; row < h; row++) {
        for (uint16_t col = 0; col < bytes_per_row; col++) {
            uint8_t b = data[(uint32_t)bytes_per_row * row + col];
            uint8_t temp1 = (b >> 4) & 0x0F;
            uint8_t temp2 = b & 0x0F;
            rowBuf[col] = (colorGet(temp1) << 4) | colorGet(temp2);
        }
        _bus->writeData(rowBuf, bytes_per_row);  // Send entire row at once
    }
    protocolFree(rowBuf);
}

void Driver_ED2208::pushColorsFlip(const uint8_t* data, uint16_t w, uint16_t h) {
    uint16_t bytes_per_row = w / 2;
    uint8_t* rowBuf = (uint8_t*)protocolAllocate(bytes_per_row);
    if (!rowBuf) return;

    _bus->writeCommand(0x10);
    for (uint16_t row = 0; row < h; row++) {
        uint16_t start = (uint32_t)row * bytes_per_row;
        for (uint16_t col = 0; col < bytes_per_row; col++) {
            uint8_t b = data[start + (bytes_per_row - 1 - col)];
            uint8_t temp1 = (b >> 4) & 0x0F;
            uint8_t temp2 = b & 0x0F;
            rowBuf[col] = (colorGet(temp2) << 4) | colorGet(temp1);
        }
        _bus->writeData(rowBuf, bytes_per_row);
    }
    protocolFree(rowBuf);
}

// New IDriver virtual method overrides (called by Panel_EPaper)
void Driver_ED2208::pushNewColors(const uint8_t* data, size_t len) {
    (void)len;
    // ED2208 uses 0x10 for data write and needs color conversion
    pushColors(data, _init_width, _init_height);
}

void Driver_ED2208::pushOldColors(const uint8_t* data, size_t len) {
    // ED2208 doesn't use old/new differential update - all data goes through 0x10
    (void)data; (void)len;
}

} }
