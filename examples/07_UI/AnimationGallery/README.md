# Component animation gallery

This runnable Arduino example uses the current NICE_GFX 1.0.0 / UI ABI 15 compiler and runtime. `project.gfx.json` is its editable source; the generated `ui.h`, `Hardware.h` and `TouchAdapter.h` use the installed NICE_GFX library's public headers.

The source project targets an ESP32-S3 with an ILI9341 240×320 SPI screen. Review the ordinary constructor pins in `Hardware.h` and the project's touch configuration before connecting your board. This repository check compiles the sketch locally; it does not flash or validate that particular physical wiring.

Run `node tools/reexport_bundled_ui.mjs` from the workspace root to refresh generated resources. Existing application `.ino` files are retained byte for byte. `--check` verifies that resources still match the editable projects. The animation runs through the normal `pollUi` loop, allowing input and tiled display transfers to keep progressing.
