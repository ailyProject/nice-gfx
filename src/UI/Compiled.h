#pragma once
#include <stdint.h>
#include <stddef.h>

// Shape motion and configurable pivots are versioned independently below.
#define NICE_GFX_UI_SHAPE_MOTION_VERSION 1
#define NICE_GFX_UI_PIVOT_MOTION_VERSION 1
#define NICE_GFX_UI_VERTICAL_RANGE_VERSION 1
#define NICE_GFX_UI_MEDIA_COLOR_KEY_VERSION 1

namespace nice_gfx { struct MonoFont; struct MonoIcon; }
namespace nice_gfx { namespace htmlui {

// Versioned, generated C++ resource ABI. Geometry is page-relative, in physical
// pixels at PageDesc's target size. Parents precede children. Resource pointers
// remain valid for the lifetime of the mounted project.
constexpr uint32_t ResourceVersion = 17;
constexpr uint16_t NoParent = 0xffff;
using Id = uint32_t;
enum class Kind : uint8_t { Container, Text, Button, Range, Checkbox, Switch, Progress, Scroll, Pager, Label, Icon, Media, Gauge, Ring, LineChart, BarChart, Spinbox, Shape, InputField };
enum class RouteAction : uint8_t { None, Push, Replace, Back, Home };
enum NodeFlags : uint16_t { Hidden = 1, Disabled = 2, Clip = 4, Horizontal = 8, Transparent = 16,
  TouchNone = 32, TouchPanX = 64, TouchPanY = 128, OverscrollContain = 256,
  GradientActive = 512, GradientDisabled = 1024, ActiveTransparent = 2048, DisabledTransparent = 4096,
  NoAntialias = 8192, VerticalRange = 16384, RangeFillGradient = 32768 };
struct Rect { int32_t x, y, width, height; };
// A shared flash-resident RGB565 palette and compiler-generated
// Q16 palette coordinates. Pixel (x,y) is local to the UNCLIPPED node;
// index = clamp((origin + x*stepX + y*stepY + 32768) >> 16, 0, 255).
// dither=0: 256 colors; dither=1: 256*4 interleaved 2x2 Bayer phases,
// addressed as index*4 + (localY&1)*2 + (localX&1). Both palette projection
// and dither phase move with the node, never with the clipped tile origin.
struct GradientDesc { const uint16_t *colors; int32_t origin, stepX, stepY; uint8_t dither; };
// Page visual tokens come from the resource. Transient keyboards share a
// standard palette and may borrow a dedicated font from InputFieldDesc.
struct Style {
  uint16_t background, color, borderColor, trackColor, fillColor, thumbColor;
  uint16_t pressedColor, disabledColor;
  uint8_t borderWidth, radius, fontSize, textAlign;
  uint16_t paddingTop, paddingRight, paddingBottom, paddingLeft;
  uint16_t trackSize, thumbWidth, thumbHeight, thumbRadius;
  const nice_gfx::MonoFont *font;
  const GradientDesc *gradient;
};
enum class AnimationEffect : uint8_t { None, FadeIn, FadeOut, SlideLeft, SlideRight, SlideUp, SlideDown, Float, Bounce, Shake, Pulse, Blink, SlideOutLeft, SlideOutRight, SlideOutUp, SlideOutDown };
enum class AnimationEasing : uint8_t { Linear, EaseIn, EaseOut, EaseInOut, BackOut, BounceOut };
enum class TransitionEffect : uint8_t { None, SlideLeft, SlideRight, SlideUp, SlideDown, Fade };
struct TransitionDesc { TransitionEffect effect; AnimationEasing easing; uint16_t duration; };
enum class AnimationTrigger : uint8_t { Enter, Click, Manual, Show };
struct AnimationDesc {
  AnimationEffect effect;
  AnimationEasing easing;
  AnimationTrigger trigger;
  uint16_t duration, delay, repeat, distance; // ms, ms, total cycles (0=infinite), px
};
// Author keyframes are normalized once by the compiler. Offsets, uniform scale
// and reveal use Q10; rotation is clockwise degrees about MotionDesc's pivot.
// Media/Icon/Shape leaves may scale/rotate; reveal clips any node and its subtree.
// Shape motion cannot combine a nonidentity scale/rotation with partial reveal.
struct MotionKeyframe {
  uint16_t offset;
  int16_t x, y;
  uint16_t scale;
  int16_t rotation;
  uint16_t reveal;
  uint8_t opacity;
};
enum MotionFlags : uint8_t { MotionAlternate = 1, MotionRevealRight = 2,
  MotionRevealUp = 4, MotionRevealDown = 6 };
// count=0 attaches direction/reveal options to an existing preset. The complete
// immutable frame table and this descriptor can be shared across all pages.
struct MotionDesc {
  const MotionKeyframe *frames; uint8_t count, flags;
  // Integer percentages of the node box, including external pivots. Shape
  // percentages span first/last pixel centers; Media/Icon span box edges.
  // Constructor defaults keep generated brace initializers usable in C++11.
  int16_t pivotX;
  int16_t pivotY;
  constexpr MotionDesc(const MotionKeyframe *f=nullptr,uint8_t n=0,uint8_t options=0,int16_t px=50,int16_t py=50)
    :frames(f),count(n),flags(options),pivotX(px),pivotY(py) {}
};
struct PagerDesc { uint16_t indicatorHeight; uint8_t dotSize, dotGap; };
struct MediaDesc { const uint8_t *pixels; uint16_t width, height, frames; uint8_t fps, bytesPerPixel; };
// Preserve descriptor ABI: 0x82 denotes wire RGB565 with a two-byte, MSB-first
// transparent-color prefix. The prefix is shared by all frames, not per pixel.
constexpr uint8_t MediaRgb565ColorKey = 0x82;
inline bool mediaColorKeyed(const MediaDesc &m) { return m.bytesPerPixel==MediaRgb565ColorKey; }
inline uint8_t mediaPixelBytes(const MediaDesc &m) { return uint8_t(m.bytesPerPixel&0x7f); }
inline const uint8_t *mediaPixels(const MediaDesc &m) { return m.pixels+(mediaColorKeyed(m)?2:0); }
enum class ShapeType : uint8_t { Circle, Ellipse, Rect, Line, Polyline, Polygon };
enum ShapeFlags : uint8_t { ShapeFill = 1, ShapeStroke = 2 };
struct ShapePoint { uint16_t x, y; };
// Logical coordinates scale with the node's content box. The renderer insets
// the center line by half of the stroke width, keeping its round caps inside
// that box. Points/descriptors remain flash-resident; rendering allocates no
// vertex buffers or bitmaps. Fill uses the even-odd rule for concave polygons.
struct ShapeDesc {
  ShapeType type;
  uint16_t fillColor, strokeColor;
  uint8_t strokeWidth, flags, count;
  uint16_t viewWidth, viewHeight;
  const ShapePoint *points;
};
struct InstrumentZone { int32_t to; uint16_t color; };
struct ChartSeriesDesc { const char *name; uint16_t color, count; const int32_t *values; };
struct ChartReferenceDesc { int32_t value; uint16_t color; uint8_t dash; const char *label; };
// Bounded immutable chart appearance. Fill blends against an opaque background.
struct ChartStyleDesc {
  uint8_t lineWidth, fillMode, fillOpacity, gridX, minorGrid, pointMode, pointRadius, lineMode, barMode, barRadius, barGap;
  bool axisTicks, legend, axes;
  uint8_t referenceCount; const ChartReferenceDesc *references;
};
struct InstrumentDesc {
  uint8_t decimals; const char *unit; int16_t start; uint16_t sweep, thickness; uint8_t ticks; uint16_t duration;
  uint8_t zoneCount; const InstrumentZone *zones;
  uint16_t capacity; uint8_t seriesCount; const ChartSeriesDesc *series;
  uint16_t labelCount; const char *const *labels; uint8_t grid; bool horizontal;
  const ChartStyleDesc *chartStyle;
};
enum class InputFieldType : uint8_t { Text, Number, Password, Pin, Tel };
enum class InputEnterAction : uint8_t { Auto, Done, Next, Search };
enum class KeyboardTheme : uint8_t { Light, Dark };
// Number values and bounds use integer fixed point, like instrument values.
// Value is a string so PINs, phone numbers and leading zeroes are preserved.
struct InputFieldDesc {
  const char *value, *placeholder, *label, *unit;
  int64_t min, max;
  uint16_t maxBytes, maxLength;
  uint8_t decimals, pinLength;
  InputFieldType type;
  InputEnterAction enterAction;
  bool required, readOnly, hasMin, hasMax;
  const nice_gfx::MonoFont *keyboardFont; // Optional 16px chrome font, compiled as a subset.
  uint16_t keyboardHeight; // Zero fills the screen; otherwise a bottom dock in pixels. Round screens stay fullscreen.
  KeyboardTheme keyboardTheme; // Light by default; the View API can override it across fields.
};
struct NodeDesc {
  uint16_t parent;
  Id id;
  Kind kind;
  Rect rect;
  Style style;
  const char *text;
  int32_t value, minimum, maximum, step;
  uint16_t flags;
  const char *route;
  RouteAction routeAction;
  Id targetId;
  AnimationDesc animation;
  PagerDesc pager;
  const nice_gfx::MonoIcon *icon;
  const MediaDesc *media;
  const InstrumentDesc *instrument;
  const ShapeDesc *shape;
  const InputFieldDesc *inputField;
  const AnimationDesc *exitAnimation; // Optional one-shot Manual departure preset.
  const MotionDesc *motion; // Optional compiled keyframes/options; null keeps the legacy fast path.
};
struct PageDesc {
  const char *path;
  const NodeDesc *nodes;
  uint16_t count, width, height;
  uint16_t background;
  const TransitionDesc *enterTransition, *exitTransition; // nullptr inherits project default; None disables.
};
struct ProjectDesc {
  const PageDesc *pages;
  uint16_t count, initial;
  uint32_t version;
  TransitionDesc transition;
};

} }
