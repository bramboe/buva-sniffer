import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome import pins
from esphome.components import text_sensor

# This is the namespace for your component
cc1101_sniffer_ns = cg.esphome_ns.namespace('cc1101_sniffer')

# This is the main component class
CC1101SnifferComponent = cc1101_sniffer_ns.class_('CC1101SnifferComponent', cg.PollingComponent)

# Auto load text_sensor component
AUTO_LOAD = ['text_sensor']

# Define configuration keys
CONF_CS_PIN = "cs_pin"
CONF_GDO0_PIN = "gdo0_pin"
CONF_GDO2_PIN = "gdo2_pin"
CONF_FREQUENCY = "frequency"

# Configuration schema
CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(CC1101SnifferComponent),
    cv.Required(CONF_CS_PIN): pins.gpio_output_pin_schema,
    cv.Required(CONF_GDO0_PIN): pins.gpio_input_pin_schema,
    cv.Optional(CONF_GDO2_PIN): pins.gpio_input_pin_schema,
    cv.Optional(CONF_FREQUENCY, default=868.3): cv.float_range(min=300.0, max=928.0),
}).extend(cv.polling_component_schema('200ms'))


async def to_code(config):
    """Generate C++ code for the component."""
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    # Set the pins
    cs_pin = await cg.gpio_pin_expression(config[CONF_CS_PIN])
    cg.add(var.set_cs_pin(cs_pin))
    
    gdo0_pin = await cg.gpio_pin_expression(config[CONF_GDO0_PIN])
    cg.add(var.set_gdo0_pin(gdo0_pin))
    
    if CONF_GDO2_PIN in config:
        gdo2_pin = await cg.gpio_pin_expression(config[CONF_GDO2_PIN])
        cg.add(var.set_gdo2_pin(gdo2_pin))
    
    # Set frequency
    cg.add(var.set_frequency(config[CONF_FREQUENCY]))

