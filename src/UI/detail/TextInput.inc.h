// Included inside View::Impl. Shared session and immutable keyboard snapshots;
// keyboard rendering uses the existing tile canvas, never a full framebuffer.
  // Borrowed, synchronous text access keeps validation and ordinary field
  // tiles allocation-free. The owning State/SoftInput snapshot remains alive
  // throughout each call; no borrowed pointer is retained by this view.
  struct InputTextView {
    const char *data;size_t length;
    InputTextView(const char *text):data(text?text:""),length(strlen(data)) {}
    InputTextView(const std::string &text):data(text.c_str()),length(text.size()) {}
    const char *c_str() const {return data;}
    const char *begin() const {return data;}
    const char *end() const {return data+length;}
    size_t size() const {return length;}
    bool empty() const {return !length;}
    char operator[](size_t i) const {return data[i];}
    size_t find(char value,size_t from=0) const {if(from>=length)return SIZE_MAX;const char *found=static_cast<const char *>(memchr(data+from,value,length-from));return found?size_t(found-data):SIZE_MAX;}
    size_t find_first_of(const char *values) const {for(size_t i=0;i<length;++i)if(strchr(values,data[i]))return i;return SIZE_MAX;}
  };
  static size_t inputCharacters(InputTextView s,size_t end=SIZE_MAX) {
    size_t n=0;end=std::min(end,s.size());for(size_t i=0;i<end;++i)if((static_cast<unsigned char>(s[i])&0xc0)!=0x80)++n;return n;
  }
  static size_t inputPrevious(InputTextView s,size_t p) {
    if(!p)return 0;--p;while(p&&(static_cast<unsigned char>(s[p])&0xc0)==0x80)--p;return p;
  }
  static size_t inputFollowing(InputTextView s,size_t p) {
    if(p>=s.size())return s.size();++p;while(p<s.size()&&(static_cast<unsigned char>(s[p])&0xc0)==0x80)++p;return p;
  }
  static bool inputUtf8(InputTextView s) {
    for(size_t i=0;i<s.size();){const unsigned char c=s[i++];if(c<32||c==127)return false;
      if(c<128)continue;int bytes=c>=0xe0?2:c>=0xc2?1:-1;
      if(bytes<0||c>=0xf0||i+bytes>s.size())return false;
      const unsigned char first=s[i];if((c==0xe0&&first<0xa0)||(c==0xed&&first>=0xa0)||(c==0xf0&&first<0x90)||(c==0xf4&&first>=0x90))return false;
      while(bytes--)if((static_cast<unsigned char>(s[i++])&0xc0)!=0x80)return false;
    }return true;
  }
  static InputFieldDesc spinInputConfig(const NodeDesc &n) {
    InputFieldDesc d{};d.type=InputFieldType::Number;d.label=n.text;d.unit=n.instrument?n.instrument->unit:"";d.maxBytes=64;d.maxLength=32;d.decimals=n.instrument?n.instrument->decimals:0;d.min=n.minimum;d.max=n.maximum;d.hasMin=d.hasMax=d.required=true;d.enterAction=InputEnterAction::Auto;return d;
  }
  const InputFieldDesc *inputDescriptor(int i) const {if(i<0||i>=int(page().count))return nullptr;if(page().nodes[i].kind==Kind::InputField||page().nodes[i].kind==Kind::Spinbox){if(page().nodes[i].inputField)return page().nodes[i].inputField;}return state()[i].implicitInput.get();}
  bool inputAvailable(int i) const {const auto *d=inputDescriptor(i);return d&&!d->readOnly&&visible(page(),state(),i,true)&&opacity(page(),state(),i)>0;}
  int nextInput(int i) const {
    std::vector<int> fields;for(uint16_t n=0;n<page().count;++n)if(inputAvailable(n)&&focusable(n,false))fields.push_back(n);
    std::stable_sort(fields.begin(),fields.end(),[&](int a,int b){const auto *fa=focusDesc(a),*fb=focusDesc(b);return (fa?fa->order:0)<(fb?fb->order:0);});
    const auto found=std::find(fields.begin(),fields.end(),i);return found!=fields.end()&&found+1!=fields.end()?*(found+1):-1;
  }
  void releaseKeyboardPointer() {
    invalidateKeyboardPressed();
    keyboardPointer=keyboardFieldPointer=keyboardSpacePointer=keyboardTrackpad=keyboardPickerPointer=keyboardBackspaceRepeat=keyboardBackspacePaused=false;keyboardPressed.clear();
  }
  bool softBegin(int i) {
    if(!inputAvailable(i)||transition.active||groupMotion.active)return false;
    releaseKeyboardPointer();
    if(softInput.node==i){softInput.visible=true;softInput.error.clear();buildKeyboard(softInput);refresh->invalidateAll();return true;}
    if(softInput.node>=0)softCancel();
    softInput=SoftInput{};softInput.node=i;softInput.descriptor=*inputDescriptor(i);softInput.theme=keyboardThemeOverride<0?softInput.descriptor.keyboardTheme:KeyboardTheme(keyboardThemeOverride);softInput.visible=true;softInput.draft=page().nodes[i].kind==Kind::Spinbox?inputNumberText(state()[i].value,softInput.descriptor.decimals):std::string(state()[i].text.c_str());softInput.cursor=softInput.draft.size();
    setFocus(i);inertia.active=animation.active=false;buildKeyboard(softInput);++inputEpoch;refresh->invalidateAll();return true;
  }
  bool inputDraftAllowed(InputTextView s,const InputFieldDesc &d) const {
    if(s.size()>(d.maxBytes?d.maxBytes:256)||inputCharacters(s)>(d.maxLength?d.maxLength:64)||!inputUtf8(s))return false;
    if(d.type==InputFieldType::Text||d.type==InputFieldType::Password)return true;
    if(d.type==InputFieldType::Tel){for(char c:s)if((c<'0'||c>'9')&&c!='+'&&c!='*'&&c!='#'&&c!=' '&&c!='-'&&c!='.'&&c!='('&&c!=')')return false;return true;}
    if(d.type==InputFieldType::Pin){for(char c:s)if(c<'0'||c>'9')return false;return s.size()<=(d.pinLength?d.pinLength:4);}
    bool dot=false;unsigned fraction=0;for(size_t i=0;i<s.size();++i){const char c=s[i];
      if(c=='-'&&i==0&&(!d.hasMin||d.min<0))continue;
      if(c=='.'&&d.decimals&&!dot){dot=true;continue;}
      if(c<'0'||c>'9')return false;if(dot&&++fraction>d.decimals)return false;
    }return true;
  }
  bool parseInputNumber(InputTextView s,uint8_t decimals,int32_t &result) const {
    if(s.empty())return false;size_t p=0;bool negative=false;if(s[p]=='-'){negative=true;if(++p==s.size())return false;}
    int64_t number=0;unsigned fraction=0,digits=0;bool dot=false;
    for(;p<s.size();++p){char c=s[p];if(c=='.'&&!dot&&digits){dot=true;continue;}if(c<'0'||c>'9')return false;
      if(dot&&++fraction>decimals)return false;if(number>(int64_t(INT32_MAX)+1)/10)return false;number=number*10+c-'0';++digits;
    }
    if(!digits)return false;for(;fraction<decimals;++fraction){if(number>(int64_t(INT32_MAX)+1)/10)return false;number*=10;}
    if(negative)number=-number;if(number<INT32_MIN||number>INT32_MAX)return false;result=int32_t(number);return true;
  }
  static std::string inputNumberText(int32_t value,uint8_t decimals) {
    char out[40];int64_t scale=1;for(uint8_t i=0;i<decimals;++i)scale*=10;
    const int64_t absolute=value<0?-int64_t(value):int64_t(value);
    if(decimals)snprintf(out,sizeof(out),"%s%ld.%0*ld",value<0?"-":"",long(absolute/scale),int(decimals),long(absolute%scale));
    else snprintf(out,sizeof(out),"%ld",long(value));return out;
  }
  const char *inputValidationError(InputTextView draft,const InputFieldDesc &d,bool require,int32_t &numeric,const NodeDesc *node=nullptr) const {
    if(require&&d.required&&draft.empty())return "必填项";
    if(!inputDraftAllowed(draft,d))return "输入无效";
    if(d.type==InputFieldType::Pin&&!draft.empty()&&draft.size()!=(d.pinLength?d.pinLength:4))return "PIN长度错";
    if(d.type==InputFieldType::Tel&&!draft.empty()&&(draft.find_first_of("0123456789*#")==std::string::npos||draft.find('+',1)!=std::string::npos))return "电话号码无效";
    if(d.type==InputFieldType::Number){
      if(draft.empty()){numeric=0;return "";}
      if(!parseInputNumber(draft,d.decimals,numeric))return "数字不完整";
      if(d.hasMin&&numeric<d.min)return "数值过小";
      if(d.hasMax&&numeric>d.max)return "数值过大";
      if(node&&node->kind==Kind::Spinbox&&node->step>0&&(int64_t(numeric)-node->minimum)%node->step)return "不符合步长";
    }return "";
  }
  bool setInputText(int i,const char *text) {
    const auto *d=inputDescriptor(i);if(!d)return false;text=text?text:"";
    if(strlen(text)>(d->maxBytes?d->maxBytes:256))return false;
    int32_t numeric=state()[i].value;if(*inputValidationError(text,*d,false,numeric))return false;
    const bool changed=state()[i].text.set(text)||(d->type==InputFieldType::Number&&state()[i].value!=numeric);
    if(d->type==InputFieldType::Number)state()[i].value=numeric;if(changed)invalidate(i);return true;
  }
  bool softInsert(const char *text) {
    if(softInput.node<0||!text||!inputAvailable(softInput.node))return false;
    const auto &d=*inputDescriptor(softInput.node);if(strlen(text)>(d.maxBytes?d.maxBytes:256)){updateInputError("输入无效");return false;}
    const std::string insertion(text);std::string next=softInput.draft;next.insert(softInput.cursor,insertion);
    if(!inputDraftAllowed(next,*inputDescriptor(softInput.node))){updateInputError("输入无效");return false;}
    if(next==softInput.draft)return true;softInput.draft.swap(next);softInput.cursor+=insertion.size();updateInputError("");
    softInput.group=-1;dispatch(softInput.node,EventType::Input);if(buildKeyboard(softInput))invalidateKeyboardKeys();invalidateKeyboardField();return true;
  }
  void invalidateKeyboardCursor(size_t previous) {
    const auto &k=softInput;if(!k.visible)return;const Style style=keyboardDraftStyle(k);const Rect field=keyboardTextRect(k);
    if(!style.font||field.width<=0){invalidateKeyboardField();return;}
    const bool masked=keyboardMasked(k);const int32_t reserve=inputGlyphAdvance(style,'n',true);
    // Match prepareKeyboardText's bounded visible window. A changed window
    // moves every glyph; an unchanged one changes only the two caret regions.
    const auto startAt=[&](size_t cursor){size_t start=std::min(cursor,k.draft.size());int32_t pixels=0;
      while(start){const size_t before=inputPrevious(k.draft,start);size_t next=before;const uint16_t code=masked?'*':nextCode(k.draft.c_str(),next,start);const int32_t advance=inputGlyphAdvance(style,code,true);if(pixels+advance+reserve>field.width)break;pixels+=advance;start=before;}return start;};
    const size_t oldStart=startAt(previous),newStart=startAt(k.cursor);if(oldStart!=newStart){invalidateKeyboardField();return;}
    const int32_t height=style.font->height*style.fontSize,y=field.y+std::max<int32_t>(0,(field.height-height)/2);
    // Pending dirty regions are retained behind immutable accepted DMA frames.
    // Edit/theme/font/layout changes already invalidate the complete field or
    // keyboard; cursor invalidation adds to them and never replaces them.
    for(size_t cursor:{previous,k.cursor}){int32_t x=field.x;for(size_t i=newStart;i<cursor;){const uint16_t code=nextCode(k.draft.c_str(),i,cursor);x+=inputGlyphAdvance(style,masked?'*':code,true);}if(x<field.x+field.width){const Rect area=intersect(k.fieldRect,{x,y,std::max<int32_t>(1,style.fontSize),height});if(area.width>0&&area.height>0)refresh->invalidate(DirtyRect(area.x,area.y,area.width,area.height));}}
  }
  bool softMove(int32_t count) {
    if(softInput.node<0)return false;count=clamp32(count,-512,512);const size_t previous=softInput.cursor;
    while(count<0){softInput.cursor=inputPrevious(softInput.draft,softInput.cursor);++count;}
    while(count>0){softInput.cursor=inputFollowing(softInput.draft,softInput.cursor);--count;}
    if(previous!=softInput.cursor)invalidateKeyboardCursor(previous);return true;
  }
  bool softBackspace() {
    if(softInput.node<0||!inputAvailable(softInput.node))return false;if(!softInput.cursor)return true;
    size_t previous=inputPrevious(softInput.draft,softInput.cursor);softInput.draft.erase(previous,softInput.cursor-previous);softInput.cursor=previous;updateInputError("");
    dispatch(softInput.node,EventType::Input);invalidateKeyboardField();return true;
  }
  bool keyboardBackspaceHeld() const {
    return keyboardBackspaceRepeat&&keyboardPointer&&keyboardPressed=="backspace"&&softInput.visible&&inputEpoch==keyboardBackspaceEpoch&&softInput.cursor&&inputAvailable(softInput.node);
  }
  void beginKeyboardBackspace(uint32_t now) {
    keyboardBackspaceRepeat=true;keyboardBackspacePaused=false;keyboardBackspaceStarted=keyboardBackspaceAt=now;keyboardBackspaceDelay=400;keyboardBackspaceEpoch=inputEpoch;
    // Delete on contact; release never deletes a second time. Input callbacks
    // may replace/hide the session, so retain no key/draft references here.
    softBackspace();if(!keyboardBackspaceHeld())keyboardBackspaceRepeat=false;
  }
  void repeatKeyboardBackspace(uint32_t now) {
    if(!keyboardBackspaceRepeat)return;
    // Configuration changes can defer rebuilding until the next frame. Check
    // them before deleting, rather than accepting one repeat in an old layout.
    if(buildKeyboard(softInput))invalidateKeyboardKeys();
    if(!keyboardBackspaceHeld()){keyboardBackspaceRepeat=false;return;}
    if(keyboardBackspacePaused)return;
    if(uint32_t(now-keyboardBackspaceAt)<keyboardBackspaceDelay)return;
    // One character per pump, with unsigned elapsed time for millis rollover.
    // Rebase after a slow frame: never catch up with a burst of deletions.
    keyboardBackspaceAt=now;keyboardBackspaceDelay=uint32_t(now-keyboardBackspaceStarted)>=2000?50:80;
    processing=true;softBackspace();endProcessing();if(!keyboardBackspaceHeld())keyboardBackspaceRepeat=false;
  }
  bool softDelete() {
    if(softInput.node<0||!inputAvailable(softInput.node))return false;if(softInput.cursor>=softInput.draft.size())return true;
    const size_t end=inputFollowing(softInput.draft,softInput.cursor);softInput.draft.erase(softInput.cursor,end-softInput.cursor);updateInputError("");dispatch(softInput.node,EventType::Input);invalidateKeyboardField();return true;
  }
  bool softClear() {
    if(softInput.node<0||!inputAvailable(softInput.node))return false;if(softInput.draft.empty()){updateInputError("");return true;}
    softInput.draft.clear();softInput.cursor=0;updateInputError("");dispatch(softInput.node,EventType::Input);invalidateKeyboardField();return true;
  }
  bool softSign() {
    if(softInput.node<0||!inputAvailable(softInput.node))return false;const auto &d=*inputDescriptor(softInput.node);if(d.type!=InputFieldType::Number||(d.hasMin&&d.min>=0))return false;
    std::string next=softInput.draft;const bool remove=!next.empty()&&next[0]=='-';if(remove)next.erase(0,1);else next.insert(0,1,'-');
    if(!inputDraftAllowed(next,d)){updateInputError("输入无效");return false;}
    softInput.draft.swap(next);softInput.cursor=remove?(softInput.cursor?softInput.cursor-1:0):softInput.cursor+1;updateInputError("");dispatch(softInput.node,EventType::Input);invalidateKeyboardField();return true;
  }
  void invalidateKeyboardField() {
    if(!softInput.visible)return;const Rect r=softInput.fieldRect;refresh->invalidate(DirtyRect(r.x,r.y,r.width,r.height));
  }
  void updateInputError(const char *message) {
    message=message?message:"";if(softInput.error==message)return;softInput.error=message;
    if(softInput.visible){const Rect r=softInput.labelRect;if(r.width>0&&r.height>0)refresh->invalidate(DirtyRect(r.x,r.y,r.width,r.height));else invalidateKeyboardField();}
  }
  void invalidateKeyboardKeys() {
    if(!softInput.visible)return;const Rect r=softInput.rect;const int32_t top=softInput.picker?r.y:softInput.fieldRect.y+softInput.fieldRect.height;refresh->invalidate(DirtyRect(r.x,top,r.width,std::max<int32_t>(0,r.y+r.height-top)));
  }
  Rect keyboardBubble(const SoftInput &k,const SoftKey &key) const {
    const auto type=k.descriptor.type;if(k.picker||k.builtRound||k.rect.width<366||(type!=InputFieldType::Text&&type!=InputFieldType::Password)||key.action!="insert"||key.value==" "||inputCharacters(key.value)!=1)return {0,0,0,0};
    const int32_t width=std::max<int32_t>(44,std::min<int32_t>(64,key.rect.width+12));
    return {clamp32(key.rect.x+key.rect.width/2-width/2,k.rect.x+2,k.rect.x+k.rect.width-width-2),std::max<int32_t>(k.rect.y+2,key.rect.y-40),width,36};
  }
  void invalidateKeyboardKey(int index) {
    if(!softInput.visible||!softInput.keys||index<0||index>=int(softInput.keys->size()))return;const auto &key=(*softInput.keys)[index];const Rect r=key.rect;refresh->invalidate(DirtyRect(r.x,r.y,r.width,r.height));
    if(keyboardPressed==key.id||snapshotKeyboard.pressed==key.id){Rect bubble=keyboardBubble(softInput,key);if(bubble.width>0){bubble=intersect({bubble.x-1,bubble.y-1,bubble.width+2,bubble.height+2},softInput.rect);refresh->invalidate(DirtyRect(bubble.x,bubble.y,bubble.width,bubble.height));}}
  }
  void invalidateKeyboardPressed() {
    if(softInput.keys&&!keyboardPressed.empty())for(size_t i=0;i<softInput.keys->size();++i)if((*softInput.keys)[i].id==keyboardPressed){invalidateKeyboardKey(int(i));break;}
  }
  bool softHide() {if(softInput.node<0)return false;releaseKeyboardPointer();softInput.visible=false;refresh->invalidateAll();++inputEpoch;return true;}
  bool softCancel() {
    if(softInput.node<0)return false;releaseKeyboardPointer();const int i=softInput.node;softInput=SoftInput{};++inputEpoch;refresh->invalidateAll();dispatch(i,EventType::Cancel);return true;
  }
  void ensureInputAvailable() {
    if(softInput.node>=0&&!inputAvailable(softInput.node)){const bool nested=processing;processing=true;softCancel();if(!nested)endProcessing();}
  }
  bool softCommit(bool next) {
    const int i=softInput.node;if(i<0||!inputAvailable(i))return false;const auto &d=*inputDescriptor(i);int32_t numeric=state()[i].value;
    releaseKeyboardPointer();
    const std::string draft=softInput.draft;
    updateInputError(inputValidationError(draft,d,true,numeric,&page().nodes[i]));
    if(!softInput.error.empty()){const bool hidden=!softInput.visible;softInput.visible=true;if(hidden){buildKeyboard(softInput);refresh->invalidateAll();}return false;}
    const bool spin=page().nodes[i].kind==Kind::Spinbox;const int eligible=nextInput(i);const int following=(next||d.enterAction==InputEnterAction::Next||(d.enterAction==InputEnterAction::Auto&&eligible>=0))?eligible:-1;const bool changed=(!spin&&strcmp(state()[i].text.c_str(),draft.c_str())!=0)||state()[i].value!=numeric;
    if(!spin)state()[i].text.set(draft.c_str());if(d.type==InputFieldType::Number){if(spin)setValue(i,numeric);else state()[i].value=numeric;}
    softInput=SoftInput{};keyboardPressed.clear();++inputEpoch;invalidate(i);refresh->invalidateAll();
    if(changed)dispatch(i,EventType::Change);dispatch(i,EventType::Submit);
    if(following>=0&&!pendingNavigation)softBegin(following);return true;
  }
  Rect keyboardRow(int32_t y,int32_t height,int32_t width) const {
    int32_t left=(int32_t(page().width)-width)/2,right=left+width;
    if(circularKeyboard){const double radius=std::min(page().width,page().height)*0.5-4,cx=page().width*0.5,cy=page().height*0.5;
      const double distance=std::max(fabs(y-cy),fabs(y+height-cy));const double chord=distance<radius?sqrt(radius*radius-distance*distance):0;
      left=std::max(left,int32_t(ceil(cx-chord)));right=std::min(right,int32_t(floor(cx+chord)));
    }return {left+3,y,std::max<int32_t>(0,right-left-6),height};
  }
  bool buildKeyboard(SoftInput &k) {
    if(k.node<0||!k.visible)return false;const auto &d=*inputDescriptor(k.node);
    const bool same=k.keys&&k.builtNode==k.node&&k.builtMode==k.mode&&k.builtGroup==k.group&&k.builtShift==k.shift&&k.builtRound==circularKeyboard;
    if(same&&k.builtEpoch==inputLayoutEpoch)return false;
    const int following=k.builtNode==k.node&&k.builtEpoch==inputLayoutEpoch?k.builtNext:nextInput(k.node);
    if(same&&following==k.builtNext){k.builtEpoch=inputLayoutEpoch;return false;}
    // A different layout invalidates the contact, even if a key keeps its ID.
    if(&k==&softInput)releaseKeyboardPointer();
    const auto previousKeys=k.keys;const SoftKey *previousFocus=previousKeys&&k.keyFocus>=0&&k.keyFocus<int(previousKeys->size())?&(*previousKeys)[k.keyFocus]:nullptr;
    std::vector<SoftKey> layoutKeys;++stats.keyboardLayouts;
    const bool numeric=d.type==InputFieldType::Number||d.type==InputFieldType::Pin||d.type==InputFieldType::Tel;
    const int32_t width=std::min<int32_t>(page().width,640),height=circularKeyboard||!d.keyboardHeight?page().height:std::min<int32_t>(page().height,std::max<int32_t>(240,d.keyboardHeight));
    k.fullscreen=height==page().height;k.picker=page().width<120||height<200;
    const bool grouped=!numeric&&(k.picker||circularKeyboard||page().width<366);
    const int32_t top=k.fullscreen?0:int32_t(page().height)-height;k.rect={0,top,page().width,height};
    const int32_t margin=k.picker?(circularKeyboard?std::max<int32_t>(4,height/16):4):circularKeyboard?std::max<int32_t>(8,height/20):6;
    const int32_t titleHeight=k.picker?(height>=144&&width>=96?20:0):height>=360&&(!circularKeyboard||width>=480)?48:24;
    const int32_t draftHeight=k.picker?(height>=144?28:24):circularKeyboard&&height<480?clamp32(height/8,32,44):clamp32(height/6,44,64);
    int32_t titleY=top+margin;
    auto titleRow=[&](int32_t y){const int32_t glyphHeight=titleHeight>=48?32:16,padding=std::max<int32_t>(0,(titleHeight-glyphHeight)/2);Rect span=keyboardRow(y+padding,glyphHeight,width);span.y=y;span.height=titleHeight;return span;};
    k.labelRect=titleRow(titleY);k.fieldRect=keyboardRow(titleY+titleHeight,draftHeight,width);
    const bool secure=d.type==InputFieldType::Password||d.type==InputFieldType::Pin;
    const Rect fieldSpan=k.fieldRect;const int32_t controlWidth=clamp32(draftHeight*3/4,28,44),controlStep=controlWidth+4;
    if(!k.picker)k.fieldRect.width=std::max<int32_t>(0,k.fieldRect.width-(secure?2:1)*controlStep);
    // Integer tenths keep weighted rows identical to the browser layout,
    // including the half-key inset on the home row and circular safe chords.
    auto addRow=[&](int32_t y,int32_t h,const std::vector<SoftKey> &keys,const std::vector<int32_t> &weights,int32_t units,int32_t inset){Rect r=keyboardRow(y,h,width);if(r.width<=0||keys.empty())return;
      if(!units)units=int32_t(keys.size())*10;int32_t used=inset;
      for(size_t n=0;n<keys.size();++n){SoftKey key=keys[n];const int32_t weight=n<weights.size()?weights[n]:10;
        const int32_t x=r.x+int32_t(int64_t(used)*r.width/units),end=r.x+int32_t(int64_t(used+weight)*r.width/units);used+=weight;
        if(key.id=="empty")continue;key.rect={x+2,y+2,std::max<int32_t>(0,end-x-4),std::max<int32_t>(0,h-4)};layoutKeys.push_back(key);}
    };
    const bool next=d.enterAction==InputEnterAction::Next||(d.enterAction==InputEnterAction::Auto&&following>=0);
    auto key=[](const char *id,const char *label,const char *value,const char *action){return SoftKey{std::string(id),std::string(label),std::string(value),std::string(action),{}};};
    const SoftKey done=key("done",d.enterAction==InputEnterAction::Search?"搜索":next?"下一项":"完成",next?"next":d.enterAction==InputEnterAction::Search?"search":"done","done");
    auto fieldControl=[&](const SoftKey &control,int32_t offset){SoftKey value=control;value.rect={fieldSpan.x+fieldSpan.width-offset,fieldSpan.y,controlWidth,fieldSpan.height};layoutKeys.push_back(value);};
    if(secure)fieldControl(key("reveal",k.reveal?"隐藏":"查看","","reveal"),controlWidth+controlStep);
    fieldControl(key("hide","收起","","hide"),controlWidth);
    int32_t keysTop=titleY+titleHeight+draftHeight+4,bottom=top+height-(circularKeyboard?std::max<int32_t>(margin,height/10):margin);
    struct Row {std::vector<SoftKey> keys;std::vector<int32_t> weights;int32_t units,inset;};std::vector<Row> rows;
    auto row=[&](const std::vector<SoftKey> &keys){rows.push_back({keys,{},0,0});};
    auto letters=[&](const char *s){std::vector<SoftKey> values;const std::string text=s;for(size_t p=0;p<text.size();){const size_t following=inputFollowing(text,p);std::string c=text.substr(p,following-p),v=c;if(k.shift&&v[0]>='a'&&v[0]<='z')v[0]-=32;values.push_back({"char-"+c,v,v,"insert",{}});p=following;}return values;};
    const SoftKey empty=key("empty","","","insert"),backspace=key("backspace","退格","","backspace"),space=key("space","空格"," ","insert"),mode=key("mode",k.mode?"ABC":grouped?"123":"?123","","mode");
    auto textFooter=[&](){rows.push_back({{mode,key("comma",",",",","insert"),space,key("period",".",".","insert"),done},{15,10,40,10,25},100,0});};
    if(numeric&&height<360&&!k.picker){
      const bool sign=d.type==InputFieldType::Number&&(!d.hasMin||d.min<0),decimal=d.type==InputFieldType::Number&&d.decimals;
      if(d.type==InputFieldType::Tel||sign||decimal){
        auto first=letters("123");first.push_back(backspace);row(first);
        auto second=letters("456");second.push_back(d.type==InputFieldType::Tel?key("plus","+","+","insert"):sign?key("sign","±","","sign"):empty);row(second);
        auto third=letters("789");third.push_back(decimal?key("decimal",".",".","insert"):empty);row(third);
        row({d.type==InputFieldType::Tel?key("star","*","*","insert"):empty,key("char-0","0","0","insert"),d.type==InputFieldType::Tel?key("hash","#","#","insert"):empty,done});
      }else {for(const char *s:{"123","456","789"})row(letters(s));row({backspace,key("char-0","0","0","insert"),done});}
    }else if(numeric){for(const char *s:{"123","456","789"})row(letters(s));
      const SoftKey first=d.type==InputFieldType::Tel?key("star","*","*","insert"):d.type==InputFieldType::Number&&d.decimals?key("decimal",".",".","insert"):empty;
      row({first,key("char-0","0","0","insert"),d.type==InputFieldType::Tel?key("hash","#","#","insert"):backspace});
      if(d.type==InputFieldType::Tel)row({key("plus","+","+","insert"),backspace,done});
      else if(d.type==InputFieldType::Number&&(!d.hasMin||d.min<0))rows.push_back({{key("sign","±","","sign"),done},{10,20},30,0});
      else row({done});
    }else if(grouped){
      if(k.mode){static const char *const pages[]={"123456","7890.-","@_/:,;","!?$%&+","()[]{}","=\"'\\#*"};const auto values=letters(pages[(k.mode-1)%6]);row({values[0],values[1],values[2]});row({values[3],values[4],values[5]});}
      else {static const char *const groups[]={"abcd","efgh","ijkl","mnop","qrst","uvwxyz"};static const char *const labels[]={"a-d","e-h","i-l","m-p","q-t","u-z"};
        if(k.group>=0){const auto values=letters(groups[k.group]);for(size_t r=0;r<2;++r){std::vector<SoftKey> keys;for(size_t c=0;c<3;++c){size_t index=r*3+c;keys.push_back(index<values.size()?values[index]:empty);}row(keys);}}
        else for(int r=0;r<2;++r){std::vector<SoftKey> keys;for(int c=0;c<3;++c){const int g=r*3+c;std::string label=labels[g];if(k.shift){label[0]-=32;label[2]-=32;}keys.push_back({"group-"+std::to_string(g),label,groups[g],"group",{}});}row(keys);}
      }
      row({!k.mode&&k.group>=0?key("groups","返回","","groups"):empty,k.mode?key("symbols","#+=","","symbols-next"):key("shift",k.shift?"小写":"大写","","shift"),backspace});
      rows.push_back({{mode,space,done},{30,40,30},100,0});
    }else if(k.mode){const bool alternate=(k.mode-1)%2;
      row(letters(alternate?"[]{}#%^*+=":"1234567890"));row(letters(alternate?"_\\|~<>$&@:":"-/:;()$&@\""));
      std::vector<SoftKey> third={key("symbols",alternate?"123":"#+=","","symbols-next")};const auto punctuation=letters(alternate?".,?!`":".,?!'");third.insert(third.end(),punctuation.begin(),punctuation.end());third.push_back(backspace);rows.push_back({third,{15,14,14,14,14,14,15},100,0});textFooter();
    }else {row(letters("qwertyuiop"));rows.push_back({letters("asdfghjkl"),{},100,5});std::vector<SoftKey> third={key("shift",k.shift?"小写":"大写","","shift")};const auto last=letters("zxcvbnm");third.insert(third.end(),last.begin(),last.end());third.push_back(backspace);rows.push_back({third,{15,10,10,10,10,10,10,10,15},100,0});textFooter();
    }
    if(k.picker){
      Rect selected{},previous{},next{};
      if(height<64&&width>=64){
        const int32_t selectionWidth=std::min<int32_t>(64,std::max<int32_t>(24,width/2));
        k.labelRect={0,top,0,0};k.fieldRect={2,top+2,std::max<int32_t>(0,width-selectionWidth-6),std::max<int32_t>(0,height-4)};
        selected={width-selectionWidth-2,top+2,selectionWidth,std::max<int32_t>(0,height-4)};
      }else {
        const int32_t fieldY=circularKeyboard&&!titleHeight?top+std::max<int32_t>(margin,height/6):titleY+titleHeight;
        k.fieldRect=keyboardRow(fieldY,draftHeight,width);
        const int32_t bodyY=fieldY+draftHeight+4,bodyHeight=std::max<int32_t>(0,top+height-(circularKeyboard?std::max<int32_t>(margin,height/4):margin)-bodyY);
        const Rect span=keyboardRow(bodyY,bodyHeight,width);
        if(width>=96){const int32_t navWidth=clamp32(span.width/4,16,32);previous={span.x,span.y,navWidth,span.height};next={span.x+span.width-navWidth,span.y,navWidth,span.height};selected={span.x+navWidth+4,span.y,std::max<int32_t>(0,span.width-2*(navWidth+4)),span.height};}
        else {const int32_t navHeight=clamp32(bodyHeight/4,16,32);previous={span.x,span.y,span.width,navHeight};next={span.x,span.y+span.height-navHeight,span.width,navHeight};selected={span.x,span.y+navHeight+4,span.width,std::max<int32_t>(0,span.height-2*(navHeight+4))};}
      }
      for(const auto &r:rows)for(const auto &item:r.keys)if(item.id!="empty")layoutKeys.push_back(item);
      for(auto &item:layoutKeys){item.rect=selected;
        if(item.action=="group"&&selected.width<36)item.label=item.label.substr(0,1)+"+";
        else if(item.id=="mode"&&selected.width<44)item.label=selected.width<36?(k.mode?"AB":"#"):(k.mode?"ABC":"123");
        else if(item.id=="space"&&selected.width<44)item.label="SP";
      }
      SoftKey prev=key("picker-prev","","","picker-prev"),following=key("picker-next","","","picker-next");prev.rect=previous;following.rect=next;layoutKeys.push_back(prev);layoutKeys.push_back(following);
    }else {
      const int32_t cap=std::min<int32_t>(480,grouped?width*14/10:numeric?width:width*65/100),available=bottom-keysTop,gridHeight=std::min(available,cap),offset=(available-gridHeight)/2;
      titleY+=offset;k.labelRect=titleRow(titleY);k.fieldRect=keyboardRow(titleY+titleHeight,draftHeight,width);const Rect movedSpan=k.fieldRect;k.fieldRect.width=std::max<int32_t>(0,k.fieldRect.width-(secure?2:1)*controlStep);
      for(auto &control:layoutKeys){control.rect.y=k.fieldRect.y;control.rect.x=movedSpan.x+movedSpan.width-(control.id=="reveal"?controlWidth+controlStep:controlWidth);}
      keysTop=titleY+titleHeight+draftHeight+4;bottom=keysTop+gridHeight;
      const int32_t rowHeight=std::max<int32_t>(8,gridHeight/std::max<int32_t>(1,int32_t(rows.size())));
      for(size_t r=0;r<rows.size();++r)addRow(keysTop+int32_t(r)*rowHeight,rowHeight,rows[r].keys,rows[r].weights,rows[r].units,rows[r].inset);
      if(!numeric&&!grouped&&!rows.empty()&&!rows.front().keys.empty()){
        // The P cell (or its symbol-page counterpart) anchors the toolbar.
        // Use the final pixel-rounded cell so the right edges match exactly.
        const std::string &lastId=rows.front().keys.back().id;Rect anchor{};
        for(const auto &item:layoutKeys)if(item.id==lastId){anchor=item.rect;break;}
        if(anchor.width>0){
          const int32_t right=anchor.x+anchor.width,step=anchor.width+4;
          k.fieldRect.width=std::max<int32_t>(0,right-k.fieldRect.x-(secure?2:1)*step);
          for(auto &control:layoutKeys)if(control.id=="hide"||control.id=="reveal"){
            control.rect.x=right-anchor.width-(control.id=="reveal"?step:0);control.rect.width=anchor.width;
          }
        }
      }
    }
    k.keys=std::make_shared<const std::vector<SoftKey>>(std::move(layoutKeys));
    int mapped=-1;
    if(k.builtNode==k.node&&previousFocus){
      auto find=[&](int kind){for(size_t i=0;i<k.keys->size();++i){const auto &key=(*k.keys)[i];
        if(kind==0&&key.id==previousFocus->id)return int(i);
        if(kind==1&&key.action==previousFocus->action&&key.value==previousFocus->value)return int(i);
        if(kind==2&&previousFocus->action=="group"&&!previousFocus->value.empty()&&key.id=="char-"+previousFocus->value.substr(0,1))return int(i);
        if(kind==2&&previousFocus->action=="insert"&&previousFocus->value.size()==1&&key.action=="group"){char letter=previousFocus->value[0];if(letter>='A'&&letter<='Z')letter+=32;if(key.value.find(letter)!=std::string::npos)return int(i);}
      }return -1;};
      for(int kind=0;kind<3&&mapped<0;++kind)mapped=find(kind);
    }
    if(mapped<0)for(size_t i=0;i<k.keys->size();++i){const auto &key=(*k.keys)[i];if(key.action=="group"||(key.action=="insert"&&!key.value.empty())){mapped=int(i);break;}}
    k.keyFocus=mapped<0?0:mapped;
    k.keyFocus=clamp32(k.keyFocus,0,int32_t(k.keys->size())-(k.picker?3:1));
    k.builtNode=k.node;k.builtMode=k.mode;k.builtGroup=k.group;k.builtNext=following;k.builtShift=k.shift;k.builtReveal=k.reveal;k.builtRound=circularKeyboard;k.builtEpoch=inputLayoutEpoch;return true;
  }
  int keyboardHit(const SoftInput &k,int32_t px,int32_t py) const {
    if(circularKeyboard){const int64_t dx=2LL*px-page().width,dy=2LL*py-page().height,r=std::min(page().width,page().height)-8;if(dx*dx+dy*dy>r*r)return -1;}
    if(k.keys)for(size_t i=0;i<k.keys->size();++i){if(k.picker&&i+2<k.keys->size()&&int(i)!=k.keyFocus)continue;const Rect key=(*k.keys)[i].rect;if(key.width<=0||key.height<=0)continue;Rect touch={key.x-2,key.y-2,key.width+4,key.height+4};const Rect safe=keyboardRow(touch.y,touch.height,std::min<int32_t>(page().width,640));touch=intersect(touch,safe);if(contains(touch,px,py))return int(i);}return -1;
  }
  bool softPickerStep(int32_t delta) {
    auto &k=softInput;if(!k.picker||!k.keys||k.keys->size()<=2)return false;const int32_t count=int32_t(k.keys->size())-2;
    invalidateKeyboardKey(k.keyFocus);keyboardBackspaceRepeat=keyboardBackspacePaused=false;k.keyFocus=((k.keyFocus+delta)%count+count)%count;invalidateKeyboardKey(k.keyFocus);return true;
  }
  static bool keyboardLabelMatches(const SoftInput &k,const SoftKey &key,const char *label) {
    // Reveal changes its icon and semantic label, never the key geometry. Keep
    // both public label APIs accurate without mutating the shared key snapshot.
    return key.action=="reveal"?!strcmp(label,k.reveal?"隐藏":"查看"):key.label==label;
  }
  bool softPress(const SoftKey &key) {
    const std::string action=key.action,value=key.value;
    if(action=="picker-prev"||action=="picker-next")return softPickerStep(action=="picker-prev"?-1:1);
    if(action=="done")return softCommit(value=="next");if(action=="hide")return softHide();
    if(action=="backspace")return softBackspace();if(action=="sign")return softSign();
    if(action=="insert")return softInsert(value.c_str());
    if(action=="group"){static const char *const groups[]={"abcd","efgh","ijkl","mnop","qrst","uvwxyz"};for(int g=0;g<6;++g)if(value==groups[g])softInput.group=g;}
    else if(action=="groups")softInput.group=-1;
    else if(action=="shift")softInput.shift=!softInput.shift;
    else if(action=="mode"){softInput.mode=softInput.mode?0:1;softInput.group=-1;}
    else if(action=="symbols"||action=="symbols-next")softInput.mode=softInput.mode%(softInput.picker||circularKeyboard||page().width<366?6:2)+1;
    else if(action=="reveal"){
      softInput.reveal=!softInput.reveal;
      if(softInput.visible){const Rect field=softInput.fieldRect;refresh->invalidate(DirtyRect(field.x,field.y,field.width,field.height));
        if(softInput.keys)for(size_t i=0;i<softInput.keys->size();++i)if((*softInput.keys)[i].action=="reveal"){invalidateKeyboardKey(int(i));break;}}
      return true;
    }
    else return false;
    if(buildKeyboard(softInput))invalidateKeyboardKeys();
    if(softInput.picker&&(action=="mode"||action=="group"||action=="groups"||action=="symbols"||action=="symbols-next")&&softInput.keys)
      for(size_t i=0;i+2<softInput.keys->size();++i){const auto &item=(*softInput.keys)[i];if(item.action=="group"||(item.action=="insert"&&!item.value.empty()&&item.value!=" ")){softInput.keyFocus=int(i);invalidateKeyboardKeys();break;}}
    invalidateKeyboardField();return true;
  }
  bool softKeyAction(InputAction action) {
    if(action==InputAction::Back||action==InputAction::Home)return softHide();
    if(action==InputAction::Confirm||action==InputAction::Activate){buildKeyboard(softInput);if(softInput.keys&&!softInput.keys->empty())softPress((*softInput.keys)[softInput.keyFocus]);return true;}
    const bool previous=action==InputAction::Previous||action==InputAction::Up||action==InputAction::Left;
    const bool next=action==InputAction::Next||action==InputAction::Down||action==InputAction::Right;
    if(previous||next){if(buildKeyboard(softInput))invalidateKeyboardKeys();if(softInput.picker)return softPickerStep(previous?-1:1);const int count=softInput.keys?int(softInput.keys->size()):0;invalidateKeyboardKey(softInput.keyFocus);if(count)softInput.keyFocus=(softInput.keyFocus+(previous?count-1:1))%count;invalidateKeyboardKey(softInput.keyFocus);}
    return true; // Modal input consumes every physical binding, including routes.
  }
  static int32_t inputGlyphAdvance(const Style &style,uint16_t code,bool compact=false) {
    if(!style.font)return 0;int32_t advance=nice_gfx::detail::monoAdvance(*style.font,code);
    // Draft text uses half-width Latin cells. Uni16 Latin ink is centered in
    // its square glyph, so inputDrawLine recenters it without scaling pixels.
    if(compact&&code>=32&&code<=126)advance=std::min<int32_t>(advance,std::max<int32_t>(1,style.font->height/2));return advance*style.fontSize;
  }
  int32_t inputLineWidth(InputTextView text,const Style &style,size_t begin,size_t end,bool compact=false) {
    if(!style.font)return 0;int32_t pixels=0;for(size_t i=begin;i<end;){const uint16_t cp=nextCode(text.c_str(),i,end);pixels+=inputGlyphAdvance(style,cp,compact);}return pixels;
  }
  size_t inputLineStart(InputTextView text,const Style &style,Rect r,size_t cursor,bool compact=false) {
    if(cursor==SIZE_MAX||!style.font)return 0;size_t start=0;int32_t remaining=inputLineWidth(text,style,0,cursor,compact)+(compact?inputGlyphAdvance(style,'n',true):style.font->width*style.fontSize);
    while(start<cursor&&remaining>r.width){const size_t following=inputFollowing(text,start);remaining-=inputLineWidth(text,style,start,following,compact);start=following;}return start;
  }
  static Rect keyboardTextRect(const SoftInput &k) {Rect field=k.fieldRect;const int32_t padding=field.width<64?2:8;field.x+=padding;field.width=std::max<int32_t>(0,field.width-2*padding);return field;}
  static int32_t keyboardCaretReserve(const Style &style,Rect field) {return field.width<48?std::max<int32_t>(1,style.fontSize):inputGlyphAdvance(style,'n',true);}
  static Rect keyboardTitleTextRect(const SoftInput &k) {Rect title=k.labelRect;title.x+=6;title.width=std::max<int32_t>(0,title.width-12);return title;}
  static bool keyboardMasked(const SoftInput &k) {return !k.reveal&&(k.descriptor.type==InputFieldType::Password||k.descriptor.type==InputFieldType::Pin);}
  static std::string keyboardDraftText(const SoftInput &k) {return keyboardMasked(k)?std::string(inputCharacters(k.draft),'*'):k.draft;}
  static size_t keyboardDraftCursor(const SoftInput &k) {return keyboardMasked(k)?inputCharacters(k.draft,k.cursor):k.cursor;}
  Style keyboardDraftStyle(const SoftInput &k) const {
    Style style=styleFor(page(),state(),k.node);if(k.descriptor.keyboardFont)style.font=k.descriptor.keyboardFont;style.fontSize=k.picker||k.rect.height<300?1:2;return style;
  }
  void prepareKeyboardText() {
    auto &frame=keyboardTextFrame;frame.draft.clear();frame.title.clear();frame.cursor=0;frame.titleFontSize=1;
    const auto &k=snapshotKeyboard;if(!k.visible||k.node<0)return;
    const auto &d=k.descriptor;const NodeDesc &n=rasterFrame.current.page->nodes[k.node];
    if(!k.error.empty())frame.title=k.error;else{frame.title=d.label?d.label:n.text?n.text:"";if(d.unit&&*d.unit){frame.title+=" (";frame.title+=d.unit;frame.title+=")";}}
    Style style=styleFor(*rasterFrame.current.page,rasterFrame.current.values,k.node);if(d.keyboardFont)style.font=d.keyboardFont;
    if(style.font){
      // Prepare title fitting once, so DMA tiles share the same typography and
      // UTF-8 suffix. Normal 32px titles have 8px of space above and below.
      const Rect title=keyboardTitleTextRect(k);style.fontSize=k.labelRect.height>=48?2:1;
      const int32_t padding=title.height>=48?8:std::max<int32_t>(0,std::min<int32_t>(4,(title.height-16)/2));
      while(style.fontSize>1&&(int32_t(style.font->height)*style.fontSize>title.height-2*padding||
        inputLineWidth(frame.title,style,0,frame.title.size(),true)>title.width))--style.fontSize;
      frame.titleFontSize=style.fontSize;
      if(inputLineWidth(frame.title,style,0,frame.title.size(),true)>title.width){
        const int32_t dot=inputGlyphAdvance(style,'.',true),suffix=3*dot;
        size_t end=0;int32_t pixels=0;
        while(end<frame.title.size()) {size_t next=end;const uint16_t code=nextCode(frame.title.c_str(),next,frame.title.size());const int32_t advance=inputGlyphAdvance(style,code,true);
          if(pixels+advance+suffix>title.width)break;pixels+=advance;end=next;}
        frame.title.resize(end);if(suffix<=title.width)frame.title+="...";
        else for(int32_t used=0;dot>0&&used+dot<=title.width;used+=dot)frame.title+='.';
      }
    }
    style.fontSize=k.picker||k.rect.height<300?1:2;
    const Rect field=keyboardTextRect(k);if(!style.font||field.width<=0)return;
    const bool masked=keyboardMasked(k);const size_t cursor=std::min(k.cursor,k.draft.size());size_t start=cursor;
    const int32_t reserve=keyboardCaretReserve(style,field);int32_t pixels=0;
    // Walk back only as far as this field can display. Long drafts and masked
    // passwords no longer allocate or measure their full contents per tile.
    while(start){const size_t previous=inputPrevious(k.draft,start);size_t next=previous;const uint16_t code=masked?'*':nextCode(k.draft.c_str(),next,start);const int32_t advance=inputGlyphAdvance(style,code,true);if(pixels+advance+reserve>field.width)break;pixels+=advance;start=previous;}
    pixels=0;for(size_t i=start;i<k.draft.size();){const size_t previous=i;const size_t next=inputFollowing(k.draft,i);size_t decode=i;const uint16_t code=masked?'*':nextCode(k.draft.c_str(),decode,next);const int32_t advance=inputGlyphAdvance(style,code,true);if(pixels+advance>field.width)break;
      if(previous==cursor)frame.cursor=frame.draft.size();if(masked)frame.draft+='*';else frame.draft.append(k.draft,previous,next-previous);pixels+=advance;i=next;
      if(i==cursor)frame.cursor=frame.draft.size();
    }
  }
  bool softPlaceCursor(int32_t px) {
    const auto &k=softInput;const Style style=keyboardDraftStyle(k);const Rect field=keyboardTextRect(k);
    if(!style.font||field.width<=0)return true;const bool masked=keyboardMasked(k);
    size_t position=std::min(k.cursor,k.draft.size());const int32_t reserve=keyboardCaretReserve(style,field);int32_t pixels=0;
    // Locate directly in the original UTF-8 draft, including password cells.
    // Only the visible prefix/suffix is measured; no full or masked copy.
    while(position){const size_t previous=inputPrevious(k.draft,position);size_t next=previous;const uint16_t code=masked?'*':nextCode(k.draft.c_str(),next,position);const int32_t advance=inputGlyphAdvance(style,code,true);if(pixels+advance+reserve>field.width)break;pixels+=advance;position=previous;}
    int32_t x=field.x;while(position<k.draft.size()){const size_t following=inputFollowing(k.draft,position);size_t next=position;const uint16_t code=masked?'*':nextCode(k.draft.c_str(),next,following);const int32_t advance=inputGlyphAdvance(style,code,true);if(x+advance>field.x+field.width||px<x+(advance+1)/2)break;x+=advance;position=following;}
    if(position!=softInput.cursor){const size_t previous=softInput.cursor;softInput.cursor=position;invalidateKeyboardCursor(previous);}return true;
  }
  int32_t keyboardCursorCell() const {
    const Style style=keyboardDraftStyle(softInput);return std::max<int32_t>(1,style.font?inputGlyphAdvance(style,'n',true):8);
  }
  void softPointerCursor(int32_t dx) {
    size_t cursor=std::min(keyboardPointerCursor,softInput.draft.size());while(cursor<softInput.draft.size()&&cursor&&(static_cast<unsigned char>(softInput.draft[cursor])&0xc0)==0x80)--cursor;int32_t count=clamp32(dx/keyboardPointerCell,-512,512);
    while(count<0){cursor=inputPrevious(softInput.draft,cursor);++count;}while(count>0){cursor=inputFollowing(softInput.draft,cursor);--count;}
    if(cursor!=softInput.cursor){const size_t previous=softInput.cursor;softInput.cursor=cursor;invalidateKeyboardCursor(previous);}
  }
  static bool inputGlyphFitsCell(const Style &style,uint16_t code,int32_t advance,int32_t offset) {
    const auto *font=style.font;const int32_t scale=style.fontSize;
    if(!font||!scale||advance<=0||(font->bitsPerPixel!=0&&font->bitsPerPixel!=1)||nice_gfx::detail::monoFallback(*font,code))return false;
    if(!offset&&int32_t(font->width)*scale<=advance)return true;
    const uint8_t *glyph=nice_gfx::detail::monoGlyph(*font,code);if(!glyph)return false;
    const int32_t left=(offset+scale-1)/scale,right=(offset+advance)/scale;
    const int32_t stride=(font->width+7)/8;
    // Binary EPX cannot create ink in a wholly empty source column: its up
    // and down samples both remain zero and the center sample is preserved.
    // Prove every excluded column empty, including enlarged pixel extents.
    for(int32_t byte=0;byte<stride;++byte){uint8_t outside=0;
      for(int32_t bit=0;bit<8;++bit){const int32_t x=byte*8+bit;if(x<font->width&&(x<left||x>=right))outside|=uint8_t(0x80u>>bit);}
      if(outside)for(int32_t y=0;y<font->height;++y)if(pgm_read_byte(glyph+y*stride+byte)&outside)return false;
    }
    return true;
  }
  void inputDrawLine(InputTextView text,const Style &style,Rect r,bool center=false,size_t cursor=SIZE_MAX,bool compact=false,size_t preparedStart=SIZE_MAX,bool masked=false,const Rect *cellViewport=nullptr) {
    if(r.width<=0||r.height<=0||!style.font)return;canvas.setFont(style.font);canvas.setTextSize(style.fontSize);canvas.setTextColor(style.color);
    const int32_t h=style.font->height*style.fontSize;const size_t start=preparedStart==SIZE_MAX?inputLineStart(text,style,r,cursor,compact):preparedStart;
    int32_t px=r.x,py=r.y+std::max<int32_t>(0,(r.height-h)/2);if(center)px+=std::max<int32_t>(0,(r.width-inputLineWidth(text,style,0,text.size(),compact))/2);
    auto caret=[&](){if(compact)canvas.fillRect(px,py,std::max<int32_t>(1,style.fontSize),h,style.fillColor);else canvas.drawFastVLine(px,py,h,style.fillColor);};
    const size_t end=text.size();for(size_t i=start;i<end&&px<r.x+r.width;){if(i==cursor)caret();uint16_t cp=nextCode(text.c_str(),i,end);if(masked)cp='*';const int32_t advance=inputGlyphAdvance(style,cp,compact),offset=compact&&cp>=32&&cp<=126?(nice_gfx::detail::monoAdvance(*style.font,cp)*style.fontSize-advance)/2:0;if(compact&&px+advance>r.x+r.width)break;
      const bool clipCell=cellViewport&&inputGlyphFitsCell(style,cp,advance,offset);
      if(clipCell){const Rect cell=intersect(*cellViewport,{px,cellViewport->y,advance,cellViewport->height});canvas.setViewport(cell.x,cell.y,cell.width,cell.height,false);}
      canvas.drawChar(cp,px-offset,py);
      if(clipCell)canvas.setViewport(cellViewport->x,cellViewport->y,cellViewport->width,cellViewport->height,false);
      px+=advance;
    }
    if(cursor==end&&px<r.x+r.width)caret();
  }
  void paintInputField(const NodeDesc &n,const State &v,const Style &style,Rect r) {
    const InputTextView text(v.text.c_str());
    if(text.empty()&&n.inputField&&n.inputField->placeholder){inputDrawLine(n.inputField->placeholder,style,r);return;}
    const bool masked=n.inputField&&(n.inputField->type==InputFieldType::Password||n.inputField->type==InputFieldType::Pin);
    inputDrawLine(text,style,r,false,SIZE_MAX,false,SIZE_MAX,masked);
  }
  bool paintKeyboardIcon(const SoftKey &key,const SoftInput &k,uint16_t color,uint16_t background) {
    using IconKind=keyboard_icons::Kind;IconKind kind;
    if(key.id=="hide")kind=IconKind::Hide;
    else if(key.id=="picker-prev")kind=IconKind::Groups;
    else if(key.id=="picker-next")kind=IconKind::Next;
    else if(key.id=="reveal")kind=k.reveal?IconKind::Mask:IconKind::Reveal;
    else if(key.id=="backspace")kind=IconKind::Backspace;
    else if(key.id=="shift")kind=IconKind::Shift;
    else if(key.id=="groups")kind=IconKind::Groups;
    else if(key.id=="done"&&k.descriptor.enterAction==InputEnterAction::Search)kind=IconKind::Search;
    else if(key.id=="done") {Style label=styleFor(*rasterFrame.current.page,rasterFrame.current.values,k.node);if(k.descriptor.keyboardFont)label.font=k.descriptor.keyboardFont;label.fontSize=1;if(label.font&&inputLineWidth(key.label,label,0,key.label.size(),true)+12<=key.rect.width&&int32_t(label.font->height)+8<=key.rect.height)return false;kind=key.value=="next"?IconKind::Next:IconKind::Check;}
    else return false;
    const Rect r=key.rect;const int32_t available=std::min(r.width,r.height);if(available<16)return true;
    // The smallest flash mask is 16px. Small numeric rows are only 24px
    // high, so relax the usual 5px inset instead of silently losing the icon.
    const int32_t size=std::min<int32_t>(28,std::min<int32_t>(available,std::max<int32_t>(16,available-10)));
    const MonoIcon &icon=keyboard_icons::get(kind,size,canvas.getAntialiasing());
    const int32_t x=r.x+(r.width-icon.width)/2,y=r.y+(r.height-icon.height)/2;
    canvas.drawIcon(x,y,icon,color,background,1);
    if(key.id=="shift"&&k.shift)canvas.fillRect(x,y+icon.height+2,icon.width,2,color);
    return true;
  }
  Style keyboardKeyTextStyle(const SoftKey &key,Style style) {
    // Size follows the actual touch cell, including docked and circular rows.
    // Keep bitmap pixels at integer scales and leave room around long labels.
    style.fontSize=uint8_t(clamp32((key.rect.height+20)/40,1,3));
    if(style.font)while(style.fontSize>1&&(int32_t(style.font->height)*style.fontSize>key.rect.height-12||
      inputLineWidth(key.label,style,0,key.label.size(),true)>key.rect.width-12))--style.fontSize;
    return style;
  }
  void paintKeyboard(Rect tile) {
    const SoftInput &k=snapshotKeyboard;if(!k.visible||k.node<0)return;
    const Rect keyboardClip=intersect(k.rect,tile);if(keyboardClip.width<=0||keyboardClip.height<=0)return;++stats.keyboardPaints;
    const auto &d=k.descriptor;Style style=styleFor(*rasterFrame.current.page,rasterFrame.current.values,k.node);if(d.keyboardFont)style.font=d.keyboardFont;style.fontSize=1;style.radius=6;style.borderWidth=1;style.textAlign=1;
    canvas.setAntialiasing(!(rasterFrame.current.values[k.node].flags&NoAntialias));
    // The complete theme belongs to the immutable keyboard frame snapshot.
    auto color=[](uint32_t rgb){return uint16_t(((rgb>>8)&0xf800)|((rgb>>5)&0x07e0)|((rgb>>3)&0x001f));};const bool dark=k.theme==KeyboardTheme::Dark;
    style.background=color(dark?0x303641:0xffffff);style.color=color(dark?0xf1f5f9:0x1f2937);style.borderColor=color(dark?0x414a5b:0xccd5e2);style.trackColor=color(dark?0x171a21:0xe8edf4);style.fillColor=color(dark?0x3b82f6:0x2563eb);style.pressedColor=color(dark?0x475569:0xbfdbfe);
    const uint16_t auxiliary=color(dark?0x232934:0xd9e1ec),fieldBackground=color(dark?0x242a35:0xffffff),muted=color(dark?0xa3b1c6:0x64748b),placeholder=color(dark?0x8795aa:0x94a3b8);
    canvas.setViewport(tile.x,tile.y,tile.width,tile.height,false);canvas.fillRect(k.rect.x,k.rect.y,k.rect.width,k.rect.height,style.trackColor);canvas.drawFastHLine(k.rect.x,k.rect.y,k.rect.width,style.borderColor);
    const Rect labelClip=intersect(k.labelRect,tile);if(labelClip.width>0&&labelClip.height>0){
      canvas.setViewport(labelClip.x,labelClip.y,labelClip.width,labelClip.height,false);
      Style statusStyle=style;statusStyle.fontSize=keyboardTextFrame.titleFontSize;statusStyle.color=k.error.empty()?muted:color(dark?0xfca5a5:0xdc2626);inputDrawLine(keyboardTextFrame.title,statusStyle,keyboardTitleTextRect(k),true,SIZE_MAX,true);
    }
    const Rect fieldClip=intersect(k.fieldRect,tile);if(fieldClip.width>0&&fieldClip.height>0){
      canvas.setViewport(fieldClip.x,fieldClip.y,fieldClip.width,fieldClip.height,false);Style fieldStyle=style;fieldStyle.radius=8;fieldStyle.fontSize=k.picker||k.rect.height<300?1:2;rounded(k.fieldRect,fieldBackground,!k.error.empty()&&!k.labelRect.height?color(dark?0xfca5a5:0xdc2626):style.borderColor,fieldStyle);
      const Rect field=keyboardTextRect(k);
      if(k.draft.empty()&&d.placeholder&&*d.placeholder){Style hintStyle=fieldStyle;hintStyle.color=placeholder;inputDrawLine(d.placeholder,hintStyle,field,false,SIZE_MAX,true);}
      inputDrawLine(keyboardTextFrame.draft,fieldStyle,field,false,keyboardTextFrame.cursor,true,0,false,&fieldClip);
    }
    if(k.keys)for(size_t i=0;i<k.keys->size();++i){if(k.picker&&i+2<k.keys->size()&&int(i)!=k.keyFocus)continue;const auto &key=(*k.keys)[i];Rect clip=intersect(key.rect,tile);if(clip.width<=0||clip.height<=0)continue;canvas.setViewport(clip.x,clip.y,clip.width,clip.height,false);
      const bool primary=key.action=="done",function=key.action!="insert";const uint16_t background=key.id==k.pressed?style.pressedColor:primary?style.fillColor:function?auxiliary:style.background;Style keyStyle=style;keyStyle.color=primary&&key.id!=k.pressed?0xffff:style.color;
      rounded(key.rect,background,snapshotKeyInput&&int(i)==k.keyFocus?style.fillColor:style.borderColor,style);if(!paintKeyboardIcon(key,k,keyStyle.color,background))inputDrawLine(key.label,keyboardKeyTextStyle(key,keyStyle),key.rect,true,SIZE_MAX,true);
    }
    if(k.keys&&!k.pressed.empty())for(const auto &key:*k.keys)if(key.id==k.pressed){const Rect bubble=keyboardBubble(k,key),clip=intersect(bubble,tile);if(clip.width>0&&clip.height>0){canvas.setViewport(clip.x,clip.y,clip.width,clip.height,false);Style bubbleStyle=style;bubbleStyle.radius=6;if(style.font&&style.font->height<=16)bubbleStyle.fontSize=2;rounded(bubble,fieldBackground,style.fillColor,bubbleStyle);inputDrawLine(key.value,bubbleStyle,bubble,true);}break;}
  }
