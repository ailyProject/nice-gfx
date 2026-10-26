#include <NICE_GFX.h>

// Required: replace every -1 with the real pins for the matching module.
constexpr int kCs=-1,kDc=-1,kReset=-1,kBusy=-1,kClock=-1,kMosi=-1;
SSD1681_200x200_EPD_SPI screen(kCs,kDc,kReset,kBusy,kClock,kMosi);
bool ready=false;
uint32_t lastReading=0;
unsigned reading=0;

void setup() {
  Serial.begin(115200);
  if(kCs<0||kDc<0||kReset<0||kBusy<0||kClock<0||kMosi<0){
    Serial.println("Set the six display pins before running EpaperRefresh.");return;
  }
  ready=screen.begin();if(!ready){Serial.println("Display initialization could not start.");return;}
  screen.setFont(nice_gfx::fonts::Fixed6x13);
  screen.startWrite();
  screen.fillScreen(GFX_WHITE);
  screen.setTextColor(GFX_BLACK,GFX_WHITE);
  screen.drawString("Reading",12,20);
  screen.drawNumber(0,12,60);
  screen.endWrite();
}
void loop() {
  if(!ready)return;
  const uint32_t now=millis();
  const auto status=screen.pollDisplay(now); // No wait loop for physical BUSY.
  if(status!=nice_gfx::Status::Ok&&status!=nice_gfx::Status::Busy){
    Serial.println("Display I/O fault. Check BUSY, power and wiring.");ready=false;return;
  }
  // A changing sensor could update this value while a prior frame is busy.
  // The backend coalesces updates; it never interrupts an active waveform.
  if(uint32_t(now-lastReading)>=1500){
    lastReading=now;++reading;
    screen.startWrite();
    screen.fillRect(12,60,150,16,GFX_WHITE);
    screen.setTextColor(GFX_BLACK,GFX_WHITE);
    screen.drawNumber(reading,12,60);
    screen.endWrite();
  }
}
