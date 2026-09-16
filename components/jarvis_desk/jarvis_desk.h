#pragma once

#include <cmath>
#include <string>

#include "esphome/components/number/number.h"
#include "esphome/components/select/select.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

#include "handset_handler.h"
#include "serial_device.h"
#include "serial_message.h"
#include "desk_settings.h"

namespace esphome {
namespace jarvis_desk {

/// Raw encoder values the control box reports at its lowest and highest
/// positions. Only RAW_MIN is used directly; RAW_MAX documents where the scale
/// factor below comes from.
static const uint16_t SYS_LIMIT_RAW_MIN = 0x13F5;
static const uint16_t SYS_LIMIT_RAW_MAX = 0x3B80;

/// Millimetres per raw encoder count:
///   (sys_limit_max - sys_limit_min) / (SYS_LIMIT_RAW_MAX - SYS_LIMIT_RAW_MIN)
///   = 649 / 10123
static const float MM_PER_RAW_COUNT = 0.0642f;
/// Millimetres to the tenths-of-an-inch the handset displays.
static const float MM_TO_TENTH_INCH = 0.393f;

/// The control box needs roughly this long to answer each settings request.
static const uint32_t SETTINGS_STEP_DELAY_MS = 70;
/// Minimum gap between wake requests forwarded to the control box. See
/// JarvisDesk::handle_handset() for why repeats must not all be passed on.
static const uint32_t WAKE_FORWARD_INTERVAL_MS = 100;
/// How often to retry until the whole settings block has been read back.
static const uint32_t SETTINGS_RETRY_INTERVAL_MS = 10000;

struct Settings {
  /// Convert a raw encoder reading into the unit the desk is currently set to.
  uint16_t raw_to_height(uint16_t raw) const {
    uint16_t height_mm = this->sys_limit_min +
                         std::round(static_cast<float>(static_cast<int>(raw) - SYS_LIMIT_RAW_MIN) * MM_PER_RAW_COUNT);
    return this->units == UnitsValue::MM
               ? height_mm
               : static_cast<uint16_t>(std::round(static_cast<float>(height_mm) * MM_TO_TENTH_INCH));
  }

  uint16_t get_preset_1() const { return this->raw_to_height(this->preset_raw_1); }
  uint16_t get_preset_2() const { return this->raw_to_height(this->preset_raw_2); }
  uint16_t get_preset_3() const { return this->raw_to_height(this->preset_raw_3); }
  uint16_t get_preset_4() const { return this->raw_to_height(this->preset_raw_4); }

  // Conventional
  uint16_t preset_raw_1{0};
  uint16_t preset_raw_2{0};
  uint16_t preset_raw_3{0};
  uint16_t preset_raw_4{0};

  UnitsValue units{UnitsValue::UNKNOWN};
  TouchModeValue touch_mode{TouchModeValue::UNKNOWN};
  KillModeValue kill_mode{KillModeValue::UNKNOWN};
  SensitivityValue sensitivity{SensitivityValue::SENSITIVITY_UNKNOWN};

  // Extended
  uint16_t sys_limit_min{0};  // converted
  uint16_t sys_limit_max{0};  // converted

  UserLimitSetValue user_limit_set{UserLimitSetValue::UNKNOWN};
  uint16_t user_limit_min{0};
  uint16_t user_limit_max{0};

  /// True once every setting has been read back from the control box.
  bool is_complete() const {
    return this->preset_raw_1 && this->preset_raw_2 && this->preset_raw_3 && this->preset_raw_4 &&
           this->units != UnitsValue::UNKNOWN && this->touch_mode != TouchModeValue::UNKNOWN &&
           this->kill_mode != KillModeValue::UNKNOWN && this->sensitivity != SensitivityValue::SENSITIVITY_UNKNOWN &&
           this->sys_limit_min && this->sys_limit_max && this->user_limit_set != UserLimitSetValue::UNKNOWN &&
           this->user_limit_min && this->user_limit_max;
  }
};

class JarvisDesk : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // -- codegen setters -----------------------------------------------------
  void set_handset_uart(uart::UARTComponent *uart) { this->handset_.set_uart_parent(uart); }
  void set_controlbox_uart(uart::UARTComponent *uart) { this->controlbox_.set_uart_parent(uart); }

  // -- actions (called by the number/select/button entities) ----------------
  void move(uint16_t height);
  void set_offset(uint16_t offset);

  void set_units(const std::string &value);
  void set_touch_mode(const std::string &value);
  void set_kill_mode(const std::string &value);
  void set_sensitivity(const std::string &value);

  /// Send one fixed handset command on behalf of a button entity.
  ///
  /// Every button is just (command, optional parameter, whether the settings
  /// need re-reading afterwards), so they share this one entry point instead of
  /// twelve near-identical methods.
  void send_command(CommandFromHandsetType cmd, uint8_t param, bool has_param, bool refresh_settings);

  // -- protocol plumbing ---------------------------------------------------
  void handle_handset();
  void handle_controlbox();

  void send_message(const SerialMessage &msg, uint8_t reps = 1);

  void wake_up();
  /// Re-read the whole settings block from the control box.
  ///
  /// The control box needs roughly 70 ms between requests, so the three
  /// commands are spread across loop() iterations rather than blocking on
  /// each other. Calling this while a sequence is running queues one more
  /// pass, so a change is always followed by a complete read-back.
  void request_all_settings();
  void extract_setting(const SerialMessage &msg);

  SUB_SENSOR(preset_1)
  SUB_SENSOR(preset_2)
  SUB_SENSOR(preset_3)
  SUB_SENSOR(preset_4)
  SUB_SENSOR(sys_limit_min)
  SUB_SENSOR(sys_limit_max)
  SUB_SENSOR(user_limit_min)
  SUB_SENSOR(user_limit_max)
  SUB_TEXT_SENSOR(user_limit_set)
  SUB_NUMBER(height)
  SUB_SELECT(units)
  SUB_SELECT(touch_mode)
  SUB_SELECT(kill_mode)
  SUB_SELECT(sensitivity)

 protected:
  /// Which command of the settings read-back sequence goes out next.
  enum class SettingsStep : uint8_t {
    IDLE,
    ABS_LIMITS,
    SETTINGS,
    SETTLING,
  };

  /// Send a setting change, then re-read the settings block to pick up the
  /// value the desk actually accepted.
  void send_setting_(CommandFromHandsetType cmd, uint8_t value);

  void start_settings_request_();
  /// Wait out the control box's response window, then take the next step.
  void schedule_next_settings_step_();
  void advance_settings_request_();

  Settings settings_;
  /// Cached settings_.is_complete(), so loop() does not re-evaluate the whole
  /// conjunction on every iteration. Settings only ever become more complete.
  bool settings_complete_{false};

  HandsetHandler handset_;
  SerialDevice controlbox_{SourceType::Controlbox};

  SettingsStep settings_step_{SettingsStep::IDLE};
  bool settings_restart_pending_{false};

  uint32_t last_wake_forward_{0};
  uint16_t dropped_wakes_{0};
};

}  // namespace jarvis_desk
}  // namespace esphome
