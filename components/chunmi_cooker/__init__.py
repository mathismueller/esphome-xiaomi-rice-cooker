import esphome.codegen as cg
from esphome.components import uart
import esphome.config_validation as cv
from esphome import automation
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["json", "select", "button", "sensor", "text_sensor", "binary_sensor"]

chunmi_cooker_ns = cg.esphome_ns.namespace("chunmi_cooker")
ChunmiCooker = chunmi_cooker_ns.class_(
    "ChunmiCooker", cg.PollingComponent, uart.UARTDevice
)

CONF_CHUNMI_COOKER_ID = "chunmi_cooker_id"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(ChunmiCooker),
        }
    )
    .extend(cv.polling_component_schema("3s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)


# Automations & Actions
StartAction = chunmi_cooker_ns.class_("StartAction", automation.Action)
SetMenuAction = chunmi_cooker_ns.class_("SetMenuAction", automation.Action)
CancelAction = chunmi_cooker_ns.class_("CancelAction", automation.Action)
SendRawAction = chunmi_cooker_ns.class_("SendRawAction", automation.Action)
SetInteractionAction = chunmi_cooker_ns.class_("SetInteractionAction", automation.Action)


@automation.register_action(
    "chunmi_cooker.start",
    StartAction,
    cv.maybe_simple_value(
        {
            cv.GenerateID(): cv.use_id(ChunmiCooker),
            cv.Required("profile"): cv.templatable(cv.string),
        },
        key="profile",
    ),
    synchronous=True,
)
async def start_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config["profile"], args, cg.std_string)
    cg.add(var.set_profile(template_))
    return var


@automation.register_action(
    "chunmi_cooker.set_menu",
    SetMenuAction,
    cv.maybe_simple_value(
        {
            cv.GenerateID(): cv.use_id(ChunmiCooker),
            cv.Required("profile"): cv.templatable(cv.string),
        },
        key="profile",
    ),
    synchronous=True,
)
async def set_menu_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config["profile"], args, cg.std_string)
    cg.add(var.set_profile(template_))
    return var


@automation.register_action(
    "chunmi_cooker.cancel",
    CancelAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(ChunmiCooker),
        }
    ),
    synchronous=True,
)
async def cancel_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)


@automation.register_action(
    "chunmi_cooker.send_raw",
    SendRawAction,
    cv.maybe_simple_value(
        {
            cv.GenerateID(): cv.use_id(ChunmiCooker),
            cv.Required("command"): cv.templatable(cv.string),
        },
        key="command",
    ),
    synchronous=True,
)
async def send_raw_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config["command"], args, cg.std_string)
    cg.add(var.set_command(template_))
    return var


@automation.register_action(
    "chunmi_cooker.set_interaction",
    SetInteractionAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(ChunmiCooker),
            cv.Optional("settings", default="0607"): cv.templatable(cv.string),
            cv.Optional("led_sec", default=5): cv.templatable(cv.uint8_t),
            cv.Optional("lid_sec", default=4): cv.templatable(cv.uint8_t),
            cv.Optional("alarm_sec", default=21): cv.templatable(cv.uint8_t),
        }
    ),
    synchronous=True,
)
async def set_interaction_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_settings = await cg.templatable(config["settings"], args, cg.std_string)
    cg.add(var.set_settings(template_settings))
    template_led = await cg.templatable(config["led_sec"], args, cg.uint8)
    cg.add(var.set_led_sec(template_led))
    template_lid = await cg.templatable(config["lid_sec"], args, cg.uint8)
    cg.add(var.set_lid_sec(template_lid))
    template_alarm = await cg.templatable(config["alarm_sec"], args, cg.uint8)
    cg.add(var.set_alarm_sec(template_alarm))
    return var
