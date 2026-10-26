// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "Driver_UC8151D.h"
#include "SeeedHal.h"
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file   Driver_UC8151D.cpp
 * @brief  UC8151D ePaper display driver implementation
 *
 * Profile-based driver for the Good Display GDEW0213I5FD 2.13" and
 * GDEW029I6FD 2.9" flexible panels. The panel OTP LUT path is:
 * BTST -> PON(+wait BUSY high) -> PSR(0x1F, OTP LUT) -> TRES -> CDI.
 * No PWR/PLL/VDCS/LUT-register writes are needed because the
 * factory-programmed OTP LUT is selected via PSR bit7=0.
 *
 * Modeled on Driver_SSD1680's profile-based init/wake/fallback structure.
 */


// Constructor

Driver_UC8151D::Driver_UC8151D(uint16_t w, uint16_t h)
    : _init_width(w), _init_height(h)
{
    _width  = w;
    _height = h;
}

// Private helpers

void Driver_UC8151D::checkBusy() {
    if (_busy_pin < 0) return;
    // UC8151D BUSY is NGFX_GXEPD2_LOW while busy and NGFX_GXEPD2_HIGH when ready -> readyHigh=true.
    (void)waitForReadyPin(_busy_pin, true);
}

void Driver_UC8151D::reset() {
    // Hardware reset via RST pin only. UC8151D has no software-reset opcode
    // (0x12 is DISPLAY REFRESH), so unlike SSD1680 we emit NO command here --
    // the GDEW029I6FD OTP init begins with BTST. Keeping reset() command-free
    // also keeps the init command stream exactly matching the panel profile.
    if (_reset_pin >= 0) {
        digitalWrite(_reset_pin, NGFX_GXEPD2_LOW);
        delay(10);
        digitalWrite(_reset_pin, NGFX_GXEPD2_HIGH);
        delay(120);
    }
    checkBusy();
}

// IDriver :: init

bool Driver_UC8151D::init(IBus& bus) {
    _bus = &bus;

    // Configure busy pin as input if provided (like SSD1680).
    if (_busy_pin >= 0) {
        pinMode(_busy_pin, NGFX_GXEPD2_INPUT);
    }
    // Configure reset pin as output and hold high if provided.
    if (_reset_pin >= 0) {
        pinMode(_reset_pin, NGFX_GXEPD2_OUTPUT);
        digitalWrite(_reset_pin, NGFX_GXEPD2_HIGH);
    }

    // Hardware reset only (no command issued).
    reset();
    if (lastOperationError() != DriverOperationError::None) return false;

    // OTP profile path: emits BTST/PON(+wait)/PSR/TRES/CDI from seeed_ep.inl.
    if (applyWaveformProfile(EPaperWaveformMode::Full, _busy_pin, true)) {
        setRotation(0);
        return lastOperationError() == DriverOperationError::None;
    }
    if (lastOperationError() != DriverOperationError::None) return false;

    // ---- Fallback: size-aware UC8151D OTP init (mirrors the profiles) -----
    // Booster soft start (3 data bytes)
    _bus->writeCommand(NGFX_SEEED_UC8151D_BTST);   // 0x06
    _bus->writeData(0x17);
    _bus->writeData(0x17);
    _bus->writeData(0x17);

    // Power on, then wait BUSY high
    _bus->writeCommand(NGFX_SEEED_UC8151D_PON);    // 0x04
    checkBusy();

    // Panel setting: bit7=0 -> LUT from OTP
    _bus->writeCommand(NGFX_SEEED_UC8151D_PSR);    // 0x00
    _bus->writeData(0x1F);

    // Resolution: one-byte source width, followed by 16-bit gate height.
    _bus->writeCommand(NGFX_SEEED_UC8151D_TRES);   // 0x61
    _bus->writeData(static_cast<uint8_t>(_init_width));
    _bus->writeData(static_cast<uint8_t>(_init_height >> 8));
    _bus->writeData(static_cast<uint8_t>(_init_height & 0xFF));

    // VCOM and data interval (border floating)
    _bus->writeCommand(NGFX_SEEED_UC8151D_CDI);    // 0x50
    _bus->writeData(0x87);

    setRotation(0);
    return lastOperationError() == DriverOperationError::None;
}

// IDriver :: setRotation

void Driver_UC8151D::setRotation(uint8_t m) {
    // Store only -- the controller stays in its native orientation and
    // Panel_EPaper owns framebuffer rotation (same approach as SSD1680). Do
    // NOT write PSR here: the profile sets PSR=0x1F for the OTP-LUT path, and
    // a rotation-driven PSR write would conflict with it.
    _rotation = m % 4;
    _width  = _init_width;
    _height = _init_height;
}

// IDriver :: display control

void Driver_UC8151D::invertDisplay(bool /*invert*/) {
    // ePaper panels do not support hardware color inversion.
}

void Driver_UC8151D::displayOn() {
    // ePaper retains the image; use update() to refresh.
}

void Driver_UC8151D::displayOff() {
    // ePaper retains the image.
}

// IDriver :: address window

