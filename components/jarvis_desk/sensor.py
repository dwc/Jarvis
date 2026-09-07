import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import CONF_NAME, STATE_CLASS_MEASUREMENT

from . import empty_dict_if_none, platform_schema, register_entities

DEPENDENCIES = ["jarvis_desk"]
CODEOWNERS = ["@dwc"]

ICON_PRESET = "mdi:arrow-expand-vertical"
ICON_LIMIT_MIN = "mdi:arrow-collapse-down"
ICON_LIMIT_MAX = "mdi:arrow-collapse-up"


def _height_sensor_schema(default_name, icon):
    # No unit_of_measurement default: the desk reports either mm or inch
    # depending on the "units" select, so the unit is not knowable at compile
    # time. The user may still set unit_of_measurement explicitly.
    return cv.All(
        empty_dict_if_none,
        sensor.sensor_schema(
            icon=icon,
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ).extend({cv.Optional(CONF_NAME, default=default_name): cv.string_strict}),
    )


# key in YAML -> its entity schema. The JarvisDesk setter is derived from the
# key, since SUB_SENSOR(preset_1) generates set_preset_1_sensor().
SENSORS = {
    "preset_1": _height_sensor_schema("Preset 1 Position", ICON_PRESET),
    "preset_2": _height_sensor_schema("Preset 2 Position", ICON_PRESET),
    "preset_3": _height_sensor_schema("Preset 3 Position", ICON_PRESET),
    "preset_4": _height_sensor_schema("Preset 4 Position", ICON_PRESET),
    "user_limit_min": _height_sensor_schema("User Limit (minimum)", ICON_LIMIT_MIN),
    "user_limit_max": _height_sensor_schema("User Limit (maximum)", ICON_LIMIT_MAX),
    "sys_limit_min": _height_sensor_schema("System Limit (minimum)", ICON_LIMIT_MIN),
    "sys_limit_max": _height_sensor_schema("System Limit (maximum)", ICON_LIMIT_MAX),
}

CONFIG_SCHEMA = platform_schema(SENSORS)


async def to_code(config):
    async def build(key, conf, entry, parent):
        var = await sensor.new_sensor(conf)
        cg.add(getattr(parent, f"set_{key}_sensor")(var))
        return var

    await register_entities(config, SENSORS, build)
