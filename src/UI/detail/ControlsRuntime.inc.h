// Members of View::Impl. Configuration is sparse and shared across pages.
std::vector<ControlDesc> controlDescriptions;
int32_t controlStartUpper=0,editUpper=0;

const ControlDesc *controlDescription(Id id) const {
  const auto at=std::lower_bound(controlDescriptions.begin(),controlDescriptions.end(),id,
    [](const ControlDesc &desc,Id value){return desc.id<value;});

  return at!=controlDescriptions.end()&&at->id==id?&*at:nullptr;
}

bool hardwareControl(int index) const {
  if(index<0)return false;

  const auto *desc=controlDescription(page().nodes[index].id);

  return desc&&desc->kind!=ControlKind::QrCode;
}

bool controlAxis(int index,bool horizontal) const {
  const auto kind=controlDescription(page().nodes[index].id)->kind;

  return kind==ControlKind::Knob||horizontal==(kind==ControlKind::RangeSlider);
}

ControlState &writableControl(int index) {
  auto &value=state()[index].control;

  if(value.use_count()!=1)value=std::make_shared<ControlState>(*value);

  return *value;
}

bool encodeControlQr(const ControlDesc &desc,const NodeDesc &node,const char *text,std::vector<uint8_t> &output) const {
  if(!text||strlen(text)>512)return false;

  const size_t capacity=qrcodegen_BUFFER_LEN_FOR_VERSION(desc.maxVersion);

  std::vector<uint8_t> data(capacity),encoded(capacity);

  const size_t bytes=strlen(text);

  if(bytes>capacity)return false;

  memcpy(data.data(),text,bytes);

  if(!qrcodegen_encodeBinary(data.data(),bytes,encoded.data(),qrcodegen_Ecc(desc.ecc),1,desc.maxVersion,qrcodegen_Mask_AUTO,false))return false;

  const Rect content=contentBox(node.rect,node.style);

  if(std::min(content.width,content.height)<qrcodegen_getSize(encoded.data())+8)return false;

  output.swap(encoded);

  return true;
}

bool initializeControl(const NodeDesc &node,State &value) {
  const auto *desc=controlDescription(node.id);

  if(!desc)return true;

  auto prepared=std::make_shared<ControlState>();
  prepared->upper=desc->upper;

  if(desc->kind==ControlKind::QrCode&&!encodeControlQr(*desc,node,value.text.c_str(),prepared->qr))return false;

  value.control=std::move(prepared);

  return true;
}

bool setControlText(int index,const char *text) {
  const auto *desc=controlDescription(page().nodes[index].id);

  if(!desc||desc->kind!=ControlKind::QrCode)return false;

  auto prepared=std::make_shared<ControlState>();

  if(!encodeControlQr(*desc,page().nodes[index],text,prepared->qr))return false;

  state()[index].text.set(text);
  state()[index].control=std::move(prepared);
  invalidate(index);

  return true;
}

bool setControlValue(int index,int32_t value) {
  const auto *desc=controlDescription(page().nodes[index].id);

  if(!desc||desc->kind==ControlKind::QrCode)return false;

  const auto &node=page().nodes[index];

  if(desc->kind==ControlKind::DatePicker&&!controls::date(value))return false;

  value=desc->kind==ControlKind::DatePicker?controls::clamp(value,node.minimum,node.maximum):controls::snap(node,value);

  if(desc->kind==ControlKind::RangeSlider)value=std::min(value,state()[index].control->upper);
  if(state()[index].value!=value){state()[index].value=value;invalidate(index);}

  return true;
}

bool setControlRange(int index,int32_t lower,int32_t upper) {
  const auto *desc=index<0?nullptr:controlDescription(page().nodes[index].id);

  if(!desc||desc->kind!=ControlKind::RangeSlider)return false;

  const auto &node=page().nodes[index];

  if(lower<node.minimum||upper>node.maximum||lower>upper||
     (int64_t(lower)-node.minimum)%node.step||(int64_t(upper)-node.minimum)%node.step)return false;

  state()[index].value=lower;
  writableControl(index).upper=upper;
  invalidate(index);

  return true;
}

bool editChanged(int index) const {
  return state()[index].value!=editValue||(state()[index].control&&state()[index].control->upper!=editUpper);
}

void restoreEdit(int index) {
  if(state()[index].control)writableControl(index).upper=editUpper;

  setValue(index,editValue);
}

