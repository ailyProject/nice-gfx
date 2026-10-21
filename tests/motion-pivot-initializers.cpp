#include "UI/Compiled.h"
#include <type_traits>
using namespace nice_gfx::htmlui;
constexpr MotionKeyframe frames[]={{0,0,0,1024,0,1024,255},{1024,0,0,1024,90,1024,255}};
constexpr MotionDesc empty{};
constexpr MotionDesc legacy={frames,2,0};
constexpr MotionDesc custom={frames,2,0,0,100};
static_assert(empty.frames==nullptr&&empty.count==0&&empty.flags==0&&empty.pivotX==50&&empty.pivotY==50,"empty motion defaults");
static_assert(legacy.frames==frames&&legacy.count==2&&legacy.flags==0&&legacy.pivotX==50&&legacy.pivotY==50,"legacy three-field initializer");
static_assert(custom.frames==frames&&custom.count==2&&custom.flags==0&&custom.pivotX==0&&custom.pivotY==100,"custom five-field initializer");
static_assert(std::is_standard_layout<MotionDesc>::value,"descriptor must retain a standard binary layout");
static_assert(std::is_trivially_copyable<MotionDesc>::value,"descriptor must remain trivially copyable");
