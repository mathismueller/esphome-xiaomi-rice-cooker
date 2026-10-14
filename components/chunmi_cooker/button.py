import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv
from . import chunmi_cooker_ns, ChunmiCooker, CONF_CHUNMI_COOKER_ID

CancelButton = chunmi_cooker_ns.class_("CancelButton", button.Button, cg.Component)
AcknowledgeButton = chunmi_cooker_ns.class_("AcknowledgeButton", button.Button, cg.Component)
MuteButton = chunmi_cooker_ns.class_("MuteButton", button.Button, cg.Component)
RefreshButton = chunmi_cooker_ns.class_("RefreshButton", button.Button, cg.Component)

StartStandardRiceButton = chunmi_cooker_ns.class_("StartStandardRiceButton", button.Button, cg.Component)
StartQuickRiceButton = chunmi_cooker_ns.class_("StartQuickRiceButton", button.Button, cg.Component)
StartPorridgeButton = chunmi_cooker_ns.class_("StartPorridgeButton", button.Button, cg.Component)
StartKeepWarmButton = chunmi_cooker_ns.class_("StartKeepWarmButton", button.Button, cg.Component)
StartCookingButton = chunmi_cooker_ns.class_("StartCookingButton", button.Button, cg.Component)
StartCakeButton = chunmi_cooker_ns.class_("StartCakeButton", button.Button, cg.Component)
StartSushiRiceButton = chunmi_cooker_ns.class_("StartSushiRiceButton", button.Button, cg.Component)
StartExpressRiceButton = chunmi_cooker_ns.class_("StartExpressRiceButton", button.Button, cg.Component)
StartSelectedMenuButton = chunmi_cooker_ns.class_("StartSelectedMenuButton", button.Button, cg.Component)
StartFavoriteButton = chunmi_cooker_ns.class_("StartFavoriteButton", button.Button, cg.Component)

CONF_CANCEL = "cancel"
CONF_ACKNOWLEDGE = "acknowledge"
CONF_MUTE = "mute"
CONF_REFRESH = "refresh"

CONF_START_STANDARD_RICE = "start_standard_rice"
CONF_START_QUICK_RICE = "start_quick_rice"
CONF_START_PORRIDGE = "start_porridge"
CONF_START_KEEP_WARM = "start_keep_warm"
CONF_START_COOKING = "start_cooking"
CONF_START_CAKE = "start_cake"
CONF_START_SUSHI_RICE = "start_sushi_rice"
CONF_START_EXPRESS_RICE = "start_express_rice"
CONF_START_SELECTED_MENU = "start_selected_menu"
CONF_START_FAVORITE = "start_favorite"

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
        cv.Optional(CONF_START_SELECTED_MENU): button.button_schema(
            StartSelectedMenuButton,
            icon="mdi:play",
        ),
        cv.Optional(CONF_START_STANDARD_RICE): button.button_schema(
            StartStandardRiceButton,
            icon="mdi:rice",
        ),
        cv.Optional(CONF_START_QUICK_RICE): button.button_schema(
            StartQuickRiceButton,
            icon="mdi:clock-fast",
        ),
        cv.Optional(CONF_START_PORRIDGE): button.button_schema(
            StartPorridgeButton,
            icon="mdi:bowl-mix",
        ),
        cv.Optional(CONF_START_KEEP_WARM): button.button_schema(
            StartKeepWarmButton,
            icon="mdi:heat-wave",
        ),
        cv.Optional(CONF_START_COOKING): button.button_schema(
            StartCookingButton,
            icon="mdi:pot-steam",
        ),
        cv.Optional(CONF_START_CAKE): button.button_schema(
            StartCakeButton,
            icon="mdi:cake-variant",
        ),
        cv.Optional(CONF_START_SUSHI_RICE): button.button_schema(
            StartSushiRiceButton,
            icon="mdi:rice",
        ),
        cv.Optional(CONF_START_EXPRESS_RICE): button.button_schema(
            StartExpressRiceButton,
            icon="mdi:flash-outline",
        ),
        cv.Optional(CONF_START_FAVORITE): button.button_schema(
            StartFavoriteButton,
            icon="mdi:star-play",
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_CHUNMI_COOKER_ID])
    button_configs = [
        (CONF_CANCEL, CancelButton),
        (CONF_ACKNOWLEDGE, AcknowledgeButton),
        (CONF_MUTE, MuteButton),
        (CONF_REFRESH, RefreshButton),
        (CONF_START_SELECTED_MENU, StartSelectedMenuButton),
        (CONF_START_STANDARD_RICE, StartStandardRiceButton),
        (CONF_START_QUICK_RICE, StartQuickRiceButton),
        (CONF_START_PORRIDGE, StartPorridgeButton),
        (CONF_START_KEEP_WARM, StartKeepWarmButton),
        (CONF_START_COOKING, StartCookingButton),
        (CONF_START_CAKE, StartCakeButton),
        (CONF_START_SUSHI_RICE, StartSushiRiceButton),
        (CONF_START_EXPRESS_RICE, StartExpressRiceButton),
        (CONF_START_FAVORITE, StartFavoriteButton),
    ]

    for conf_key, _ in button_configs:
        if conf_key in config:
            b = await button.new_button(config[conf_key])
            await cg.register_component(b, config[conf_key])
            cg.add(b.set_parent(hub))
