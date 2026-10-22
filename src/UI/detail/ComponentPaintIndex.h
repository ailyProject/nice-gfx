#pragma once
#include "../Components.h"
#include <algorithm>
#include <vector>

namespace nice_gfx { namespace htmlui { namespace detail {

struct ComponentPaint { bool selected=false,select=false,expanded=false,list=false; };

// Relationships are immutable for one page/component configuration. Cache only
// indices here: selection/expansion always comes from the actual painted frame,
// including the outgoing side of a transition, never from live page state.
class ComponentPaintIndex {
 public:
  bool matches(const PageDesc &page,const ComponentDesc *components,size_t count) const {
    return page_==&page&&components_==components&&count_==count;
  }
  void prepare(const PageDesc &page,const ComponentDesc *components,size_t count) {
    if(matches(page,components,count))return;
    page_=nullptr;
    bindings_.clear();nodes_.clear();
    if(count) {
      nodes_.resize(page.count);
      for(uint16_t i=0;i<page.count;++i)nodes_[i]={page.nodes[i].id,i};
      std::sort(nodes_.begin(),nodes_.end(),[](const Node &a,const Node &b){return a.id<b.id;});
      for(size_t i=0;i<count;++i) {
        const auto &d=components[i];
        if(d.kind!=ComponentKind::Choice&&d.kind!=ComponentKind::Tabs&&d.kind!=ComponentKind::Select&&d.kind!=ComponentKind::List)continue;
        if(find(d.id)<0)continue;
        const int value=find(d.kind==ComponentKind::Tabs?d.pageId:d.id),panel=find(d.expandedId);
        if(d.kind==ComponentKind::Select&&d.textId)bindings_.push_back({d.textId,value,panel,-1,true,false});
        for(uint8_t option=0;option<d.optionCount;++option)
          bindings_.push_back({d.options[option].id,value,panel,int(option),false,d.kind==ComponentKind::List});
      }
      // Preserve the first descriptor's precedence for a shared visual target.
      std::stable_sort(bindings_.begin(),bindings_.end(),[](const Binding &a,const Binding &b){return a.id<b.id;});
      bindings_.erase(std::unique(bindings_.begin(),bindings_.end(),[](const Binding &a,const Binding &b){return a.id==b.id;}),bindings_.end());
    }
    // Publish the key only after every potentially allocating operation.
    page_=&page;components_=components;count_=count;
  }
  template<class Values> ComponentPaint paint(Id id,const Values &values) const {
    const auto found=std::lower_bound(bindings_.begin(),bindings_.end(),id,[](const Binding &a,Id b){return a.id<b;});
    if(found==bindings_.end()||found->id!=id)return {};
    ComponentPaint result;
    result.select=found->select;
    result.expanded=found->select&&found->panel>=0&&!(values[size_t(found->panel)].flags&Hidden);
    result.list=found->list;
    result.selected=!found->list&&found->option>=0&&found->value>=0&&values[size_t(found->value)].value==found->option;
    return result;
  }
  size_t storageBytes() const {return bindings_.capacity()*sizeof(Binding)+nodes_.capacity()*sizeof(Node);}
 private:
  struct Node {Id id;uint16_t index;};
  struct Binding {Id id;int value,panel,option;bool select,list;};
  const PageDesc *page_=nullptr;
  const ComponentDesc *components_=nullptr;
  size_t count_=0;
  std::vector<Node> nodes_;
  std::vector<Binding> bindings_;
  int find(Id id) const {
    if(!id)return -1;
    const auto found=std::lower_bound(nodes_.begin(),nodes_.end(),id,[](const Node &a,Id b){return a.id<b;});
    return found!=nodes_.end()&&found->id==id?int(found->index):-1;
  }
};

} } }
