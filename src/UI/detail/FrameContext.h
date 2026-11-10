#pragma once
#include "../Compiled.h"
#include "RetainedStorage.h"
#include "FrameGeometryStorage.h"
#include <vector>
#ifndef NICE_GFX_UI_SHAPE_INK_DAMAGE
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#define NICE_GFX_UI_SHAPE_INK_DAMAGE 1
#else
#define NICE_GFX_UI_SHAPE_INK_DAMAGE 0
#endif
#endif

namespace nice_gfx { namespace htmlui { namespace detail {

struct FrameTextLine { size_t begin,end;int32_t width; };
struct FrameGeometry {
  Rect rect,clip;
  uint32_t lineFirst=0;
  uint16_t lineCount=0,children=0;
  int32_t textY=0;
  uint8_t alpha=255;
  uint8_t ellipsisCount=0;
  bool visible=true,inputVisible=true;
#if NICE_GFX_UI_SHAPE_INK_DAMAGE
  Rect paintRect;
#endif
};
inline Rect framePaintBounds(const FrameGeometry &geometry){
#if NICE_GFX_UI_SHAPE_INK_DAMAGE
  return geometry.paintRect;
#else
  return geometry.rect;
#endif
}
struct FrameGeometryCache {
  FrameGeometryVector<FrameGeometry> nodes;
  RetainedVector<FrameTextLine> lines;
  std::vector<uint32_t> bands;
  size_t words=0;
  int32_t bandHeight=1,visibleHeight=-1;
  // Authored media bounds survive image-only geometry reuse. Pages without
  // media reject opaque-image coverage without scanning their whole node set.
  uint16_t mediaFirst=NoParent,mediaEnd=0;
};

// Each side owns every dynamic value and motion pose used by its rasterizer.
// Live page writers can replace their state without changing an accepted frame.
// Geometry capacity survives release, while image/text leases do not.
template<class State,class Motion> struct FramePage {
  const PageDesc *page=nullptr;
  RetainedVector<State> values;
  std::vector<Motion> motions;
  FrameGeometryCache geometry;
  void release() { values.clear();motions.clear(); }
};
template<class State,class Motion> struct FrameContext {
  FramePage<State,Motion> current,preview;
  void release() {
    current.release();preview.release();preview.page=nullptr;
  }
};

} } }
