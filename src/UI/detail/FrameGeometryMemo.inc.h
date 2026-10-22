// Included inside View::Impl after retained surfaces. Shared by tiled and
// mapped-framebuffer rendering; keeps no old image or text payload alive.
// These keys deliberately own no Image, instrument or text string. Retain only
// immutable DynamicNode identity, preserving Surface's COW/ABA protection.
// Keep the scalar geometry fields in sync with sameSurfaceGeometry().
struct FrameGeometryKey {
  struct Text {const char *compiled=nullptr;uint64_t revision=0;} text;
  int32_t scrollX=0,scrollY=0,animationX=0,animationY=0;
  std::shared_ptr<const DynamicNode> dynamic;
  uint32_t motionRevision=0;
  uint16_t flags=0;
  uint8_t opacity=255;
  bool matches(const State &value) const {
    return scrollX==value.scrollX&&scrollY==value.scrollY&&flags==value.flags&&
      animationX==value.animationX&&animationY==value.animationY&&opacity==value.opacity&&
      motionRevision==value.motionRevision&&dynamic==value.dynamic;
  }
  void assign(const State &value) {
    text.compiled=value.text.compiled;text.revision=value.text.revision;
    scrollX=value.scrollX;scrollY=value.scrollY;flags=value.flags;
    animationX=value.animationX;animationY=value.animationY;opacity=value.opacity;
    motionRevision=value.motionRevision;dynamic=value.dynamic;
  }
};
const PageDesc *geometryMemoPage=nullptr;
RetainedVector<FrameGeometryKey> geometryMemoKey;
FrameCache geometryMemoCache;
void clearFrameGeometryMemo() {
  geometryMemoPage=nullptr;RetainedVector<FrameGeometryKey>().swap(geometryMemoKey);geometryMemoCache=FrameCache{};
}
void retainFrameGeometry(const PageDesc &p,const RetainedVector<State> &values,const FrameCache &cache) {
  geometryMemoKey.resize(values.size());
  for(size_t i=0;i<values.size();++i)geometryMemoKey[i].assign(values[i]);
  geometryMemoCache=cache;geometryMemoPage=&p;
}
void prepareFrameGeometry(const PageDesc &p,const RetainedVector<State> &values,FrameCache &cache,int32_t visibleHeight=-1) {
  if(geometryMemoPage==&p&&geometryMemoKey.size()==values.size()&&geometryMemoCache.nodes.size()==values.size()&&geometryMemoCache.visibleHeight==visibleHeight){
    bool same=true;for(size_t i=0;i<values.size();++i)if(!geometryMemoKey[i].matches(values[i])){same=false;break;}
    if(same){prepareVisualCache(p,values,geometryMemoKey,geometryMemoCache,cache);retainFrameGeometry(p,values,cache);return;}
  }
  for(const auto &entry:surfaces)if(entry.page==&p&&entry.ready&&entry.key.size()==values.size()&&entry.geometry.nodes.size()==values.size()&&entry.geometry.visibleHeight==visibleHeight){
    bool same=true;for(size_t i=0;i<values.size();++i)if(!sameSurfaceGeometry(entry.key[i],values[i])){same=false;break;}
    if(same){prepareVisualCache(p,values,entry.key,entry.geometry,cache);retainFrameGeometry(p,values,cache);return;}
  }
  prepareCache(p,values,cache,visibleHeight);retainFrameGeometry(p,values,cache);
}
