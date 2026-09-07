#pragma once

#include "serial_device.h"

namespace esphome {
namespace jarvis_desk {

/// The handset end of the man-in-the-middle link.
///
/// Everything the control box sends is forwarded to the handset unchanged; the
/// only state kept here is the last height the desk reported, which the hub
/// needs for its move hysteresis correction.
class HandsetHandler : public SerialDevice {
 public:
  HandsetHandler() : SerialDevice(SourceType::Handset) {}

  void handle_cb_message(SerialMessage &msg);
  uint16_t get_last_reported_height() const { return this->last_reported_height_; }

 protected:
  uint16_t last_reported_height_{0};
};

}  // namespace jarvis_desk
}  // namespace esphome
