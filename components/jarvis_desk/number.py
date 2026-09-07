import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import (
    CONF_MAX_VALUE,
    CONF_MIN_VALUE,
    CONF_NAME,
    CONF_STEP,
    ENTITY_CATEGORY_CONFIG,
)

from . import (
    empty_dict_if_none,
    jarvis_desk_ns,
    platform_schema,
    register_entities,
)

DEPENDENCIES = ["jarvis_desk"]
CODEOWNERS = ["@dwc"]

ICON_HEIGHT = "mdi:arrow-up-down"
ICON_OFFSET = "mdi:plus-minus-variant"

# The handset display accepts 1..1800; every one of these numbers is expressed
# in those display units, so they share the range.
DISPLAY_MIN = 1
DISPLAY_MAX = 1800

JarvisNumber = jarvis_desk_ns.class_("JarvisNumber", number.Number)
JarvisNumberAction = jarvis_desk_ns.enum("JarvisNumberAction", is_class=True)


def _number_schema(default_name, icon, default_step, entity_category=cv.UNDEFINED):
    return cv.All(
        empty_dict_if_none,
        number.number_schema(
            JarvisNumber, icon=icon, entity_category=entity_category
        ).extend(
            {
                cv.Optional(CONF_NAME, default=default_name): cv.string_strict,
                cv.Optional(CONF_MIN_VALUE, default=DISPLAY_MIN): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=DISPLAY_MAX): cv.float_,
                cv.Optional(CONF_STEP, default=default_step): cv.positive_float,
            }
        ),
    )


# key in YAML -> (schema, JarvisNumberAction enumerator, whether the hub needs a
# pointer back to this entity). Only "height" is non-optimistic, so only it is
# published by the hub; offset confirms itself in control().
NUMBERS = {
    "height": (
        _number_schema("Height", ICON_HEIGHT, 10),
        JarvisNumberAction.HEIGHT,
        True,
    ),
    "offset": (
        _number_schema("Offset", ICON_OFFSET, 1, ENTITY_CATEGORY_CONFIG),
        JarvisNumberAction.OFFSET,
        False,
    ),
}

CONFIG_SCHEMA = platform_schema({key: entry[0] for key, entry in NUMBERS.items()})


async def to_code(config):
    async def build(key, conf, entry, parent):
        _, action, hub_needs_pointer = entry
        var = await number.new_number(
            conf,
            min_value=conf[CONF_MIN_VALUE],
            max_value=conf[CONF_MAX_VALUE],
            step=conf[CONF_STEP],
        )
        cg.add(var.set_action(action))
        if hub_needs_pointer:
            cg.add(getattr(parent, f"set_{key}_number")(var))
        return var

    await register_entities(config, NUMBERS, build, parented=True)
