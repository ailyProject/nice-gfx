// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "Driver_IT8951.h"
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file Driver_ED103TC2.h
 * @brief ED103TC2 panel specialization using the IT8951 external TCON.
 */
#ifndef NGFX_SEEED_SEEED_GFX_DRIVER_ED103TC2_H
#define NGFX_SEEED_SEEED_GFX_DRIVER_ED103TC2_H

class Driver_ED103TC2 : public Driver_IT8951 {
public:
    Driver_ED103TC2(uint16_t w = NGFX_SEEED_IT8951_PANEL_WIDTH,
                    uint16_t h = NGFX_SEEED_IT8951_PANEL_HEIGHT,
                    int8_t busyPin = -1);

    const char* name() const override { return "ED103TC2"; }
    uint8_t colorDepth() const override { return 1; }
    bool supportsPartialRefresh() const override { return true; }
    uint16_t partialXAlignment() const override { return 16; }
    bool supportsGrayRefresh(uint8_t levels) const override { return levels == 16; }

    void setAddrWindow(uint16_t xs, uint16_t ys,
                       uint16_t xe, uint16_t ye) override;
    void update() override;
    void updatePartial() override;
    void updateGray() override;

    void pushNewColors(const uint8_t* data, size_t len) override;
    void pushOldColors(const uint8_t* data, size_t len) override;
    void pushGrayColors(const uint8_t* data, size_t len) override;
    void setTemperature(int8_t temp) override;

private:
    uint16_t _panelWidth;
    uint16_t _panelHeight;
    TCONAreaImgInfo _window;
};

#endif

} }
