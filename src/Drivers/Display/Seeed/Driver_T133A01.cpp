// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "Driver_T133A01.h"
#include "SeeedHal.h"
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file   Driver_T133A01.cpp
 * @brief  T133A01 ePaper display driver implementation
 *
 * 1200x1600 4bpp indexed Spectra 6 driver with dual-chip
 * (master/slave) architecture. The physical colors are Black, White, Red,
 * Yellow, Green, and Blue.
 * Ported from TFT_Drivers/T133A01_Defines.h and T133A01_Init.h
 */


Driver_T133A01::Driver_T133A01(uint16_t w, uint16_t h, int8_t busyPin)
    : _init_width(w), _init_height(h), _busyPin(busyPin)
{
    _width = w;
    _height = h;
}

void Driver_T133A01::busyWait() {
    if (_busyPin < 0) return;
    // T133A01 asserts BUSY after accepting PON/DRF/POF. Give it time to
    // drive the pin before sampling; otherwise a stale READY-high level can
    // be mistaken for command completion. This matches the original
    // T133A01 CHECK_BUSY() guard interval.
    delay(10);
    (void)waitForReadyPin(_busyPin, true);
}

void Driver_T133A01::writeCommandData(ChipSelectTarget target, uint8_t cmd,
                                      const uint8_t* data, size_t len) {
    _bus->selectChip(target);
    _bus->beginWrite();
    _bus->writeCommand(cmd);
    if (data && len) _bus->writeData(data, len);
    _bus->endWrite();
}

bool Driver_T133A01::init(IBus& bus) {
    _bus = &bus;
    if (!bus.supportsSecondaryChipSelect()) return false;

    if (_busyPin >= 0) pinMode(_busyPin, NGFX_GXEPD2_INPUT);
    if (_resetPin >= 0) {
        gfxPinModeOutput(_resetPin);
        gfxDigitalWrite(_resetPin, false);
        delay(20);
        gfxDigitalWrite(_resetPin, true);
        delay(20);
    }
    busyWait();

    static const uint8_t r74[] = {0x00, 0x0C, 0x0C, 0xD9, 0xDD, 0xDD, 0x15, 0x15, 0x55};
    static const uint8_t rf0[] = {0x49, 0x55, 0x13, 0x5D, 0x05, 0x10};
    static const uint8_t psr[] = {0xDF, 0x69};
    static const uint8_t dcdc[] = {0x44, 0x54, 0x00};
    static const uint8_t cdi[] = {0x37};
    static const uint8_t r60[] = {0x03, 0x03};
    static const uint8_t r86[] = {0x10};
    static const uint8_t pws[] = {0x22};
    static const uint8_t tres[] = {0x04, 0xB0, 0x03, 0x20};
    static const uint8_t pwr[] = {0x0F, 0x00, 0x28, 0x2C, 0x28, 0x38};
    static const uint8_t rb6[] = {0x07};
    static const uint8_t btst[] = {0xE0, 0x20};
    static const uint8_t rb7[] = {0x01};
    static const uint8_t rb0[] = {0x01};
    static const uint8_t rb1[] = {0x02};

    writeCommandData(ChipSelectTarget::Primary,   0x74, r74, sizeof(r74));
    writeCommandData(ChipSelectTarget::Both,      0xF0, rf0, sizeof(rf0));
    delay(10);
    writeCommandData(ChipSelectTarget::Both,      0x00, psr, sizeof(psr));
    delay(10);
    writeCommandData(ChipSelectTarget::Primary,   0xA5, dcdc, sizeof(dcdc));
    delay(10);
    writeCommandData(ChipSelectTarget::Both,      0x50, cdi, sizeof(cdi));
    delay(10);
    writeCommandData(ChipSelectTarget::Both,      0x60, r60, sizeof(r60));
    delay(10);
    writeCommandData(ChipSelectTarget::Both,      0x86, r86, sizeof(r86));
    delay(10);
    writeCommandData(ChipSelectTarget::Both,      0xE3, pws, sizeof(pws));
    delay(10);
    writeCommandData(ChipSelectTarget::Both,      0x61, tres, sizeof(tres));
    delay(10);
    writeCommandData(ChipSelectTarget::Primary,   0x01, pwr, sizeof(pwr));
    delay(10);
    writeCommandData(ChipSelectTarget::Primary,   0xB6, rb6, sizeof(rb6));
    delay(10);
    writeCommandData(ChipSelectTarget::Primary,   0x06, btst, sizeof(btst));
    delay(10);
    writeCommandData(ChipSelectTarget::Primary,   0xB7, rb7, sizeof(rb7));
    delay(10);
    writeCommandData(ChipSelectTarget::Primary,   0x05, btst, sizeof(btst));
    delay(10);
    writeCommandData(ChipSelectTarget::Primary,   0xB0, rb0, sizeof(rb0));
    delay(10);
    writeCommandData(ChipSelectTarget::Primary,   0xB1, rb1, sizeof(rb1));
    delay(10);
    _bus->selectChip(ChipSelectTarget::Primary);

    return lastOperationError() == DriverOperationError::None;
}

