// ChipIntelli CI130X + SSD1306 128x64 I2C OLED.
// The application initializes Wire; NICE_GFX borrows the existing bus.
#include <NICE_GFX.h>
#include <Wire.h>
#include <Platform/Arduino/I2cWire.h>
#include <stdio.h>

constexpr uint8_t kOledAddress = 0x3C;
constexpr uint32_t kI2cFrequency = 400000;

nice_gfx::I2cWire transport(Wire);
SSD1306_128x64_I2C gfx(transport, kOledAddress);

bool displayReady = false;
const char *displayFailure = "setup pending";
unsigned long lastRefresh = 0;

void drawScreen(unsigned long seconds) {
  char uptime[24];
  snprintf(uptime, sizeof(uptime), "Uptime: %lu s", seconds);

  gfx.startWrite();
  gfx.fillScreen(GFX_BLACK);
  gfx.drawString("CI130X + NICE_GFX", 4, 4);
  gfx.drawString("SSD1306 / 400kHz", 4, 23);
  gfx.drawString(uptime, 4, 42);
  gfx.endWrite();

  Serial.print("OLED frame t=");
  Serial.print(seconds);
  Serial.print("s status=");
  Serial.println(static_cast<unsigned>(gfx.lastError()));
}

void setup() {
  Serial.begin(115200);
  Serial.print("NICE_GFX SSD1306 SDA=");
  Serial.print(SDA);
  Serial.print(" SCL=");
  Serial.println(SCL);

  // CI13XX Wire accepts only the selected variant's fixed SDA/SCL route.
  // Check the core's result before giving the initialized bus to the display.
  if (!Wire.begin(SDA, SCL, kI2cFrequency)) {
    displayFailure = "Wire.begin failed";
    Serial.println("Wire.begin failed: check board and shared pins");
    return;
  }
  Wire.beginTransmission(kOledAddress);
  Serial.print("OLED address 0x");
  Serial.print(kOledAddress, 16);
  Serial.print(" ACK status=");
  Serial.println(Wire.endTransmission());
  if (!gfx.begin()) {
    displayFailure = "NICE_GFX begin failed";
    Serial.println("NICE_GFX begin failed: check OLED and memory");
    return;
  }

  gfx.setFont(nice_gfx::fonts::Fixed6x13);
  gfx.setTextColor(GFX_WHITE, GFX_BLACK);
  displayReady = true;
  displayFailure = nullptr;
  lastRefresh = millis();
  drawScreen(lastRefresh / 1000UL);
}

void loop() {
  const unsigned long now = millis();
  if (now - lastRefresh >= 1000UL) {
    lastRefresh = now;
    if (displayReady) {
      drawScreen(now / 1000UL);
    } else {
      Serial.print("OLED not ready: ");
      Serial.println(displayFailure);
    }
  }

  // Leave time for the core's FreeRTOS and audio tasks.
  delay(10);
}
