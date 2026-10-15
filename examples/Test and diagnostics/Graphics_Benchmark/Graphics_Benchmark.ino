// Comprehensive display diagnostic. See README.md for wiring and measurement rules.
#include <NICE_GFX.h>
#include <Display/Presets/ExampleBoard.h>
#include "BenchmarkPlatformArduino.h"

#if defined(__AVR__)
#error "Graphics_Benchmark needs a 32-bit board with native UI support."
#endif

// Replace with your panel's public preset and constructor pins.
ExampleDisplay gfx;
#include "BenchmarkApp.h"

void setup() { graphics_benchmark::setup(); }
void loop() { graphics_benchmark::loop(); }