void Driver_T133A01::setRotation(uint8_t rotation) {
    _rotation = rotation % 4;
    switch (_rotation) {
        case 0: case 2: _width = _init_width; _height = _init_height; break;
        case 1: case 3: _width = _init_height; _height = _init_width; break;
    }
}

void Driver_T133A01::invertDisplay(bool invert) {
    (void)invert;
}

void Driver_T133A01::displayOn() {
    update();
}

void Driver_T133A01::displayOff() {
    sleep();
}

void Driver_T133A01::setAddrWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye) {
    (void)xs; (void)ys; (void)xe; (void)ye;
}

void Driver_T133A01::writePixel(uint16_t color) {
    (void)color;
}

void Driver_T133A01::writePixels(const uint16_t* data, size_t len) {
    (void)data; (void)len;
}

void Driver_T133A01::writeFill(uint16_t color, size_t len) {
    (void)color; (void)len;
}

void Driver_T133A01::sleep() {
    const uint8_t sleepCode = 0xA5;
    writeCommandData(ChipSelectTarget::Primary, 0x07, &sleepCode, 1);
    busyWait();
}

void Driver_T133A01::wake() {
    // Hardware reset via RST, then re-init
    init(*_bus);
}

void Driver_T133A01::update() {
    static const uint8_t drf = 0x01;
    static const uint8_t pof = 0x00;
    writeCommandData(ChipSelectTarget::Both, 0x04, nullptr, 0);
    busyWait();
    delay(30);
    writeCommandData(ChipSelectTarget::Both, 0x12, &drf, 1);
    busyWait();
    delay(30);
    writeCommandData(ChipSelectTarget::Both, 0x02, &pof, 1);
    busyWait();
    delay(30);
    _bus->selectChip(ChipSelectTarget::Primary);
}

uint8_t Driver_T133A01::colorGet(uint8_t color) {
    switch (color & 0x0F) {
        case 0x0F: return 0x00; // black
        case 0x00: return 0x01; // white
        case 0x0B: return 0x02; // yellow
        case 0x06: return 0x03; // red
        case 0x0D: return 0x05; // blue
        case 0x02: return 0x06; // green
        default:   return 0x01;
    }
}

void Driver_T133A01::pushColors(const uint8_t* data, uint16_t w, uint16_t h) {
    if (!data || w < 4) return;
    const uint16_t halfBytes = static_cast<uint16_t>(w / 4);
    const uint16_t rowBytes = static_cast<uint16_t>(w / 2);
    const uint8_t ccset = 0x01;
    writeCommandData(ChipSelectTarget::Both, 0xE0, &ccset, 1);
    busyWait();
    for (uint8_t chip = 0; chip < 2; ++chip) {
        _bus->selectChip(chip == 0 ? ChipSelectTarget::Primary
                                  : ChipSelectTarget::Secondary);
        _bus->beginWrite();
        _bus->writeCommand(0x10);
        for (uint16_t row = 0; row < h; ++row) {
            const uint8_t* src = data + static_cast<size_t>(row) * rowBytes
                               + static_cast<size_t>(chip) * halfBytes;
            for (uint16_t col = 0; col < halfBytes; ++col) {
                const uint8_t value = src[col];
                _bus->writeData(static_cast<uint8_t>(
                    (colorGet(value >> 4) << 4) | colorGet(value)));
            }
        }
        _bus->endWrite();
    }
    _bus->selectChip(ChipSelectTarget::Primary);
}

void Driver_T133A01::pushColorsFlip(const uint8_t* data, uint16_t w, uint16_t h) {
    // Panel_EPaper already mirrors the packed 4bpp frame before calling the
    // generic pushNewColors() entry point.
    pushColors(data, w, h);
}

void Driver_T133A01::pushOldColors(const uint8_t* data, uint16_t w, uint16_t h) {
    (void)data; (void)w; (void)h;
}

void Driver_T133A01::pushOldColorsFlip(const uint8_t* data, uint16_t w, uint16_t h) {
    (void)data; (void)w; (void)h;
}

void Driver_T133A01::setTemp(uint8_t temp) {
    writeCommandData(ChipSelectTarget::Primary, 0xE5, &temp, 1);
    _bus->selectChip(ChipSelectTarget::Primary);
}

} }
