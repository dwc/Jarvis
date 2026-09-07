"""Fully Jarvis standing desk hub component.

The hub sits between the desk's handset and its control box: it forwards every
message in both directions and, along the way, publishes/consumes the settings
via the ESPHome entity platforms in this component.
"""

import esphome.codegen as cg
from esphome.components import uart
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = ["@dwc"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "text_sensor", "number", "select", "button"]

CONF_JARVIS_DESK_ID = "jarvis_desk_id"
CONF_HANDSET_UART_ID = "handset_uart_id"
CONF_CONTROLBOX_UART_ID = "controlbox_uart_id"

JARVIS_BAUD_RATE = 9600

jarvis_desk_ns = cg.esphome_ns.namespace("jarvis_desk")
JarvisDesk = jarvis_desk_ns.class_("JarvisDesk", cg.Component)

# Unscoped C++ enum, so the enumerators render as jarvis_desk::MoveToPreset1.
CommandFromHandsetType = jarvis_desk_ns.enum("CommandFromHandsetType")

def _distinct_uarts(config):
    """The two ends of the link must be different buses.

    Pointing both at one bus validates fine but produces two UARTDevices racing
    for the same byte stream, which is very hard to diagnose from the logs.
    """
    if config[CONF_HANDSET_UART_ID] == config[CONF_CONTROLBOX_UART_ID]:
        raise cv.Invalid(
            f"'{CONF_HANDSET_UART_ID}' and '{CONF_CONTROLBOX_UART_ID}' must "
            f"reference different uart buses"
        )
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(JarvisDesk),
            cv.Required(CONF_HANDSET_UART_ID): cv.use_id(uart.UARTComponent),
            cv.Required(CONF_CONTROLBOX_UART_ID): cv.use_id(uart.UARTComponent),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    _distinct_uarts,
)

FINAL_VALIDATE_SCHEMA = cv.All(
    uart.final_validate_device_schema(
        "jarvis_desk",
        uart_bus=CONF_HANDSET_UART_ID,
        baud_rate=JARVIS_BAUD_RATE,
        require_tx=True,
        require_rx=True,
    ),
    uart.final_validate_device_schema(
        "jarvis_desk",
        uart_bus=CONF_CONTROLBOX_UART_ID,
        baud_rate=JARVIS_BAUD_RATE,
        require_tx=True,
        require_rx=True,
    ),
)

# Schema fragment for the jarvis_desk entity platforms; extend it so that every
# platform gets a reference to its hub.
JARVIS_DESK_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_JARVIS_DESK_ID): cv.use_id(JarvisDesk),
    }
)


def empty_dict_if_none(value):
    """Allow ``go_preset_1:`` with no options, since every entity has defaults."""
    return {} if value is None else value


def platform_schema(entity_schemas):
    """Build a platform CONFIG_SCHEMA from its {yaml key: entity schema} table.

    Deriving the schema from the same table that drives to_code() keeps the two
    from drifting; a key present in one but not the other silently stops working.
    """
    return JARVIS_DESK_SCHEMA.extend(
        {cv.Optional(key): schema for key, schema in entity_schemas.items()}
    )


async def register_entities(config, table, build, parented=False):
    """Create every configured entity of one platform and bind it to the hub.

    ``build`` receives (key, entity config, table entry, hub), constructs the
    entity, hands it to the hub, and returns it.

    Writable platforms pass ``parented=True`` so the entity gets a pointer back
    to the hub for its control()/press_action() to use. That is done here rather
    than in each build(): forgetting it leaves a null parent that only crashes
    at runtime, when the entity is first used.
    """
    parent = await cg.get_variable(config[CONF_JARVIS_DESK_ID])
    for key, entry in table.items():
        conf = config.get(key)
        if conf is None:
            continue
        var = await build(key, conf, entry, parent)
        if parented:
            await cg.register_parented(var, parent)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    handset_uart = await cg.get_variable(config[CONF_HANDSET_UART_ID])
    cg.add(var.set_handset_uart(handset_uart))

    controlbox_uart = await cg.get_variable(config[CONF_CONTROLBOX_UART_ID])
    cg.add(var.set_controlbox_uart(controlbox_uart))
