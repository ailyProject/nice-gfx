// Inspect the display object and the GPIO arguments used to construct it.
#include <NICE_GFX.h>
#include "Hardware.h"

ExampleDisplay gfx;

void setup() {
  Serial.begin(115200);
  gfx.begin();
  const DisplayInfo info = gfx.displayInfo();
  Serial.print("NICE_GFX version: "); Serial.println(info.version);
  Serial.print("Controller ID: 0x"); Serial.println(info.controllerId, HEX);
  Serial.print("Initialized: "); Serial.println(info.initialized ? "yes" : "no");
  Serial.print("Asynchronous transport: "); Serial.println(info.asynchronous ? "yes" : "no");
  Serial.print("Native size: "); Serial.print(info.nativeWidth);
  Serial.print(" x "); Serial.println(info.nativeHeight);
  Serial.print("Current size: "); Serial.print(info.width);
  Serial.print(" x "); Serial.println(info.height);
  Serial.print("Rotation: "); Serial.println(info.rotation);
  Serial.print("Bus frequency (Hz): "); Serial.println(info.bus.frequency);
  Serial.print("Data lanes: "); Serial.println(info.bus.lanes);
  Serial.print("CS: "); Serial.println(info.bus.cs);
  Serial.print("DC: "); Serial.println(info.bus.dc);
  Serial.print("Reset: "); Serial.println(info.resetPin);
  Serial.print("Clock: "); Serial.println(info.bus.clock);
  Serial.print("Read strobe: "); Serial.println(info.bus.read);
  Serial.print("Write strobe: "); Serial.println(info.bus.write);
  for (uint8_t i = 0; i < 16; ++i) {
    if (info.bus.data[i] < 0) continue;
    Serial.print("Data pin "); Serial.print(i);
    Serial.print(": "); Serial.println(info.bus.data[i]);
  }
}

void loop() {}
