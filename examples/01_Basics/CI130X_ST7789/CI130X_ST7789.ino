// easyVoice CI1306 + ST7789 240x240, RGB565, write-only four-wire SPI.
#include "Hardware.h"
#include <stdio.h>
#include <string.h>

ExampleDisplay gfx(CI130X_TFT_CS, CI130X_TFT_DC, CI130X_TFT_RST,
                   CI130X_TFT_SCK, CI130X_TFT_MOSI, -1, CI130X_TFT_SPI_HZ);

constexpr uint32_t kAnimationIntervalMs = 50;
constexpr int kBoxSize = 24, kBoxY = 132, kBoxLeft = 14, kBoxRight = 202;
constexpr int kBoxStep = 4;
bool displayReady = false, running = true;
bool stripMode = true;
int boxX = kBoxLeft, boxDirection = 1;
uint32_t lastAnimationMs = 0, lastReportMs = 0;
uint32_t updates = 0, totalUpdates = 0, animationMicros = 0;
uint32_t maxUpdateMicros = 0, lastUpdateMicros = 0;
uint32_t animationPixels = 0;
char serialLine[40];
size_t serialLength = 0;
char previousStatus[2][37] = {};

void drawChangedStatus(const char *text, unsigned row, int y) {
  // Fixed6x13 advances by six pixels. Redraw only the changed character span,
  // padding with spaces so that shrinking numbers never leave old digits.
  char padded[37]; memset(padded, ' ', 36); padded[36] = 0;
  const size_t length = strlen(text);
  memcpy(padded, text, length < 36 ? length : 36);
  size_t first = 0, end = 36;
  if (previousStatus[row][0]) {
    while (first < 36 && padded[first] == previousStatus[row][first]) ++first;
    while (end > first && padded[end - 1] == previousStatus[row][end - 1]) --end;
  }
  memcpy(previousStatus[row], padded, sizeof(padded));
  if (first == end) return;
  padded[end] = 0;
  gfx.drawString(padded + first, 12 + int(first) * 6, y);
}

const char *animationModeName() { return stripMode ? "STRIP" : "FULL"; }

uint32_t pixelsPerStep() {
  return 2U * uint32_t(kBoxSize) * uint32_t(stripMode ? kBoxStep : kBoxSize);
}

void resetAnimationMetrics() {
  updates = animationMicros = maxUpdateMicros = animationPixels = 0;
  lastUpdateMicros = 0;
  lastAnimationMs = lastReportMs = millis();
}

void reportInfo() {
  const DisplayInfo info = gfx.displayInfo();
  Serial.print("INFO CI130X_ST7789 size=");
  Serial.print(gfx.width()); Serial.print('x'); Serial.print(gfx.height());
  Serial.print(" rotation="); Serial.print(info.rotation);
  Serial.print(" requestedHz="); Serial.print(CI130X_TFT_SPI_HZ);
  Serial.print(" configuredHz="); Serial.print(info.bus.frequency);
  Serial.print(" sck="); Serial.print(CI130X_TFT_SCK);
  Serial.print(" mosi="); Serial.print(CI130X_TFT_MOSI);
  Serial.print(" cs="); Serial.print(CI130X_TFT_CS);
  Serial.print(" dc="); Serial.print(CI130X_TFT_DC);
  Serial.print(" rst="); Serial.print(CI130X_TFT_RST);
  Serial.print(" bl="); Serial.print(CI130X_TFT_BL);
  Serial.print(" mode="); Serial.print(animationModeName());
  Serial.print(" stepPixels="); Serial.print(pixelsPerStep());
  Serial.print(" status="); Serial.println(unsigned(gfx.lastError()));
}

void drawStatus() {
  char line[40];
  gfx.startWrite();
  gfx.setTextColor(GFX_WHITE, GFX_BLACK);
  snprintf(line, sizeof(line), "UP %lu s / %s %s", (unsigned long)(millis() / 1000),
           running ? "RUN" : "PAUSE", animationModeName());
  drawChangedStatus(line, 0, 182);
  snprintf(line, sizeof(line), "STEP %lu / %lu us", (unsigned long)totalUpdates,
           (unsigned long)lastUpdateMicros);
  drawChangedStatus(line, 1, 199);
  gfx.endWrite();
}

