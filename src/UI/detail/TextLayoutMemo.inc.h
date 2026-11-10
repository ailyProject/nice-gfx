#ifndef NICE_GFX_UI_TEXT_LAYOUT_MEMO
#define NICE_GFX_UI_TEXT_LAYOUT_MEMO 0
#endif
#if NICE_GFX_UI_TEXT_LAYOUT_MEMO
// Included inside View::Impl. Cache only two-line-or-shorter layout metadata,
// never text/image payloads or a rendered backdrop. All inputs to prepareText
// are represented; translation/opacity/clip do not change line breaking.
struct TextLayoutMemoKey {
  const char *text=nullptr,*compiled=nullptr;uint64_t revision=0;
  const nice_gfx::MonoFont *font=nullptr;
  int32_t width=0,height=0;Kind kind=Kind::Text;
  uint8_t fontSize=0;bool marquee=false,ellipsis=false;
  bool operator==(const TextLayoutMemoKey &b)const{
    return text==b.text&&compiled==b.compiled&&revision==b.revision&&font==b.font&&width==b.width&&height==b.height&&
      kind==b.kind&&fontSize==b.fontSize&&marquee==b.marquee&&ellipsis==b.ellipsis;
  }
};
struct TextLayoutMemoEntry {
  TextLayoutMemoKey key;TextLine lines[2]{};int32_t textY=0;uint8_t count=0,ellipsisCount=0;bool ready=false;
};
struct TextLayoutMemoPage {
  const PageDesc *page=nullptr;uint64_t used=0;std::vector<TextLayoutMemoEntry> entries;
};
TextLayoutMemoPage textLayoutMemoPages[2];uint64_t textLayoutMemoClock=0;
void clearTextLayoutMemoPage(const PageDesc *page){for(auto &p:textLayoutMemoPages)if(p.page==page){p.page=nullptr;p.used=0;std::vector<TextLayoutMemoEntry>().swap(p.entries);}}
void clearTextLayoutMemo(){for(auto &p:textLayoutMemoPages){p.page=nullptr;std::vector<TextLayoutMemoEntry>().swap(p.entries);}textLayoutMemoClock=0;}
size_t textLayoutMemoBytes()const{size_t bytes=0;for(const auto &p:textLayoutMemoPages)bytes+=p.entries.capacity()*sizeof(TextLayoutMemoEntry);return bytes;}
TextLayoutMemoEntry *textLayoutMemoEntry(const PageDesc &p,const NodeDesc &n){
  if(!p.count||p.count>512)return nullptr;
  const ptrdiff_t index=&n-p.nodes;if(index<0||size_t(index)>=p.count)return nullptr;
  TextLayoutMemoPage *slot=nullptr;for(auto &v:textLayoutMemoPages)if(v.page==&p){slot=&v;break;}
  if(!slot){slot=&textLayoutMemoPages[0];if(textLayoutMemoPages[1].used<slot->used)slot=&textLayoutMemoPages[1];
    slot->entries.assign(p.count,TextLayoutMemoEntry{});slot->page=&p;}
  if(slot->entries.size()!=p.count){slot->entries.assign(p.count,TextLayoutMemoEntry{});}
  slot->used=++textLayoutMemoClock;return &slot->entries[size_t(index)];
}
struct TextLayoutMemoGuard {
  Impl &self;FrameCache &cache;Geometry &g;TextLayoutMemoEntry *entry;TextLayoutMemoKey key;bool hit=false;
  TextLayoutMemoGuard(Impl &owner,const PageDesc &p,FrameCache &c,Geometry &geometry,const NodeDesc &n,const State &v,const Style &style,const Rect &r,const char *text)
      :self(owner),cache(c),g(geometry),entry(owner.textLayoutMemoEntry(p,n)){
    key.text=text;key.compiled=v.text.compiled;key.revision=v.text.revision;key.font=style.font;
    key.width=r.width;key.height=r.height;key.kind=v.kind;key.fontSize=style.fontSize;
    key.marquee=n.id==self.snapshotMarqueeId&&self.snapshotMarqueeId;
    key.ellipsis=self.menuMotionEnabled()&&self.menuMotionConfig.ellipsis;
  }
  bool reuse(){
    if(!entry||!entry->ready||!(entry->key==key))return false;
    g.lineFirst=uint32_t(cache.lines.size());g.lineCount=entry->count;g.textY=entry->textY;g.ellipsisCount=entry->ellipsisCount;
    cache.lines.insert(cache.lines.end(),entry->lines,entry->lines+entry->count);
    ++self.stats.textLayoutReuses;hit=true;return true;
  }
  ~TextLayoutMemoGuard(){
    if(!entry||hit)return;
    entry->ready=false;if(g.lineCount>2||size_t(g.lineFirst)+g.lineCount>cache.lines.size())return;
    entry->key=key;entry->count=uint8_t(g.lineCount);entry->textY=g.textY;entry->ellipsisCount=g.ellipsisCount;
    for(uint8_t i=0;i<entry->count;++i)entry->lines[i]=cache.lines[g.lineFirst+i];entry->ready=true;
  }
};
#endif