bool controlKey(int index,InputAction action) {
  if(!hardwareControl(index))return false;

  const auto &desc=*controlDescription(page().nodes[index].id);
  const bool confirm=action==InputAction::Confirm||action==InputAction::Activate;

  if(confirm) {
    if(editNode==index){editNode=-1;if(editChanged(index))dispatch(index,EventType::Change);}
    else {editNode=index;editValue=state()[index].value;editUpper=state()[index].control->upper;}

    invalidate(index);

    return true;
  }

  const bool explicitStep=action==InputAction::Increment||action==InputAction::Decrement;

  if(editNode!=index&&!explicitStep)return false;

  auto &value=writableControl(index);

  if((action==InputAction::Left||action==InputAction::Right)&&controls::columns(desc)>1) {
    value.part=uint8_t(controls::clamp(int(value.part)+(action==InputAction::Left?-1:1),0,controls::columns(desc)-1));

    invalidate(index);

    return true;
  }

  const bool picker=desc.kind==ControlKind::Roller||desc.kind==ControlKind::DatePicker||desc.kind==ControlKind::TimePicker;

  const int delta=action==InputAction::Next||action==InputAction::Increment||action==InputAction::Right?1:
    action==InputAction::Previous||action==InputAction::Decrement||action==InputAction::Left?-1:
    action==InputAction::Up?(picker?-1:1):action==InputAction::Down?(picker?1:-1):0;

  if(!delta)return false;

  const int32_t before=state()[index].value,upper=value.upper;
  const int32_t next=controls::shift(desc,page().nodes[index],value.part&&desc.kind==ControlKind::RangeSlider?upper:before,value.part,delta);

  if(desc.kind==ControlKind::RangeSlider&&value.part)value.upper=std::max(before,next);
  else setValue(index,next);

  if(before!=state()[index].value||upper!=state()[index].control->upper){invalidate(index);dispatch(index,EventType::Input);if(editNode!=index)dispatch(index,EventType::Change);}

  return true;
}

int32_t controlPoint(int index) const {
  const auto &node=page().nodes[index];
  const auto &desc=*controlDescription(node.id);
  const Rect rect=contentBox(poseRect(page(),state(),index),styleFor(page(),state(),index));
  const auto local=inputPoint(index,x,y);
  const double localX=local.x,localY=local.y;
  double fraction=0;

  if(desc.kind==ControlKind::Knob) {
    const double dx=localX-rect.x-rect.width/2.0,dy=localY-rect.y-rect.height/2.0;
    const double inner=std::max(0,std::min(rect.width,rect.height)/2-desc.thickness-8);

    if(dx*dx+dy*dy<inner*inner)return state()[index].value;

    double offset=fmod(atan2(localY-rect.y-rect.height/2.0,localX-rect.x-rect.width/2.0)*180.0/3.141592653589793-desc.start+720.0,360.0);

    fraction=offset<=desc.sweep?offset/desc.sweep:offset-desc.sweep<360-offset?1:0;
  } else {
    const int inset=std::min(10,rect.width/2);

    fraction=double(localX-rect.x-inset)/std::max(1,rect.width-2*inset);
  }

  fraction=std::max(0.0,std::min(1.0,fraction));
  const int64_t steps=int64_t(floor(fraction*(int64_t(node.maximum)-node.minimum)/node.step+0.5));

  return controls::snap(node,int64_t(node.minimum)+steps*node.step);
}

void beginControl(int index) {
  auto &value=writableControl(index);
  const auto &desc=*controlDescription(page().nodes[index].id);
  const Rect rect=contentBox(poseRect(page(),state(),index),styleFor(page(),state(),index));

  controlStartUpper=value.upper;

  if(desc.kind==ControlKind::RangeSlider) {
    const int32_t point=controlPoint(index);

    value.part=llabs(int64_t(point)-value.upper)<llabs(int64_t(point)-state()[index].value)||
      (value.upper==state()[index].value&&point>=value.upper)?1:0;
  } else value.part=uint8_t(controls::clamp(int64_t(inputPoint(index,x,y).x-rect.x)*controls::columns(desc)/std::max(1,rect.width),0,controls::columns(desc)-1));

  invalidate(index);
}

void moveControl(int index) {
  const auto &desc=*controlDescription(page().nodes[index].id);
  const auto &node=page().nodes[index];
  auto &value=writableControl(index);
  const int32_t before=state()[index].value,upper=value.upper;
  int32_t next;

  if(desc.kind==ControlKind::Knob||desc.kind==ControlKind::RangeSlider)next=controlPoint(index);
  else {
    const Rect rect=contentBox(poseRect(page(),state(),index),styleFor(page(),state(),index));
    const int row=std::max(1,rect.height/desc.rows);
    const auto local=inputPoint(index,x,y),start=inputPoint(index,startX,startY);
    const int delta=moved?int(floor((start.y-local.y)/row+0.5)):int(floor((local.y-rect.y-rect.height/2)/row+0.5));

    next=controls::shift(desc,node,startValue,value.part,delta);
  }

  if(desc.kind==ControlKind::RangeSlider&&value.part)value.upper=std::max(before,next);
  else setValue(index,next);

  if(before!=state()[index].value||upper!=state()[index].control->upper){invalidate(index);dispatch(index,EventType::Input);}
}

