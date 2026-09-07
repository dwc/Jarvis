import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.const import CONF_NAME

from . import empty_dict_if_none, platform_schema, register_entities

DEPENDENCIES = ["jarvis_desk"]
CODEOWNERS = ["@dwc"]

ICON_USER_LIMIT_SET = "mdi:arrow-expand-vertical"

# key in YAML -> its entity schema. The JarvisDesk setter is derived from the
# key, since SUB_TEXT_SENSOR(user_limit_set) generates
# set_user_limit_set_text_sensor().
TEXT_SENSORS = {
    "user_limit_set": cv.All(
        empty_dict_if_none,
        text_sensor.text_sensor_schema(icon=ICON_USER_LIMIT_SET).extend(
            {cv.Optional(CONF_NAME, default="User Limit"): cv.string_strict}
        ),
    ),
}

CONFIG_SCHEMA = platform_schema(TEXT_SENSORS)


async def to_code(config):
    async def build(key, conf, entry, parent):
        var = await text_sensor.new_text_sensor(conf)
        cg.add(getattr(parent, f"set_{key}_text_sensor")(var))
        return var

    await register_entities(config, TEXT_SENSORS, build)
