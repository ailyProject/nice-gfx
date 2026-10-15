#pragma once
#include "../../../Display/VideoControllerTypes.h"
#include "VideoControllerProfiles.h"

namespace nice_gfx {
namespace video {

inline Status validateProgram(VideoInitProgram program) {
  if (!program.commands && program.count) return Status::InvalidArgument;
  for (size_t i = 0; i < program.count; ++i)
    if (program.commands[i].dataBytes && !program.commands[i].data) return Status::InvalidArgument;
  return Status::Ok;
}

inline Status writeByte(VideoWrite write, void *context, uint8_t command, uint8_t data) {
  return write(command, &data, 1, context);
}

// Restore the actual chip command page, rather than assuming all chips use FF.
inline Status selectBase(VideoController controller, VideoWrite write, void *context) {
  const uint8_t st7701[] = {0x77,0x01,0x00,0x00,0x00};
  const uint8_t ili9881[] = {0x98,0x81,0x00};
  const uint8_t st7102[] = {0x71,0x02,0x00};
  const uint8_t st7123[] = {0x71,0x23,0x00};
  const uint8_t st7121[] = {0x71,0x21,0x00};
  const uint8_t st77916[] = {0x00,0x00};
  Status status = Status::Ok;
  switch (controller) {
    case VideoController::ST7701: return write(0xFF, st7701, sizeof(st7701), context);
    case VideoController::ILI9881C: return write(0xFF, ili9881, sizeof(ili9881), context);
    case VideoController::ST77922: return writeByte(write, context, 0xF0, 0);
    case VideoController::ST7102: return write(0x99, st7102, sizeof(st7102), context);
    case VideoController::ST7123: return write(0x60, st7123, sizeof(st7123), context);
    case VideoController::ST7121: return write(0x60, st7121, sizeof(st7121), context);
    case VideoController::JD9365: return writeByte(write, context, 0xE0, 0);
    case VideoController::JD9366: return writeByte(write, context, 0x30, 0);
    case VideoController::HI8561: return writeByte(write, context, 0xDE, 0);
    case VideoController::HX8399: return writeByte(write, context, 0xB9, 0);
    case VideoController::CO5300: return writeByte(write, context, 0xFE, 0);
    case VideoController::ST77916: return write(0xF0, st77916, sizeof(st77916), context);
    case VideoController::NV3052:
      status = writeByte(write, context, 0xFF, 0x30); if (status != Status::Ok) return status;
      status = writeByte(write, context, 0xFF, 0x52); if (status != Status::Ok) return status;
      return writeByte(write, context, 0xFF, 0);
    default: return Status::Ok;
  }
}

inline uint8_t colorMode(VideoController controller, VideoInterface interface, unsigned bits) {
  const uint8_t value = bits == 16 ? 5 : bits == 18 ? 6 : 7;
  if (controller == VideoController::ST77903) return value;
  if (interface == VideoInterface::RGB && (controller == VideoController::ST7701 || controller == VideoController::GC9503))
    return uint8_t(value << 4);
  return uint8_t(value | (value << 4));
}

// The matching command numbers in manufacturer banks are unrelated registers.
// Tracking pages is essential: ILI9881C page 3/4 and JD9365 page 1/4 contain 36/3A.
inline void updatePage(VideoController controller, const VideoInitCommand &command, bool &base) {
  if (!command.dataBytes) return;
  const uint8_t *data = command.data;
  switch (controller) {
    case VideoController::ST7701: if (command.command == 0xFF && command.dataBytes >= 5) base = !(data[4] & 0x10); break;
    case VideoController::ILI9881C: if (command.command == 0xFF && command.dataBytes >= 3) base = data[2] == 0; break;
    case VideoController::JD9365: if (command.command == 0xE0) base = data[0] == 0; break;
    case VideoController::ST77922: if (command.command == 0xF0) base = true; else if (command.command == 0xF1 || command.command == 0xF2) base = false; break;
    case VideoController::ST7102: if (command.command == 0x99 && command.dataBytes >= 3) base = data[2] == 0; break;
    case VideoController::NV3052: if (command.command == 0xFF) base = data[0] == 0; break;
    case VideoController::CO5300: if (command.command == 0xFE) base = data[0] == 0; break;
    case VideoController::JD9366: if (command.command == 0x30) base = data[0] == 0; break;
    case VideoController::HI8561: if (command.command == 0xDE) base = data[0] == 0; break;
    default: break;
  }
}

} // namespace video

inline Status videoRunProgram(VideoInitProgram program, VideoWrite write, VideoDelay delay, void *context) {
  Status status = video::validateProgram(program);
  if (status != Status::Ok) return status;
  if (!program.count) return Status::Ok;
  if (!write || !delay) return Status::InvalidArgument;
  for (size_t i = 0; i < program.count; ++i) {
    const VideoInitCommand &command = program.commands[i];
    status = write(command.command, command.data, command.dataBytes, context);
    if (status != Status::Ok) return status;
    if (command.delayMs) delay(command.delayMs, context);
  }
  return Status::Ok;
}

inline Status videoInitialize(VideoController controller, VideoInterface interface,
    unsigned bits, unsigned lanes, bool bgr, const VideoInitOverride &override,
    VideoWrite write, VideoDelay delay, void *context, unsigned width = 0, unsigned height = 0) {
  const VideoControllerProfile *profile = videoControllerProfile(controller, interface);
  if (!profile) return Status::Unsupported;
  const uint32_t bitMask = interface == VideoInterface::RGB ? profile->rgbBitsMask : profile->mipiBitsMask;
  if (bits >= 32 || !(bitMask & (uint32_t(1) << bits))) return Status::Unsupported;
  if (interface == VideoInterface::MIPI) {
    if (!lanes) lanes = profile->defaultLanes;
    if (lanes > 4 || !(profile->laneMask & (1u << lanes))) return Status::Unsupported;
  }
  if (bgr && !profile->colorOrderSupported) return Status::Unsupported;
  if (!override.commands && override.count) return Status::InvalidArgument;
  const bool custom = override.commands != nullptr;
  const VideoInitProgram program = custom ? VideoInitProgram(override.commands, override.count) : profile->program;
  Status status = video::validateProgram(program);
  if (status != Status::Ok) return status;
  if (profile->noCommand) return custom && program.count ? Status::Unsupported : Status::Ok;
  if (!write || !delay) return Status::InvalidArgument;
  if (controller == VideoController::ST77903 && (!width || !height || width % 8 || height % 2 || width > 2048 || height > 512)) return Status::InvalidArgument;
  if (controller == VideoController::CO5300 && bits == 24 && (!width || !height || (uint64_t(width) * height) % 8)) return Status::InvalidArgument;
  status = video::selectBase(controller, write, context); if (status != Status::Ok) return status;
  const uint8_t colmod = video::colorMode(controller, interface, bits);
  const uint8_t madctl = bgr ? 0x08 : 0x00;
  // Preserve special registers and initialization order from the chip protocol.
  if (controller == VideoController::ILI9881C) {
    const uint8_t page1[] = {0x98,0x81,0x01};
    status = write(0xFF, page1, sizeof(page1), context); if (status != Status::Ok) return status;
    status = video::writeByte(write, context, 0xB7, lanes == 2 ? 0x03 : 0x02); if (status != Status::Ok) return status;
    status = video::selectBase(controller, write, context); if (status != Status::Ok) return status;
    status = write(0x11, nullptr, 0, context); if (status != Status::Ok) return status; delay(120, context);
  } else if (controller == VideoController::EK79007) {
    status = video::writeByte(write, context, 0xB2, lanes == 2 ? 0x10 : 0); if (status != Status::Ok) return status;
  } else if (controller == VideoController::HX8399 || controller == VideoController::HX8394) {
    status = write(0x11, nullptr, 0, context); if (status != Status::Ok) return status; delay(120, context);
  } else if (controller == VideoController::ST77903) {
    for (unsigned i = 0; i < 3; ++i) {
      const uint8_t unlock[] = {0xC3,0x96,0xA5};
      status = video::writeByte(write, context, 0xF0, unlock[i]); if (status != Status::Ok) return status;
    }
    const uint8_t resolution[] = {uint8_t(height / 2 - 1), uint8_t(width / 8 - 1)};
    status = write(0xB6, resolution, sizeof(resolution), context); if (status != Status::Ok) return status;
  }
  // These controller profiles do not implement MADCTL color selection. Reject
  // BGR above instead of inventing a register write (ST712x / AXS15260D).
  if (profile->colorOrderSupported) {
    status = video::writeByte(write, context, controller == VideoController::GC9503 ? 0xB1 : 0x36,
        controller == VideoController::GC9503 ? uint8_t(0x10 | (bgr ? 0x20 : 0)) : madctl);
    if (status != Status::Ok) return status;
  }
  const bool hasColmod = controller != VideoController::EK79007 && controller != VideoController::ST7123 && controller != VideoController::ST7121 &&
      controller != VideoController::JD9165 && controller != VideoController::OTA7290B && controller != VideoController::JD9366 &&
      controller != VideoController::HI8561 && controller != VideoController::SD5207 && controller != VideoController::ST77916 && controller != VideoController::AXS15260D;
  if (hasColmod) { status = video::writeByte(write, context, 0x3A, colmod); if (status != Status::Ok) return status; }
  if (controller == VideoController::JD9365) {
    status = video::writeByte(write, context, 0x80, uint8_t((lanes - 1) < 2 ? lanes - 1 : 0x10 + lanes - 3)); if (status != Status::Ok) return status;
  } else if (controller == VideoController::HX8399) {
    const uint8_t unlock[] = {0xFF,0x83,0x99};
    status = write(0xB9, unlock, sizeof(unlock), context); if (status != Status::Ok) return status;
    status = video::writeByte(write, context, 0xBA, uint8_t((lanes - 1) < 2 ? lanes - 1 : 0x10 + lanes - 3)); if (status != Status::Ok) return status;
  } else if (controller == VideoController::HX8394) {
    status = video::writeByte(write, context, 0xBA, uint8_t(0x60 + lanes - 1)); if (status != Status::Ok) return status;
  }
  bool base = true;
  for (size_t i = 0; i < program.count; ++i) {
    const VideoInitCommand &command = program.commands[i];
    const uint8_t *data = command.data;
    uint8_t patched[128];
    // A custom module sequence remains authoritative, as documented in the
    // override contract. Defaults must agree with the selected pixel/lane mode.
    if (!custom && command.dataBytes && base && (command.command == 0x36 || command.command == 0x3A || (controller == VideoController::HX8394 && command.command == 0xBA))) {
      if (command.dataBytes > sizeof(patched)) return Status::InvalidArgument;
      memcpy(patched, data, command.dataBytes);
      if (command.command == 0x36 && profile->colorOrderSupported) patched[0] = uint8_t((patched[0] & ~0x08u) | madctl);
      if (command.command == 0x3A) {
        // HX8394's native video-mode sequence selects only the DPI nibble (50h).
        patched[0] = controller == VideoController::HX8394 ? uint8_t(colmod & 0x70) : colmod;
      }
      if (controller == VideoController::HX8394 && command.command == 0xBA) patched[0] = uint8_t(0x60 + lanes - 1);
      data = patched;
    } else if (controller == VideoController::AXS15260D && command.command == 0x3A && command.dataBytes) {
      // Apply the selected pixel format even in custom sequences. Use a local
      // copy to preserve the caller's const module data.
      if (command.dataBytes > sizeof(patched)) return Status::InvalidArgument;
      memcpy(patched, data, command.dataBytes); patched[0] = colmod; data = patched;
    }
    status = write(command.command, data, command.dataBytes, context);
    if (status != Status::Ok) return status;
    if (command.delayMs) delay(command.delayMs, context);
    if (controller == VideoController::ST7123 || controller == VideoController::ST7121) delay(5, context);
    video::updatePage(controller, command, base);
  }
  return Status::Ok;
}

inline Status videoReset(VideoController controller, VideoInterface interface, VideoWrite write, VideoDelay delay, void *context) {
  const VideoControllerProfile *profile = videoControllerProfile(controller, interface);
  if (!profile) return Status::Unsupported;
  if (profile->noCommand) return Status::Ok;
  if (!write || !delay) return Status::InvalidArgument;
  Status status = video::selectBase(controller, write, context); if (status != Status::Ok) return status;
  status = write(0x01, nullptr, 0, context);
  if (status == Status::Ok && profile->softResetMs) delay(profile->softResetMs, context);
  return status;
}

inline Status videoDisplay(VideoController controller, VideoInterface interface, bool on,
    VideoWrite write, VideoDelay delay, void *context) {
  const VideoControllerProfile *profile = videoControllerProfile(controller, interface);
  if (!profile) return Status::Unsupported;
  if (profile->noCommand) return Status::Unsupported; // Backend controls DISP GPIO.
  if (!write || !delay) return Status::InvalidArgument;
  Status status = video::selectBase(controller, write, context); if (status != Status::Ok) return status;
  // EK79007 has sleep controls, not standard Display OFF/ON. Datasheet pp23-25.
  status = write(controller == VideoController::EK79007 ? (on ? 0x11 : 0x10) : (on ? 0x29 : 0x28), nullptr, 0, context);
  if (status == Status::Ok && controller == VideoController::EK79007) delay(120, context);
  return status;
}

inline Status videoInvert(VideoController controller, VideoInterface interface, bool on, VideoWrite write, void *context) {
  const VideoControllerProfile *profile = videoControllerProfile(controller, interface);
  if (!profile || profile->noCommand) return Status::Unsupported;
  // These RGB-only source protocols provide no inversion command operation.
  // Do not infer standard DCS inversion from their unrelated register numbers.
  if (controller == VideoController::GC9503 || controller == VideoController::NV3052) return Status::Unsupported;
  if (!write) return Status::InvalidArgument;
  Status status = video::selectBase(controller, write, context); if (status != Status::Ok) return status;
  return write(on ? 0x21 : 0x20, nullptr, 0, context);
}
} // namespace nice_gfx
