// Included inside View::Impl. Only the actual open mapped Native display frame
// may use the transport's existing full-row copy. Cache capture and staging
// targets retain the independent PPA/canvas paths.
Status copySystemModalMappedRows(PixelView source,size_t sourceBytes,Rect destination,
                                 size_t sourceX,size_t sourceY){
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  if(cacheOrder!=PixelOrder::Native||renderOrder!=PixelOrder::Native||
     !nativeOverlayTarget||stagingNativeTarget||buildingSurface||nativeCacheRepairing||
     foregroundCapture||systemModalPanelBuilding||
     !canvas.targetsNativeFramebuffer(*nativeOverlayTarget))return Status::Unsupported;
  auto *backend=nice_gfx::detail::DisplayAccess::panel(display);
  const auto &target=*nativeOverlayTarget;
  if(!backend||!backend->frameOpen()||backend->busy()||backend->supportsNativePresentationPipeline()||
     destination.x||destination.y<0||destination.width<=0||destination.height<=0||sourceX||
     source.order!=PixelOrder::Native||target.stride!=target.width||
     source.stride!=source.width||source.width!=target.width||
     size_t(destination.width)!=target.width||backend->width()!=target.width||
     backend->height()!=target.height)return Status::Unsupported;
  // Prove the complete original allocations, strides, source/destination ROI,
  // integer extents and non-overlap before constructing a cropped source view.
  if(!ppaFade.canCopyIntoNative(target,0,destination.y,source,sourceBytes,
       0,sourceY,size_t(destination.width),size_t(destination.height)))return Status::Unsupported;
  const size_t offset=sourceY*source.stride*2;
  const PixelView rows(source.data+offset,source.width,size_t(destination.height),source.stride,PixelOrder::Native);
  if(!backend->canFrameBlit(rows))return Status::Unsupported;
  const Status copied=backend->flush(0,destination.y,rows,false);
  if(copied==Status::Busy||backend->busy()){
    // A backend may hint async ownership even for a blocking request. The open
    // frame makes finish a source fence, not a frame publication. Keep both
    // source leases alive until this accepted read has drained, including errors.
    const Status drained=backend->finish();
    if(copied==Status::Ok)return drained;
    // Busy need not certify that the copy was accepted. Fence it, but never
    // turn that uncertain dispatch into a successful completed copy.
    if(copied==Status::Busy)return drained==Status::Ok?copied:drained;
    // Unsupported must be untouched. An unexpected retained read cannot take
    // the PPA fallback after it may already have written the mapped target.
    if(copied==Status::Unsupported)return drained==Status::Ok?Status::IoError:drained;
  }
  return copied;
#else
  (void)source;(void)sourceBytes;(void)destination;(void)sourceX;(void)sourceY;
  return Status::Unsupported;
#endif
}