void drawStaticScreen() {
  previousStatus[0][0] = previousStatus[1][0] = 0;
  static const uint16_t colors[] = {
    GFX_RED, GFX_GREEN, GFX_BLUE, GFX_CYAN, GFX_MAGENTA, GFX_YELLOW, GFX_WHITE
  };
  static const char *const labels[] = {"R", "G", "B", "C", "M", "Y", "W"};
  gfx.startWrite();
  gfx.fillScreen(GFX_BLACK);
  gfx.drawRect(0, 0, 240, 240, GFX_WHITE);
  gfx.fillRect(2, 2, 8, 8, GFX_RED);
  gfx.fillRect(230, 2, 8, 8, GFX_GREEN);
  gfx.fillRect(2, 230, 8, 8, GFX_BLUE);
  gfx.fillRect(230, 230, 8, 8, GFX_YELLOW);
  gfx.setTextColor(GFX_WHITE, GFX_BLACK);
  gfx.drawString("TL", 12, 2); gfx.drawString("TR", 216, 2);
  gfx.drawString("BL", 12, 224); gfx.drawString("BR", 216, 224);
  gfx.drawString("CI1306 + NICE_GFX", 12, 22);
  gfx.drawString("ST7789 240x240 / RGB565", 12, 39);
  for (unsigned i = 0; i < 7; ++i) {
    gfx.fillRect(14 + int(i) * 30, 58, 30, 26, colors[i]);
    gfx.drawString(labels[i], 26 + int(i) * 30, 87);
  }
  gfx.drawRect(10, 108, 220, 68, GFX_DARKGREY);
  gfx.drawString("LOCAL UPDATE / 20 Hz TARGET", 14, 113);
  boxX = kBoxLeft; boxDirection = 1;
  gfx.fillRect(boxX, kBoxY, kBoxSize, kBoxSize, GFX_CYAN);
  gfx.drawString("FULL STRIP PAUSE INFO", 56, 222);
  gfx.endWrite();
  drawStatus();
}

void animateOneStep() {
  int nextX = boxX + boxDirection * kBoxStep;
  if (nextX >= kBoxRight) { nextX = kBoxRight; boxDirection = -1; }
  if (nextX <= kBoxLeft) { nextX = kBoxLeft; boxDirection = 1; }
  const int distance = nextX > boxX ? nextX - boxX : boxX - nextX;
  uint32_t submittedPixels;
  const uint32_t started = micros();
  gfx.startWrite();
  if (stripMode && distance > 0 && distance < kBoxSize) {
    // The overlap stays cyan. Only clear the departing strip and paint the
    // entering strip: 2 * 4 * 24 = 192 submitted pixels for a normal step.
    const int clearX = nextX > boxX ? boxX : nextX + kBoxSize;
    const int paintX = nextX > boxX ? boxX + kBoxSize : nextX;
    gfx.fillRect(clearX, kBoxY, distance, kBoxSize, GFX_BLACK);
    gfx.fillRect(paintX, kBoxY, distance, kBoxSize, GFX_CYAN);
    submittedPixels = 2U * uint32_t(distance) * uint32_t(kBoxSize);
  } else {
    // Preserve the original FULL workload for a matched transport benchmark.
    gfx.fillRect(boxX, kBoxY, kBoxSize, kBoxSize, GFX_BLACK);
    gfx.fillRect(nextX, kBoxY, kBoxSize, kBoxSize, GFX_CYAN);
    submittedPixels = 2U * uint32_t(kBoxSize) * uint32_t(kBoxSize);
  }
  gfx.endWrite();
  lastUpdateMicros = micros() - started;
  animationMicros += lastUpdateMicros;
  if (lastUpdateMicros > maxUpdateMicros) maxUpdateMicros = lastUpdateMicros;
  animationPixels += submittedPixels;
  boxX = nextX; ++updates; ++totalUpdates;
}

void setAnimationMode(bool useStrips) {
  stripMode = useStrips;
  // Reconstruct the current track once so that either mode starts from an
  // identical solid box. This repair and status drawing are outside the next
  // animation measurement window; position, direction and pause state persist.
  gfx.startWrite();
  gfx.fillRect(kBoxLeft, kBoxY, kBoxRight - kBoxLeft + kBoxSize, kBoxSize, GFX_BLACK);
  gfx.fillRect(boxX, kBoxY, kBoxSize, kBoxSize, GFX_CYAN);
  gfx.endWrite();
  drawStatus();
  resetAnimationMetrics();
}

