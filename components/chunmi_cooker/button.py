import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv
from . import chunmi_cooker_ns, ChunmiCooker, CONF_CHUNMI_COOKER_ID

CancelButton = chunmi_cooker_ns.class_("CancelButton", button.Button, cg.Component)
AcknowledgeButton = chunmi_cooker_ns.class_("AcknowledgeButton", button.Button, cg.Component)
MuteButton = chunmi_cooker_ns.class_("MuteButton", button.Button, cg.Component)
RefreshButton = chunmi_cooker_ns.class_("RefreshButton", button.Button, cg.Component)

CONF_CANCEL = "cancel"
CONF_ACKNOWLEDGE = "acknowledge"
CONF_MUTE = "mute"
CONF_REFRESH = "refresh"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_CHUNMI_COOKER_ID): cv.use_id(ChunmiCooker),
        cv.Optional(CONF_CANCEL): button.button_schema(
            CancelButton,
            icon="mdi:cancel",
        ),
        cv.Optional(CONF_ACKNOWLEDGE): button.button_schema(
            AcknowledgeButton,
            icon="mdi:bell-cancel-outline",
        ),
        cv.Optional(CONF_MUTE): button.button_schema(
            MuteButton,
            icon="mdi:volume-off",
        ),
        cv.Optional(CONF_REFRESH): button.button_schema(
            RefreshButton,
            icon="mdi:refresh",
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_CHUNMI_COOKER_ID])
    if CONF_CANCEL in config:
        b = await button.new_button(config[CONF_CANCEL])
        cg.add(b.set_parent(hub))
    if CONF_ACKNOWLEDGE in config:
        b = await button.new_button(config[CONF_ACKNOWLEDGE])
        cg.add(b.set_parent(hub))
    if CONF_MUTE in config:
        b = await button.new_button(config[CONF_MUTE])
        cg.add(b.set_parent(hub))
    if CONF_REFRESH in config:
        b = await button.new_button(config[CONF_REFRESH])
        cg.add(b.set_parent(hub))
