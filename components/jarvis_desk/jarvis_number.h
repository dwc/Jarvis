#pragma once

#include "esphome/components/number/number.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace jarvis_desk {

class JarvisDesk;

/// Which JarvisDesk action a JarvisNumber drives. Kept in sync with
/// number.py (JarvisNumberAction) via jarvis_desk_ns.enum().
enum class JarvisNumberAction : uint8_t {
  HEIGHT = 0,
  OFFSET = 1,
};

/// A single number entity that forwards writes to the JarvisDesk hub.
///
/// One class parameterised by JarvisNumberAction rather than one subclass per
/// entity. HEIGHT is not optimistic (the hub publishes the confirmed height);
/// OFFSET is optimistic and publishes its own state, which
/// matches the behaviour of the old template numbers.
class JarvisNumber : public number::Number, public Parented<JarvisDesk> {
 public:
  void set_action(JarvisNumberAction action) { this->action_ = action; }

 protected:
  void control(float value) override;

  JarvisNumberAction action_{JarvisNumberAction::HEIGHT};
};

}  // namespace jarvis_desk
}  // namespace esphome