void processCommand(char *line) {
  if (!strcmp(line, "INFO")) { reportInfo(); return; }
  if (!displayReady) { Serial.println("FAIL not ready"); return; }
  if (!strcmp(line, "RUN")) { running = true; lastAnimationMs = millis(); }
  else if (!strcmp(line, "PAUSE")) running = false;
  else if (!strcmp(line, "FULL")) setAnimationMode(false);
  else if (!strcmp(line, "STRIP")) setAnimationMode(true);
  else if (!strcmp(line, "REDRAW")) drawStaticScreen();
  else if (line[0] == 'R' && line[1] >= '0' && line[1] <= '3' && !line[2]) {
    gfx.setRotation(uint8_t(line[1] - '0')); drawStaticScreen();
  } else if (!strcmp(line, "FILL")) {
    const uint32_t started = micros();
    gfx.startWrite(); gfx.fillScreen(GFX_BLUE); gfx.endWrite();
    const uint32_t elapsed = micros() - started;
    Serial.print("BENCH fillPixels=57600 pixelBytes=115200 us=");
    Serial.print(elapsed); Serial.print(" status="); Serial.println(unsigned(gfx.lastError()));
    drawStaticScreen();
  } else { Serial.println("FAIL command"); return; }
  Serial.print("OK "); Serial.print(line);
  Serial.print(" status="); Serial.println(unsigned(gfx.lastError()));
}

void setup() {
  Serial.begin(115200);
  Serial.println("BOOT CI1306 ST7789");
  pinMode(CI130X_TFT_BL, OUTPUT); digitalWrite(CI130X_TFT_BL, LOW);
  if (!gfx.begin()) {
    digitalWrite(CI130X_TFT_BL, HIGH);
    Serial.print("FAIL begin status="); Serial.println(unsigned(gfx.lastError()));
    return;
  }
  gfx.setRotation(0);
  gfx.setFont(nice_gfx::fonts::Fixed6x13);
  gfx.setTextDatum(TL_DATUM);
  gfx.setTextColor(GFX_WHITE, GFX_BLACK);
  gfx.setAntialiasing(false);
  drawStaticScreen();
  displayReady = gfx.lastError() == nice_gfx::Status::Ok;
  digitalWrite(CI130X_TFT_BL, HIGH);
  lastAnimationMs = lastReportMs = millis();
  Serial.println(displayReady ? "READY CI130X_ST7789" : "FAIL draw");
  reportInfo();
}

void loop() {
  while (Serial.available()) {
    const char c = char(Serial.read());
    if (c == '\n') { serialLine[serialLength] = 0; processCommand(serialLine); serialLength = 0; }
    else if (c != '\r' && serialLength < sizeof(serialLine) - 1) serialLine[serialLength++] = c;
  }
  const uint32_t now = millis();
  if (displayReady && running && now - lastAnimationMs >= kAnimationIntervalMs) {
    lastAnimationMs += kAnimationIntervalMs;
    // Drop a long pause instead of replaying a burst of stale animation steps.
    if (now - lastAnimationMs >= kAnimationIntervalMs) lastAnimationMs = now;
    animateOneStep();
  }
  const uint32_t reportNow = millis();
  const uint32_t elapsed = reportNow - lastReportMs;
  if (elapsed >= 1000) {
    // Use a wall-clock window that includes the previous report/status overhead.
    // animUs measures only the two fills; it excludes status text and serial I/O.
    lastReportMs = reportNow;
    Serial.print("TFT ms="); Serial.print(elapsed);
    Serial.print(" mode="); Serial.print(animationModeName());
    Serial.print(" stepPixels="); Serial.print(pixelsPerStep());
    Serial.print(" updates="); Serial.print(updates);
    Serial.print(" pixels="); Serial.print(animationPixels);
    Serial.print(" animUs="); Serial.print(animationMicros);
    Serial.print(" maxUpdateUs="); Serial.print(maxUpdateMicros);
    Serial.print(" status="); Serial.print(unsigned(gfx.lastError()));
    Serial.print(" running="); Serial.println(unsigned(running && displayReady));
    updates = animationMicros = maxUpdateMicros = animationPixels = 0;
    if (displayReady) drawStatus();
  }
  // The core's FreeRTOS and audio tasks retain scheduling opportunities.
  delay(1);
}

