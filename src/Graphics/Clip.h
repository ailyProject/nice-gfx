#pragma once
#include "../Core/Geometry.h"

// Shared clipping for native, indexed, masked, transparent and DMA images.
#define PI_CLIP \
  if (_vpOoB) return; \
  nice_gfx::ClippedRect clipped; \
  if (!nice_gfx::clipRect(x,y,w,h,_xDatum,_yDatum,_vpX,_vpY,_vpW,_vpH,clipped)) return; \
  x=clipped.x; y=clipped.y; \
  int32_t dx=clipped.sourceX, dy=clipped.sourceY, dw=clipped.width, dh=clipped.height
