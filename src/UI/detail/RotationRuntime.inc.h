// Included inside View::Impl. Static rotation never changes layout or owns
// animation state. The same cumulative map serves paint, damage and input.
bool pageRotated(const PageDesc &p) const {
  for(uint16_t i=0;i<p.count;++i)if(p.nodes[i].rotation%360)return true;
  return false;
}

bool rotated(const PageDesc &p,const RetainedVector<State> &s,int i) const {
  for(int child=i,at=i;at>=0;child=at,at=s[at].parent==NoParent?-1:s[at].parent){
    if(at!=i&&viewportOverlayRoot(p,child))break;
    if(p.nodes[at].rotation%360)return true;
  }
  return false;
}

RotationMatrix rotationMatrix(const PageDesc &p,const RetainedVector<State> &s,int i) const {
  RotationMatrix result;
  const Rect own=poseRect(p,s,i);
  if(p.nodes[i].motion){const auto &m=motionPose(p,s,i);
    if(m.scale!=1024||m.rotation%360){int64_t px,py;
      motion_geometry::pivot100(own,p.nodes[i].motion->pivotX,p.nodes[i].motion->pivotY,s[i].kind==Kind::Shape,px,py);
      result.then(RotationMatrix::around(own.x+double(px)/100,own.y+double(py)/100,m.rotation,m.scale));
    }
  }
  for(int child=i,at=i;at>=0;child=at,at=s[at].parent==NoParent?-1:s[at].parent){
    if(at!=i&&viewportOverlayRoot(p,child))break;
    const int angle=p.nodes[at].rotation%360;
    if(angle){const Rect box=poseRect(p,s,at);result.then(RotationMatrix::around(box.x+double(box.width)/2,box.y+double(box.height)/2,angle));}
  }
  return result;
}

bool rotatedClipContains(const PageDesc &p,const RetainedVector<State> &s,int i,double x,double y,bool own=true) const {
  const auto inside=[](Rect box,RotationPoint point){return point.x>=box.x&&point.y>=box.y&&point.x<double(box.x)+box.width&&point.y<double(box.y)+box.height;};
  for(int child=i,at=i;at>=0;child=at,at=s[at].parent==NoParent?-1:s[at].parent){
    if(at!=i&&viewportOverlayRoot(p,child))break;
    const auto local=rotationMatrix(p,s,at).inverse().point(x,y);
    const Rect box=poseRect(p,s,at);
    if(at==i&&own&&!inside(box,local))return false;
    if(at!=i&&((s[at].flags&Clip)||scrolling(s[at].kind))&&!inside(childBox(p,s,at,box),local))return false;
    if(p.nodes[at].motion){const auto &m=motionPose(p,s,at);
      if(m.reveal<1024&&!inside(motion_geometry::reveal(box,m.reveal,p.nodes[at].motion->flags),local))return false;
    }
  }
  return true;
}

RotationPoint inputPoint(int i,int32_t px,int32_t py) const {
  return rotated(page(),state(),i)?rotationMatrix(page(),state(),i).inverse().point(px+.5,py+.5):RotationPoint{double(px),double(py)};
}

RotationPoint inputDelta(int i,int32_t dx,int32_t dy) const {
  if(i<0||!rotated(page(),state(),i))return {double(dx),double(dy)};
  const auto inverse=rotationMatrix(page(),state(),i).inverse();
  return {inverse.a*dx+inverse.c*dy,inverse.b*dx+inverse.d*dy};
}

bool inputHorizontal(int i,int32_t dx,int32_t dy) const {
  const auto delta=inputDelta(i,dx,dy);return fabs(delta.x)>=fabs(delta.y);
}

struct RotationClip { RotationMatrix inverse;Rect rect; };
std::vector<RotationClip> rotationClips;

