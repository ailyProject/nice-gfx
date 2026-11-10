# Bundled license and notice index

Original NICE_GFX code is licensed under the top-level [Apache-2.0 license](../LICENSE),
with project attribution in [NOTICE](../NOTICE). This directory indexes third-party
licenses and notices; it does not replace those terms. See the detailed
[third-party scope and unresolved example permissions](../THIRD_PARTY_NOTICES.md).
Preserve applicable notices when redistributing code, firmware, fonts or generated assets.

| Material | Existing license or notice |
|---|---|
| Graphics contributor code | [Bodmer notice](../src/Graphics/LICENSE-Bodmer.txt) |
| Supplementary Bodmer / Adafruit origin chain | [TFT_eSPI upstream license](TFT-eSPI-upstream-license.txt) |
| u8g2 display drivers | [Bundled license](../src/Drivers/Display/U8g2/LICENSE) and [upstream manifest](../src/Drivers/Display/U8g2/upstream-manifest.json) |
| ESP-IDF-derived DMA2D adapter | Copyright and modification origin in `src/Platform/ESP32/Rgb565Dma2dCopy.h`; [Apache-2.0](Apache-2.0.txt) |
| Font Awesome graphics | [Font Awesome license](../src/Graphics/LICENSE-Font-Awesome.txt) |
| Generated Font Awesome keyboard icons | [Keyboard icon notice and license](../src/UI/LICENSE-Keyboard-Font-Awesome-6.txt) |
| Bundled bitmap fonts | [Per-font licenses and notices](../src/Fonts/licenses) |
| OV3640 camera example sources | [OV3640 license](../examples/08_Applications/MosaicoCameraDemo/data/ov3640/LICENSE) |
| DVP camera stability example sources | [DVP license](../examples/08_Applications/MosaicoCameraDemo/data/dvp-stability/LICENSE), [BSP license](../examples/08_Applications/MosaicoCameraDemo/data/dvp-stability/bsp/LICENSE) |
| ILI9341_due-derived screenshot helper | Copyright in `examples/08_Applications/ESP8266_uncannyEyes/screenshotToConsole.ino`; [LGPL-2.1-or-later](LGPL-2.1-or-later.txt), [referenced GPLv2](GPL-2.0-only.txt) |

The earlier [audit](AUDIT-2026-11-10.zh-CN.md) and
[conditional-commercial-license discussion draft](NICE-GFX-LICENSE-DRAFT.zh-CN.md)
are historical records. The draft was never activated and is superseded by the
top-level Apache-2.0 license for original project code; it adds no website,
first-recommendation, noncommercial-only or approval requirements.
