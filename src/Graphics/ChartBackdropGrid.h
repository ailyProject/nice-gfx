#pragma once
#include <stdint.h>
#include <stddef.h>
#ifndef NICE_GFX_UI_CHART_FUSED_BACKDROP_GRID
#define NICE_GFX_UI_CHART_FUSED_BACKDROP_GRID 1
#endif
#ifndef NICE_GFX_UI_CHART_FUSED_PPA_FRAME
#define NICE_GFX_UI_CHART_FUSED_PPA_FRAME 1
#endif
#define NICE_GFX_UI_CHART_FUSED_PPA_FRAME_VERSION 1
#define NICE_GFX_UI_CHART_FUSED_BACKDROP_GRID_STATS_VERSION 1
namespace nice_gfx {
class TileCanvas;
// Stack-only, immutable between prepare and paint. No framebuffer/source lease
// escapes the synchronous View node painter.
class ChartBackdropGridPlan {
  friend class TileCanvas;
  const TileCanvas *owner=nullptr;uint16_t *target=nullptr;size_t stride=0;
  int32_t x=0,y=0,width=0,height=0,left=0,right=0,top=0,bottom=0,tileX=0,tileY=0;
  uint16_t background=0,radius=0,rows[64],rowColors[64],columns[96],columnColors[96];
  uint8_t rowCount=0,columnCount=0;
};
struct ChartBackdropGridResult {uint64_t pixels=0,holes=0,rows=0;};
}
