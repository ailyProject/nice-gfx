#include <NICE_GFX.h>
#include <esp_heap_caps.h>

CH13613_480x480_QSPI gfx(45, 7, 17, 8, 16, 15, 18);
constexpr size_t width = 480, rows = 48, frames = 30;
uint16_t *buffers[2] = {nullptr,nullptr};

static void render(uint16_t *pixels, unsigned frame, unsigned top) {
  for (unsigned y=0; y<rows; ++y)
    for (unsigned x=0; x<width; ++x)
      pixels[y*width+x] = uint16_t(((x+frame)&31)<<11 | ((y+top+frame)&63)<<5 | (frame&31));
}
static void measure(bool overlap) {
  gfx.wait();
  if (overlap) gfx.initDMA(); else gfx.deInitDMA();
  gfx.setSwapBytes(true); // The renderer produces native RGB565 words.
  const size_t heapBefore = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  const uint32_t begin = micros();
  unsigned next = 0;
  for (unsigned frame=0; frame<frames; ++frame) {
    for (unsigned top=0; top<480; top+=rows) {
      // Only the other buffer may still be owned by DMA here.
      render(buffers[next], frame, top);
      gfx.pushImageDMA(0, top, width, rows, buffers[next]);
      if (gfx.lastError() != DisplayStatus::Ok) {
        Serial.printf("Transfer error %u\n", unsigned(gfx.lastError())); return;
      }
      next ^= 1;
    }
  }
  if (gfx.wait() != DisplayStatus::Ok) { Serial.println("DMA completion error"); return; }
  const uint32_t elapsed = micros()-begin;
  const double bytes = double(width)*480*2*frames;
  Serial.printf("%s: %.2f fps, %.2f MB/s, %lu us, heap before/after %u/%u\n",
    overlap ? "double buffer" : "synchronous", frames*1e6/elapsed, bytes/elapsed,
    (unsigned long)elapsed,unsigned(heapBefore),unsigned(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)));
}
void setup() {
  Serial.begin(115200);
  if (!gfx.begin()) { Serial.println("Display init failed"); return; }
  for (auto &buffer : buffers) {
    buffer = static_cast<uint16_t *>(heap_caps_malloc(width*rows*2, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
    if (!buffer) { Serial.println("Buffer allocation failed"); return; }
  }
  Serial.println("480x480 RGB565, QSPI 40 MHz; includes rendering and final DMA drain");
  measure(false);
  measure(true);
}
void loop() { delay(1000); }
