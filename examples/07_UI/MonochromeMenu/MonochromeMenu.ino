#include <NICE_GFX.h>
#include <Wire.h>
#include <Platform/Arduino/I2cWire.h>
#include <UI/Keys.h>

// Set these to three real active-low buttons, or leave disconnected (-1).
constexpr int kPrevious=-1,kNext=-1,kConfirm=-1;
nice_gfx::I2cWire menuBus(Wire);
SSD1306_128x64_I2C screen(menuBus);
nice_gfx::htmlui::KeyInput previousKey,nextKey,confirmKey;
const char *items[] = {"Status","Settings","Back"};
uint8_t selected=0;
bool ready=false,dirty=true,confirmed=false;
uint32_t lastPaint=0;

bool pressed(int pin) { return pin>=0&&digitalRead(pin)==LOW; }
void drawMenu() {
  screen.startWrite();
  screen.fillScreen(GFX_BLACK);
  screen.setTextColor(GFX_WHITE,GFX_BLACK);
  screen.drawString(confirmed?"Selected":"Menu",2,0);
  for(uint8_t i=0;i<3;++i){
    const int y=16+int(i)*15;
    const uint16_t background=i==selected?GFX_WHITE:GFX_BLACK;
    screen.fillRect(0,y,128,14,background);
    screen.setTextColor(i==selected?GFX_BLACK:GFX_WHITE,background);
    screen.drawString(items[i],3,y);
  }
  screen.endWrite(); // One logical update; the driver sends only changed bytes.
}
void setup() {
  Wire.begin(); // Select SDA/SCL here if the board requires explicit pins.
  const int pins[]={kPrevious,kNext,kConfirm};
  for(int pin : pins)if(pin>=0)pinMode(pin,INPUT_PULLUP);
  ready=screen.begin();if(!ready)return;
  screen.setFont(nice_gfx::fonts::Fixed6x13);
}
void loop() {
  if(!ready)return;
  const uint32_t now=millis();
  using nice_gfx::htmlui::KeyGesture;
  const auto previous=previousKey.update(pressed(kPrevious),now,false,true);
  const auto next=nextKey.update(pressed(kNext),now,false,true);
  const auto confirm=confirmKey.update(pressed(kConfirm),now);
  if(previous==KeyGesture::Press||previous==KeyGesture::Repeat){selected=(selected+2)%3;confirmed=false;dirty=true;}
  if(next==KeyGesture::Press||next==KeyGesture::Repeat){selected=(selected+1)%3;confirmed=false;dirty=true;}
  if(confirm==KeyGesture::Press){confirmed=true;dirty=true;}
  // Debounce/input keeps running even when no pixels need updating.
  if(dirty&&uint32_t(now-lastPaint)>=33){drawMenu();dirty=false;lastPaint=now;}
}
