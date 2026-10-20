#include <NICE_GFX.h>
#if !defined(SEEED_WIO_TERMINAL)
#error "Select Seeeduino Wio Terminal from Seeed SAMD Boards"
#endif

WioTerminalDisplay display;
bool displayReady=false;
void setup() {
  Serial.begin(115200);
  pinMode(LCD_BACKLIGHT, OUTPUT);
  digitalWrite(LCD_BACKLIGHT, HIGH);
  if (!display.begin()) return;
  displayReady=true;
  display.setRotation(3);
  display.fillScreen(GFX_BLACK);
  display.fillRect(0, 0, 106, 120, GFX_RED);
  display.fillRect(106, 0, 107, 120, GFX_GREEN);
  display.fillRect(213, 0, 107, 120, GFX_BLUE);
  display.setFont(nice_gfx::fonts::ProFont6x12);
  display.setTextColor(GFX_WHITE, GFX_BLACK);
  display.setTextSize(2);
  display.drawString("Wio Terminal / SPI3", 12, 140);
  display.drawString("320 x 240 / RGB565", 12, 170);
}
void loop() {
  static uint32_t last=0;
  if (Serial && uint32_t(millis()-last)>=2000) {
    last=millis();Serial.print("WIO_BASIC ok=");Serial.print(displayReady);
    if (displayReady) {
      Serial.print(" RGB=");Serial.print(display.readPixel(20,20),HEX);Serial.print(',');
      Serial.print(display.readPixel(120,20),HEX);Serial.print(',');Serial.print(display.readPixel(240,20),HEX);
    }
    Serial.println();
  }
  if(Serial.available())Serial.read();
  delay(10);
}
