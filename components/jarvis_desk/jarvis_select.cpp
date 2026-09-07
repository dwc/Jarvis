#include "jarvis_select.h"

#include "esphome/core/log.h"

#include "jarvis_desk.h"

namespace esphome {
namespace jarvis_desk {

static const char *const TAG = "jarvis_desk.select";

void JarvisSelect::control(const std::string &value) {
  if (this->parent_ == nullptr) {
    // Should be impossible: register_entities() wires every writable entity to
    // the hub. Guarded anyway because getting it wrong is a null dereference,
    // which on an ESP8266 is a hard crash rather than a bad value.
    ESP_LOGE(TAG, "No parent jarvis_desk hub configured");
    return;
  }

  ESP_LOGD(TAG, "Requesting '%s'", value.c_str());

  switch (this->action_) {
    case JarvisSelectAction::UNITS:
      this->parent_->set_units(value);
      break;
    case JarvisSelectAction::TOUCH_MODE:
      this->parent_->set_touch_mode(value);
      break;
    case JarvisSelectAction::KILL_MODE:
      this->parent_->set_kill_mode(value);
      break;
    case JarvisSelectAction::SENSITIVITY:
      this->parent_->set_sensitivity(value);
      break;
  }
  // Not optimistic: the hub publishes the state once the desk confirms it.
}

}  // namespace jarvis_desk
}  // namespace esphome
