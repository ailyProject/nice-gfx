# Arduino examples

These sketches are included in the standard Arduino library `examples/` directory. After installing NICE_GFX, open them from the Arduino IDE examples menu. From the workspace root, compile a sketch with `arduino-cli compile --fqbn <board> --library nice_gfx nice_gfx/examples/<example-directory>`.

Display wiring is supplied through the display object's constructor arguments.
The examples use the same single-object API as applications.
All bundled fonts are declared by `NICE_GFX.h`; select one with
`gfx.setFont(nice_gfx::fonts::Fixed6x13)`. No font macros or extra font headers
are needed. Unreferenced font data is removed during linking; no bundled font
is selected implicitly. See [Monospace_Fonts](Generic/Monospace_Fonts/Monospace_Fonts.ino)
and the [font guide](../src/Fonts/README.md) for MonoFont selection, UTF-8 Chinese
and generated project subsets. The graphics, Sprite and touch diagnostics use MonoFont objects. Select a
font on each display or Sprite before drawing text; use text measurement when
changing fonts or scale.
Public display names include the default resolution at rotation 0, for example
`CO5300_480x480_SPI` and `ILI9341_240x320_SPI`.

Include `NICE_GFX.h` first in an Arduino sketch so the build tools can discover
the installed library. It is also the convenience entry for displays, graphics,
sprites and fonts.
Module headers can also be used directly: `Graphics/Sprite.h` for sprites,
`Graphics/TextDrawing.h` for temporary font selection and drawing helpers,
`UI/View.h` for compiled UI, and `Fonts/Builtin.h` for bundled font declarations.
Generated UI headers include their font declarations explicitly; their hardware
headers include the display presets separately.

The CH13613 sketches use one owning object:

```cpp
#include <NICE_GFX.h>
CH13613_480x480_QSPI gfx(45, 7, 17, 8, 16, 15, 18);
// CS, reset, clock, data 0, data 1, data 2, data 3
```

The portable graphics sketches use the same one-object API with a sample board
preset. Edit the ordinary pin arguments in `Display/Presets/ExampleBoard.h` to match
your wiring:

```cpp
#include <Display/Presets/ExampleBoard.h>
ExampleDisplay gfx;
```

The sample wiring is:

| Target | Panel | Clock / MOSI / MISO / CS / DC / reset |
| --- | --- | --- |
| ESP32-S3 | CH13613 QSPI | clock 17; data 8,16,15,18; CS 45; reset 7 |
| Original ESP32 | ILI9341 SPI | 18 / 23 / 19 / 5 / 2 / 4 |
| Other ESP32 targets | ILI9341 SPI | 4 / 6 / 5 / 7 / 2 / 3 |
| ESP8266 | ILI9341 SPI | 14 / 13 / 12 / 15 / 0 / 2 |
| RP2040 | ILI9341 SPI | 18 / 19 / 16 / 17 / 20 / 21 |
| Other Arduino cores | ILI9341 SPI | variant SPI pins / CS 10 / DC 9 / reset 8 |

These are editable sample connections. The resolution folders describe the
original demo layout; select a matching controller and size for that layout.
`Animated_Eyes_2`, `ESP8266_uncannyEyes` and `ESP32_SDcard_jpeg` retain their
dedicated wiring in their single display object's constructor.

Touch examples call `exampleAttachTouch(gfx)` to attach a separately constructed
XPT2046 controller. Its wiring is in `exampleAttachTouch()` in the same header. The
ESP32-S3 sample uses an **external** XPT2046 on clock 10, MOSI 11, MISO 12, CS 13;
this does not imply that the CH13613 module contains an XPT2046. Update these
ordinary constructor arguments for the touch hardware actually connected.

External image decoders, storage, Wi-Fi and other libraries used by individual
examples are still required by those sketches.

Application interfaces are generated from designer projects. Start with
[HTML_UI/GfxTouchDemo](HTML_UI/GfxTouchDemo/README.md), which includes its editable
project JSON, generated ui.h and application callbacks. Touch calibration remains
available in [Touch_calibrate](Generic/Touch_calibrate/Touch_calibrate.ino).

For a complete on-device rendering test, open
[Graphics_Benchmark](<Test and diagnostics/Graphics_Benchmark/README.md>).
It cycles through fills, clipped lines, antialiased shapes, UTF-8 text,
gradients, Sprite animation and native UI widgets, pointer input and page
transitions. The screen shows completed-frame FPS, latency and memory; the
serial console records per-scene CSV results. It needs a 32-bit board and a
display of at least 240 x 240 pixels. See its README for sample wiring,
Sprite DMA/synchronous comparison and the precise measurement definitions.
