import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.const import CONF_MODE
from . import ChunmiCooker, CONF_CHUNMI_COOKER_ID

CONF_STAGE = "stage"
CONF_MENU = "menu"
CONF_LAST_BUTTON = "last_button"
CONF_LAST_EVENT = "last_event"
CONF_RAW_STATUS = "raw_status"
CONF_HARDWARE_VERSION = "hardware_version"
CONF_MCU_VERSION = "mcu_version"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_CHUNMI_COOKER_ID): cv.use_id(ChunmiCooker),
        cv.Optional(CONF_MODE): text_sensor.text_sensor_schema(
            icon="mdi:rice",
        ),
        cv.Optional(CONF_STAGE): text_sensor.text_sensor_schema(
            icon="mdi:pot-steam",
        ),
        cv.Optional(CONF_MENU): text_sensor.text_sensor_schema(
            icon="mdi:book-open-outline",
        ),
        cv.Optional(CONF_LAST_BUTTON): text_sensor.text_sensor_schema(
            icon="mdi:gesture-tap-button",
        ),
        cv.Optional(CONF_LAST_EVENT): text_sensor.text_sensor_schema(
            icon="mdi:bell-alert-outline",
        ),
        cv.Optional(CONF_RAW_STATUS): text_sensor.text_sensor_schema(
            icon="mdi:code-json",
        ),
        cv.Optional(CONF_HARDWARE_VERSION): text_sensor.text_sensor_schema(
            icon="mdi:chip",
        ),
        cv.Optional(CONF_MCU_VERSION): text_sensor.text_sensor_schema(
            icon="mdi:numeric",
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_CHUNMI_COOKER_ID])
    if CONF_MODE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_MODE])
        cg.add(hub.set_mode_text_sensor(sens))
    if CONF_STAGE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_STAGE])
        cg.add(hub.set_stage_text_sensor(sens))
    if CONF_MENU in config:
        sens = await text_sensor.new_text_sensor(config[CONF_MENU])
        cg.add(hub.set_menu_text_sensor(sens))
    if CONF_LAST_BUTTON in config:
        sens = await text_sensor.new_text_sensor(config[CONF_LAST_BUTTON])
        cg.add(hub.set_last_button_text_sensor(sens))
    if CONF_LAST_EVENT in config:
        sens = await text_sensor.new_text_sensor(config[CONF_LAST_EVENT])
        cg.add(hub.set_last_event_text_sensor(sens))
    if CONF_RAW_STATUS in config:
        sens = await text_sensor.new_text_sensor(config[CONF_RAW_STATUS])
        cg.add(hub.set_raw_status_text_sensor(sens))
    if CONF_HARDWARE_VERSION in config:
        sens = await text_sensor.new_text_sensor(config[CONF_HARDWARE_VERSION])
        cg.add(hub.set_hardware_version_text_sensor(sens))
    if CONF_MCU_VERSION in config:
        sens = await text_sensor.new_text_sensor(config[CONF_MCU_VERSION])
        cg.add(hub.set_mcu_version_text_sensor(sens))
