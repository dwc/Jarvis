#include "desk_settings.h"

namespace esphome {
namespace jarvis_desk {

namespace {

constexpr DeskOption UNITS[] = {
    {"inch", static_cast<uint8_t>(UnitsValue::INCH)},
    // The desk reports millimetres, but the handset and the original
    // configuration both label the metric option "cm".
    {"cm", static_cast<uint8_t>(UnitsValue::MM)},
};

constexpr DeskOption TOUCH_MODES[] = {
    {"Continuous", static_cast<uint8_t>(TouchModeValue::CONTINUOUS)},
    {"Single", static_cast<uint8_t>(TouchModeValue::SINGLE)},
};

constexpr DeskOption KILL_MODES[] = {
    {"Kill", static_cast<uint8_t>(KillModeValue::KILL)},
    {"LetLive", static_cast<uint8_t>(KillModeValue::LET_LIVE)},
};

constexpr DeskOption SENSITIVITIES[] = {
    {"High", static_cast<uint8_t>(SensitivityValue::SENSITIVITY_HIGH)},
    {"Medium", static_cast<uint8_t>(SensitivityValue::SENSITIVITY_MEDIUM)},
    {"Low", static_cast<uint8_t>(SensitivityValue::SENSITIVITY_LOW)},
};

constexpr DeskOption USER_LIMITS[] = {
    {"None", static_cast<uint8_t>(UserLimitSetValue::NONE)},
    {"Max", static_cast<uint8_t>(UserLimitSetValue::MAX)},
    {"Min", static_cast<uint8_t>(UserLimitSetValue::MIN)},
    {"Both", static_cast<uint8_t>(UserLimitSetValue::BOTH)},
};

constexpr const char *UNSET = "unset";

/// Resolve a table entry, falling back to "unset" for unknown bytes.
template<typename T, size_t N> const char *to_string(const DeskOption (&options)[N], T value) {
  const char *name = option_to_string(options, N, static_cast<uint8_t>(value));
  return name != nullptr ? name : UNSET;
}

template<typename T, size_t N> bool from_string(const DeskOption (&options)[N], const std::string &name, T &out) {
  uint8_t value = 0;
  if (!option_from_string(options, N, name, value))
    return false;
  out = static_cast<T>(value);
  return true;
}

}  // namespace

const char *option_to_string(const DeskOption *options, size_t len, uint8_t value) {
  for (size_t i = 0; i < len; i++) {
    if (options[i].value == value)
      return options[i].name;
  }
  return nullptr;
}

bool option_from_string(const DeskOption *options, size_t len, const std::string &name, uint8_t &out) {
  for (size_t i = 0; i < len; i++) {
    if (name == options[i].name) {
      out = options[i].value;
      return true;
    }
  }
  return false;
}

const char *val_to_string(UnitsValue units) { return to_string(UNITS, units); }
const char *val_to_string(UserLimitSetValue limits) { return to_string(USER_LIMITS, limits); }
const char *val_to_string(TouchModeValue mode) { return to_string(TOUCH_MODES, mode); }
const char *val_to_string(KillModeValue mode) { return to_string(KILL_MODES, mode); }
const char *val_to_string(SensitivityValue sens) { return to_string(SENSITIVITIES, sens); }

bool val_from_string(const std::string &name, UnitsValue &out) { return from_string(UNITS, name, out); }
bool val_from_string(const std::string &name, TouchModeValue &out) { return from_string(TOUCH_MODES, name, out); }
bool val_from_string(const std::string &name, KillModeValue &out) { return from_string(KILL_MODES, name, out); }
bool val_from_string(const std::string &name, SensitivityValue &out) { return from_string(SENSITIVITIES, name, out); }

}  // namespace jarvis_desk
}  // namespace esphome
