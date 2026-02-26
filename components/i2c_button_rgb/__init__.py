import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation, pins
from esphome.components import i2c, binary_sensor
from esphome.const import (
    CONF_ID,
    CONF_INDEX,
    CONF_NAME,
    CONF_UPDATE_INTERVAL,
)

CODEOWNERS = []
DEPENDENCIES = ["i2c"]
AUTO_LOAD = ["binary_sensor"]

CONF_WS2812_PIN = "ws2812_pin"
CONF_MAX_LEDS = "max_leds"

CONF_DYNAMIC_ADDRESS_PIN = "dynamic_address_pin"
CONF_ADDRESSES_START_POINT = "addresses_start_point"
CONF_I2C_RESET_ADDRESS = "i2c_reset_address"
CONF_I2C_REQUESTING_ADDRESS = "i2c_requesting_address"
CONF_I2C_MESSAGE_RESET_ADDRESS = "i2c_message_reset_address"
CONF_I2C_MESSAGE_REQUEST_ADDRESS = "i2c_message_request_address"

CONF_BUTTONS = "buttons"

ns = cg.esphome_ns.namespace("i2c_button_rgb")
I2CButtonRGB = ns.class_("I2CButtonRGB", cg.PollingComponent, i2c.I2CDevice)

SetPixelAction = ns.class_("SetPixelAction", automation.Action)
FillAction = ns.class_("FillAction", automation.Action)
ClearAction = ns.class_("ClearAction", automation.Action)
ShowAction = ns.class_("ShowAction", automation.Action)

BUTTON_SCHEMA = binary_sensor.binary_sensor_schema().extend(
    {
        cv.Required(CONF_INDEX): cv.int_range(min=0, max=1023),
    }
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(I2CButtonRGB),

            cv.Required(CONF_WS2812_PIN): pins.internal_gpio_output_pin_schema,
            cv.Optional(CONF_MAX_LEDS, default=128): cv.int_range(min=1, max=1024),

            cv.Optional(CONF_UPDATE_INTERVAL, default="10ms"): cv.update_interval,

            cv.Optional(CONF_DYNAMIC_ADDRESS_PIN): pins.internal_gpio_input_pin_schema,
            cv.Optional(CONF_ADDRESSES_START_POINT, default=2): cv.int_range(min=0, max=127),
            cv.Optional(CONF_I2C_RESET_ADDRESS, default=0x7F): cv.int_range(min=0, max=127),
            cv.Optional(CONF_I2C_REQUESTING_ADDRESS, default=0x7E): cv.int_range(min=0, max=127),
            cv.Optional(CONF_I2C_MESSAGE_RESET_ADDRESS, default=0x0A): cv.int_range(min=0, max=255),
            cv.Optional(CONF_I2C_MESSAGE_REQUEST_ADDRESS, default=0x0B): cv.int_range(min=0, max=255),

            cv.Optional(CONF_BUTTONS, default=[]): cv.ensure_list(BUTTON_SCHEMA),
        }
    )
    .extend(cv.polling_component_schema("10ms"))
    .extend(i2c.i2c_device_schema(None))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    ws_pin = await cg.gpio_pin_expression(config[CONF_WS2812_PIN])
    cg.add(var.set_ws2812_pin(ws_pin))
    cg.add(var.set_max_leds(config[CONF_MAX_LEDS]))

    if CONF_DYNAMIC_ADDRESS_PIN in config:
        dyn_pin = await cg.gpio_pin_expression(config[CONF_DYNAMIC_ADDRESS_PIN])
        cg.add(var.set_dynamic_address_pin(dyn_pin))

    cg.add(var.set_addresses_start_point(config[CONF_ADDRESSES_START_POINT]))
    cg.add(var.set_i2c_reset_address(config[CONF_I2C_RESET_ADDRESS]))
    cg.add(var.set_i2c_requesting_address(config[CONF_I2C_REQUESTING_ADDRESS]))
    cg.add(var.set_i2c_message_reset_address(config[CONF_I2C_MESSAGE_RESET_ADDRESS]))
    cg.add(var.set_i2c_message_request_address(config[CONF_I2C_MESSAGE_REQUEST_ADDRESS]))

    for bconf in config[CONF_BUTTONS]:
        sens = await binary_sensor.new_binary_sensor(bconf)
        cg.add(var.add_button_sensor(bconf[CONF_INDEX], sens))


# -------------------------
# Actions
# -------------------------

SET_PIXEL_ACTION_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID): cv.use_id(I2CButtonRGB),
        cv.Required("index"): cv.templatable(cv.uint16_t),
        cv.Required("r"): cv.templatable(cv.uint8_t),
        cv.Required("g"): cv.templatable(cv.uint8_t),
        cv.Required("b"): cv.templatable(cv.uint8_t),
    }
)

FILL_ACTION_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID): cv.use_id(I2CButtonRGB),
        cv.Required("r"): cv.templatable(cv.uint8_t),
        cv.Required("g"): cv.templatable(cv.uint8_t),
        cv.Required("b"): cv.templatable(cv.uint8_t),
    }
)

CLEAR_ACTION_SCHEMA = cv.Schema({cv.Required(CONF_ID): cv.use_id(I2CButtonRGB)})
SHOW_ACTION_SCHEMA = cv.Schema({cv.Required(CONF_ID): cv.use_id(I2CButtonRGB)})


@automation.register_action("i2c_button_rgb.set_pixel", SetPixelAction, SET_PIXEL_ACTION_SCHEMA)
async def set_pixel_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)

    idx = await cg.templatable(config["index"], args, cg.uint16)
    r = await cg.templatable(config["r"], args, cg.uint8)
    g = await cg.templatable(config["g"], args, cg.uint8)
    b = await cg.templatable(config["b"], args, cg.uint8)

    cg.add(var.set_index(idx))
    cg.add(var.set_r(r))
    cg.add(var.set_g(g))
    cg.add(var.set_b(b))
    return var


@automation.register_action("i2c_button_rgb.fill", FillAction, FILL_ACTION_SCHEMA)
async def fill_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)

    r = await cg.templatable(config["r"], args, cg.uint8)
    g = await cg.templatable(config["g"], args, cg.uint8)
    b = await cg.templatable(config["b"], args, cg.uint8)

    cg.add(var.set_r(r))
    cg.add(var.set_g(g))
    cg.add(var.set_b(b))
    return var


@automation.register_action("i2c_button_rgb.clear", ClearAction, CLEAR_ACTION_SCHEMA)
async def clear_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    return var


@automation.register_action("i2c_button_rgb.show", ShowAction, SHOW_ACTION_SCHEMA)
async def show_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    return var