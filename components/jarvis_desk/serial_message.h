#pragma once

#include <cstddef>
#include <cstdint>

#include "esphome/core/helpers.h"

namespace esphome {
namespace jarvis_desk {

static const size_t MIN_PACKET_SIZE = 6;
static const size_t MAX_PARAM_SIZE = 4;
static const size_t MAX_PACKET_SIZE = MIN_PACKET_SIZE + MAX_PARAM_SIZE;

/// End-of-packet marker used by the Jarvis protocol.
static const uint8_t PACKET_END = 0x7E;

inline uint8_t first_byte(uint16_t value) { return static_cast<uint8_t>(value >> 8); }
inline uint8_t second_byte(uint16_t value) { return static_cast<uint8_t>(value); }

enum SourceType : uint8_t {
  Handset = 0xF1,
  Controlbox = 0xF2,
};

enum CommandFromControlboxType : uint8_t {
  Height = 0x01,
  Error = 0x02,
  Reset = 0x04,
  AbsLimits = 0x07,
  // Calibration and MovingTo share command 0x1B; the meaning depends on
  // context, so only one of the two names can appear in a switch.
  Calibration = 0x1B,
  MovingTo = 0x1B,
  Version = 0x1C,
  MinMaxSet = 0x20,
  MaxHeight = 0x21,
  MinHeight = 0x22,
  MinMaxReached = 0x23,
  Preset = 0x92,

  // Settings
  LocPreset1 = 0x25,
  LocPreset2 = 0x26,
  LocPreset3 = 0x27,
  LocPreset4 = 0x28,
  Units = 0x0E,
  TouchMode = 0x19,
  KillMode = 0x17,
  Sensitivity = 0x1D,
};

enum CommandFromHandsetType : uint8_t {
  Up = 0x01,
  Down = 0x02,
  SetPreset1 = 0x03,
  SetPreset2 = 0x04,
  SetPreset3 = 0x25,
  SetPreset4 = 0x26,
  MoveToPreset1 = 0x05,
  MoveToPreset2 = 0x06,
  MoveToPreset3 = 0x27,
  MoveToPreset4 = 0x28,
  GetSettings = 0x07,
  GetAbsLimits = 0x0C,
  SetUnits = 0x0E,
  SetOffset = 0x10,
  SetKillMode = 0x17,
  SetTouchMode = 0x19,
  MoveTo = 0x1B,
  GetVersion = 0x1C,  // unknown
  SetSensitivity = 0x1D,
  GetUserLimits = 0x20,
  SetMaxHeight = 0x21,
  SetMinHeight = 0x22,
  ClearMinMax = 0x23,
  Wake = 0x29,
  SetMoveToLoc = 0x80,
  EnterCalibration = 0x91,
};

class SerialMessage {
 public:
  SerialMessage() = default;

  SerialMessage(CommandFromHandsetType cmd);
  SerialMessage(CommandFromControlboxType cmd);

  SerialMessage(CommandFromHandsetType cmd, uint8_t p0);
  SerialMessage(CommandFromControlboxType cmd, uint8_t p0);

  SerialMessage(CommandFromHandsetType cmd, uint8_t p0, uint8_t p1);
  SerialMessage(CommandFromControlboxType cmd, uint8_t p0, uint8_t p1);

  SerialMessage(CommandFromHandsetType cmd, uint16_t p01);
  SerialMessage(CommandFromControlboxType cmd, uint16_t p01);

  SerialMessage(uint8_t source_type, uint8_t command_type, size_t param_size = 0, uint8_t p0 = 0, uint8_t p1 = 0,
                uint8_t p2 = 0, uint8_t p3 = 0);

  /// Buffer size format_to() needs.
  static constexpr size_t STRING_BUFFER_SIZE = format_hex_pretty_size(MAX_PACKET_SIZE);
  /// Render the packet as a hex string into `buffer`.
  ///
  /// Takes a caller supplied buffer rather than returning a std::string: the
  /// rendered packet exceeds the small-string optimisation, so returning one
  /// would heap allocate on every logged packet.
  void format_to(char *buffer, size_t buffer_size) const;

  uint8_t get_type() const { return this->type_; }

  size_t get_packet_length() const { return MIN_PACKET_SIZE + this->param_size_; }

  SourceType get_source_id() const { return static_cast<SourceType>(this->source_id_); }

  template<typename T> T get_param() const { return static_cast<T>(this->params_[0]); }

  /// Read a 16 bit parameter that does not start at offset 0.
  uint16_t get_param_u16(size_t index) const { return encode_uint16(this->params_[index], this->params_[index + 1]); }

  template<typename T> bool set_param_value(T value) { return this->set_param(static_cast<uint8_t>(value)); }

  bool set_param(uint8_t p0);
  bool set_param(uint8_t p0, uint8_t p1);
  bool set_param(uint16_t p01);


  void construct(uint8_t *data) const;
  bool set_packet(const uint8_t *data, size_t data_size);
  static bool verify_packet(const uint8_t *data, size_t data_size);

  static uint8_t compute_checksum(uint8_t command, uint8_t param_size, const uint8_t *params);

 protected:
  bool set_param_n(uint8_t value, size_t n);

  uint8_t source_id_{0};
  uint8_t type_{0};
  uint8_t param_size_{0};
  uint8_t params_[MAX_PARAM_SIZE]{0, 0, 0, 0};
};

template<> inline uint16_t SerialMessage::get_param<uint16_t>() const {
  return encode_uint16(this->params_[0], this->params_[1]);
}

}  // namespace jarvis_desk
}  // namespace esphome
