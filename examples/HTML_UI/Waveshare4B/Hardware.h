#pragma once
#include <Display/Presets/VideoAliases.h>
#include <Platform/Arduino/I2cWire.h>
#include "TCA9554.h"
#include <Drivers/Display/Video/ST7701_RGB.h>
// Board wiring belongs to the generated application, not the chip drivers.
inline nice_gfx::I2cWire &boardI2cBus() {static nice_gfx::I2cWire bus(Wire);return bus;}
inline waveshare4b::Tca9554CommandBus &displayCommandBus() {
  static waveshare4b::Tca9554CommandBus commands(boardI2cBus(),0x20,0,1,2);return commands;
}
inline bool beginBoardPeripherals() {using namespace nice_gfx;
    static bool ready=false; if(ready)return true;
    auto &commands=displayCommandBus();
    pinMode(4, OUTPUT); digitalWrite(4, HIGH); // off during initialization
    if (!Wire.begin(47, 48, 400000)) return false;
    Wire.setTimeOut(20);
    if (commands.begin() != Status::Ok) return false;
    // LCD RESET is P7; it is separate from the GT911 RESET on P5.
    if (commands.level(0x80, true) != Status::Ok ||
        commands.direction(0x80, false) != Status::Ok ||
        commands.level(0x80, false) != Status::Ok) return false;
    delay(20);
    if (commands.level(0x80, true) != Status::Ok) return false;
    delay(200);
    // Hold GT911 INT P6 low while resetting P5 to select address 0x5D.
    if (commands.level(0x60, false) != Status::Ok ||
        commands.direction(0x60, false) != Status::Ok) return false;
    delay(200);
    if (commands.level(0x20, true) != Status::Ok) return false;
    delay(200);
    if (commands.direction(0x40, true) != Status::Ok) return false;
    ready = true; return true;
}
inline nice_gfx::RgbConfig displayConfig() {using namespace nice_gfx;
    RgbConfig c;
    c.controller = VideoController::ST7701; c.createIo = false;
    c.commandWrite = waveshare4b::Tca9554CommandBus::write; c.commandContext = &displayCommandBus();
    c.init = VideoInitOverride(video::ST7701_RGB_Type1_Init,
      sizeof(video::ST7701_RGB_Type1_Init) / sizeof(VideoInitCommand));
    c.rgb.data_width = 16; c.rgb.flags.fb_in_psram = true;
    c.rgb.num_fbs = 1; c.rgb.bounce_buffer_size_px = 480 * 20;
    c.frameAtomic = true;
    // Core 0 services RGB bounce DMA while Arduino's UI runs on Core 1.
    c.interruptCore = 0;
    c.rgb.pclk_gpio_num = 9; c.rgb.hsync_gpio_num = 46;
    c.rgb.vsync_gpio_num = 3; c.rgb.de_gpio_num = 17;
    const int pins[] = {40,41,42,2,1,21,8,18,45,38,39,10,11,12,13,14};
    for (unsigned i = 0; i < 16; ++i)
      c.rgb.data_gpio_nums[i] = static_cast<decltype(c.rgb.disp_gpio_num)>(pins[i]);
    c.rgb.timings.pclk_hz = 12000000;
    c.rgb.timings.h_res = c.rgb.timings.v_res = 480;
    c.rgb.timings.hsync_pulse_width = c.rgb.timings.hsync_back_porch = 10;
    c.rgb.timings.hsync_front_porch = 20;
    c.rgb.timings.vsync_pulse_width = c.rgb.timings.vsync_back_porch = c.rgb.timings.vsync_front_porch = 10;
    c.rgb.timings.flags.pclk_active_neg = false;
    return c;
}
