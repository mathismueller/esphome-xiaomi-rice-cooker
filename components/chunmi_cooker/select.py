import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from . import chunmi_cooker_ns, ChunmiCooker, CONF_CHUNMI_COOKER_ID

MenuSelect = chunmi_cooker_ns.class_("MenuSelect", select.Select, cg.Component)

CONF_MENU_SELECT = "menu_select"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_CHUNMI_COOKER_ID): cv.use_id(ChunmiCooker),
        cv.Optional(CONF_MENU_SELECT): select.select_schema(
            MenuSelect,
            icon="mdi:silverware-fork-knife",
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_CHUNMI_COOKER_ID])
    if CONF_MENU_SELECT in config:
        sel = await select.new_select(
            config[CONF_MENU_SELECT],
            options=[
                "Standard Rice",
                "Quick Rice",
                "Porridge",
                "Keep Warm",
                "Cooking / Steam",
                "Cake",
                "Sushi Rice (Experimental)",
                "Express Rice (Experimental)",
            ],
        )
        await cg.register_component(sel, config[CONF_MENU_SELECT])
        cg.add(sel.set_parent(hub))
        cg.add(hub.set_menu_select(sel))
