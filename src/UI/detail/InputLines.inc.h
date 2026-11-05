struct InputLine {
  size_t start=0,end=0;
};

std::vector<InputLine> inputLines(InputTextView text,const Style &style,int32_t width,bool compact) {
  std::vector<InputLine> lines;
  size_t start=0;
  int32_t pixels=0;

  for(size_t index=0;index<text.size();) {
    const size_t before=index;
    const uint16_t code=nextCode(text.data,index,text.size());

    if(code=='\n') {
      lines.push_back({start,before});
      start=index;
      pixels=0;
      continue;
    }

    const int32_t advance=inputGlyphAdvance(style,code,compact);

    if(before>start && pixels+advance>std::max<int32_t>(1,width)) {
      lines.push_back({start,before});
      start=before;
      pixels=0;
    }

    pixels+=advance;
  }

  lines.push_back({start,text.size()});
  return lines;
}

void inputDrawMultiline(InputTextView text,const Style &style,Rect rect,
  size_t cursor=SIZE_MAX,bool compact=false,const Rect *viewport=nullptr) {
  if(!style.font || rect.width<=0 || rect.height<=0) return;

  const int32_t reserve=cursor==SIZE_MAX?0:std::max<int32_t>(1,style.fontSize);
  const auto lines=inputLines(text,style,rect.width-reserve,compact);
  const int32_t height=style.font->height*style.fontSize;
  const size_t rows=size_t(std::max<int32_t>(1,rect.height/std::max<int32_t>(1,height)));
  size_t current=0;

  if(cursor!=SIZE_MAX) {
    for(size_t index=0;index<lines.size();++index) {
      if(cursor>=lines[index].start) current=index;
    }
  }

  const size_t first=current>=rows?current-rows+1:0;

  for(size_t index=first;index<lines.size() && index<first+rows;++index) {
    const auto &line=lines[index];
    const std::string value(text.data+line.start,line.end-line.start);
    const Rect box{rect.x,rect.y+int32_t(index-first)*height,rect.width,height};
    const size_t caret=index==current && cursor!=SIZE_MAX?cursor-line.start:SIZE_MAX;

    inputDrawLine(value,style,box,false,caret,compact,0,false,viewport);
  }
}

bool placeMultilineCursor(int32_t px,int32_t py,const Style &style,Rect field) {
  const auto lines=inputLines(softInput.draft,style,field.width-std::max<int32_t>(1,style.fontSize),true);
  const int32_t height=style.font->height*style.fontSize;
  size_t current=0;

  for(size_t index=0;index<lines.size();++index) {
    if(softInput.cursor>=lines[index].start) current=index;
  }

  const size_t rows=size_t(std::max<int32_t>(1,field.height/std::max<int32_t>(1,height)));
  const size_t first=current>=rows?current-rows+1:0;
  const size_t index=std::min(lines.size()-1,
    first+size_t(std::max<int32_t>(0,(py-field.y)/std::max<int32_t>(1,height))));
  const auto &line=lines[index];
  size_t position=line.start;
  int32_t x=field.x;

  while(position<line.end) {
    size_t next=position;
    const uint16_t code=nextCode(softInput.draft.c_str(),next,line.end);
    const int32_t advance=inputGlyphAdvance(style,code,true);

    if(px<x+(advance+1)/2) break;

    x+=advance;
    position=next;
  }

  if(position!=softInput.cursor) {
    softInput.cursor=position;
    invalidateKeyboardField();
  }

  return true;
}