void Driver_UC8151D::setAddrWindow(uint16_t xs, uint16_t ys,
                                    uint16_t xe, uint16_t ye) {
    // Partial window (0x90 PTLW), modeled on UC8179's shape. Partial mode is
    // only entered by 0x91 (PTLIN), which the full-refresh path does not
    // issue, so configuring the window registers here is harmless during a
    // full refresh (the subsequent 0x10/0x13 writes cover the whole RAM).
    _bus->writeCommand(NGFX_SEEED_UC8151D_PTLW);   // 0x90
    _bus->writeData(static_cast<uint8_t>(xs >> 8));
    _bus->writeData(static_cast<uint8_t>(xs & 0xFF));
    _bus->writeData(static_cast<uint8_t>(xe >> 8));
    _bus->writeData(static_cast<uint8_t>(xe & 0xFF));
    _bus->writeData(static_cast<uint8_t>(ys >> 8));
    _bus->writeData(static_cast<uint8_t>(ys & 0xFF));
    _bus->writeData(static_cast<uint8_t>(ye >> 8));
    _bus->writeData(static_cast<uint8_t>(ye & 0xFF));
    _bus->writeData(0x01);   // scan mode
}

// IDriver :: pixel writing (handled by Panel layer for ePaper)

void Driver_UC8151D::writePixel(uint16_t /*color*/) {
    // ePaper writes are done via pushNewColors / pushOldColors.
}

void Driver_UC8151D::writePixels(const uint16_t* /*data*/, size_t /*len*/) {
    // ePaper writes are done via pushNewColors / pushOldColors.
}

void Driver_UC8151D::writeFill(uint16_t /*color*/, size_t /*len*/) {
    // ePaper writes are done via pushNewColors / pushOldColors.
}

// IDriver :: power management

void Driver_UC8151D::sleep() {
    // GDEW029I6FD deep-sleep sequence: re-assert CDI, power off, wait BUSY,
    // then enter deep sleep with 0x07 + 0xA5.
    _bus->writeCommand(NGFX_SEEED_UC8151D_CDI);    // 0x50
    _bus->writeData(0x87);
    _bus->writeCommand(NGFX_SEEED_UC8151D_POF);    // 0x02
    checkBusy();
    _bus->writeCommand(NGFX_SEEED_UC8151D_DSLP);   // 0x07
    _bus->writeData(0xA5);
    delay(100);
}

void Driver_UC8151D::wake() {
    // Mirrors SSD1680::wake: reset, try the profile, else fallback init, then
    // restore the saved rotation.
    reset();
    if (lastOperationError() != DriverOperationError::None) return;
    if (applyWaveformProfile(EPaperWaveformMode::Full, _busy_pin, true)) {
        setRotation(_rotation);
        return;
    }
    if (lastOperationError() != DriverOperationError::None) return;

    // Fallback: size-aware UC8151D OTP init.
    _bus->writeCommand(NGFX_SEEED_UC8151D_BTST);
    _bus->writeData(0x17);
    _bus->writeData(0x17);
    _bus->writeData(0x17);
    _bus->writeCommand(NGFX_SEEED_UC8151D_PON);
    checkBusy();
    _bus->writeCommand(NGFX_SEEED_UC8151D_PSR);
    _bus->writeData(0x1F);
    _bus->writeCommand(NGFX_SEEED_UC8151D_TRES);
    _bus->writeData(static_cast<uint8_t>(_init_width));
    _bus->writeData(static_cast<uint8_t>(_init_height >> 8));
    _bus->writeData(static_cast<uint8_t>(_init_height & 0xFF));
    _bus->writeCommand(NGFX_SEEED_UC8151D_CDI);
    _bus->writeData(0x87);

    setRotation(_rotation);
}

// ePaper-specific :: update

void Driver_UC8151D::update() {
    // Display refresh, then wait BUSY high.
    _bus->writeCommand(NGFX_SEEED_UC8151D_REFRESH);  // 0x12
    checkBusy();
}

// ePaper-specific :: push color data with horizontal flip

void Driver_UC8151D::pushNewColorsFlip(uint16_t w, uint16_t h,
                                        const uint8_t* colors) {
    // Copied from UC8179/SSD1680 flip logic: for each row, reverse the byte
    // order and flip the bits within each byte. Writes to new-color RAM (0x13).
    if (!_bus || !colors) return;
    _bus->writeCommand(NGFX_SEEED_UC8151D_DTM2);   // 0x13

    uint16_t bytes_per_row = w / 8;
    for (uint16_t row = 0; row < h; row++) {
        uint16_t start = row * bytes_per_row;
        for (uint16_t col = 0; col < bytes_per_row; col++) {
            uint8_t b = colors[start + (bytes_per_row - 1 - col)];
            // Reverse the bits within the byte:
            // 1. Swap nibbles
            b = ((b & 0xF0) >> 4) | ((b & 0x0F) << 4);
            // 2. Swap adjacent pairs of bits
            b = ((b & 0xCC) >> 2) | ((b & 0x33) << 2);
            // 3. Swap every other bit
            b = ((b & 0xAA) >> 1) | ((b & 0x55) << 1);
            _bus->writeData(b);
        }
    }
}

void Driver_UC8151D::pushOldColorsFlip(uint16_t w, uint16_t h,
                                       const uint8_t* colors) {
    // Same row-reverse + bit-reverse logic as pushNewColorsFlip, but writes
    // to old-color RAM (0x10).
    if (!_bus || !colors) return;
    _bus->writeCommand(NGFX_SEEED_UC8151D_DTM1);   // 0x10

    uint16_t bytes_per_row = w / 8;
    for (uint16_t row = 0; row < h; row++) {
        uint16_t start = row * bytes_per_row;
        for (uint16_t col = 0; col < bytes_per_row; col++) {
            uint8_t b = colors[start + (bytes_per_row - 1 - col)];
            b = ((b & 0xF0) >> 4) | ((b & 0x0F) << 4);
            b = ((b & 0xCC) >> 2) | ((b & 0x33) << 2);
            b = ((b & 0xAA) >> 1) | ((b & 0x55) << 1);
            _bus->writeData(b);
        }
    }
}

} }
