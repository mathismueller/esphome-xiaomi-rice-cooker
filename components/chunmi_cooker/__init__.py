import esphome.codegen as cg
from esphome.components import uart
import esphome.config_validation as cv
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["json"]
MULTI_CONF = True

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
