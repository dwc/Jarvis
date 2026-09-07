#include "jarvis_number.h"

#include <cmath>

#include "esphome/core/log.h"

#include "jarvis_desk.h"

namespace esphome {
namespace jarvis_desk {

static const char *const TAG = "jarvis_desk.number";

void JarvisNumber::control(float value) {
  if (this->parent_ == nullptr) {
    // Should be impossible: register_entities() wires every writable entity to
    // the hub. Guarded anyway because getting it wrong is a null dereference,
    // which on an ESP8266 is a hard crash rather than a bad value.
    ESP_LOGE(TAG, "No parent jarvis_desk hub configured");
    return;
  }

  switch (this->action_) {
    case JarvisNumberAction::HEIGHT:
      // The handset protocol works in tenths of the displayed unit.
      ESP_LOGD(TAG, "Moving to height %.0f", value);
      this->parent_->move(static_cast<uint16_t>(lroundf(value * 10.0f)));
      // Not optimistic: the hub publishes the height once the desk confirms it.
      break;

    case JarvisNumberAction::OFFSET: {
      const uint16_t raw = static_cast<uint16_t>(lroundf(value));
      ESP_LOGD(TAG, "Setting offset to %u", raw);
      this->parent_->set_offset(raw);
      this->publish_state(value);
      break;
    }
  }
}

}  // namespace jarvis_desk
}  // namespace esphome
