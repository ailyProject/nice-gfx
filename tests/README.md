# Host regression tests

Run `npm test` from the library root with Node.js 20+, CMake and a C++17 compiler. `CXX` and `CMAKE` accept executable paths; otherwise tools are found through PATH. Zig is supported as a compiler executable without adding `c++` to `CXX`.

Only `.build/` is ignored. Regression sources, runner scripts and fixtures are versioned. The suite checks CMake registration, standalone headers and compatibility includes, standalone Image ownership, Waveshare4B command framing and GT911 sampling, and the real UI motion renderer. Host tests do not replace target hardware validation.
