#include "serial_device.h"

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace jarvis_desk {

static const char *const TAG = "jarvis_desk.serial";

void SerialDevice::send_message(const SerialMessage &msg, uint8_t repetition) {
  uint8_t packet[MAX_PACKET_SIZE];
  msg.construct(packet);

#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERY_VERBOSE
  char buffer[SerialMessage::STRING_BUFFER_SIZE];
  msg.format_to(buffer, sizeof(buffer));
  ESP_LOGVV(TAG, "Out (0x%02X): %s", this->id_, buffer);
#endif

  for (int i = 0; i < repetition; ++i)
    this->write_array(packet, msg.get_packet_length());
}

bool SerialDevice::fetch_message(SerialMessage &msg) {
  if (!this->fetch_next_command_())
    return false;

  return msg.set_packet(this->partial_message_, this->pm_size_);
}

bool SerialDevice::fetch_next_command_() {
  uint8_t oktet = 0;
  // available() must be checked first: read_byte() goes through
  // UARTComponent::check_read_timeout_(), which busy-waits 100 ms and logs an
  // error when the buffer is empty. An empty buffer is the normal case here,
  // since this runs on every loop().
  while (this->available() > 0 && this->read_byte(&oktet)) {
    if (this->process_data_(oktet))
      return true;
  }
  return false;
}

void SerialDevice::log_discard_(const char *reason, uint8_t oktet) {
#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE
  char buffer[format_hex_pretty_size(MAX_PACKET_SIZE)];
  format_hex_pretty_to(buffer, this->partial_message_, this->pm_size_, ' ');
  ESP_LOGV(TAG, "%s: %s -- faulty byte: 0x%02X", reason, buffer, oktet);
#endif
}

bool SerialDevice::process_data_(uint8_t oktet) {
  switch (this->sm_state_) {
    case StateMachineState::START: {
      this->pm_size_ = 0;
      this->partial_message_[this->pm_size_++] = oktet;
      if (oktet == this->id_) {
        this->sm_state_ = StateMachineState::ID;
      } else {
        this->sm_state_ = StateMachineState::START;
      }
      break;
    }
    case StateMachineState::ID: {
      if (oktet == this->id_) {
        this->partial_message_[this->pm_size_++] = oktet;
        this->sm_state_ = StateMachineState::COMMAND;
      } else {
        this->sm_state_ = StateMachineState::START;
      }
      break;
    }
    case StateMachineState::COMMAND: {
      this->partial_message_[this->pm_size_++] = oktet;
      this->sm_state_ = StateMachineState::PARAM_SIZE;
      break;
    }
    case StateMachineState::PARAM_SIZE: {
      this->partial_message_[this->pm_size_++] = oktet;
      if (oktet > MAX_PARAM_SIZE) {
        this->log_discard_("Discard in ParamSize", oktet);
        this->sm_state_ = StateMachineState::START;
      } else {
        this->params_remaining_ = oktet;
        this->sm_state_ = oktet == 0 ? StateMachineState::CHECKSUM : StateMachineState::PARAM;
      }
      break;
    }
    case StateMachineState::PARAM: {
      this->partial_message_[this->pm_size_++] = oktet;
      if (--this->params_remaining_ == 0)
        this->sm_state_ = StateMachineState::CHECKSUM;
      break;
    }
    case StateMachineState::CHECKSUM: {
      uint8_t cmd = this->partial_message_[2];
      uint8_t param_size = this->partial_message_[3];

      uint8_t chk = SerialMessage::compute_checksum(cmd, param_size, this->partial_message_ + 4);
      if (chk == oktet) {
        this->partial_message_[this->pm_size_++] = oktet;
        this->sm_state_ = StateMachineState::END;
      } else {
        this->log_discard_("Discard", oktet);
        this->sm_state_ = StateMachineState::START;
      }
      break;
    }
    case StateMachineState::END: {
      this->partial_message_[this->pm_size_++] = oktet;
      this->sm_state_ = StateMachineState::START;
      return oktet == PACKET_END;
    }
  }
  return false;
}

}  // namespace jarvis_desk
}  // namespace esphome
