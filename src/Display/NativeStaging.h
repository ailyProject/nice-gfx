#pragma once
#include "../Core/Status.h"
#include "../Core/Pixel.h"
#ifndef NICE_GFX_UI_NATIVE_STAGE
#define NICE_GFX_UI_NATIVE_STAGE 0
#endif
#ifndef NICE_GFX_P4_NATIVE_STAGE_PROVIDER
#define NICE_GFX_P4_NATIVE_STAGE_PROVIDER 0
#endif
// Diagnostic mode preserves C66 full poison/digest/padding and per-geometry
// qualification. Production explicitly selects 0 and relies on the fixed SDK
// identity-copy contract for legal per-operation geometry, not sampled coverage.
#ifndef NICE_GFX_NATIVE_STAGE_DIAGNOSTIC_VALIDATION
#define NICE_GFX_NATIVE_STAGE_DIAGNOSTIC_VALIDATION 1
#endif
namespace nice_gfx {
using NativeStageClock=uint32_t(*)();
struct NativeStageSource {uint16_t*allocation=nullptr;size_t allocationBytes=0;PixelView pixels{nullptr,0};uint64_t owner=0,snapshot=0;NativeStageClock clock=nullptr;};
struct NativeStageProviderStats {uint64_t invocations=0,callbacks=0,coldTransfers=0,coldWords=0,sourceWords=0,nativePixels=0,verifiedNativeWords=0,timeouts=0,errors=0,clockReads=0,cacheMicros=0,driverMicros=0,coldMicros=0;
#if NICE_GFX_P4_NATIVE_STAGE_PROVIDER
 uint64_t cacheBytes=0,smallStripeFallbacks=0,qualificationCount=0;
#endif
uint64_t areaBuckets[6]{};bool pending=false,disabled=false;};
// Host prototype boundary. Production transports remain unavailable until a
// separately reviewed owned SRM implementation supplies the same contract.
struct NativeStageTransfer {Status status=Status::Unsupported;bool accepted=false,completed=false;};
// accepted means MAY own any lease, including SDK invoke followed by a non-OK
// result whose completion is uncertain. Unsupported is legal only pre-invoke.
// completed=true is valid only together with accepted=true for the same sole
// outstanding operation: its generation-matched real callback has drained all
// SDK leases. Only Status::Ok also certifies successful checked post-cache
// coherence. IoError+completed permits abort/destruction, never publication or
// implicit reuse/retry; the View remains fatal. Old receipts are forbidden.
// A provider must retain PixelView by
// value, never its temporary argument address. Errors/timeouts still drain.
// copy/drain are noexcept: after submission no allocation or throwing callback
// may escape and bypass the lease drain. The provider must preserve this contract in both validation modes.
}
