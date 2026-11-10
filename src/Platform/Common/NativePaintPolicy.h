#pragma once
// Included only by RenderBackend when explicitly enabled. The transport keeps
// using its original hardware Context; this policy belongs to the UI instance.
namespace nice_gfx { namespace platform { namespace render_backend {
class Context : public DeviceContext {
 public:
  class CpuOnlyNativePaint {
   public:
    CpuOnlyNativePaint(Context &context,bool active):context_(active?&context:nullptr){
      if(context_){previous_=context_->cpuOnlyDepth_;context_->cpuOnlyDepth_=previous_==UINT32_MAX?UINT32_MAX:previous_+1;
#ifdef NICE_GFX_NATIVE_PAINT_SCOPE_TRACE
        NICE_GFX_NATIVE_PAINT_SCOPE_TRACE(true);
#endif
      }
    }
    ~CpuOnlyNativePaint() noexcept {if(context_){
#ifdef NICE_GFX_NATIVE_PAINT_SCOPE_TRACE
      NICE_GFX_NATIVE_PAINT_SCOPE_TRACE(false);
#endif
      context_->cpuOnlyDepth_=previous_;}}
    CpuOnlyNativePaint(const CpuOnlyNativePaint&)=delete;CpuOnlyNativePaint&operator=(const CpuOnlyNativePaint&)=delete;
   private:Context *context_;uint32_t previous_=0;
  };
  bool cpuOnlyNativePaint()const{return cpuOnlyDepth_!=0;}
  uint64_t suppressedNativeOperations()const{return suppressed_;}
  bool blend(uint16_t *a,size_t b,const uint16_t*c,size_t d,size_t e,size_t f,uint8_t g){if(suppressed())return false;return DeviceContext::blend(a,b,c,d,e,f,g);}
  const uint16_t *blendSourcesToWire(const uint16_t*a,size_t b,const uint16_t*c,size_t d,size_t e,size_t f,uint8_t g){if(suppressed())return nullptr;return DeviceContext::blendSourcesToWire(a,b,c,d,e,f,g);}
  const uint16_t *blendSourcesToNative(const uint16_t*a,size_t b,const uint16_t*c,size_t d,size_t e,size_t f,uint8_t g){if(suppressed())return nullptr;return DeviceContext::blendSourcesToNative(a,b,c,d,e,f,g);}
  const uint16_t *blendSourceRegionsToNative(const uint16_t*a,size_t b,const uint16_t*c,size_t d,size_t e,size_t f,size_t g,size_t h,size_t i,uint8_t j){if(suppressed())return nullptr;return DeviceContext::blendSourceRegionsToNative(a,b,c,d,e,f,g,h,i,j);}
  bool blendSourcesIntoNative(uint16_t*a,size_t b,size_t c,const uint16_t*d,size_t e,const uint16_t*f,size_t g,size_t h,size_t i,uint8_t j){if(suppressed())return false;return DeviceContext::blendSourcesIntoNative(a,b,c,d,e,f,g,h,i,j);}
  bool blendSolidIntoNative(uint16_t*a,size_t b,size_t c,const uint16_t*d,size_t e,size_t f,size_t g,uint16_t h,uint8_t i){if(suppressed())return false;return DeviceContext::blendSolidIntoNative(a,b,c,d,e,f,g,h,i);}
  Status blendSolidNativeIntoNative(uint16_t*a,size_t b,size_t c,const uint16_t*d,size_t e,size_t f,size_t g,uint16_t h,uint8_t i){if(suppressed())return Status::Unsupported;return DeviceContext::blendSolidNativeIntoNative(a,b,c,d,e,f,g,h,i);}
  bool canCopyIntoNative(const NativeFramebuffer&a,int32_t b,int32_t c,PixelView d,size_t e,size_t f,size_t g,size_t h,size_t i)const{if(suppressed())return false;return DeviceContext::canCopyIntoNative(a,b,c,d,e,f,g,h,i);}
  bool canBeginCopyIntoNative(const NativeFramebuffer &target,int32_t x,int32_t y,PixelView source,
      size_t sourceBytes,size_t sourceX,size_t sourceY,size_t width,size_t height)const{
    if(suppressed())return false;
    return DeviceContext::canBeginCopyIntoNative(target,x,y,source,sourceBytes,sourceX,sourceY,width,height);
  }
  Status beginCopyIntoNative(const NativeFramebuffer &target,int32_t x,int32_t y,PixelView source,
      size_t sourceBytes,size_t sourceX,size_t sourceY,size_t width,size_t height){
    if(suppressed())return Status::Unsupported;
    return DeviceContext::beginCopyIntoNative(target,x,y,source,sourceBytes,sourceX,sourceY,width,height);
  }
  // Preserve the real drain for a copy accepted before entering this scope.
  Status finishCopyIntoNative(){return DeviceContext::finishCopyIntoNative();}
  Status copyIntoNative(const NativeFramebuffer&a,int32_t b,int32_t c,PixelView d,size_t e,size_t f,size_t g,size_t h,size_t i){if(suppressed())return Status::Unsupported;return DeviceContext::copyIntoNative(a,b,c,d,e,f,g,h,i);}
  Status copyIntoWire(const NativeFramebuffer&a,int32_t b,int32_t c,PixelView d,size_t e,size_t f,size_t g,size_t h,size_t i){if(suppressed())return Status::Unsupported;return DeviceContext::copyIntoWire(a,b,c,d,e,f,g,h,i);}
  Status fillIntoNative(const NativeFramebuffer&a,const DirtyRect&b,uint16_t c){if(suppressed())return Status::Unsupported;return DeviceContext::fillIntoNative(a,b,c);}
  Status blendColorIntoNative(const NativeFramebuffer&a,const DirtyRect&b,uint16_t c,uint8_t d){if(suppressed())return Status::Unsupported;return DeviceContext::blendColorIntoNative(a,b,c,d);}
  bool canBlendOverlayIntoNative(const NativeFramebuffer&a,int32_t b,int32_t c,const OverlayView&d,size_t e,size_t f,size_t g,size_t h)const{if(suppressed())return false;return DeviceContext::canBlendOverlayIntoNative(a,b,c,d,e,f,g,h);}
  Status blendOverlayIntoNative(const NativeFramebuffer&a,int32_t b,int32_t c,const OverlayView&d,size_t e,size_t f,size_t g,size_t h){if(suppressed())return Status::Unsupported;return DeviceContext::blendOverlayIntoNative(a,b,c,d,e,f,g,h);}
  bool close(){if(suppressed())return false;return DeviceContext::close();}
 private:
  bool suppressed()const{if(!cpuOnlyDepth_)return false;++suppressed_;return true;}
  uint32_t cpuOnlyDepth_=0;mutable uint64_t suppressed_=0;
};
}}}
