bool View::configureControls(const ControlDesc *descriptions,size_t count) {
  auto &s=*impl_;

  if(!s.project||s.processing||s.refresh->busy()||count>128||(count&&!descriptions))return false;

  std::vector<ControlDesc> prepared;

  if(count)prepared.assign(descriptions,descriptions+count);

  std::sort(prepared.begin(),prepared.end(),[](const ControlDesc &a,const ControlDesc &b){return a.id<b.id;});

  for(size_t at=0;at<count;++at) {
    const auto &desc=prepared[at];

    if(!desc.id||(at&&prepared[at-1].id==desc.id)||uint8_t(desc.kind)>uint8_t(ControlKind::QrCode)||
       (desc.rows!=3&&desc.rows!=5)||(desc.flags&~3u)||desc.optionCount>128||
       desc.start < -360||desc.start>360||!desc.sweep||desc.sweep>360||!desc.thickness||desc.thickness>64||
       desc.ecc>3||!desc.maxVersion||desc.maxVersion>10)return false;

    const NodeDesc *node=nullptr;

    for(uint16_t page=0;page<s.project->count;++page)for(uint16_t i=0;i<s.project->pages[page].count;++i)
      if(s.project->pages[page].nodes[i].id==desc.id)node=&s.project->pages[page].nodes[i];

    if(!node||node->kind!=Kind::Container||node->step<=0||node->maximum<node->minimum||
       node->value<node->minimum||node->value>node->maximum)return false;

    if(desc.kind!=ControlKind::QrCode&&!node->style.font)return false;

    if(desc.kind==ControlKind::Roller) {
      if(!desc.optionCount||!desc.options||node->minimum!=0||node->maximum!=desc.optionCount-1)return false;
      for(size_t option=0;option<desc.optionCount;++option)if(!desc.options[option]||!*desc.options[option]||strlen(desc.options[option])>128)return false;
    } else if(desc.optionCount||desc.options)return false;

    if(desc.kind==ControlKind::DatePicker&&(!controls::date(node->minimum)||!controls::date(node->maximum)||!controls::date(node->value)))return false;
    if(desc.kind==ControlKind::TimePicker&&(node->minimum!=0||node->maximum!=86399||node->step!=((desc.flags&2)?1:60)))return false;

    if(desc.kind==ControlKind::RangeSlider&&(node->maximum==node->minimum||desc.upper<node->value||desc.upper>node->maximum||
       (int64_t(desc.upper)-node->minimum)%node->step))return false;

    if(desc.kind==ControlKind::Knob&&node->maximum==node->minimum)return false;
    if(desc.kind==ControlKind::QrCode){std::vector<uint8_t> qr;if(!s.encodeControlQr(desc,*node,node->text,qr))return false;}
  }

  s.cancel();s.controlDescriptions.swap(prepared);

  for(uint16_t page=0;page<s.project->count;++page)for(size_t i=0;i<s.pages[page].size();++i) {
    s.pages[page][i].control.reset();

    if(!s.initializeControl(s.project->pages[page].nodes[i],s.pages[page][i]))return false;
  }

  ++s.inputLayoutEpoch;s.refresh->invalidateAll();

  return true;
}

bool View::setRange(Id id,int32_t lower,int32_t upper) {
  return impl_->setControlRange(impl_->find(id),lower,upper);
}

int32_t View::upperValue(Id id) const {
  const int index=impl_->find(id);

  return index>=0&&impl_->state()[index].control?impl_->state()[index].control->upper:0;
}