void paintRotatedNode(int i,Rect clipped,const DirtyRect &area,uint16_t *pixels,size_t stride,uint8_t alpha) {
  const auto &p=*rasterFrame.current.page;const auto &s=rasterFrame.current.values;
  RotationMatrix matrix=rotationMatrix(p,s,i);matrix.x+=paintOffsetX;matrix.y+=paintOffsetY;
  const Rect source=intersect(poseRect(p,s,i),matrix.inverse().bounds(clipped));
  if(source.width<=0||source.height<=0)return;
  if(alpha<255){underlay.resize(size_t(clipped.width)*clipped.height);
    for(int32_t y=0;y<clipped.height;++y)memcpy(underlay.data()+size_t(y)*clipped.width,
      pixels+size_t(clipped.y-area.y+y)*stride+clipped.x-area.x,size_t(clipped.width)*2);
  }
  rotationClips.clear();
  for(int child=i,at=i;at>=0;child=at,at=s[at].parent==NoParent?-1:s[at].parent){
    if(at!=i&&viewportOverlayRoot(p,child))break;
    const Rect box=poseRect(p,s,at);const auto inverse=rotationMatrix(p,s,at).inverse();
    if(at!=i&&((s[at].flags&Clip)||scrolling(s[at].kind)))rotationClips.push_back({inverse,childBox(p,s,at,box)});
    if(p.nodes[at].motion){const auto &m=motionPose(p,s,at);
      if(m.reveal<1024)rotationClips.push_back({inverse,motion_geometry::reveal(box,m.reveal,p.nodes[at].motion->flags)});
    }
  }
  struct Context {Impl *self;int32_t x,y;} context{this,paintOffsetX,paintOffsetY};
  canvas.transform(matrix,source,[](void *data,double x,double y){
    auto &c=*static_cast<Context *>(data);auto &v=*c.self;
    for(const auto &clip:v.rotationClips){const auto point=clip.inverse.point(x-c.x,y-c.y);const auto &r=clip.rect;
      if(point.x<r.x||point.y<r.y||point.x>=double(r.x)+r.width||point.y>=double(r.y)+r.height)return false;
    }
    return true;
  },&context);
  // The painter continues to use the original page coordinate system.
  const int32_t oldX=paintOffsetX,oldY=paintOffsetY;paintOffsetX=paintOffsetY=0;
  paintNode(i,source);
  paintOffsetX=oldX;paintOffsetY=oldY;canvas.clearTransform();
  if(alpha<255){
    for(int32_t y=0;y<clipped.height;++y)for(int32_t x=0;x<clipped.width;++x){
      uint16_t &pixel=pixels[size_t(clipped.y-area.y+y)*stride+size_t(clipped.x-area.x+x)];
      const uint16_t original=underlay[size_t(y)*clipped.width+x];
      const uint16_t foreground=renderOrder==PixelOrder::Wire?rgb565_blend::fromWire(pixel):pixel;
      const uint16_t background=renderOrder==PixelOrder::Wire?rgb565_blend::fromWire(original):original;
      const uint16_t color=rgb565_blend::color(foreground,background,alpha);
      pixel=renderOrder==PixelOrder::Wire?rgb565_blend::toWire(color):color;
    }
  }
}

// Focus is a final overlay, independent of node alpha and surface captures.
void paintRotatedFocus(int i,const DirtyRect &area,uint16_t *pixels,size_t stride) {
  const auto &p=*rasterFrame.current.page;const auto &s=rasterFrame.current.values;
  const Rect box=poseRect(p,s,i);const Style style=styleFor(p,s,i);
  const auto matrix=rotationMatrix(p,s,i),inverse=matrix.inverse();
  Rect target=matrix.bounds(box);target.x+=paintOffsetX;target.y+=paintOffsetY;
  target=intersect(target,{area.x,area.y,area.width,area.height});
  for(int32_t y=target.y;y<target.y+target.height;++y)for(int32_t x=target.x;x<target.x+target.width;++x){
    const double px=x-paintOffsetX+.5,py=y-paintOffsetY+.5;
    if(!rotatedClipContains(p,s,i,px,py))continue;
    const auto point=inverse.point(px,py);const int32_t lx=int32_t(floor(point.x))-box.x,ly=int32_t(floor(point.y))-box.y;
    bool ring=false;
    for(int n=0;n<snapshotFocusWidth&&2*n<std::min(box.width,box.height);++n){
      if(motionCornerPixel(lx-n,ly-n,box.width-2*n,box.height-2*n,style,n)&&
          !motionCornerPixel(lx-n-1,ly-n-1,box.width-2*n-2,box.height-2*n-2,style,n+1)){ring=true;break;}
    }
    if(!ring)continue;
    uint16_t &pixel=pixels[size_t(y-area.y)*stride+size_t(x-area.x)];
    const uint16_t under=renderOrder==PixelOrder::Wire?rgb565_blend::fromWire(pixel):pixel;
    const uint16_t color=displayPolicy.monochrome?DisplayPolicy::foregroundOn(under):snapshotFocusColor;
    pixel=renderOrder==PixelOrder::Wire?rgb565_blend::toWire(color):color;
  }
}
