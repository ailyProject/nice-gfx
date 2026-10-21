// Included inside View::Impl after Surface/state comparison helpers.
struct CompressedSurface {
  const PageDesc *page=nullptr;
  RetainedVector<State> key;
  FrameCache geometry;
  size_t offset=0,words=0;
  // Published streams are immutable until this record is replaced/cleared.
  // A successful structural validation can therefore be reused by later
  // animation frames; record reset also resets this memoized pixel count.
  mutable size_t verifiedPixels=0;
  uint64_t used=0;
};
std::vector<CompressedSurface> compressedSurfaces; // Exactly project->count records.
uint16_t *compressedStorage=nullptr;
size_t compressedCapacity=0,compressedUsed=0;
struct CompressedBuilder {
  Surface *surface=nullptr;
  const PageDesc *page=nullptr;
  const uint16_t *pixels=nullptr;
  uint64_t revision=0;
  size_t words=0,offset=0;
  rle565::Encoder encoder;
  // 0=count, 1=reserve output, 2=encode. No key or pixel allocation.
  uint8_t phase=0;
};
CompressedBuilder compressedBuilder;

void updateCompressedStats(){
  surfaceStats.compressedPages=surfaceStats.compressedBytes=0;
  surfaceStats.compressedCapacityBytes=compressedCapacity*sizeof(uint16_t);
  for(const auto &entry:compressedSurfaces)if(entry.page&&entry.words){++surfaceStats.compressedPages;surfaceStats.compressedBytes+=entry.words*sizeof(uint16_t);}
}
CompressedSurface *compressedSurfaceFor(const PageDesc &p){
  for(auto &entry:compressedSurfaces)if(entry.page==&p&&entry.words)return &entry;
  return nullptr;
}
bool compressedSurfaceReady(int index)const{
  if(index<0||!project||size_t(index)>=pages.size()||pages[size_t(index)].empty())return false;
  for(const auto &entry:compressedSurfaces)if(entry.page==&project->pages[index]&&entry.words&&sameSurfaceState(entry.key,pages[size_t(index)]))return true;
  return false;
}
void clearCompressedPage(const PageDesc *page){
  if(compressedBuilder.page==page)compressedBuilder=CompressedBuilder{};
  for(auto &entry:compressedSurfaces)if(entry.page==page)entry=CompressedSurface{};
  updateCompressedStats();
}
void clearCompressedSurfaces(){
  compressedBuilder=CompressedBuilder{};
  std::vector<CompressedSurface>().swap(compressedSurfaces);
  compressedStorage=nullptr;compressedCapacity=compressedUsed=0;
}
void compactCompressedSurfaces(){
  // In-place stable compaction. Pick the next original offset without an
  // allocation or a temporary page. Published streams never overlap.
  size_t next=0,write=0;
  for(;;){CompressedSurface *chosen=nullptr;
    for(auto &entry:compressedSurfaces)if(entry.page&&entry.words&&entry.offset>=next&&(!chosen||entry.offset<chosen->offset))chosen=&entry;
    if(!chosen)break;
    const size_t old=chosen->offset;next=old+chosen->words;
    if(old!=write)memmove(compressedStorage+write,compressedStorage+old,chosen->words*sizeof(uint16_t));
    chosen->offset=write;write+=chosen->words;
  }
  compressedUsed=write;
}
bool beginCompressedSurface(Surface &surface){
  if(!compressedStorage||surface.compressionAttempted||!surface.page||!surface.ready)return false;
  surface.compressionAttempted=true;
  // A cold key must not extend the lifetime of a full RAM map/media image.
  // Ineligible or incompressible keys are tried once, until their pixels change.
  for(const auto &value:surface.key)if(value.imageOverride){clearCompressedPage(surface.page);++surfaceStats.compressedRejected;return false;}
  if(auto *existing=compressedSurfaceFor(*surface.page))if(sameSurfaceState(existing->key,surface.key)){existing->used=++surfaceClock;return true;}
  const size_t index=size_t(surface.page-project->pages);
  if(index>=compressedSurfaces.size()){++surfaceStats.compressedRejected;return false;}
  compressedBuilder=CompressedBuilder{};
  auto &job=compressedBuilder;job.surface=&surface;job.page=surface.page;
  job.pixels=surface.pixels;job.revision=surface.pixelsRevision;
  job.encoder.begin(surface.pixels,size_t(surface.page->width)*surface.page->height,nullptr,compressedCapacity);
  return true;
}
bool compressedBuilderValid()const{
  const auto &job=compressedBuilder;
  return job.surface&&job.surface->ready&&job.surface->page==job.page&&
    job.surface->pixels==job.pixels&&job.surface->pixelsRevision==job.revision;
}
bool reserveCompressedOutput(){
  auto &job=compressedBuilder;
  if(job.words<=compressedCapacity-compressedUsed){job.offset=compressedUsed;return true;}
  // Compaction moves published streams. Keep that occasional maintenance out
  // of animation ticks, and never compact when the unused tail already fits.
  if(animationsRunning()||motionsPending())return false;
  size_t live=0;for(const auto &entry:compressedSurfaces)if(entry.page&&entry.words)live+=entry.words;
  while(job.words>compressedCapacity-live){
    CompressedSurface *oldest=nullptr;
    for(auto &entry:compressedSurfaces)if(entry.page&&entry.words&&(!oldest||entry.used<oldest->used))oldest=&entry;
    if(!oldest)return false;
    live-=oldest->words;*oldest=CompressedSurface{};++surfaceStats.compressedEvictions;
  }
  compactCompressedSurfaces();updateCompressedStats();job.offset=compressedUsed;return true;
}
void publishCompressedSurface(){
  auto &job=compressedBuilder;auto &surface=*job.surface;
  const size_t index=size_t(job.page-project->pages);auto &record=compressedSurfaces[index];
  record=CompressedSurface{};
  record.key=surface.key;record.geometry=surface.geometry;
  for(auto &value:record.key)value.text.owned.reset();
  record.offset=job.offset;record.words=job.words;record.used=++surfaceClock;record.page=job.page;
  compressedUsed=job.offset+job.words;compressedBuilder=CompressedBuilder{};
  ++surfaceStats.compressedStores;updateCompressedStats();
}
bool stepCompressedSurfaces(){
  if(!compressedStorage||refresh->busy())return false;
  if(compressedBuilder.surface&&!compressedBuilderValid())compressedBuilder=CompressedBuilder{};
  if(!compressedBuilder.surface){
    for(auto &entry:surfaces)if(entry.ready&&!entry.compressionAttempted){beginCompressedSurface(entry);if(!compressedBuilder.surface)return true;break;}
  }
  if(!compressedBuilder.surface)return false;
  size_t budget=8192;
  auto &job=compressedBuilder;
  for(;;){
    if(job.phase==1){
      if(!reserveCompressedOutput())return true;
      job.encoder.begin(job.pixels,size_t(job.page->width)*job.page->height,compressedStorage+job.offset,job.words);
      job.phase=2;
    }
    budget-=job.encoder.step(budget);
    if(job.encoder.failed){compressedBuilder=CompressedBuilder{};++surfaceStats.compressedRejected;return true;}
    if(!job.encoder.complete())return true;
    if(job.phase==2){
      if(job.encoder.words==job.words&&compressedBuilderValid())publishCompressedSurface();
      else {compressedBuilder=CompressedBuilder{};++surfaceStats.compressedRejected;}
      return true;
    }
    job.words=job.encoder.words;job.phase=1;
    // Do not add a second page pass when this tick used its whole budget.
    if(!budget)return true;
  }
}
Surface *restoreSurfacePage(const PageDesc &p,const PageDesc *excludePageA=nullptr,const PageDesc *excludePageB=nullptr){
  if(!project||!compressedStorage||refresh->busy())return nullptr;
  for(auto &entry:surfaces)if(entry.page==&p&&entry.ready)return &entry;
  auto *record=compressedSurfaceFor(p);if(!record)return nullptr;
  const size_t pixels=size_t(p.width)*p.height;
  if(record->offset>compressedCapacity||record->words>compressedCapacity-record->offset||
     !rle565::validate(compressedStorage+record->offset,record->words,pixels)){
    clearCompressedPage(&p);++surfaceStats.compressedRejected;return nullptr;
  }
  Surface *slot=nullptr;
  for(auto &entry:surfaces){
    if(&entry==snapshotSurface||&entry==previewSurface||&entry==snapshotAnimationSurface||
       (entry.page&&(entry.page==excludePageA||entry.page==excludePageB)))continue;
    if(entry.page==&p){slot=&entry;break;}
    if(!slot||!entry.page||(slot->page&&entry.used<slot->used))slot=&entry;
    if(!entry.page)break;
  }
  if(!slot)return nullptr;
  // Preserve the compressed key exactly. The ordinary cache hit, atlas guard
  // and patch paths still decide whether these terminal pixels match live state.
  ++slot->pixelsRevision;slot->ready=false;slot->page=nullptr;slot->key=record->key;slot->geometry=record->geometry;
  if(!rle565::decode(compressedStorage+record->offset,record->words,slot->pixels,pixels))return nullptr;
  slot->page=&p;slot->row=p.height;slot->column=0;slot->paused=false;slot->ready=true;slot->compressionAttempted=true;
  record->used=slot->used=++surfaceClock;++surfaceStats.compressedRestores;return slot;
}
