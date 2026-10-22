# Host regression tests

Run `npm test` from the library root with Node.js 20.11+, CMake and a C++17 compiler. `CXX` and `CMAKE` accept executable paths; otherwise tools are found through PATH. Zig is supported as a compiler executable without adding `c++` to `CXX`.

The suite is self-contained: it needs neither the editor repository nor an
Arduino/ESP-IDF installation. Only `.build/` is ignored; regression sources,
runner scripts and generated resource fixtures are versioned. Native host shims
replace only the clock, heap and FreeRTOS integration points.

The unified entry checks:

- Metadata synchronization, example configuration, documentation links and layer boundaries.
- CMake profiles, feature overrides, source/font selection, early component expansion and invalid settings.
- Standalone public headers, compatibility includes and C++11 resource initialization.
- Shared display initialization, eight scan directions, color order and SPI bridge formats.
- Complete editor board catalog, fixed-board startup/rotation/backlight policy and failure/retry behavior.
- Image copy, stride, byte order, failure preservation and shared ownership.
- Waveshare4B command framing and GT911 sampling.
- Actual View/TileCanvas motion, clipped fills and native/wire pixel conversion.
- Async frame commit/abort, source leases and recovery after transport failures.
- Pixel-for-pixel native framebuffer versus tiled rendering, transitions and fallback/error paths.
- Allocation failure reporting, render policies and software backend fallback.

C++ behavior regressions force assertions on and fail on undefined-behavior
sanitizer diagnostics. The CI workflow
runs the same entry with GCC and Clang. CMake checks exercise component source
registration without requiring a target SDK; they do not compile a board image.
Host tests do not replace target hardware validation.

`arduino-spi.mjs` compiles the actual Arduino bus implementation against a
SAMD/SPI shim and checks atomic pin routing, polling/DMA selection, immutable
sources, RGB565/666/888 fill boundaries and strided images. `benchmark-stats.mjs`
checks 64-bit counters and decimal output supported by newlib-nano.

For physical Wio Terminal GRAM readback and allocation checks, see
[WioTerminal_Validation](../examples/10_Performance/WioTerminal_Validation/README.md).
Run `tools/wio-terminal-test.ps1 -ArduinoCli <path> -Mode Validation` or
`-Mode Benchmark -Rounds 3`. The runner selects a connected Wio, compiles the
current library, uploads it, saves the serial transcript and rejects incomplete
or failed runs. Benchmark defaults to rotation 3 (320×240); pass `-Rotation` to
change it. Its local outputs live in `.build/wio-terminal/`.

To run an individual regression, use for example:

```sh
node tests/frame-lifecycle.mjs
node tests/native-render.mjs
node tests/memory-failures.mjs
```

`host-tools.mjs` shares compiler discovery, IDF shims and the View/Image/Gfx
translation-unit list. New runtime regressions should use
`runCppTest(name, {runtime:true})` so an out-of-line implementation cannot be
accidentally omitted from a private compiler command. Its default source is
`tests/<name>.cpp`; `sources`, `includes`, `defines`, `flags` and `fonts` can be
supplied explicitly. Relative paths resolve against the library root.

Fixed-board constructors and startup code can also be compiled against an
installed target SDK without uploading: run
`node tests/board-targets.mjs --commands=/path/to/compile_commands.json` using
an existing Arduino or ESP-IDF build's structured compilation database. This
also verifies that an unsupported board produces the intended diagnostic.

The [native rendering fixture](fixtures/native-render/README.md) is regenerated
only when its source or resource ABI changes. Normal tests consume its checked-in
`ui.h` and require no editor compiler.
