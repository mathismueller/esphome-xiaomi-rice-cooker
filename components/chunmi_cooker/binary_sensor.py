import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import (
    DEVICE_CLASS_PROBLEM,
)
from . import ChunmiCooker, CONF_CHUNMI_COOKER_ID

CONF_POT_PRESENT = "pot_present"
CONF_ERROR = "error"
CONF_AUTO_KEEP_WARM = "auto_keep_warm"
CONF_LID_OPEN_WARNING = "lid_open_warning"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_CHUNMI_COOKER_ID): cv.use_id(ChunmiCooker),
        cv.Optional(CONF_POT_PRESENT): binary_sensor.binary_sensor_schema(
            icon="mdi:pot",
        ),
        cv.Optional(CONF_ERROR): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_PROBLEM,
        ),
        cv.Optional(CONF_AUTO_KEEP_WARM): binary_sensor.binary_sensor_schema(
            icon="mdi:heat-wave",
        ),
        cv.Optional(CONF_LID_OPEN_WARNING): binary_sensor.binary_sensor_schema(
            icon="mdi:bell-alert",
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_CHUNMI_COOKER_ID])
    if CONF_POT_PRESENT in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_POT_PRESENT])
        cg.add(hub.set_pot_present_binary_sensor(sens))
    if CONF_ERROR in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_ERROR])
        cg.add(hub.set_error_binary_sensor(sens))
    if CONF_AUTO_KEEP_WARM in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_AUTO_KEEP_WARM])
        cg.add(hub.set_auto_keep_warm_binary_sensor(sens))
    if CONF_LID_OPEN_WARNING in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_LID_OPEN_WARNING])
        cg.add(hub.set_lid_open_warning_binary_sensor(sens))
