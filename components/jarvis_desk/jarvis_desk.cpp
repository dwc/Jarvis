#include "jarvis_desk.h"

#include "esphome/core/log.h"

namespace esphome {
namespace jarvis_desk {

static const char *const TAG = "jarvis_desk";

/// Scheduler slot names, so a queued step cannot stack up on itself.
static const char *const SETTINGS_STEP_TIMER = "settings_step";
static const char *const SETTINGS_RETRY_TIMER = "settings_retry";

/// The desk accepts an offset 6 units below the value it reports.
static const uint16_t OFFSET_CORRECTION = 6;
/// Upward moves undershoot by 2 units without this nudge.
static const uint16_t MOVE_HYSTERESIS = 2;

namespace {

/// Log a forwarded packet. Compiles away entirely below VERY_VERBOSE, so the
/// render buffer never reaches the stack at normal log levels.
void log_packet(const char *direction, const SerialMessage &msg) {
#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERY_VERBOSE
  char buffer[SerialMessage::STRING_BUFFER_SIZE];
  msg.format_to(buffer, sizeof(buffer));
  ESP_LOGVV(TAG, "%s: %s", direction, buffer);
#else
  (void) direction;
  (void) msg;
#endif
}

/// Publish to an entity only if the user configured it. Every entity in this
/// component is optional, so each publish site would otherwise need its own
/// null check.
template<typename Entity, typename Value> void publish_if(Entity *entity, Value value) {
  if (entity != nullptr)
    entity->publish_state(value);
}

}  // namespace

void JarvisDesk::setup() {
  this->wake_up();

  // Give the control box a moment to wake, then keep asking for the settings
  // block until all of it has come back. Both waits go through the scheduler so
  // nothing here blocks the main loop.
  this->set_timeout(SETTINGS_STEP_DELAY_MS, [this]() {
    this->set_interval(SETTINGS_RETRY_TIMER, SETTINGS_RETRY_INTERVAL_MS, [this]() {
      if (this->settings_complete_ || this->settings_step_ != SettingsStep::IDLE)
        return;
      ESP_LOGI(TAG, "Requesting settings from controlbox...");
      this->request_all_settings();
    });
  });
}

void JarvisDesk::loop() {
  this->handle_handset();
  this->handle_controlbox();
}

void JarvisDesk::dump_config() {
  ESP_LOGCONFIG(TAG, "Jarvis Desk:");
  LOG_SENSOR("  ", "Preset 1", this->preset_1_sensor_);
  LOG_SENSOR("  ", "Preset 2", this->preset_2_sensor_);
  LOG_SENSOR("  ", "Preset 3", this->preset_3_sensor_);
  LOG_SENSOR("  ", "Preset 4", this->preset_4_sensor_);
  LOG_SENSOR("  ", "System limit (minimum)", this->sys_limit_min_sensor_);
  LOG_SENSOR("  ", "System limit (maximum)", this->sys_limit_max_sensor_);
  LOG_SENSOR("  ", "User limit (minimum)", this->user_limit_min_sensor_);
  LOG_SENSOR("  ", "User limit (maximum)", this->user_limit_max_sensor_);
  LOG_TEXT_SENSOR("  ", "User limit", this->user_limit_set_text_sensor_);
  LOG_NUMBER("  ", "Height", this->height_number_);
  LOG_SELECT("  ", "Units", this->units_select_);
  LOG_SELECT("  ", "Touch mode", this->touch_mode_select_);
  LOG_SELECT("  ", "Kill mode", this->kill_mode_select_);
  LOG_SELECT("  ", "Sensitivity", this->sensitivity_select_);
}

void JarvisDesk::handle_handset() {
  SerialMessage in_msg;
  if (!this->handset_.fetch_message(in_msg)) {
    return;
  }

  log_packet("Handset", in_msg);

  // On powerup make the handset stop sending wake commands.
  switch (in_msg.get_type()) {
    case CommandFromHandsetType::SetPreset1:
    case CommandFromHandsetType::SetPreset2:
    case CommandFromHandsetType::SetPreset3:
    case CommandFromHandsetType::SetPreset4:
    case CommandFromHandsetType::SetUnits:
    case CommandFromHandsetType::SetOffset:
    case CommandFromHandsetType::SetTouchMode:
    case CommandFromHandsetType::SetSensitivity:
    case CommandFromHandsetType::SetMaxHeight:
    case CommandFromHandsetType::SetMinHeight:
    case CommandFromHandsetType::ClearMinMax:
      this->send_message(in_msg);
      this->request_all_settings();
      break;
    default:
      this->send_message(in_msg);  // handset->controlbox uninterrupted
  }
}

void JarvisDesk::handle_controlbox() {
  SerialMessage in_msg;
  if (!this->controlbox_.fetch_message(in_msg)) {
    return;
  }

  log_packet("Controlbox", in_msg);

  this->extract_setting(in_msg);

  this->handset_.handle_cb_message(in_msg);
}

void JarvisDesk::extract_setting(const SerialMessage &msg) {
  switch (msg.get_type()) {
    case CommandFromControlboxType::LocPreset1:
      this->settings_.preset_raw_1 = msg.get_param<uint16_t>();
      break;
    case CommandFromControlboxType::LocPreset2:
      this->settings_.preset_raw_2 = msg.get_param<uint16_t>();
      break;
    case CommandFromControlboxType::LocPreset3:
      this->settings_.preset_raw_3 = msg.get_param<uint16_t>();
      break;
    case CommandFromControlboxType::LocPreset4:
      this->settings_.preset_raw_4 = msg.get_param<uint16_t>();
      break;
    case CommandFromControlboxType::Units:
      this->settings_.units = msg.get_param<UnitsValue>();
      break;
    case CommandFromControlboxType::TouchMode:
      this->settings_.touch_mode = msg.get_param<TouchModeValue>();
      break;
    case CommandFromControlboxType::KillMode:
      this->settings_.kill_mode = msg.get_param<KillModeValue>();
      break;
    case CommandFromControlboxType::Sensitivity: {
      // The sensitivity setting is the last one reported by the control box, so
      // this is where the complete settings block gets published.
      this->settings_.sensitivity = msg.get_param<SensitivityValue>();

      publish_if(this->preset_1_sensor_, this->settings_.get_preset_1());
      publish_if(this->preset_2_sensor_, this->settings_.get_preset_2());
      publish_if(this->preset_3_sensor_, this->settings_.get_preset_3());
      publish_if(this->preset_4_sensor_, this->settings_.get_preset_4());
      publish_if(this->units_select_, val_to_string(this->settings_.units));
      publish_if(this->touch_mode_select_, val_to_string(this->settings_.touch_mode));
      publish_if(this->kill_mode_select_, val_to_string(this->settings_.kill_mode));
      publish_if(this->sensitivity_select_, val_to_string(this->settings_.sensitivity));
      break;
    }
    case CommandFromControlboxType::MinMaxSet:
      this->settings_.user_limit_set = msg.get_param<UserLimitSetValue>();
      publish_if(this->user_limit_set_text_sensor_, val_to_string(this->settings_.user_limit_set));
      break;
    case CommandFromControlboxType::MinHeight:
      this->settings_.user_limit_min = msg.get_param<uint16_t>();
      publish_if(this->user_limit_min_sensor_, this->settings_.user_limit_min);
      break;
    case CommandFromControlboxType::MaxHeight:
      this->settings_.user_limit_max = msg.get_param<uint16_t>();
      publish_if(this->user_limit_max_sensor_, this->settings_.user_limit_max);
      break;
    case CommandFromControlboxType::AbsLimits: {
      // -1 = probably rounding error inside
      this->settings_.sys_limit_max = msg.get_param_u16(0) - 1;
      this->settings_.sys_limit_min = msg.get_param_u16(2) - 1;

      publish_if(this->sys_limit_min_sensor_, this->settings_.sys_limit_min);
      publish_if(this->sys_limit_max_sensor_, this->settings_.sys_limit_max);

      break;
    }
    case CommandFromControlboxType::Height:
      publish_if(this->height_number_, static_cast<float>(msg.get_param<uint16_t>()));
      break;
    default:
      break;
  }

  this->settings_complete_ = this->settings_.is_complete();
}

void JarvisDesk::send_message(const SerialMessage &msg, uint8_t reps) {
  switch (msg.get_source_id()) {
    case SourceType::Handset:
      this->controlbox_.send_message(msg, reps);
      break;
    case SourceType::Controlbox:
      this->handset_.send_message(msg, reps);
      break;
    default:
      break;
  }
}

void JarvisDesk::wake_up() { this->send_message(SerialMessage(CommandFromHandsetType::Wake)); }

void JarvisDesk::request_all_settings() {
  if (this->settings_step_ != SettingsStep::IDLE) {
    // A read-back is already in flight. Queue one more pass so the change that
    // triggered this call is still followed by a complete read.
    this->settings_restart_pending_ = true;
    return;
  }
  this->start_settings_request_();
}

void JarvisDesk::start_settings_request_() {
  this->send_message(SerialMessage(CommandFromHandsetType::GetUserLimits));
  this->settings_step_ = SettingsStep::ABS_LIMITS;
  this->schedule_next_settings_step_();
}

void JarvisDesk::schedule_next_settings_step_() {
  this->set_timeout(SETTINGS_STEP_TIMER, SETTINGS_STEP_DELAY_MS, [this]() { this->advance_settings_request_(); });
}

void JarvisDesk::advance_settings_request_() {
  switch (this->settings_step_) {
    case SettingsStep::ABS_LIMITS:
      this->send_message(SerialMessage(CommandFromHandsetType::GetAbsLimits));
      this->settings_step_ = SettingsStep::SETTINGS;
      this->schedule_next_settings_step_();
      break;

    case SettingsStep::SETTINGS:
      this->send_message(SerialMessage(CommandFromHandsetType::GetSettings));
      this->settings_step_ = SettingsStep::SETTLING;
      this->schedule_next_settings_step_();
      break;

    case SettingsStep::SETTLING:
      // The control box has had its final 70 ms to answer.
      this->settings_step_ = SettingsStep::IDLE;
      if (this->settings_restart_pending_) {
        this->settings_restart_pending_ = false;
        this->start_settings_request_();
      }
      break;

    case SettingsStep::IDLE:
      break;
  }
}

void JarvisDesk::set_offset(uint16_t offset) {
  this->send_message(
      SerialMessage(CommandFromHandsetType::SetOffset, static_cast<uint16_t>(offset - OFFSET_CORRECTION)));
}

void JarvisDesk::send_setting_(CommandFromHandsetType cmd, uint8_t value) {
  SerialMessage newcmd(cmd);
  newcmd.set_param_value<uint8_t>(value);
  this->send_message(newcmd);
  this->request_all_settings();
}

void JarvisDesk::set_units(const std::string &value) {
  UnitsValue v;
  if (!val_from_string(value, v)) {
    ESP_LOGW(TAG, "Unknown units value received: [%s]", value.c_str());
    return;
  }
  this->send_setting_(CommandFromHandsetType::SetUnits, static_cast<uint8_t>(v));
}

void JarvisDesk::set_touch_mode(const std::string &value) {
  TouchModeValue v;
  if (!val_from_string(value, v)) {
    ESP_LOGW(TAG, "Unknown touch mode value received: [%s]", value.c_str());
    return;
  }
  this->send_setting_(CommandFromHandsetType::SetTouchMode, static_cast<uint8_t>(v));
}

void JarvisDesk::set_kill_mode(const std::string &value) {
  KillModeValue v;
  if (!val_from_string(value, v)) {
    ESP_LOGW(TAG, "Unknown kill mode value received: [%s]", value.c_str());
    return;
  }
  this->send_setting_(CommandFromHandsetType::SetKillMode, static_cast<uint8_t>(v));
}

void JarvisDesk::set_sensitivity(const std::string &value) {
  SensitivityValue v;
  if (!val_from_string(value, v)) {
    ESP_LOGW(TAG, "Unknown sensitivity value received: [%s]", value.c_str());
    return;
  }
  this->send_setting_(CommandFromHandsetType::SetSensitivity, static_cast<uint8_t>(v));
}

void JarvisDesk::move(uint16_t height) {
  // Hysteresis correction
  if (height > this->handset_.get_last_reported_height())
    height += MOVE_HYSTERESIS;
  this->send_message(SerialMessage(CommandFromHandsetType::MoveTo, static_cast<uint16_t>(height)));
}

void JarvisDesk::send_command(CommandFromHandsetType cmd, uint8_t param, bool has_param, bool refresh_settings) {
  if (has_param) {
    this->send_message(SerialMessage(cmd, param));
  } else {
    this->send_message(SerialMessage(cmd));
  }

  if (refresh_settings)
    this->request_all_settings();
}

}  // namespace jarvis_desk
}  // namespace esphome
