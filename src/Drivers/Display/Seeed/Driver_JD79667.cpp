// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "Driver_JD79667.h"
#include "seeed_ep.h"
#include "SeeedHal.h"
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file   Driver_JD79667.cpp
 * @brief  JD79667 ePaper display driver implementation
 *
 * Ported from TFT_Drivers/JD79667_Defines.h, JD79667_Init.h, JD79667_Rotation.h
 * 4-bit color depth (16-level grayscale) JD796xx family.
 */



Driver_JD79667::Driver_JD79667(uint16_t w, uint16_t h, int8_t busyPin)
    : _init_width(w), _init_height(h), _busyPin(busyPin) {
    _width = w; _height = h;
}

void Driver_JD79667::busyWait() {
    if (_busyPin < 0) return;
    (void)waitForReadyPin(_busyPin, true);
}

uint8_t Driver_JD79667::colorGet(uint8_t color) {
    switch (color) {
        case 0x00: return 0x01;
        case 0x0B: return 0x02;
        case 0x06: return 0x03;
        case 0x0F: return 0x00;
        default:   return 0x00;
    }
}

bool Driver_JD79667::init(IBus& bus) {
    _bus = &bus;
    if (_busyPin >= 0) pinMode(_busyPin, NGFX_GXEPD2_INPUT);
    hardwareReset(20, 20);
    busyWait();
    if (lastOperationError() != DriverOperationError::None) return false;
    _rotation = 0;
    _width = _init_width;
    _height = _init_height;
    if (applyWaveformProfile(EPaperWaveformMode::Full, _busyPin, true)) {
        setRotation(0);
        return lastOperationError() == DriverOperationError::None;
    }
    if (lastOperationError() != DriverOperationError::None) return false;
    _bus->writeCommand(0x4D);
    _bus->writeData(0x78);
    _bus->writeCommand(0x00); // PSR
    _bus->writeData(0x0F);
    _bus->writeData(0x29);
    _bus->writeCommand(0x01); // PWRR
    _bus->writeData(0x07);
    _bus->writeData(0x00);
    _bus->writeCommand(0x03); // POFS
    _bus->writeData(0x10);
    _bus->writeData(0x54);
    _bus->writeData(0x44);
    _bus->writeCommand(0x06); // BTST_P
    _bus->writeData(0x05);
    _bus->writeData(0x00);
    _bus->writeData(0x3F);
    _bus->writeData(0x0A);
    _bus->writeData(0x25);
    _bus->writeData(0x12);
    _bus->writeData(0x1A);
    _bus->writeCommand(0x50); // CDI
    _bus->writeData(0x37);
    _bus->writeCommand(0x60); // TCON
    _bus->writeData(0x02);
    _bus->writeData(0x02);
    _bus->writeCommand(0x61); // TRES
    _bus->writeData(128 / 256);
    _bus->writeData(128 % 256);
    _bus->writeData(296 / 256);
    _bus->writeData(296 % 256);
    _bus->writeCommand(0xE7);
    _bus->writeData(0x1C);
    _bus->writeCommand(0xE3);
    _bus->writeData(0x22);
    _bus->writeCommand(0xB4);
    _bus->writeData(0xD0);
    _bus->writeCommand(0xB5);
    _bus->writeData(0x03);
    _bus->writeCommand(0xE9);
    _bus->writeData(0x01);
    _bus->writeCommand(0x30);
    _bus->writeData(0x08);
    _bus->writeCommand(0x04); // Power on
    busyWait();
    setRotation(0);
    return lastOperationError() == DriverOperationError::None;
}

void Driver_JD79667::setRotation(uint8_t m) {
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

void Driver_JD79667::invertDisplay(bool) {}
void Driver_JD79667::displayOn()  { update(); }
void Driver_JD79667::displayOff() {}
void Driver_JD79667::setAddrWindow(uint16_t, uint16_t, uint16_t, uint16_t) {}
void Driver_JD79667::writePixel(uint16_t) {}
void Driver_JD79667::writePixels(const uint16_t*, size_t) {}
void Driver_JD79667::writeFill(uint16_t, size_t) {}

void Driver_JD79667::sleep() {
    _bus->writeCommand(0x02);
    busyWait();
    delay(100);
    _bus->writeCommand(0x07);
    _bus->writeData(0xA5);
}

void Driver_JD79667::wake() {
    init(*_bus);
}

void Driver_JD79667::update() {
    _bus->writeCommand(0x12);
    _bus->writeData(0x00);
    busyWait();
}

void Driver_JD79667::pushColors(const uint8_t* colors, uint16_t w, uint16_t h) {
    uint16_t bytes_per_row = w / 2;
    _bus->writeCommand(0x10);
    for (uint16_t row = 0; row < h; row++) {
        for (uint16_t col = 0; col < bytes_per_row; col += 2) {
            uint8_t b = colors[bytes_per_row * row + col];
            uint8_t c = colors[bytes_per_row * row + col + 1];
            uint8_t temp1 = (b >> 4) & 0x0F;
            uint8_t temp2 =  b       & 0x0F;
            uint8_t temp3 = (c >> 4) & 0x0F;
            uint8_t temp4 =  c       & 0x0F;
            _bus->writeData((colorGet(temp1) << 6) | (colorGet(temp2) << 4) |
                           (colorGet(temp3) << 2) | (colorGet(temp4) << 0));
        }
    }
}

void Driver_JD79667::pushColorsFlip(const uint8_t* colors, uint16_t w, uint16_t h) {
    uint16_t bytes_per_row = w / 2;
    _bus->writeCommand(0x10);
    for (uint16_t row = 0; row < h; row++) {
        for (uint16_t col = 0; col < bytes_per_row; col += 2) {
            uint8_t b = colors[bytes_per_row * row + (bytes_per_row - 1 - col)];
            uint8_t c = colors[bytes_per_row * row + (bytes_per_row - 1 - col) - 1];
            uint8_t temp1 = (b >> 4) & 0x0F;
            uint8_t temp2 =  b       & 0x0F;
            uint8_t temp3 = (c >> 4) & 0x0F;
            uint8_t temp4 =  c       & 0x0F;
            _bus->writeData((colorGet(temp1) << 6) | (colorGet(temp2) << 4) |
                           (colorGet(temp3) << 2) | (colorGet(temp4) << 0));
        }
    }
}

void Driver_JD79667::pushOldColors(const uint8_t*, uint16_t, uint16_t) {}
void Driver_JD79667::pushOldColorsFlip(const uint8_t*, uint16_t, uint16_t) {}

} }
