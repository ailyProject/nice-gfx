# Third-party code and resource notices

The top-level [LICENSE](LICENSE) applies to original NICE_GFX code and contributions
that their copyright holders make available under Apache-2.0. It does not replace
licenses on upstream code, fonts, icons, examples or assets, and does not establish
commercial rights to material whose source or permission has not been confirmed.
Preserve each applicable copyright, license, attribution and modification notice
when redistributing source code, firmware or generated assets.

| Material | License / retained evidence |
| --- | --- |
| Bodmer graphics and Sprite code | [Original bundled Bodmer notice](src/Graphics/LICENSE-Bodmer.txt); [supplementary TFT_eSPI upstream notice](LICENSES/TFT-eSPI-upstream-license.txt) |
| Adafruit-derived graphics algorithms | Adafruit MIT/BSD attributions retained in the [TFT_eSPI upstream notice](LICENSES/TFT-eSPI-upstream-license.txt); see the Adafruit origin comment in `src/Graphics/Shapes.inc` |
| u8g2 display drivers | [Bundled license](src/Drivers/Display/U8g2/LICENSE); BSD-2-Clause driver code; revision and transformations in [upstream manifest](src/Drivers/Display/U8g2/upstream-manifest.json) |
| ESP32 DMA2D adapter | Apache-2.0; `src/Platform/ESP32/Rgb565Dma2dCopy.h` retains Espressif's 2023–2024 copyright and identifies the source as IDF v5.5.5 `esp_async_fbcpy.c`; [license text](LICENSES/Apache-2.0.txt) |
| Bitmap fonts, including converted data and subsets | [Per-font licenses and notices](src/Fonts/licenses) and [font provenance](src/Fonts/README.md); OFL, MIT, BSD, custom permissive and public-domain sources as individually documented |
| Font Awesome graphics | [Bundled Font Awesome license](src/Graphics/LICENSE-Font-Awesome.txt); icon, font and code licenses differ |
| Font Awesome 6.7.2 raster keyboard icons | CC BY 4.0; Fonticons, Inc.; raster conversion and combined hide icon recorded in [keyboard notice](src/UI/LICENSE-Keyboard-Font-Awesome-6.txt) and `src/UI/detail/KeyboardIcons.h` |
| OV3640 / DVP camera example patches | Apache-2.0; [OV3640](examples/08_Applications/MosaicoCameraDemo/data/ov3640/LICENSE), [DVP](examples/08_Applications/MosaicoCameraDemo/data/dvp-stability/LICENSE), [BSP](examples/08_Applications/MosaicoCameraDemo/data/dvp-stability/bsp/LICENSE), plus their provenance and modification records |
| ILI9341_due-derived screenshot helper | `examples/08_Applications/ESP8266_uncannyEyes/screenshotToConsole.ino`: Copyright 2014 Marek Buriak, LGPL-2.1-or-later; [LGPL text](LICENSES/LGPL-2.1-or-later.txt) and [referenced GPLv2 text](LICENSES/GPL-2.0-only.txt) |
| Independent example contributions | MIT and other notices retained in their source files, including `Orrery/astronomy.h`, `SpriteRotatingCube`, `Arduino_Life` and `Cellular_Automata` |

## Scope of the graphics attribution record

The supplemental TFT_eSPI text was retrieved from the official
[Bodmer/TFT_eSPI license file](https://github.com/Bodmer/TFT_eSPI/blob/master/license.txt)
on 2026-11-10. It retains the Adafruit_ILI9341, Adafruit_GFX and Bodmer attribution
chain. It supplements, and does not overwrite, the pre-existing Bodmer notice.
It is not a claim that every file in this checkout was imported from that exact
upstream revision; the exact historic import mapping remains to be completed.

## Fonts and icons in closed-source products

The OFL fonts can be embedded in commercial software, but their font data and
derivatives remain under the OFL, with the relevant reserved-name requirements.
This project selects the OFL option for its GNU Unifont glyph data; the presence
of GPL text in Unifont's upstream COPYING file does not make that selected font
data or the entire NICE_GFX runtime GPL-only.

Font Awesome's CC BY icon attribution, license link and modification information
must remain available in an appropriate location. Compiling away source comments
does not replace the need to supply applicable binary-distribution notices.

## Example assets and sources requiring separate verification

The new Apache-2.0 grant does not grant additional rights to the following
materials whose complete commercial permission record has not been confirmed:

- The `GFX_Pong` / `Pong_v3` examples referring to `rparrett/pongclock`, and the
  `UGFX_demo`, `UGFX_demo_fast` and `UGFX_Demo_Wide` examples referring to Henning Karlsen.
- Example images including `Flash_transparent_PNG/SpongeBob.h`,
  `ESP32_SDcard_jpeg/Data/lena20k.jpg`, EagleEye, Panda and `Animated_dial/dial.h`.
- Eye bitmaps and logos where the exact resource-to-upstream license mapping is
  not recorded, even if adjacent animation code has a MIT notice.

Check, replace or obtain permission for such material before including it in a
commercial product. Its presence does not relicense the library's original code.

## External dependencies

Arduino cores, ESP-IDF components, PNGdec, JPEGDecoder, TJpg_Decoder, board SDKs and
other optional example dependencies retain their own licenses. Their version,
linking and distribution requirements must be checked for the actual firmware.
In particular, an Apache-2.0 grant from NICE_GFX does not waive obligations on
LGPL/GPL components used by an application.
