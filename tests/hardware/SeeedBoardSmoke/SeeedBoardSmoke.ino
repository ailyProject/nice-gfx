#include <NICE_GFX.h>
// Compile fixture; these independent instances reuse pins and must not be uploaded.
#if defined(CONFIG_IDF_TARGET_ESP32S3)
nice_gfx::SeeedReterminalE1001Display mono;
nice_gfx::SeeedReterminalE1002Display six;
nice_gfx::SeeedReterminalE1003Display gray;
nice_gfx::SeeedReterminalE1004Display dual;
nice_gfx::SeeedReterminalStickyDisplay automatic;
nice_gfx::SeeedTrmnl75DiyDisplay kit;
nice_gfx::SeeedEe02Spectra133Display ee02;
nice_gfx::SeeedEe03103Display ee03;
nice_gfx::SeeedEe0475Display ee04;
nice_gfx::SeeedEe0575Display ee05;
NICE_GFX *fixtures[]={&mono,&six,&gray,&dual,&automatic,&kit,&ee02,&ee03,&ee04,&ee05};
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
nice_gfx::SeeedXiao75PanelDisplay panel;
NICE_GFX *fixtures[]={&panel};
#endif
void setup(){for(auto *display:fixtures){if(display->beginChecked()!=nice_gfx::Status::Ok)continue;display->fillScreen(GFX_WHITE);}}
void loop(){for(auto *display:fixtures)display->pollDisplay(millis());}
