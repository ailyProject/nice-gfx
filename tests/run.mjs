import {run} from './host-tools.mjs';
for(const script of ['tools/check-library.mjs','tests/cmake-modes.mjs','tests/headers.mjs','tests/image.mjs','tests/waveshare4b.mjs','tests/motion-pivot.mjs'])run(process.execPath,[script]);
