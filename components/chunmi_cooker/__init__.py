import esphome.codegen as cg
from esphome.components import uart
import esphome.config_validation as cv
from esphome import automation
from esphome.const import CONF_ID, CONF_TRIGGER_ID

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["json"]
MULTI_CONF = True

chunmi_cooker_ns = cg.esphome_ns.namespace("chunmi_cooker")
ChunmiCooker = chunmi_cooker_ns.class_(
    "ChunmiCooker", cg.PollingComponent, uart.UARTDevice
)
WiFiResetTrigger = chunmi_cooker_ns.class_(
    "WiFiResetTrigger", automation.Trigger.template()
)

CONF_CHUNMI_COOKER_ID = "chunmi_cooker_id"
CONF_ON_WIFI_RESET = "on_wifi_reset"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(ChunmiCooker),
            cv.Optional(CONF_ON_WIFI_RESET): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(WiFiResetTrigger),
                }
            ),
        }
    )
    .extend(cv.polling_component_schema("3s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    for conf in config.get(CONF_ON_WIFI_RESET, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)
