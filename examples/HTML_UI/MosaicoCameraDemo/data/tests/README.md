# Portable image-processing tests

Run from the sketch directory containing `FrameProcessing.h`. These tests exercise the actual portable header; Arduino and camera hardware are not required.

```sh
g++ -std=c++17 -O1 -Wall -Wextra -Werror -pedantic -I . data/tests/frame_processing_tests.cpp -o frame_processing_tests
./frame_processing_tests
```

`clang++` can replace `g++`. On Windows, run `frame_processing_tests.exe`.

Coverage: all seven source formats, RGB565 byte order, padded rows, truncated buffers, overflow and unknown-format rejection, contain scaling and black letterboxing, output guards, color rectangles and sample-count filtering. Includes 1,200 deterministic random conversion cases and 800 deterministic detection-bound cases.

The suite validates pixel processing and detection geometry. It does not validate ESP32-S31 camera, GPIO, display, FreeRTOS scheduling, firmware linking or measured hardware frame rate.
