bool View::configureBehaviors(const BehaviorDesc *descriptions,size_t count) {
  auto &s=*impl_;if(!s.project||s.processing||s.refresh->busy()||count>128||(count&&!descriptions))return false;
  std::vector<Impl::BehaviorState> prepared;prepared.reserve(count);
  for(size_t n=0;n<count;++n){const auto &d=descriptions[n];
    if(!d.id||uint8_t(d.kind)>uint8_t(BehaviorKind::Timer)||uint8_t(d.motion)>uint8_t(DialogMotion::Circular)||
       !d.intervalMs||d.intervalMs>60000||d.durationMs>3600000||d.motionMs<40||d.motionMs>2000||(d.flags&~31u))return false;
    Impl::BehaviorState entry;entry.desc=d;bool found=false;
    for(uint16_t p=0;p<s.project->count;++p){const auto &page=s.project->pages[p];
      for(uint16_t i=0;i<page.count;++i)if(page.nodes[i].id==d.id){entry.page=p;found=true;
        for(Id wanted:{d.target,d.initialFocus})if(wanted){bool valid=false;for(uint16_t k=0;k<page.count;++k)if(page.nodes[k].id==wanted){valid=true;break;}if(!valid)return false;}
      }
    }
    if(!found||(d.kind==BehaviorKind::Editor&&!d.target))return false;
    prepared.push_back(std::move(entry));
  }
  std::sort(prepared.begin(),prepared.end(),[](const Impl::BehaviorState &a,const Impl::BehaviorState &b){return a.desc.id<b.desc.id;});
  for(size_t n=1;n<prepared.size();++n)if(prepared[n-1].desc.id==prepared[n].desc.id)return false;
  s.behaviorVisuals.reserve(count);s.behaviors.swap(prepared);s.behaviorOwner=this;s.behaviorClockReady=false;s.behaviorPage=NoParent;return true;
}
bool View::startBehavior(Id id){auto &s=*impl_;auto *b=s.behavior(id);if(!b)return false;const bool nested=s.processing;s.processing=true;const bool result=s.startBehaviorState(*b);if(!nested)s.endProcessing();return result;}
bool View::pauseBehavior(Id id,bool paused){auto &s=*impl_;auto *b=s.behavior(id);if(!b||!b->running)return false;b->paused=paused;b->pending=!paused;++b->revision;return true;}
bool View::finishBehavior(Id id,bool commit){auto &s=*impl_;auto *b=s.behavior(id);if(!b||b->page!=s.history.back())return false;const bool nested=s.processing;s.processing=true;const bool result=s.finishBehaviorState(*b,commit);if(!nested)s.endProcessing();return result;}
bool View::behaviorRunning(Id id)const{auto &s=*impl_;auto *b=s.behavior(id);return b&&b->page==s.history.back()&&b->running&&!b->paused;}
bool View::setNavigationLimit(uint16_t depth){if(!depth||depth>64||impl_->processing)return false;impl_->navigationLimit=depth;return true;}
