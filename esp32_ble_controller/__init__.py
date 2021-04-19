import re

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_TRIGGER_ID
from esphome import automation
from esphome.core import coroutine

CODEOWNERS = ['@wifwucite']

### Shared stuff ########################################################################################################

esp32_ble_controller_ns = cg.esphome_ns.namespace('esp32_ble_controller')
ESP32BLEController = esp32_ble_controller_ns.class_('ESP32BLEController', cg.Component, cg.Controller)

### Configuration validation ############################################################################################

# BLE services and characteristics
CONF_BLE_SERVICES = "services"
CONF_BLE_SERVICE = "service"
CONF_BLE_CHARACTERISTICS = "characteristics"
CONF_BLE_CHARACTERISTIC = "characteristic"
CONF_BLE_USE_2902 = "use_BLE2902"
CONF_EXPOSES_COMPONENT = "exposes"

def validate_UUID(value):
    # print("UUID«", value)
    value = cv.string(value)
    # TASK improve the regex
    if re.match(r'^[0-9a-fA-F\-]{8,36}$', value) is None:
        raise cv.Invalid("valid UUID required")
    return value

BLE_CHARACTERISTIC = cv.Schema({
    cv.Required("characteristic"): validate_UUID,
    cv.GenerateID(CONF_EXPOSES_COMPONENT): cv.use_id(cg.Nameable), # TASK validate that only supported Nameables are referenced
    cv.Optional(CONF_BLE_USE_2902, default=True): cv.boolean,
})

BLE_SERVICE = cv.Schema({
    cv.Required(CONF_BLE_SERVICE): validate_UUID,
    cv.Required(CONF_BLE_CHARACTERISTICS): cv.ensure_list(BLE_CHARACTERISTIC),
})

# security mode enumeration
CONF_SECURITY_MODE = 'security_mode'
CONF_SECURITY_MODE_NONE = 'none'
CONF_SECURITY_MODE_SHOW_PASS_KEY = 'show_pass_key'
SECURTY_MODE_OPTIONS = {
    CONF_SECURITY_MODE_NONE: False,
    CONF_SECURITY_MODE_SHOW_PASS_KEY: True,
}

# authetication automations
CONF_ON_SHOW_PASS_KEY = "on_show_pass_key"
BLEControllerShowPassKeyTrigger = esp32_ble_controller_ns.class_('BLEControllerShowPassKeyTrigger', automation.Trigger.template())

CONF_ON_AUTHENTICATION_COMPLETE = "on_authentication_complete"
BLEControllerAuthenticationCompleteTrigger = esp32_ble_controller_ns.class_('BLEControllerAuthenticationCompleteTrigger', automation.Trigger.template())

def require_config_setting_for_automation(automation_id, setting_key, required_setting_value, component_config):
<<<<<<< HEAD
    """Validates that a given automation is only present if a given setting has a given value."""
=======
>>>>>>> 31418efddfc84403878b9c02010cc02b0659afd9
    if automation_id in component_config and component_config[setting_key] != required_setting_value:
        raise cv.Invalid("Automation '" + automation_id + "' only available if " + setting_key + " = " + required_setting_value)

def automations_available(component_config):
<<<<<<< HEAD
    """Validates that the pass key related automations are only present if the security mode is set to show a pass key."""
=======
>>>>>>> 31418efddfc84403878b9c02010cc02b0659afd9
    require_config_setting_for_automation(CONF_ON_SHOW_PASS_KEY, CONF_SECURITY_MODE, CONF_SECURITY_MODE_SHOW_PASS_KEY, component_config)
    require_config_setting_for_automation(CONF_ON_AUTHENTICATION_COMPLETE, CONF_SECURITY_MODE, CONF_SECURITY_MODE_SHOW_PASS_KEY, component_config)
    return component_config

# Schema for the controller (incl. validation)
CONFIG_SCHEMA = cv.All(cv.only_on_esp32, cv.Schema({
    cv.GenerateID(): cv.declare_id(ESP32BLEController),

    cv.Optional(CONF_BLE_SERVICES): cv.ensure_list(BLE_SERVICE),

    cv.Optional(CONF_SECURITY_MODE, default='show_pass_key'): cv.enum(SECURTY_MODE_OPTIONS),

    cv.Optional(CONF_ON_SHOW_PASS_KEY): automation.validate_automation({
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(BLEControllerShowPassKeyTrigger),
    }),
    cv.Optional(CONF_ON_AUTHENTICATION_COMPLETE): automation.validate_automation({
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(BLEControllerAuthenticationCompleteTrigger),
    }),

    }), automations_available)

### Code generation ############################################################################################

@coroutine
def to_code_characteristic(ble_controller_var, service_uuid, characteristic_description):
    """Coroutine that registers the given characteristic of the given service with BLE controller, 
    i.e. generates a single controller->register_component(...) call"""
    characteristic_uuid = characteristic_description[CONF_BLE_CHARACTERISTIC]
    component_id = characteristic_description[CONF_EXPOSES_COMPONENT]
    component = yield cg.get_variable(component_id)
    use_BLE2902 = characteristic_description[CONF_BLE_USE_2902]
    cg.add(ble_controller_var.register_component(component, service_uuid, characteristic_uuid, use_BLE2902))
    
@coroutine
def to_code_service(ble_controller_var, service):
    """Coroutine that registers all characteristics of the given service with BLE controller"""
    service_uuid = service[CONF_BLE_SERVICE]
    characteristics = service[CONF_BLE_CHARACTERISTICS]
    for characteristic_description in characteristics:
        yield to_code_characteristic(ble_controller_var, service_uuid, characteristic_description)

def to_code(config):
    """Generates the C++ code for the BLE controller configuration"""
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)

    if CONF_BLE_SERVICES in config:
        for service in config[CONF_BLE_SERVICES]:
            yield to_code_service(var, service)

    security_enabled = SECURTY_MODE_OPTIONS[config[CONF_SECURITY_MODE]]
    cg.add(var.set_security_enabled(config[CONF_SECURITY_MODE]))

    for conf in config.get(CONF_ON_SHOW_PASS_KEY, []):
        if security_enabled:
            trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
            yield automation.build_automation(trigger, [(cg.std_string, 'pass_key')], conf)

    for conf in config.get(CONF_ON_AUTHENTICATION_COMPLETE, []):
        if security_enabled:
            trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
            yield automation.build_automation(trigger, [(cg.bool_, 'success')], conf)
