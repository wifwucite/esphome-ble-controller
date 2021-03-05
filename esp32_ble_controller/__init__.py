import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_TRIGGER_ID
from esphome import automation

CODEOWNERS = ['@wifwucite']
# DEPENDENCIES = ['i2c']
# AUTO_LOAD = ['sensor', 'text_sensor']
# MULTI_CONF = True

esp32_ble_controller_ns = cg.esphome_ns.namespace('esp32_ble_controller')

CONF_SECURITY_MODE = 'security_mode'
SecurityMode = esp32_ble_controller_ns.enum('SecurityMode')
SECURTY_MODE_OPTIONS = {
    'none': False,
    'show_pass_key': True,
    'show_pin': True,
}

CONF_ON_SHOW_PASS_KEY = "on_show_pass_key"
BLEControllerShowPassKeyTrigger = esp32_ble_controller_ns.class_('BLEControllerShowPassKeyTrigger', automation.Trigger.template())
CONF_ON_AUTHENTICATION_COMPLETE = "on_authentication_complete"
BLEControllerAuthenticationCompleteTrigger = esp32_ble_controller_ns.class_('BLEControllerAuthenticationCompleteTrigger', automation.Trigger.template())

ESP32BLEController = esp32_ble_controller_ns.class_('ESP32BLEController', cg.Component, cg.Controller)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(ESP32BLEController),
    cv.Optional(CONF_SECURITY_MODE, default='show_pass_key'):
        cv.enum(SECURTY_MODE_OPTIONS),
    cv.Optional(CONF_ON_SHOW_PASS_KEY): automation.validate_automation({
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(BLEControllerShowPassKeyTrigger),
    }),
    cv.Optional(CONF_ON_AUTHENTICATION_COMPLETE): automation.validate_automation({
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(BLEControllerAuthenticationCompleteTrigger),
    }),
    })


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)

    cg.add(var.set_security_enabled(config[CONF_SECURITY_MODE]))

    for conf in config.get(CONF_ON_SHOW_PASS_KEY, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        yield automation.build_automation(trigger, [(cg.std_string, 'pass_key')], conf)

    for conf in config.get(CONF_ON_AUTHENTICATION_COMPLETE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        yield automation.build_automation(trigger, [], conf)
