#pragma once
#include "MotionGeometry.h"
#include <math.h>

namespace nice_gfx { namespace htmlui {
// Page-space affine coordinates. Layout remains untransformed; compose each
// node's rotation about its own center before applying its ancestors.
struct RotationPoint { double x,y; };

struct RotationMatrix {
  double a=1,b=0,c=0,d=1,x=0,y=0;

  RotationPoint point(double px,double py) const { return {a*px+c*py+x,b*px+d*py+y}; }

  RotationMatrix inverse() const {
    const double determinant=a*d-b*c;
    RotationMatrix result;

    result.a=d/determinant;result.b=-b/determinant;result.c=-c/determinant;result.d=a/determinant;
    result.x=-(result.a*x+result.c*y);result.y=-(result.b*x+result.d*y);

    return result;
  }

  // Apply the next transform after this one.
  void then(const RotationMatrix &next) {
    const RotationMatrix old=*this;

    a=next.a*old.a+next.c*old.b;b=next.b*old.a+next.d*old.b;
    c=next.a*old.c+next.c*old.d;d=next.b*old.c+next.d*old.d;
    x=next.a*old.x+next.c*old.y+next.x;y=next.b*old.x+next.d*old.y+next.y;
  }

  static RotationMatrix around(double px,double py,int angle,uint16_t scale=1024,uint8_t flip=0) {
    RotationMatrix result;
    const double s=double(motion_geometry::sine(angle))/16384*scale/1024;
    const double c=double(motion_geometry::sine(angle+90))/16384*scale/1024;

    result.a=result.d=c;result.b=s;result.c=-s;

    if(flip&1){result.a=-result.a;result.b=-result.b;}
    if(flip&2){result.c=-result.c;result.d=-result.d;}

    result.x=px-result.a*px-result.c*py;result.y=py-result.b*px-result.d*py;

    return result;
  }

  Rect bounds(Rect rect) const {
    const auto p0=point(rect.x,rect.y),p1=point(double(rect.x)+rect.width,rect.y);
    const auto p2=point(rect.x,double(rect.y)+rect.height),p3=point(double(rect.x)+rect.width,double(rect.y)+rect.height);
    const double left=fmin(fmin(p0.x,p1.x),fmin(p2.x,p3.x)),top=fmin(fmin(p0.y,p1.y),fmin(p2.y,p3.y));
    const double right=fmax(fmax(p0.x,p1.x),fmax(p2.x,p3.x)),bottom=fmax(fmax(p0.y,p1.y),fmax(p2.y,p3.y));
    const int32_t x0=int32_t(floor(left)),y0=int32_t(floor(top));

    return {x0,y0,int32_t(ceil(right))-x0,int32_t(ceil(bottom))-y0};
  }
};
} }
