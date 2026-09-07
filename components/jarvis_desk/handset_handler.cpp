#include "handset_handler.h"

namespace esphome {
namespace jarvis_desk {

void HandsetHandler::handle_cb_message(SerialMessage &msg) {
  if (msg.get_type() == CommandFromControlboxType::Height)
    this->last_reported_height_ = msg.get_param<uint16_t>();

  this->send_message(msg);
}

}  // namespace jarvis_desk
}  // namespace esphome
