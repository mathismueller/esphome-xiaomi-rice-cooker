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
)
async def send_raw_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config["command"], args, cg.std_string)
    cg.add(var.set_command(template_))
    return var
