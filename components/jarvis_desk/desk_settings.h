#pragma once

/// The desk's setting vocabulary: the values the control box reports for each
/// setting, and the strings Home Assistant exchanges with us for them.

#include <cstddef>
#include <cstdint>
#include <string>

namespace esphome {
namespace jarvis_desk {

enum class UnitsValue : uint8_t {
  MM = 0x00,
  INCH = 0x01,
  UNKNOWN = 0xFF,
};

enum class UserLimitSetValue : uint8_t {
  NONE = 0x00,
  MAX = 0x01,
  MIN = 0x10,
  BOTH = 0x11,
  UNKNOWN = 0xFF,
};

/// Reported by the control box when a user limit is hit. Documented for
/// completeness; nothing consumes it yet.
enum class UserLimitReachedValue : uint8_t {
  MAX_REACHED = 0x01,
  MIN_REACHED = 0x02,
  UNKNOWN = 0xFF,
};

enum class TouchModeValue : uint8_t {
  SINGLE = 0x00,
  CONTINUOUS = 0x01,
  UNKNOWN = 0xFF,
};

enum class KillModeValue : uint8_t {
  KILL = 0x00,
  LET_LIVE = 0x01,
  UNKNOWN = 0xFF,
};

// NOTE: the enumerators are prefixed because HIGH and LOW are object-like
// macros defined by the Arduino framework headers.
enum class SensitivityValue : uint8_t {
  SENSITIVITY_HIGH = 0x01,
  SENSITIVITY_MEDIUM = 0x02,
  SENSITIVITY_LOW = 0x03,
  SENSITIVITY_UNKNOWN = 0xFF,
};

/// One option of a desk setting: the string Home Assistant exchanges with us,
/// and the byte the desk protocol uses for it.
///
/// These tables are the single source of truth for both directions of the
/// mapping, so a rename cannot desynchronise val_to_string() from
/// val_from_string(). The option *lists* are mirrored in select.py, which is
/// what Home Assistant validates against -- keep the spellings in step.
struct DeskOption {
  const char *name;
  uint8_t value;
};

/// Look a byte up in an option table. Returns nullptr when unknown.
const char *option_to_string(const DeskOption *options, size_t len, uint8_t value);
/// Look a string up in an option table. Returns false when unknown.
bool option_from_string(const DeskOption *options, size_t len, const std::string &name, uint8_t &out);

/// The strings below are the ones published to / accepted from the select and
/// text sensor entities. They intentionally match the option lists of the
/// legacy YAML configuration.
const char *val_to_string(UnitsValue units);
const char *val_to_string(UserLimitSetValue limits);
const char *val_to_string(TouchModeValue mode);
const char *val_to_string(KillModeValue mode);
const char *val_to_string(SensitivityValue sens);

bool val_from_string(const std::string &name, UnitsValue &out);
bool val_from_string(const std::string &name, TouchModeValue &out);
bool val_from_string(const std::string &name, KillModeValue &out);
bool val_from_string(const std::string &name, SensitivityValue &out);

}  // namespace jarvis_desk
}  // namespace esphome
