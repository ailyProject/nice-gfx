import {run} from './host-tools.mjs';
for(const script of ['tools/check-library.mjs','tests/metadata.mjs','tests/cmake-modes.mjs',
  'tests/headers.mjs','tests/display-presets.mjs','tests/board-presets.mjs','tests/image.mjs','tests/waveshare4b.mjs','tests/motion-pivot.mjs',
  'tests/frame-lifecycle.mjs','tests/native-render.mjs','tests/frame-runtime.mjs','tests/memory-failures.mjs',
  'tests/render-policies.mjs','tests/benchmark-stats.mjs','tests/arduino-spi.mjs','tests/wio-buzzer.mjs'])run(process.execPath,[script]);
