// ESP32-S31-Korvo-1 + ESP32-S3-LCD-EV-Board-SUB3 (GT1151).
// Official BSP wiring: SDA=0, SCL=1, address=0x14, no GPIO RESET/IRQ.
// Serial uses the UART connector (115200 baud, USB CDC on boot disabled).
#include <NICE_GFX.h>
#include <Platform/Arduino/TouchWire.h>

nice_gfx::TouchWire touchBus(Wire);
nice_gfx::Gt1151 touch(touchBus, 800, 480);
bool ready = false, pressed = false;
uint32_t polls = 0, samples = 0, errors = 0, presses = 0, releases = 0;
uint32_t lastSample = 0, lastPrint = 0, lastStats = 0;

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.printf("GT1151_TEST chip=%s revision=%u sdk=%s\n",
                String(ESP.getChipModel()).c_str(), ESP.getChipRevision(), ESP.getSdkVersion());
  if (!Wire.begin(0, 1, 100000)) { Serial.println("FAIL I2C begin"); return; }
  Wire.setTimeOut(50);
  uint8_t reg[] = {0x81,0x40}, identity[11] = {};
  if (touchBus.transfer(0x14, reg, 2, identity, sizeof(identity)) != nice_gfx::Status::Ok) {
    Serial.println("FAIL GT1151 identity read at 0x14 (SDA=0 SCL=1)"); return;
  }
  Serial.print("GT1151_ID bytes=");
  for (uint8_t byte : identity) Serial.printf("%02X ", byte);
  Serial.println();
  const auto status = touch.begin();
  ready = status == nice_gfx::Status::Ok;
  Serial.printf("GT1151_BEGIN status=%u ready=%u\n", unsigned(status), ready);
  if (ready) Serial.println("READY touch/drag/release the screen; send s for counters");
}

void printStats() {
  Serial.printf("STATS polls=%lu samples=%lu presses=%lu releases=%lu errors=%lu heap=%u\n",
                (unsigned long)polls, (unsigned long)samples, (unsigned long)presses,
                (unsigned long)releases, (unsigned long)errors, ESP.getFreeHeap());
}

void loop() {
  if (!ready) { delay(1000); return; }
  const uint32_t now = millis();
  nice_gfx::RawTouch point;
  ++polls;
  if (touch.read(point)) {
    ++samples; lastSample = now;
    if (!pressed) { pressed = true; ++presses; Serial.println("PRESS"); }
    if (now - lastPrint >= 50) {
      Serial.printf("POINT x=%u y=%u strength=%u\n", point.x, point.y, point.pressure);
      lastPrint = now;
    }
  } else if (touch.status() != nice_gfx::Status::Ok) {
    ++errors;
    if (pressed) { pressed = false; Serial.println("CANCEL"); }
    if (errors <= 5 || now - lastPrint >= 1000) {
      Serial.printf("ERROR status=%u\n", unsigned(touch.status())); lastPrint = now;
    }
  } else if (pressed && now - lastSample >= 30) {
    pressed = false; ++releases; Serial.println("RELEASE");
  }
  if (now - lastStats >= 5000) { printStats(); lastStats = now; }
  while (Serial.available()) if (Serial.read() == 's') printStats();
  delay(5);
}
