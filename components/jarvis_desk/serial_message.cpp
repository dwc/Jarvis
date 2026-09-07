#include "serial_message.h"

#include <cstring>

#include "esphome/core/helpers.h"

namespace esphome {
namespace jarvis_desk {

SerialMessage::SerialMessage(CommandFromControlboxType cmd) : SerialMessage(SourceType::Controlbox, cmd, 0) {}

SerialMessage::SerialMessage(CommandFromControlboxType cmd, uint8_t p0)
    : SerialMessage(SourceType::Controlbox, cmd, 1, p0) {}

SerialMessage::SerialMessage(CommandFromControlboxType cmd, uint8_t p0, uint8_t p1)
    : SerialMessage(SourceType::Controlbox, cmd, 2, p0, p1) {}

SerialMessage::SerialMessage(CommandFromControlboxType cmd, uint16_t p01)
    : SerialMessage(SourceType::Controlbox, cmd, 2, first_byte(p01), second_byte(p01)) {}

SerialMessage::SerialMessage(CommandFromHandsetType cmd) : SerialMessage(SourceType::Handset, cmd, 0) {}

SerialMessage::SerialMessage(CommandFromHandsetType cmd, uint8_t p0) : SerialMessage(SourceType::Handset, cmd, 1, p0) {}

SerialMessage::SerialMessage(CommandFromHandsetType cmd, uint8_t p0, uint8_t p1)
    : SerialMessage(SourceType::Handset, cmd, 2, p0, p1) {}

SerialMessage::SerialMessage(CommandFromHandsetType cmd, uint16_t p01)
    : SerialMessage(SourceType::Handset, cmd, 2, first_byte(p01), second_byte(p01)) {}

SerialMessage::SerialMessage(uint8_t source_type, uint8_t command_type, size_t param_size, uint8_t p0, uint8_t p1,
                             uint8_t p2, uint8_t p3)
    : source_id_(source_type), type_(command_type), param_size_(static_cast<uint8_t>(param_size)) {
  this->params_[0] = p0;
  this->params_[1] = p1;
  this->params_[2] = p2;
  this->params_[3] = p3;
}

void SerialMessage::format_to(char *buffer, size_t buffer_size) const {
  uint8_t packet[MAX_PACKET_SIZE];
  this->construct(packet);
  format_hex_pretty_to(buffer, buffer_size, packet, this->get_packet_length(), ' ');
}

bool SerialMessage::set_param_n(uint8_t value, size_t n) {
  bool is_index_valid = n < MAX_PARAM_SIZE;
  if (is_index_valid) {
    this->params_[n] = value;
  }
  return is_index_valid;
}

bool SerialMessage::set_param(uint8_t p0) {
  this->param_size_ = 1;
  return this->set_param_n(p0, 0);
}

bool SerialMessage::set_param(uint8_t p0, uint8_t p1) {
  this->param_size_ = 2;
  return this->set_param_n(p0, 0) && this->set_param_n(p1, 1);
}

bool SerialMessage::set_param(uint16_t p01) {
  this->param_size_ = 2;
  return this->set_param(first_byte(p01), second_byte(p01));
}

uint8_t SerialMessage::compute_checksum(uint8_t command, uint8_t param_size, const uint8_t *params) {
  uint8_t checksum = (command + param_size) % 256;
  for (int i = 0; i < param_size; ++i)
    checksum = (checksum + params[i]) % 256;
  return checksum;
}

void SerialMessage::construct(uint8_t *data) const {
  data[0] = data[1] = this->source_id_;
  data[2] = this->type_;
  data[3] = this->param_size_;
  data[4 + this->param_size_] = compute_checksum(this->type_, this->param_size_, this->params_);
  data[5 + this->param_size_] = PACKET_END;

  memcpy(data + 4, this->params_, this->param_size_);
}

bool SerialMessage::set_packet(const uint8_t *data, size_t data_size) {
  if (!verify_packet(data, data_size)) {
    return false;
  }

  this->source_id_ = data[0];
  this->type_ = data[2];
  this->param_size_ = data[3];

  memcpy(this->params_, data + 4, this->param_size_);

  return true;
}

bool SerialMessage::verify_packet(const uint8_t *data, size_t data_size) {
  uint8_t param_size = data[3];
  if (param_size > MAX_PARAM_SIZE) {
    return false;
  }

  // compute_checksum() reads the parameters in place, so there is no need to
  // copy them out first.
  return data[0] == data[1] && (data[0] == SourceType::Handset || data[0] == SourceType::Controlbox) &&
         data_size == static_cast<size_t>(MIN_PACKET_SIZE + param_size) &&
         data[data_size - 2] == compute_checksum(data[2], data[3], data + 4) && data[data_size - 1] == PACKET_END;
}

}  // namespace jarvis_desk
}  // namespace esphome
