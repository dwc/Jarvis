#pragma once

#include <string>

#include "esphome/components/select/select.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace jarvis_desk {

class JarvisDesk;

/// Which JarvisDesk setting a JarvisSelect drives. Kept in sync with
/// select.py (JarvisSelectAction) via jarvis_desk_ns.enum().
enum class JarvisSelectAction : uint8_t {
  UNITS = 0,
  TOUCH_MODE = 1,
  KILL_MODE = 2,
  SENSITIVITY = 3,
};

/// A single select entity that forwards writes to the JarvisDesk hub.
///
/// None of these are optimistic: control() only sends the request, and the hub
/// publishes the new state once the desk confirms the setting.
class JarvisSelect : public select::Select, public Parented<JarvisDesk> {
 public:
  void set_action(JarvisSelectAction action) { this->action_ = action; }

 protected:
  void control(const std::string &value) override;

  JarvisSelectAction action_{JarvisSelectAction::UNITS};
};

}  // namespace jarvis_desk
}  // namespace esphome
