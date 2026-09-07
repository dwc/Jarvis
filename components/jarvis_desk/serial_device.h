#pragma once

#include "esphome/components/uart/uart.h"

#include "serial_message.h"

namespace esphome {
namespace jarvis_desk {

/// One end of the handset <-> control box serial link.
///
/// The byte level protocol state machine below is a faithful port of the original
/// SerialDevice implementation; the only change is that the Arduino Print/Serial
/// plumbing has been replaced by esphome's uart::UARTDevice.
class SerialDevice : public uart::UARTDevice {
 public:
  explicit SerialDevice(uint8_t id) : id_(id) {}

  void send_message(const SerialMessage &msg, uint8_t repetition = 1);

  bool fetch_message(SerialMessage &msg);

 protected:
  enum class StateMachineState {
    START,
    ID,
    COMMAND,
    PARAM_SIZE,
    PARAM,
    CHECKSUM,
    END,
  };

  /// Pump the UART until a complete, checksum-valid frame sits in
  /// partial_message_. Returns false when no full frame is available yet.
  bool fetch_next_command_();
  bool process_data_(uint8_t oktet);

  /// Emit a verbose hex dump of the frame being dropped.
  void log_discard_(const char *reason, uint8_t oktet);

  uint8_t id_;
  /// Parameter bytes still expected while in StateMachineState::PARAM.
  uint8_t params_remaining_{0};
  uint8_t partial_message_[MAX_PACKET_SIZE]{};
  size_t pm_size_{0};
  StateMachineState sm_state_{StateMachineState::START};
};

}  // namespace jarvis_desk
}  // namespace esphome
