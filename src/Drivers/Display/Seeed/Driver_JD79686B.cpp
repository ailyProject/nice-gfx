// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "Driver_JD79686B.h"
#include "seeed_ep.h"
#include "SeeedHal.h"
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file   Driver_JD79686B.cpp
 * @brief  JD79686B ePaper display driver implementation
 *
 * Ported from TFT_Drivers/JD79686B_Defines.h, JD79686B_Init.h, JD79686B_Rotation.h
 */



Driver_JD79686B::Driver_JD79686B(uint16_t w, uint16_t h, int8_t busyPin)
    : _init_width(w), _init_height(h), _busyPin(busyPin) {
    _width = w; _height = h;
}

void Driver_JD79686B::busyWait() {
    if (_busyPin < 0) return;
    (void)waitForReadyPin(_busyPin, true);
}

bool Driver_JD79686B::init(IBus& bus) {
    _bus = &bus;
    if (_busyPin >= 0) pinMode(_busyPin, NGFX_GXEPD2_INPUT);
    hardwareReset(20, 50);
    busyWait();
    if (lastOperationError() != DriverOperationError::None) return false;
    _rotation = 0;
    _width = _init_width;
    _height = _init_height;
    if (applyWaveformProfile(EPaperWaveformMode::Full, _busyPin, true)) {
        // Keep the vendor PSR bytes (0xF7, 0x0D) written by the full profile.
        // setRotation() remains available as an explicit controller override,
        // but invoking it here would immediately replace the two-byte panel
        // setup with the one-byte rotation value 0x1F.
        return lastOperationError() == DriverOperationError::None;
    }
    if (lastOperationError() != DriverOperationError::None) return false;
    // Exact fallback copy of JD79686B_Init.h. The normal profile path above
    // sends the same sequence; keep this complete so a missing profile cannot
    // silently skip the controller-specific power-on setup.
    _bus->writeCommand(0x4D);
    _bus->writeData(0x55);
    _bus->writeCommand(0xA6);
    _bus->writeData(0x38);
    _bus->writeCommand(0xB4);
    _bus->writeData(0x5D);
    _bus->writeCommand(0xB6);
    _bus->writeData(0x80);
    _bus->writeCommand(0xB7);
    _bus->writeData(0x00);
    _bus->writeCommand(0xF7);
    _bus->writeData(0x02);
    _bus->writeCommand(0x00); // PSR
    _bus->writeData(0xF7);
    _bus->writeData(0x0D);
    return lastOperationError() == DriverOperationError::None;
}

void Driver_JD79686B::setRotation(uint8_t m) {
    _rotation = m % 4;
    _bus->writeCommand(0x00);
    switch (_rotation) {
        case 0:
            _bus->writeData(0x1F);
            _width = _init_width; _height = _init_height;
            break;
        case 1:
            _bus->writeData(0x1B);
            _width = _init_height; _height = _init_width;
            break;
        case 2:
            _bus->writeData(0x13);
            _width = _init_width; _height = _init_height;
            break;
        case 3:
            _bus->writeData(0x17);
            _width = _init_height; _height = _init_width;
            break;
    }
}

void Driver_JD79686B::invertDisplay(bool) {}
void Driver_JD79686B::displayOn()  { update(); }
void Driver_JD79686B::displayOff() {}

void Driver_JD79686B::setAddrWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    _bus->writeCommand(0x91);
    _bus->writeCommand(0x90);
    _bus->writeData((x1 >> 8) & 0xFF);
    _bus->writeData(x1 & 0xFF);
    _bus->writeData((x2 >> 8) & 0xFF);
    _bus->writeData(x2 & 0xFF);
    _bus->writeData((y1 >> 8) & 0xFF);
    _bus->writeData(y1 & 0xFF);
    _bus->writeData((y2 >> 8) & 0xFF);
    _bus->writeData(y2 & 0xFF);
    _bus->writeData(0x01);
}

void Driver_JD79686B::writePixel(uint16_t) {}
void Driver_JD79686B::writePixels(const uint16_t*, size_t) {}
void Driver_JD79686B::writeFill(uint16_t, size_t) {}

void Driver_JD79686B::sleep() {
    _bus->writeCommand(0x02);
    busyWait();
    _bus->writeCommand(0x07);
    _bus->writeData(0xA5);
}

void Driver_JD79686B::wake() {
    (void)init(*_bus);
}

void Driver_JD79686B::update() {
    _bus->writeCommand(0x04);
    busyWait();
    _bus->writeCommand(0x12);
    busyWait();
}

void Driver_JD79686B::pushColors(const uint8_t* data, uint16_t w, uint16_t h) {
    _bus->writeCommand(0x13);
    uint32_t count = (uint32_t)w * h / 8;
    for (uint32_t i = 0; i < count; i++) {
        _bus->writeData(data[i]);
    }
}

void Driver_JD79686B::pushColorsFlip(const uint8_t* data, uint16_t w, uint16_t h) {
    _bus->writeCommand(0x13);
    uint16_t bytes_per_row = w / 8;
    for (uint16_t row = 0; row < h; row++) {
        uint16_t start = row * bytes_per_row;
        for (uint16_t col = 0; col < bytes_per_row; col++) {
            uint8_t b = data[start + (bytes_per_row - 1 - col)];
            b = ((b & 0xF0) >> 4) | ((b & 0x0F) << 4);
            b = ((b & 0xCC) >> 2) | ((b & 0x33) << 2);
            b = ((b & 0xAA) >> 1) | ((b & 0x55) << 1);
            _bus->writeData(b);
        }
    }
}

void Driver_JD79686B::pushOldColors(const uint8_t* data, uint16_t w, uint16_t h) {
    _bus->writeCommand(0x10);
    uint32_t count = (uint32_t)w * h / 8;
    for (uint32_t i = 0; i < count; i++) {
        _bus->writeData(data[i]);
    }
}

void Driver_JD79686B::pushOldColorsFlip(const uint8_t* data, uint16_t w, uint16_t h) {
    _bus->writeCommand(0x10);
    uint16_t bytes_per_row = w / 8;
    for (uint16_t row = 0; row < h; row++) {
        uint16_t start = row * bytes_per_row;
        for (uint16_t col = 0; col < bytes_per_row; col++) {
            uint8_t b = data[start + (bytes_per_row - 1 - col)];
            b = ((b & 0xF0) >> 4) | ((b & 0x0F) << 4);
            b = ((b & 0xCC) >> 2) | ((b & 0x33) << 2);
            b = ((b & 0xAA) >> 1) | ((b & 0x55) << 1);
            _bus->writeData(b);
        }
    }
}

} }