void paintControl(const ControlDesc &desc,const NodeDesc &node,const State &value,const Style &style,Rect rect,bool disabled) {
  const auto &control=*value.control;

  if(desc.kind==ControlKind::QrCode) {
    if(control.qr.empty())return;

    const int modules=qrcodegen_getSize(control.qr.data()),scale=std::min(rect.width,rect.height)/(modules+8);

    if(scale<1)return;

    const int side=(modules+8)*scale,left=rect.x+(rect.width-side)/2,top=rect.y+(rect.height-side)/2;

    for(int row=0;row<modules;++row)for(int column=0;column<modules;++column)
      if(qrcodegen_getModule(control.qr.data(),column,row))canvas.fillRect(left+(column+4)*scale,top+(row+4)*scale,scale,scale,style.color);

    return;
  }

  if(!style.font)return;

  canvas.setFont(style.font);canvas.setTextSize(style.fontSize);canvas.setTextDatum(TL_DATUM);canvas.setTextPadding(0);

  const int line=style.font->height*style.fontSize;

  auto label=[&](const char *text,int left,int top,int width,uint16_t color){
    const int pixels=canvas.textWidth(text);

    if(pixels>width||top<rect.y||top+line>rect.y+rect.height)return;

    canvas.setTextColor(color);canvas.drawString(text,left+(width-pixels)/2,top);
  };

  if(desc.kind==ControlKind::Knob) {
    const int cx=rect.x+rect.width/2,cy=rect.y+rect.height/2,radius=std::max(1,std::min(rect.width,rect.height)/2-3);
    const int progress=int((int64_t(value.value)-node.minimum)*desc.sweep/(int64_t(node.maximum)-node.minimum));

    instrumentArc(canvas,cx,cy,radius,desc.thickness,desc.start,desc.sweep,style.trackColor);

    if(progress)instrumentArc(canvas,cx,cy,radius,desc.thickness,desc.start,progress,style.fillColor);

    const int mid=std::max(1,radius-desc.thickness/2);

    canvas.fillCircle(instrumentX(cx,mid,desc.start+progress),instrumentY(cy,mid,desc.start+progress),3,style.thumbColor);
    char text[24];snprintf(text,sizeof(text),"%ld",long(value.value));label(text,rect.x,cy-line/2,rect.width,style.color);

    return;
  }

  if(desc.kind==ControlKind::RangeSlider) {
    const int inset=std::min(10,rect.width/2),length=std::max(1,rect.width-2*inset),cy=rect.y+rect.height/2;
    const int lower=rect.x+inset+int((int64_t(value.value)-node.minimum)*length/(int64_t(node.maximum)-node.minimum));
    const int upper=rect.x+inset+int((int64_t(control.upper)-node.minimum)*length/(int64_t(node.maximum)-node.minimum));

    canvas.fillRoundRect(rect.x+inset,cy-2,length,4,2,style.trackColor);

    if(upper>lower)canvas.fillRoundRect(lower,cy-2,upper-lower,4,2,style.fillColor);

    const int radius=std::min(8,std::max(1,rect.height/2));

    canvas.fillCircle(lower,cy,radius,style.thumbColor);canvas.fillCircle(upper,cy,radius,style.thumbColor);
    canvas.drawCircle(control.part?upper:lower,cy,radius,style.color);

    return;
  }

  const int columns=controls::columns(desc),rowHeight=std::max(1,rect.height/desc.rows),center=rect.y+(rect.height-rowHeight)/2;

  canvas.fillRoundRect(rect.x,center,rect.width,rowHeight,style.radius,style.fillColor);

  if(columns>1&&!disabled)canvas.drawRoundRect(rect.x+int(control.part)*rect.width/columns,center,std::max(1,rect.width/columns),rowHeight,style.radius,style.thumbColor);

  for(int column=0;column<columns;++column)for(int offset=-desc.rows/2;offset<=desc.rows/2;++offset) {
    char buffer[24];const char *text=buffer;

    if(desc.kind==ControlKind::Roller) {
      int option=value.value+offset;

      if(desc.flags&1)option=(option%desc.optionCount+desc.optionCount)%desc.optionCount;
      if(option<0||option>=desc.optionCount)continue;

      text=desc.options[option];
    } else {
      const bool date=desc.kind==ControlKind::DatePicker;
      const int fields[]={date?value.value/10000:value.value/3600,date?value.value/100%100:value.value/60%60,date?value.value%100:value.value%60};
      const int part=fields[column]+offset,minimum=date?1:0,maximum=date?(column==0?9999:column==1?12:controls::days(fields[0],fields[1])):column==0?23:59;

      if(part<minimum||part>maximum)continue;

      snprintf(buffer,sizeof(buffer),date&&column==0?"%04d":"%02d",part);
    }

    label(text,rect.x+column*rect.width/columns,center+offset*rowHeight+(rowHeight-line)/2,rect.width/columns,offset?style.trackColor:style.color);
  }
}
