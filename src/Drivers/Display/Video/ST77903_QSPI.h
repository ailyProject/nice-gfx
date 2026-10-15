// ST77903 QSPI module register values. The GFX scanner manages the bus,
// framebuffers, scan timing and lifecycle.
// Reference: ST77903_SPEC_P0.5.pdf, commands B5/B6 and QSPI interface;
// ST77903_Customer_Application_Notes.pdf, pp. 2-3, continuous line protocol.
#pragma once
#include "VideoControllers.h"

namespace nice_gfx { namespace video {
namespace st77903_scan_values {
static const uint8_t key0[] = {0xC3}, key1[] = {0x96}, key2[] = {0xA5};
static const uint8_t e9[] = {0x20};
static const uint8_t e7[] = {0x80,0x77,0x1F,0xCC};
static const uint8_t c1[] = {0x77,0x07,0xCF,0x16};
static const uint8_t c3[] = {0x22,0x02,0x22,0x04};
static const uint8_t c5[] = {0xED};
static const uint8_t gamma[] = {0x87,0x09,0x0C,0x06,0x05,0x03,0x29,0x32,0x49,0x0F,0x1B,0x17,0x2A,0x2F};
static const uint8_t analog[] = {0xBE,0xF5,0xB1,0x22,0x22,0x25,0x10,0x22,0x22,0x22,0x22,0x22,0x22,0x22};
static const uint8_t ec[] = {0x40,0x03};
static const uint8_t zero[] = {0x00}, one[] = {0x01};
static const uint8_t a5[] = {0,0,0,0,0,0x15,0x2A,0x8A,0x02};
static const uint8_t ba[] = {0x0A,0x5A,0x23,0x10,0x25,0x02,0x00};
static const uint8_t bb[] = {0x00,0x30,0x00,0x2C,0x82,0x87,0x18,0x00};
static const uint8_t bd[] = {0xA1,0xB2,0x2B,0x1A,0x56,0x43,0x34,0x65,0xFF,0xFF,0x0F};
}
static const VideoInitCommand st77903ScanInit[] = {
  {0xF0,st77903_scan_values::key0,1,0},
  {0xF0,st77903_scan_values::key1,1,0},
  {0xF0,st77903_scan_values::key2,1,0},
  {0xE9,st77903_scan_values::e9,1,0},
  {0xE7,st77903_scan_values::e7,4,0},
  {0xC1,st77903_scan_values::c1,4,0},
  {0xC2,st77903_scan_values::c1,4,0},
  {0xC3,st77903_scan_values::c3,4,0},
  {0xC4,st77903_scan_values::c3,4,0},
  {0xC5,st77903_scan_values::c5,1,0},
  {0xE0,st77903_scan_values::gamma,14,0},
  {0xE1,st77903_scan_values::gamma,14,0},
  {0xE5,st77903_scan_values::analog,14,0},
  {0xE6,st77903_scan_values::analog,14,0},
  {0xEC,st77903_scan_values::ec,2,0},
  {0xB2,st77903_scan_values::zero,1,0},
  {0xB3,st77903_scan_values::one,1,0},
  {0xB4,st77903_scan_values::zero,1,0},
  {0xA5,st77903_scan_values::a5,9,0},
  {0xA6,st77903_scan_values::a5,9,0},
  {0xBA,st77903_scan_values::ba,7,0},
  {0xBB,st77903_scan_values::bb,8,0},
  {0xBC,st77903_scan_values::bb,8,0},
  {0xBD,st77903_scan_values::bd,11,0},
  {0x35,st77903_scan_values::zero,1,0},
  {0x21,nullptr,0,0},
  {0x11,nullptr,0,120}
};
} } // namespace nice_gfx::video
