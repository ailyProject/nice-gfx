// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "Driver_GDEB0709E01.h"
#include "SeeedHal.h"
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file   Driver_GDEB0709E01.cpp
 * @brief  GDEB0709E01 ePaper display driver implementation
 *
 * Good Display GDEB0709E01, 7.09" E Ink Spectra 6, 1200x1600, 4bpp indexed.
 * Drive ICs (vendor statement 2026-08): NT61522 (PVT61522) x1 + EK73601 x1.
 * The physical colors are Black, White, Red, Yellow, Green, and Blue.
 *
 * Cloned from Driver_T133A01 (13.3" Spectra 6, reTerminal E1004) which is
 * the same dual-COG architecture: waveforms live in the COG OTP, so there is
 * no host LUT to download; CCSET(0xE0)=0x01 selects the OTP waveform group
 * and must be re-broadcast before every data push (the vendor example only
 * writes it during init because it re-runs the full init before every
 * frame; Seeed_GFX2 initializes once and updates many times).
 *
 * Register value-set decision (2026-08-28):
 *   This init keeps the T133A01 values byte for byte:
 *     - empirically verified on Seeed hardware: a colleague flashed this
 *       exact 7.09" panel using the 13.3" board and code;
 *     - the vendor endorsed "reference the 13.3" E6 directly" when handing
 *       over the IC part numbers.
 *   The Good Display ESP32 example (main/GDEP133C02.c) differs in four
 *   places. Those values were only verified on the vendor's 5V carrier
 *   board, NOT on Seeed hardware. Keep them here as the documented first
 *   fallback if light-up fails:
 *     0x74 AN_TM = {C0 1E 1E CE CE CE 15 15 55}  (here: 00 0C 0C D9 DD DD
 *                                                 15 15 55, panel analog
 *                                                 timing)
 *     0x50 CDI   = F7                             (here: 37)
 *     0x06/0x05 BTST = E8 28                      (here: E0 20; power-
 *                                                  related, acts on the
 *                                                  board-side pump)
 *     0xA5       = not sent at all in the example (here: 44 54 00 is kept;
 *                  undocumented DC-DC/power command, ESPHome names it
 *                  RA5_DCDC; the Seeed pump circuitry is tuned together
 *                  with T133A01, so on a pump-start failure this value is
 *                  the first suspect to try adding/removing after
 *                  measuring VGH/VGL/VDDP/VDDN/VCOM build-up)
 *
 * References:
 *   - Good Display example: d:\SEEED\GDEB0709E01-ESP32 code example\...
 *   - BOE module spec A01 (24116-01390): BUSY ready=NGFX_GXEPD2_HIGH (external pull-up
 *     required), full refresh ~27s @ 25°C, 4-wire SPI write-only.
 *   - Plan doc: d:\SEEED\text\Seeed_GFX2_7.09寸GDEB0709E01支持方案.md
 */


Driver_GDEB0709E01::Driver_GDEB0709E01(uint16_t w, uint16_t h, int8_t busyPin)
    : _init_width(w), _init_height(h), _busyPin(busyPin)
{
    _width = w;
    _height = h;
}

void Driver_GDEB0709E01::busyWait() {
    if (_busyPin < 0) return;
    // The controller asserts BUSY after accepting PON/DRF/POF. Give it time
    // to drive the pin before sampling; otherwise a stale READY-high level
    // can be mistaken for command completion (same guard interval as the
    // T133A01 CHECK_BUSY() logic).
    delay(10);
    // Full refresh takes ~27s at 25°C and slows down toward the 0°C end of
    // the operating range, so the timeout must sit well above 30s. On
    // timeout the caller must NOT power-cut or deep-sleep the panel mid
    // refresh, otherwise the frame freezes at an intermediate state.
    (void)waitForReadyPin(_busyPin, true, 60000);
}

void Driver_GDEB0709E01::writeCommandData(ChipSelectTarget target, uint8_t cmd,
                                          const uint8_t* data, size_t len) {
    _bus->selectChip(target);
    _bus->beginWrite();
    _bus->writeCommand(cmd);
    if (data && len) _bus->writeData(data, len);
    _bus->endWrite();
}

bool Driver_GDEB0709E01::init(IBus& bus) {
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

    // Register values: T133A01 set, see the file header for the decision
    // and for the four vendor-example fallback values.
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

void Driver_GDEB0709E01::setRotation(uint8_t rotation) {
    _rotation = rotation % 4;
    switch (_rotation) {
        case 0: case 2: _width = _init_width; _height = _init_height; break;
        case 1: case 3: _width = _init_height; _height = _init_width; break;
    }
}

void Driver_GDEB0709E01::invertDisplay(bool invert) {
    (void)invert;
}

void Driver_GDEB0709E01::displayOn() {
    update();
}

void Driver_GDEB0709E01::displayOff() {
    sleep();
}

void Driver_GDEB0709E01::setAddrWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye) {
    (void)xs; (void)ys; (void)xe; (void)ye;
}

void Driver_GDEB0709E01::writePixel(uint16_t color) {
    (void)color;
}

void Driver_GDEB0709E01::writePixels(const uint16_t* data, size_t len) {
    (void)data; (void)len;
}

void Driver_GDEB0709E01::writeFill(uint16_t color, size_t len) {
    (void)color; (void)len;
}

void Driver_GDEB0709E01::sleep() {
    // 0x07+0xA5 deep sleep extrapolated from the 13.3" T133A01; the
    // GDEB0709E01 vendor example does not demonstrate deep sleep, so this
    // path is unverified on this panel. Wake requires a hardware reset.
    const uint8_t sleepCode = 0xA5;
    writeCommandData(ChipSelectTarget::Primary, 0x07, &sleepCode, 1);
    busyWait();
}

void Driver_GDEB0709E01::wake() {
    // Hardware reset via RST, then re-init
    init(*_bus);
}

void Driver_GDEB0709E01::update() {
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

uint8_t Driver_GDEB0709E01::colorGet(uint8_t color) {
    // Panel nibble codes match the vendor example color macros:
    // BLACK=0x00 WHITE=0x11 YELLOW=0x22 RED=0x33 BLUE=0x55 GREEN=0x66
    // (i.e. nibble 0 black, 1 white, 2 yellow, 3 red, 5 blue, 6 green).
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

void Driver_GDEB0709E01::pushColors(const uint8_t* data, uint16_t w, uint16_t h) {
    if (!data || w < 4) return;
    const uint16_t halfBytes = static_cast<uint16_t>(w / 4);
    const uint16_t rowBytes = static_cast<uint16_t>(w / 2);
    // Re-broadcast CCSET before every data push (OTP waveform group
    // select). Do not fold this into init() only: the vendor example gets
    // away with init-time CCSET solely because it re-initializes the panel
    // before every frame.
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

void Driver_GDEB0709E01::pushColorsFlip(const uint8_t* data, uint16_t w, uint16_t h) {
    // Panel_EPaper already mirrors the packed 4bpp frame before calling the
    // generic pushNewColors() entry point.
    pushColors(data, w, h);
}

void Driver_GDEB0709E01::pushOldColors(const uint8_t* data, uint16_t w, uint16_t h) {
    (void)data; (void)w; (void)h;
}

void Driver_GDEB0709E01::pushOldColorsFlip(const uint8_t* data, uint16_t w, uint16_t h) {
    (void)data; (void)w; (void)h;
}

void Driver_GDEB0709E01::setTemp(uint8_t temp) {
    // 0xE5 temperature write carried over from T133A01; the vendor example
    // does not use register-based temperature compensation (the module has
    // an I2C temperature sensor instead), so this path is unverified.
    writeCommandData(ChipSelectTarget::Primary, 0xE5, &temp, 1);
    _bus->selectChip(ChipSelectTarget::Primary);
}

} }
