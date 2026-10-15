// Waveshare 1.9inch Touch LCD: ST7789V2 + CST816, Arduino Mega.
// Connect LCD clock/MOSI to the board's SCK/MOSI pins. The complete NICE_GFX
// example exceeds the Arduino UNO's 32 KiB Flash; use Mega or a 32-bit board.
// With an ESP32, change the GPIOs and initialize Wire with its SDA/SCL pins.
#include <NICE_GFX.h>
#include <Platform/Arduino/TouchWire.h>

ST7789V2_170x320_SPI gfx(10, 7, 8, SCK, MOSI, -1, 10000000);
nice_gfx::TouchWire touchBus(Wire, 4, 3); // Touch reset, interrupt.
nice_gfx::Cst816 touch(touchBus, 170, 320);
bool initialized = false;

void setup() {
  Serial.begin(115200);
  Wire.begin(); // ESP32 example: Wire.begin(actualSda, actualScl).
  Wire.setClock(100000);
  pinMode(9, OUTPUT); digitalWrite(9, HIGH); // Module backlight.
  if (touch.begin() != nice_gfx::Status::Ok || !gfx.begin()) {
    Serial.println("Display or touch initialization failed"); return;
  }
  gfx.attachTouch(touch);
  gfx.fillScreen(GFX_BLACK);
  initialized = true;
}
void loop() {
  if (!initialized) return;
  uint16_t x, y;
  if (gfx.getTouch(&x, &y)) gfx.drawPixel(x, y, GFX_WHITE);
  else if (touch.status() != nice_gfx::Status::Ok) Serial.println("Touch I2C read failed");
  delay(10);
}
