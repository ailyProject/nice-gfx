#pragma once
#include "../Compiled.h"
#include "../../Display/Transport.h"

namespace nice_gfx { namespace htmlui { namespace detail {

// The caller owns the complete Native frame and an immutable cache source.
// Keep the original full-picture preflight (including whole-buffer alias
// checks), then expose only the copied full-stride rows to the PPA driver.
// This bounds driver cache maintenance without changing pixel coordinates.
template<class Copier>
inline Status copySystemModalNativeRows(Copier &copier,const NativeFramebuffer &target,
                                        PixelView source,size_t sourceBytes,Rect roi) {
  if(roi.x<0||roi.y<0||roi.width<=0||roi.height<=0||
     !copier.canCopyIntoNative(target,roi.x,roi.y,source,sourceBytes,
                               size_t(roi.x),size_t(roi.y),size_t(roi.width),size_t(roi.height)))return Status::Unsupported;
  // The original preflight proved both full strides/heights, address extents,
  // ROI containment and non-overlap before any subview pointer is formed.
  const size_t outOffset=size_t(roi.y)*target.stride,inOffset=size_t(roi.y)*source.stride*2;
  NativeFramebuffer rows=target;rows.pixels+=outOffset;rows.height=size_t(roi.height);rows.bytes-=outOffset*2;
  PixelView input(source.data+inOffset,source.width,size_t(roi.height),source.stride,source.order);
  const size_t inputBytes=source.stride*size_t(roi.height)*2;
  const Status status=copier.copyIntoNative(rows,roi.x,0,input,inputBytes,size_t(roi.x),0,size_t(roi.width),size_t(roi.height));
  if(status!=Status::Unsupported)return status;
  // A cropped row can fail output alignment even though the complete picture
  // is supported. Preserve the existing safe whole-picture copy in that case.
  return copier.copyIntoNative(target,roi.x,roi.y,source,sourceBytes,
                               size_t(roi.x),size_t(roi.y),size_t(roi.width),size_t(roi.height));
}

// Included inside nice_gfx::htmlui::detail by the isolated preparation script.
// Preserve the full-picture preflight before forming either translated view.
// A copied panel strip can start at a different source and output row.
template<class Copier>
inline Status copySystemModalTranslatedNativeRows(Copier &copier,const NativeFramebuffer &target,
    PixelView source,size_t sourceBytes,Rect destination,size_t sourceX,size_t sourceY){
  if(destination.x<0||destination.y<0||destination.width<=0||destination.height<=0||
     !copier.canCopyIntoNative(target,destination.x,destination.y,source,sourceBytes,
       sourceX,sourceY,size_t(destination.width),size_t(destination.height)))return Status::Unsupported;
  // The original validation proved complete capacities, stride products,
  // address extents, source/destination containment and whole-buffer aliasing.
  const size_t outputOffset=size_t(destination.y)*target.stride,inputOffset=sourceY*source.stride*2;
  const auto original=[&](){return copier.copyIntoNative(target,destination.x,destination.y,source,sourceBytes,
    sourceX,sourceY,size_t(destination.width),size_t(destination.height));};
  // Raw cache images use a 64-byte-aligned allocation. Preserve that source
  // base's cache-line phase; awkward source row strides keep the original
  // supported picture instead of presenting a newly unaligned DMA base.
  if(inputOffset&63u)return original();
  NativeFramebuffer rows=target;rows.pixels+=outputOffset;rows.height=size_t(destination.height);rows.bytes-=outputOffset*2;
  PixelView input(source.data+inputOffset,source.width,size_t(destination.height),source.stride,source.order);
  const size_t inputBytes=source.stride*size_t(destination.height)*2;
  const Status status=copier.copyIntoNative(rows,destination.x,0,input,inputBytes,
    sourceX,0,size_t(destination.width),size_t(destination.height));
  // Cropped output alignment may reject an otherwise-supported complete frame.
  // Unsupported promises no partial writes; real driver errors propagate once.
  return status==Status::Unsupported?original():status;
}

}}} // namespace nice_gfx::htmlui::detail
