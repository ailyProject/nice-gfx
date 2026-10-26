#pragma once
#include "../../../Drivers/Display/Epaper/EpaperTypes.h"
namespace nice_gfx {
namespace epaper {
// These waveforms and voltage bytes are specific to the named Waveshare V2
// modules, not generic SSD168x defaults. Do not reuse on an unverified glass.
// Source: https://github.com/waveshareteam/e-Paper/tree/master/Arduino
//
// Copyright (C) Waveshare, 2019-2020.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
inline const uint8_t *waveshare1in54V2PartialLut(){
  static const uint8_t lut[]={
    0,0x40,0,0,0,0,0,0,0,0,0,0,
    0x80,0x80,0,0,0,0,0,0,0,0,0,0,
    0x40,0x40,0,0,0,0,0,0,0,0,0,0,
    0,0x80,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,
    0x0F,0,0,0,0,0,0, 1,1,0,0,0,0,0,
    0,0,0,0,0,0,0, 0,0,0,0,0,0,0, 0,0,0,0,0,0,0,
    0,0,0,0,0,0,0, 0,0,0,0,0,0,0, 0,0,0,0,0,0,0,
    0,0,0,0,0,0,0, 0,0,0,0,0,0,0, 0,0,0,0,0,0,0, 0,0,0,0,0,0,0,
    0x22,0x22,0x22,0x22,0x22,0x22,0,0,0,
    0x02,0x17,0x41,0xB0,0x32,0x28
  };static_assert(sizeof(lut)==159,"1.54 V2 partial waveform");return lut;
}
inline const uint8_t *waveshare2in9V2PartialLut(){
  static const uint8_t lut[]={
    0,0x40,0,0,0,0,0,0,0,0,0,0,
    0x80,0x80,0,0,0,0,0,0,0,0,0,0,
    0x40,0x40,0,0,0,0,0,0,0,0,0,0,
    0,0x80,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,
    0x0A,0,0,0,0,0,2, 1,0,0,0,0,0,0, 1,0,0,0,0,0,0,
    0,0,0,0,0,0,0, 0,0,0,0,0,0,0, 0,0,0,0,0,0,0,
    0,0,0,0,0,0,0, 0,0,0,0,0,0,0, 0,0,0,0,0,0,0,
    0,0,0,0,0,0,0, 0,0,0,0,0,0,0, 0,0,0,0,0,0,0,
    0x22,0x22,0x22,0x22,0x22,0x22,0,0,0,
    0x22,0x17,0x41,0xB0,0x32,0x36
  };static_assert(sizeof(lut)==159,"2.9 V2 partial waveform");return lut;
}
inline const EpaperProfile &waveshare1in54V2(){
  static const uint8_t lut[]={
    0x80,0x48,0x40,0,0,0,0,0,0,0,0,0,
    0x40,0x48,0x80,0,0,0,0,0,0,0,0,0,
    0x80,0x48,0x40,0,0,0,0,0,0,0,0,0,
    0x40,0x48,0x80,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,
    0x0A,0,0,0,0,0,0,
    0x08,0x01,0,0x08,0x01,0,0x02,
    0x0A,0,0,0,0,0,0,
    0,0,0,0,0,0,0, 0,0,0,0,0,0,0, 0,0,0,0,0,0,0,
    0,0,0,0,0,0,0, 0,0,0,0,0,0,0, 0,0,0,0,0,0,0,
    0,0,0,0,0,0,0, 0,0,0,0,0,0,0, 0,0,0,0,0,0,0,
    0x22,0x22,0x22,0x22,0x22,0x22,0,0,0,
    0x22,0x17,0x41,0,0x32,0x20
  };
  static_assert(sizeof(lut)==159,"Waveshare 1.54 V2 waveform length");
  static const EpaperProfile profile={"Waveshare 1.54 V2 B/W",EpaperController::SSD1681,200,200,lut,sizeof(lut),3000,15000,200,true,waveshare1in54V2PartialLut(),500};
  return profile;
}
inline const EpaperProfile &waveshare2in9V2(){
  static const uint8_t lut[]={
    0x80,0x66,0,0,0,0,0,0,0x40,0,0,0,
    0x10,0x66,0,0,0,0,0,0,0x20,0,0,0,
    0x80,0x66,0,0,0,0,0,0,0x40,0,0,0,
    0x10,0x66,0,0,0,0,0,0,0x20,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,
    0x14,0x08,0,0,0,0,0x01,
    0x0A,0x0A,0,0x0A,0x0A,0,0x01,
    0,0,0,0,0,0,0, 0,0,0,0,0,0,0,
    0,0,0,0,0,0,0, 0,0,0,0,0,0,0,
    0,0,0,0,0,0,0, 0,0,0,0,0,0,0,
    0x14,0x08,0,0x01,0,0,0x01,
    0,0,0,0,0,0,0x01,
    0,0,0,0,0,0,0, 0,0,0,0,0,0,0,
    0x44,0x44,0x44,0x44,0x44,0x44,0,0,0,
    0x22,0x17,0x41,0,0x32,0x36
  };
  static_assert(sizeof(lut)==159,"Waveshare 2.9 V2 waveform length");
  static const EpaperProfile profile={"Waveshare 2.9 V2 B/W",EpaperController::SSD1680,128,296,lut,sizeof(lut),3000,15000,5,false,waveshare2in9V2PartialLut(),500};
  return profile;
}
} // namespace epaper
} // namespace nice_gfx
