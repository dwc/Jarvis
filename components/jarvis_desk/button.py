import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv
from esphome.const import CONF_NAME, ENTITY_CATEGORY_CONFIG

from . import (
    CommandFromHandsetType,
    empty_dict_if_none,
    jarvis_desk_ns,
    platform_schema,
    register_entities,
)

DEPENDENCIES = ["jarvis_desk"]
CODEOWNERS = ["@dwc"]

ICON_GO_PRESET = "mdi:arrow-decision"
ICON_SET_PRESET = "mdi:content-save-cog"
ICON_SET_MAX_HEIGHT = "mdi:arrow-collapse-up"
ICON_SET_MIN_HEIGHT = "mdi:arrow-collapse-down"
ICON_CLEAR_MAX_HEIGHT = "mdi:arrow-expand-up"
ICON_CLEAR_MIN_HEIGHT = "mdi:arrow-expand-down"

JarvisButton = jarvis_desk_ns.class_("JarvisButton", button.Button)


def _button_schema(default_name, icon, entity_category=cv.UNDEFINED):
    return cv.All(
        empty_dict_if_none,
        button.button_schema(
            JarvisButton, icon=icon, entity_category=entity_category
        ).extend({cv.Optional(CONF_NAME, default=default_name): cv.string_strict}),
    )


# Every button is one handset command, an optional parameter byte, and whether
# the settings block has to be re-read afterwards. Keeping that as data means a
# new button is one row here and no C++ change at all.
#
# key -> (schema, command, param or None, refresh settings)
BUTTONS = {
    "go_preset_1": (
        _button_schema("Go to Preset 1", ICON_GO_PRESET),
        CommandFromHandsetType.MoveToPreset1,
        None,
        False,
    ),
    "go_preset_2": (
        _button_schema("Go to Preset 2", ICON_GO_PRESET),
        CommandFromHandsetType.MoveToPreset2,
        None,
        False,
    ),
    "go_preset_3": (
        _button_schema("Go to Preset 3", ICON_GO_PRESET),
        CommandFromHandsetType.MoveToPreset3,
        None,
        False,
    ),
    "go_preset_4": (
        _button_schema("Go to Preset 4", ICON_GO_PRESET),
        CommandFromHandsetType.MoveToPreset4,
        None,
        False,
    ),
    "set_preset_1": (
        _button_schema("Set Preset 1", ICON_SET_PRESET, ENTITY_CATEGORY_CONFIG),
        CommandFromHandsetType.SetPreset1,
        None,
        True,
    ),
    "set_preset_2": (
        _button_schema("Set Preset 2", ICON_SET_PRESET, ENTITY_CATEGORY_CONFIG),
        CommandFromHandsetType.SetPreset2,
        None,
        True,
    ),
    "set_preset_3": (
        _button_schema("Set Preset 3", ICON_SET_PRESET, ENTITY_CATEGORY_CONFIG),
        CommandFromHandsetType.SetPreset3,
        None,
        True,
    ),
    "set_preset_4": (
        _button_schema("Set Preset 4", ICON_SET_PRESET, ENTITY_CATEGORY_CONFIG),
        CommandFromHandsetType.SetPreset4,
        None,
        True,
    ),
    "set_max_height": (
        _button_schema(
            "Set Maximum Height", ICON_SET_MAX_HEIGHT, ENTITY_CATEGORY_CONFIG
        ),
        CommandFromHandsetType.SetMaxHeight,
        None,
        False,
    ),
    "set_min_height": (
        _button_schema(
            "Set Minimum Height", ICON_SET_MIN_HEIGHT, ENTITY_CATEGORY_CONFIG
        ),
        CommandFromHandsetType.SetMinHeight,
        None,
        False,
    ),
    "clear_max_height": (
        _button_schema(
            "Clear Maximum Height", ICON_CLEAR_MAX_HEIGHT, ENTITY_CATEGORY_CONFIG
        ),
        CommandFromHandsetType.ClearMinMax,
        0x01,
        False,
    ),
    "clear_min_height": (
        _button_schema(
            "Clear Minimum Height", ICON_CLEAR_MIN_HEIGHT, ENTITY_CATEGORY_CONFIG
        ),
        CommandFromHandsetType.ClearMinMax,
        0x02,
        False,
    ),
}

CONFIG_SCHEMA = platform_schema({key: entry[0] for key, entry in BUTTONS.items()})


async def to_code(config):
    async def build(key, conf, entry, parent):
        _, command, param, refresh = entry
        var = await button.new_button(conf)
        cg.add(var.set_command(command))
        if param is not None:
            cg.add(var.set_param(param))
        cg.add(var.set_refresh_settings(refresh))
        return var

    await register_entities(config, BUTTONS, build, parented=True)
