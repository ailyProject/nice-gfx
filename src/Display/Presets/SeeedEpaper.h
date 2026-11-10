#pragma once
#include "../NativeEpaperDisplay.h"
#include "../../Drivers/Display/Epaper/NativeMono.h"
#include "NativeEpaperSsd.h"
#include "NativeEpaperUc.h"
// Original NICE_GFX panel descriptions. See docs/epaper-product-sources.md.
// Undocumented revisions remain selectable for discovery, but begin() returns
// Unsupported before allocation or GPIO access. No guessed fallback protocol.
namespace nice_gfx { namespace seeed_profiles {
struct P_154_BW { static const NativeEpaperProfile &profile() {
  return epaper_profiles::GDEY0154D67::profile();
}};
struct P_213_BW { static const NativeEpaperProfile &profile() {
  return epaper_profiles::GDEY0213B74::profile();
}};
struct P_290_BW { static const NativeEpaperProfile &profile() {
  return epaper_profiles::GDEY029T94::profile();
}};
struct P_290_FLEX_BW { static const NativeEpaperProfile &profile() {
  static const NativeEpaperProfile p={"NGFX_SEEED_290_FLEX_BW",NativeEpaperController::UC8151D,128,296,128,
    NativeEpaperEncoding::Mono,5000,120000,false,1,true,
    "https://files.seeedstudio.com/wiki/Other_Display/29-flexible/IC%20Driver%20UC8151D.pdf",&native_epaper::ucProtocol()};return p;
}};
struct P_213_FLEX_BW { static const NativeEpaperProfile &profile() {
  static const NativeEpaperProfile p={"NGFX_SEEED_213_FLEX_BW",NativeEpaperController::UC8151D,104,212,104,
    NativeEpaperEncoding::Mono,5000,120000,false,1,true,
    "https://files.seeedstudio.com/wiki/Other_Display/29-flexible/IC%20Driver%20UC8151D.pdf",&native_epaper::ucProtocol()};return p;
}};
struct P_420_BW { static const NativeEpaperProfile &profile() {
  return epaper_profiles::GDEY042T81::profile();
}};
struct P_426_BW { static const NativeEpaperProfile &profile() {
  return epaper_profiles::GDEQ0426T82::profile();
}};
struct P_583_BW { static const NativeEpaperProfile &profile() {
  return epaper_profiles::GDEY0583T81::profile();
}};
struct P_750_BW { static const NativeEpaperProfile &profile() {
  return epaper_profiles::GDEY075T7::profile();
}};
struct P_750_BW_JD79686B { static const NativeEpaperProfile &profile() {
  static const NativeEpaperProfile p={"NGFX_SEEED_750_BW_JD79686B",NativeEpaperController::JD79686B,800,480,800,
    NativeEpaperEncoding::Mono,5000,120000,false,1,false,
    "https://www.good-display.com/companyfile/5/",nullptr};return p;
}};
struct P_1030_GRAY16 { static const NativeEpaperProfile &profile() {
  static const NativeEpaperProfile p={"NGFX_SEEED_1030_GRAY16",NativeEpaperController::IT8951,1872,1404,1872,
    NativeEpaperEncoding::Gray16,1500,120000,false,1,true,
    "https://files.waveshare.com/upload/b/b0/IT8951_I80%2BProgrammingGuide_16bits_20170904_v2.7_common_CXDX.zip",nullptr};return p;
}};
struct P_400_SPECTRA6 { static const NativeEpaperProfile &profile() {
  static const NativeEpaperProfile p={"NGFX_SEEED_400_SPECTRA6",NativeEpaperController::ED2208,400,600,400,
    NativeEpaperEncoding::SixColor,60000,120000,false,1,false,
    "https://files.seeedstudio.com/wiki/Other_Display/7.3-E6-color/7.3_E6_eInk_%20Display_%20module_%20Datasheet.pdf",nullptr};return p;
}};
struct P_730_SPECTRA6 { static const NativeEpaperProfile &profile() {
  static const NativeEpaperProfile p={"NGFX_SEEED_730_SPECTRA6",NativeEpaperController::ED2208,800,480,800,
    NativeEpaperEncoding::SixColor,60000,120000,false,1,false,
    "https://files.seeedstudio.com/wiki/Other_Display/7.3-E6-color/7.3_E6_eInk_%20Display_%20module_%20Datasheet.pdf",nullptr};return p;
}};
struct P_1330_SPECTRA6 { static const NativeEpaperProfile &profile() {
  static const NativeEpaperProfile p={"NGFX_SEEED_1330_SPECTRA6",NativeEpaperController::T133A01,1200,1600,1200,
    NativeEpaperEncoding::SixColor,60000,120000,false,2,false,
    "https://files.seeedstudio.com/wiki/Other_Display/1330-E6-epaper/13.3_E6_eInk_Display_module_Datasheet.pdf",nullptr};return p;
}};
struct P_709_SPECTRA6 { static const NativeEpaperProfile &profile() {
  static const NativeEpaperProfile p={"NGFX_SEEED_709_SPECTRA6",NativeEpaperController::GDEB0709E01,1200,1600,1200,
    NativeEpaperEncoding::SixColor,60000,120000,false,2,false,
    "https://www.good-display.com/companyfile/2049.html",nullptr};return p;
}};
struct P_154_BWRY { static const NativeEpaperProfile &profile() {
  static const NativeEpaperProfile p={"NGFX_SEEED_154_BWRY",NativeEpaperController::JD79660,200,200,200,
    NativeEpaperEncoding::FourColor,30000,120000,false,1,false,
    "https://www.good-display.com/companyfile/5/",nullptr};return p;
}};
struct P_213_BWRY { static const NativeEpaperProfile &profile() {
  static const NativeEpaperProfile p={"NGFX_SEEED_213_BWRY",NativeEpaperController::JD79676,122,250,128,
    NativeEpaperEncoding::FourColor,30000,120000,false,1,false,
    "https://files.seeedstudio.com/wiki/Other_Display/213-quadruple/GDEY0213F51.pdf",nullptr};return p;
}};
struct P_290_BWRY { static const NativeEpaperProfile &profile() {
  static const NativeEpaperProfile p={"NGFX_SEEED_290_BWRY",NativeEpaperController::JD79667,128,296,128,
    NativeEpaperEncoding::FourColor,30000,120000,false,1,false,
    "https://files.seeedstudio.com/wiki/Other_Display/29-quadruple/GDEY029F51.pdf",nullptr};return p;
}};
struct P_RETERMINAL_E1001 { static const NativeEpaperProfile &profile() {
  static const NativeEpaperProfile p={"NGFX_SEEED_RETERMINAL_E1001",NativeEpaperController::UC8179,800,480,800,
    NativeEpaperEncoding::Mono,5000,120000,false,1,true,
    "https://files.seeedstudio.com/wiki/Other_Display/750-epaper/IC%20Driver%20UC8179.pdf",&native_epaper::ucProtocol()};return p;
}};
struct P_RETERMINAL_E1002 { static const NativeEpaperProfile &profile() {
  static const NativeEpaperProfile p={"NGFX_SEEED_RETERMINAL_E1002",NativeEpaperController::ED2208,800,480,800,
    NativeEpaperEncoding::SixColor,60000,120000,false,1,false,
    "https://files.seeedstudio.com/wiki/Other_Display/7.3-E6-color/7.3_E6_eInk_%20Display_%20module_%20Datasheet.pdf",nullptr};return p;
}};
struct P_RETERMINAL_E1003 { static const NativeEpaperProfile &profile() {
  static const NativeEpaperProfile p={"NGFX_SEEED_RETERMINAL_E1003",NativeEpaperController::IT8951,1872,1404,1872,
    NativeEpaperEncoding::Gray16,1500,120000,false,1,true,
    "https://files.waveshare.com/upload/b/b0/IT8951_I80%2BProgrammingGuide_16bits_20170904_v2.7_common_CXDX.zip",nullptr};return p;
}};
struct P_RETERMINAL_E1004 { static const NativeEpaperProfile &profile() {
  static const NativeEpaperProfile p={"NGFX_SEEED_RETERMINAL_E1004",NativeEpaperController::T133A01,1200,1600,1200,
    NativeEpaperEncoding::SixColor,60000,120000,false,2,false,
    "https://files.seeedstudio.com/wiki/Other_Display/1330-E6-epaper/13.3_E6_eInk_Display_module_Datasheet.pdf",nullptr};return p;
}};
struct P_RETERMINAL_STICKY { static const NativeEpaperProfile &profile() {
  static const NativeEpaperProfile p={"NGFX_SEEED_RETERMINAL_STICKY",NativeEpaperController::StickyAuto,800,480,800,
    NativeEpaperEncoding::Mono,5000,120000,false,1,false,
    "https://wiki.seeedstudio.com/reterminal_sticky/",nullptr};return p;
}};
struct P_154_FLEX_BW { static const NativeEpaperProfile &profile() {
  static const NativeEpaperProfile p={"GDEW0154I9FC",NativeEpaperController::UC8151D,152,152,152,
    NativeEpaperEncoding::Mono,5000,120000,false,1,true,
    "https://files.seeedstudio.com/wiki/Other_Display/154-flexible-epaper/GDEW0154I9FC%20Datasheet.pdf",&native_epaper::ucProtocol()};return p;
}};
} } // namespace nice_gfx::seeed_profiles
using NGFX_SEEED_154_BW=nice_gfx::NativeEpaperDisplay<nice_gfx::seeed_profiles::P_154_BW>;
using NGFX_SEEED_213_BW=nice_gfx::NativeEpaperDisplay<nice_gfx::seeed_profiles::P_213_BW>;
using NGFX_SEEED_290_BW=nice_gfx::NativeEpaperDisplay<nice_gfx::seeed_profiles::P_290_BW>;
using NGFX_SEEED_290_FLEX_BW=nice_gfx::NativeEpaperDisplay<nice_gfx::seeed_profiles::P_290_FLEX_BW>;
using NGFX_SEEED_213_FLEX_BW=nice_gfx::NativeEpaperDisplay<nice_gfx::seeed_profiles::P_213_FLEX_BW>;
using NGFX_SEEED_420_BW=nice_gfx::NativeEpaperDisplay<nice_gfx::seeed_profiles::P_420_BW>;
using NGFX_SEEED_426_BW=nice_gfx::NativeEpaperDisplay<nice_gfx::seeed_profiles::P_426_BW>;
using NGFX_SEEED_583_BW=nice_gfx::NativeEpaperDisplay<nice_gfx::seeed_profiles::P_583_BW>;
using NGFX_SEEED_750_BW=nice_gfx::NativeEpaperDisplay<nice_gfx::seeed_profiles::P_750_BW>;
using NGFX_SEEED_750_BW_JD79686B=nice_gfx::NativeEpaperDisplay<nice_gfx::seeed_profiles::P_750_BW_JD79686B>;
using NGFX_SEEED_1030_GRAY16=nice_gfx::NativeEpaperDisplay<nice_gfx::seeed_profiles::P_1030_GRAY16>;
using NGFX_SEEED_400_SPECTRA6=nice_gfx::NativeEpaperDisplay<nice_gfx::seeed_profiles::P_400_SPECTRA6>;
using NGFX_SEEED_730_SPECTRA6=nice_gfx::NativeEpaperDisplay<nice_gfx::seeed_profiles::P_730_SPECTRA6>;
using NGFX_SEEED_1330_SPECTRA6=nice_gfx::NativeEpaperDisplay<nice_gfx::seeed_profiles::P_1330_SPECTRA6>;
using NGFX_SEEED_709_SPECTRA6=nice_gfx::NativeEpaperDisplay<nice_gfx::seeed_profiles::P_709_SPECTRA6>;
using NGFX_SEEED_154_BWRY=nice_gfx::NativeEpaperDisplay<nice_gfx::seeed_profiles::P_154_BWRY>;
using NGFX_SEEED_213_BWRY=nice_gfx::NativeEpaperDisplay<nice_gfx::seeed_profiles::P_213_BWRY>;
using NGFX_SEEED_290_BWRY=nice_gfx::NativeEpaperDisplay<nice_gfx::seeed_profiles::P_290_BWRY>;
using NGFX_SEEED_RETERMINAL_E1001=nice_gfx::NativeEpaperDisplay<nice_gfx::seeed_profiles::P_RETERMINAL_E1001>;
using NGFX_SEEED_RETERMINAL_E1002=nice_gfx::NativeEpaperDisplay<nice_gfx::seeed_profiles::P_RETERMINAL_E1002>;
using NGFX_SEEED_RETERMINAL_E1003=nice_gfx::NativeEpaperDisplay<nice_gfx::seeed_profiles::P_RETERMINAL_E1003>;
using NGFX_SEEED_RETERMINAL_E1004=nice_gfx::NativeEpaperDisplay<nice_gfx::seeed_profiles::P_RETERMINAL_E1004>;
using NGFX_SEEED_RETERMINAL_STICKY=nice_gfx::NativeEpaperDisplay<nice_gfx::seeed_profiles::P_RETERMINAL_STICKY>;
using NGFX_SEEED_154_FLEX_BW=nice_gfx::NativeEpaperDisplay<nice_gfx::seeed_profiles::P_154_FLEX_BW>;
