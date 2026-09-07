import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.const import CONF_NAME, ENTITY_CATEGORY_CONFIG

from . import (
    empty_dict_if_none,
    jarvis_desk_ns,
    platform_schema,
    register_entities,
)

DEPENDENCIES = ["jarvis_desk"]
CODEOWNERS = ["@dwc"]

ICON_UNITS = "mdi:ruler"
ICON_TOUCH_MODE = "mdi:gesture-tap"
ICON_KILL_MODE = "mdi:power-plug-off"
ICON_SENSITIVITY = "mdi:tune"

JarvisSelect = jarvis_desk_ns.class_("JarvisSelect", select.Select)
JarvisSelectAction = jarvis_desk_ns.enum("JarvisSelectAction", is_class=True)

# The option lists are fixed by the desk protocol, not user configurable.
# Spelling must match the DeskOption tables in desk_settings.cpp, which are what the
# firmware maps to and from the protocol bytes.
UNITS_OPTIONS = ["inch", "cm"]
TOUCH_MODE_OPTIONS = ["Continuous", "Single"]
KILL_MODE_OPTIONS = ["Kill", "LetLive"]
SENSITIVITY_OPTIONS = ["High", "Medium", "Low"]


def _select_schema(default_name, icon):
    return cv.All(
        empty_dict_if_none,
        select.select_schema(
            JarvisSelect, icon=icon, entity_category=ENTITY_CATEGORY_CONFIG
        ).extend({cv.Optional(CONF_NAME, default=default_name): cv.string_strict}),
    )


# key in YAML -> (schema, JarvisSelectAction enumerator, fixed options). The
# JarvisDesk setter is derived from the key, since SUB_SELECT(units) generates
# set_units_select().
SELECTS = {
    "units": (
        _select_schema("Units", ICON_UNITS),
        JarvisSelectAction.UNITS,
        UNITS_OPTIONS,
    ),
    "touch_mode": (
        _select_schema("Touch Mode", ICON_TOUCH_MODE),
        JarvisSelectAction.TOUCH_MODE,
        TOUCH_MODE_OPTIONS,
    ),
    "kill_mode": (
        _select_schema("Kill Mode", ICON_KILL_MODE),
        JarvisSelectAction.KILL_MODE,
        KILL_MODE_OPTIONS,
    ),
    "sensitivity": (
        _select_schema("Sensitivity", ICON_SENSITIVITY),
        JarvisSelectAction.SENSITIVITY,
        SENSITIVITY_OPTIONS,
    ),
}

CONFIG_SCHEMA = platform_schema({key: entry[0] for key, entry in SELECTS.items()})


async def to_code(config):
    async def build(key, conf, entry, parent):
        _, action, options = entry
        var = await select.new_select(conf, options=options)
        cg.add(var.set_action(action))
        cg.add(getattr(parent, f"set_{key}_select")(var))
        return var

    await register_entities(config, SELECTS, build, parented=True)
