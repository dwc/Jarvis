#include "jarvis_button.h"

#include "esphome/core/log.h"

#include "jarvis_desk.h"

namespace esphome {
namespace jarvis_desk {

static const char *const TAG = "jarvis_desk.button";

void JarvisButton::press_action() {
  if (this->parent_ == nullptr) {
    // Should be impossible: register_entities() wires every writable entity to
    // the hub. Guarded anyway because getting it wrong is a null dereference,
    // which on an ESP8266 is a hard crash rather than a bad value.
    ESP_LOGE(TAG, "No parent jarvis_desk hub configured");
    return;
  }

  ESP_LOGD(TAG, "Pressed '%s'", this->get_name().c_str());

  this->parent_->send_command(this->command_, this->param_, this->has_param_, this->refresh_settings_);
}

}  // namespace jarvis_desk
}  // namespace esphome
