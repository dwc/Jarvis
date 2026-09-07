#pragma once

#include "esphome/components/button/button.h"
#include "esphome/core/helpers.h"

#include "serial_message.h"

namespace esphome {
namespace jarvis_desk {

class JarvisDesk;

/// A button entity that sends one fixed handset command.
///
/// Every Jarvis button is the same three facts -- which command, an optional
/// parameter byte, and whether the settings block needs re-reading afterwards --
/// so codegen fills those in from button.py's table rather than each button
/// getting its own class or its own branch of a switch.
class JarvisButton : public button::Button, public Parented<JarvisDesk> {
 public:
  void set_command(CommandFromHandsetType command) { this->command_ = command; }
  void set_param(uint8_t param) {
    this->param_ = param;
    this->has_param_ = true;
  }
  void set_refresh_settings(bool refresh) { this->refresh_settings_ = refresh; }

 protected:
  void press_action() override;

  CommandFromHandsetType command_{};
  uint8_t param_{0};
  bool has_param_{false};
  bool refresh_settings_{false};
};

}  // namespace jarvis_desk
}  // namespace esphome
