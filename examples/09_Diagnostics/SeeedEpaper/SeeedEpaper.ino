#include <NICE_GFX.h>
#include <Display/Presets/SeeedEpaper.h>
#include <Display/Presets/SeeedEpaperBoards.h>

// EE04 + 7.5-inch UC8179. Select the exact panel revision you wired.
constexpr auto board=nice_gfx::SeeedEpaperBoard::EE04;
nice_gfx::SeeedEpaperBoardIo boardIo(board);
nice_gfx::GxEpd2Config config(){auto c=nice_gfx::seeedEpaperConfig(board);c.io=&boardIo;return c;}
NGFX_SEEED_750_BW screen(config());
void setup(){
  if(!screen.begin())return;
  screen.startWrite();screen.fillScreen(GFX_WHITE);
  screen.drawRect(8,8,screen.width()-16,screen.height()-16,GFX_BLACK);
  screen.setFont(nice_gfx::fonts::Fixed6x13);
  screen.setTextColor(GFX_BLACK,GFX_WHITE);screen.drawString("Seeed ePaper",20,20);
  screen.endWrite();
}
void loop(){screen.pollDisplay(millis());}
