#pragma once
#include "../../../Display/Transport.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace nice_gfx {
namespace uc81xx {

// Original protocol implementation from UltraChip register descriptions; see
// docs/epaper-uc-sources.md. Only monochrome, OTP-waveform, full updates are
// supported. The caller owns reset, power sequencing and active-low BUSY_N.
// Call configuration after hardware reset and the required reset recovery.
// Transport command()/data() must consume their payload before returning.

inline Status configureSmall(Transport &bus, uint16_t width, uint16_t height) {
  if (!((width == 104 && height == 212) || (width == 128 && height == 296) ||
        (width == 152 && height == 152)))
    return Status::InvalidArgument;
  // Documented UC8151D defaults and the I9FC/T5D OTP operating flows.
  const uint8_t booster[] = {0x17, 0x17, 0x17};
  Status result = bus.command(0x06, booster, sizeof(booster));
  if (result != Status::Ok) return result;
  // UC8151D PSR: 128x296 base resolution, OTP LUT, KW, forward source/gate.
  // The 152x152 module specifies PSR=0x1f. TRES overrides either base size.
  const uint8_t panel = width == 152 ? 0x1f : 0x9f;
  result = bus.command(0x00, &panel, 1);
  if (result != Status::Ok) return result;
  const uint8_t resolution[] = {uint8_t(width), uint8_t(height >> 8), uint8_t(height)};
  result = bus.command(0x61, resolution, sizeof(resolution));
  if (result != Status::Ok) return result;
  const uint8_t origin[] = {0, 0, 0};
  result = bus.command(0x65, origin, sizeof(origin));
  if (result != Status::Ok) return result;
  // VBD=11 (floating), DDX=11 (NEW only, 0=black/1=white), CDI=7.
  const uint8_t interval = 0xf7;
  return bus.command(0x50, &interval, 1);
}

inline Status configure(Transport &bus, uint16_t width, uint16_t height, bool large = false) {
  if (large ? !((width == 648 || width == 800) && height == 480)
            : !(width == 400 && height == 300))
    return Status::InvalidArgument;
  if (!large) {
    // GDEW042T2 OTP operating flow specifies these booster timings.
    const uint8_t booster[] = {0x17, 0x17, 0x17};
    const Status result = bus.command(0x06, booster, sizeof(booster));
    if (result != Status::Ok) return result;
  }
  // UC8176/UC8179: OTP LUT, monochrome, default scan, booster/reset enabled.
  const uint8_t panel = 0x1f;
  Status result = bus.command(0x00, &panel, 1);
  if (result != Status::Ok) return result;
  const uint8_t resolution[] = {uint8_t(width >> 8), uint8_t(width),
                                uint8_t(height >> 8), uint8_t(height)};
  result = bus.command(0x61, resolution, sizeof(resolution));
  if (result != Status::Ok) return result;
  const uint8_t origin[] = {0, 0, 0, 0};
  result = bus.command(0x65, origin, sizeof(origin));
  if (result != Status::Ok) return result;
  // DDX=11 selects the full-update NEW-only polarity: 0=black, 1=white.
  // UC8179 separates CDI into a second byte; its default border is LUTBD.
  const uint8_t interval[] = {large ? uint8_t(0x33) : uint8_t(0xf7), 0x07};
  return bus.command(0x50, interval, large ? 2 : 1);
}

inline Status startOld(Transport &bus) { return bus.command(0x10, nullptr, 0); }
inline Status startNew(Transport &bus) { return bus.command(0x13, nullptr, 0); }

// Optional synchronous full-frame helper. NULL oldPixels emits white in bounded
// chunks; the configured NEW-only waveform does not depend on the old plane.
// The caller must match bytes to the dimensions it successfully configured.
inline Status writeFrame(Transport &bus, const uint8_t *oldPixels,
                         const uint8_t *newPixels, size_t bytes) {
  if (!newPixels || !(bytes == 2756 || bytes == 2888 || bytes == 4736 || bytes == 15000 ||
                       bytes == 38880 || bytes == 48000))
    return Status::InvalidArgument;
  Status result;
  if (oldPixels) {
    result = bus.command(0x10, oldPixels, bytes);
  } else {
    result = startOld(bus);
    if (result != Status::Ok) return result;
    uint8_t white[256];
    memset(white, 0xff, sizeof(white));
    for (size_t remaining = bytes; remaining; ) {
      const size_t count = remaining < sizeof(white) ? remaining : sizeof(white);
      result = bus.data(white, count);
      if (result != Status::Ok) return result;
      remaining -= count;
    }
  }
  return result == Status::Ok ? bus.command(0x13, newPixels, bytes) : result;
}

inline Status powerOn(Transport &bus) { return bus.command(0x04, nullptr, 0); }
inline Status refresh(Transport &bus) { return bus.command(0x12, nullptr, 0); }
inline Status powerOff(Transport &bus) { return bus.command(0x02, nullptr, 0); }
inline Status sleep(Transport &bus) {
  const uint8_t key = 0xa5;
  return bus.command(0x07, &key, 1);
}

} // namespace uc81xx
} // namespace nice_gfx
