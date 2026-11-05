// Reflow is limited to compiler-declared vertical scopes. Immutable DynamicNode
// snapshots keep layout changes coherent with in-flight raster frames.
void rebuildComponentFlow() {
  if(!project || state().empty()) return;

  prepareComponents();
  bool needed=false;

  for(const auto &link:componentLinks) {
    if(link.desc->kind==ComponentKind::Flow || link.desc->kind==ComponentKind::Disclosure || link.query>=0) {
      needed=true;
      break;
    }
  }

  if(!needed) return;

  struct Geometry {
    int32_t y=0,height=-1;
    bool hidden=false;
  };

  std::vector<Geometry> next(page().count);

  const auto authored=[&](int index) {
    Rect rect=page().nodes[index].rect;
    const auto &dynamic=state()[index].dynamic;

    if(dynamic && dynamic->boundsOverride) {
      rect.x+=dynamic->offsetX;
      rect.y+=dynamic->offsetY;
      rect.width=dynamic->width;
      rect.height=dynamic->height;
    }

    return rect;
  };

  const auto folded=[](const char *value) {
    std::string result=value?value:"";

    for(char &character:result) {
      if(character>='A' && character<='Z') character+=char('a'-'A');
    }

    return result;
  };

  for(const auto &link:componentLinks) {
    if(link.query<0) continue;

    const char *draft=softInput.visible && softInput.node==link.query
      ? softInput.draft.c_str() : state()[link.query].text.c_str();
    const std::string query=folded(draft);
    size_t matches=0;

    for(uint8_t option=0;option<link.desc->optionCount;++option) {
      const int index=find(link.desc->options[option].id);
      if(index<0) continue;

      next[size_t(index)].hidden=folded(state()[index].text.c_str()).find(query)==std::string::npos;
      if(!next[size_t(index)].hidden && !(state()[index].flags&Hidden)) ++matches;
    }

    if(link.empty>=0) next[size_t(link.empty)].hidden=matches!=0;

    if(link.desc->optionCount) {
      const int option=find(link.desc->options[0].id);
      const int results=option>=0?parent(option):-1;

      if(results>=0) {
        next[size_t(results)].hidden=matches==0;

        if(matches && link.empty>=0 && parent(results)==parent(link.empty)) {
          const Rect end=authored(link.empty);
          next[size_t(results)].height=std::max<int32_t>(0,end.y+end.height-authored(results).y);
        }
      }
    }
  }

  for(size_t position=componentLinks.size();position>0;--position) {
    const auto &link=componentLinks[position-1];
    const auto &desc=*link.desc;

    if(desc.kind==ComponentKind::Disclosure && (desc.flags&ComponentAutoHeight)
      && link.panel>=0 && (state()[link.panel].flags&Hidden)) {
      const Rect root=authored(link.root),trigger=authored(link.text);
      const auto &style=styleFor(page(),state(),link.root);

      next[size_t(link.root)].height=std::min(root.height,std::max<int32_t>(0,
        trigger.y+trigger.height-root.y+style.paddingBottom+style.borderWidth));
    }

    if(desc.kind!=ComponentKind::Flow) continue;

    const Rect root=authored(link.root);
    const auto &style=styleFor(page(),state(),link.root);
    int32_t cursor=root.y+style.paddingTop+style.borderWidth;
    bool placed=false;

    for(uint8_t option=0;option<desc.optionCount;++option) {
      const int index=find(desc.options[option].id);
      if(index<0) continue;

      const Rect box=authored(index);
      if((state()[index].flags&Hidden) || next[size_t(index)].hidden) continue;

      if(placed) cursor+=desc.gap;
      next[size_t(index)].y=cursor-box.y;
      cursor+=next[size_t(index)].height>=0?next[size_t(index)].height:box.height;
      placed=true;
    }

    if(desc.flags&ComponentAutoHeight) {
      next[size_t(link.root)].height=std::min(root.height,std::max<int32_t>(0,
        cursor-root.y+style.paddingBottom+style.borderWidth));
    }
  }

  bool changed=false;

  for(size_t index=0;index<next.size();++index) {
    const auto &value=next[index];
    const auto &old=state()[index].dynamic;

    if((old?old->flowOffsetY:0)==value.y && (old?old->flowHeight:-1)==value.height
      && (old?old->flowHidden:false)==value.hidden) continue;

    auto geometry=old?std::make_shared<DynamicNode>(*old):std::make_shared<DynamicNode>();
    geometry->flowOffsetY=value.y;
    geometry->flowHeight=value.height;
    geometry->flowHidden=value.hidden;
    state()[index].dynamic=geometry;
    changed=true;
  }

  if(changed) {
    ++inputLayoutEpoch;
    clearFrameGeometryMemo();

    for(uint16_t index=0;index<page().count;++index) {
      if(state()[index].kind==Kind::Scroll) scroll(index,state()[index].scrollX,state()[index].scrollY,false);
    }

    refresh->invalidateAll();
  }
}
