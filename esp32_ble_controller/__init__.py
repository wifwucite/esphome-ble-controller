import re

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_TRIGGER_ID
from esphome import automation
from esphome.core import coroutine

CODEOWNERS = ['@wifwucite']

### Shared stuff ########################################################################################################

# Improvement of cv.typed_schema, which allows specifying a default option
CONF_TYPE = "key"
def cv_typed_schema(schemas, **kwargs):
    """Create a schema that has a key to distinguish between schemas"""
    key = kwargs.pop('key', CONF_TYPE)
    default_schema_option = kwargs.pop('default', None)
    key_validator = cv.one_of(*schemas, **kwargs)

    def validator(value):
        if not isinstance(value, dict):
            raise cv.Invalid("Value must be dict")
        schema_option = value.pop(key, default_schema_option)
        if schema_option is None:
            raise cv.Invalid(key + " not specified!")
        value = value.copy()
        key_v = key_validator(schema_option)
        value = schemas[key_v](value)
        value[key] = key_v
        return value

    return validator

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
#SecurityModeEnum = esp32_ble_controller_ns.enum('SecurityMode')

# authetication automations
CONF_ON_SHOW_PASS_KEY = "on_show_pass_key"
BLEControllerShowPassKeyTrigger = esp32_ble_controller_ns.class_('BLEControllerShowPassKeyTrigger', automation.Trigger.template())

CONF_ON_AUTHENTICATION_COMPLETE = "on_authentication_complete"
BLEControllerAuthenticationCompleteTrigger = esp32_ble_controller_ns.class_('BLEControllerAuthenticationCompleteTrigger', automation.Trigger.template())

BASIC_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(ESP32BLEController),

    cv.Optional(CONF_BLE_SERVICES): cv.ensure_list(BLE_SERVICE),
})

# Schema for the controller (incl. validation)
CONFIG_SCHEMA = cv.All(cv_typed_schema({
    CONF_SECURITY_MODE_NONE: BASIC_SCHEMA,

    CONF_SECURITY_MODE_SHOW_PASS_KEY: BASIC_SCHEMA.extend({
        cv.Optional(CONF_ON_SHOW_PASS_KEY): automation.validate_automation({
            cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(BLEControllerShowPassKeyTrigger),
        }),
        cv.Optional(CONF_ON_AUTHENTICATION_COMPLETE): automation.validate_automation({
            cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(BLEControllerAuthenticationCompleteTrigger),
        }),
    }),
 }, key = CONF_SECURITY_MODE, default = CONF_SECURITY_MODE_SHOW_PASS_KEY), cv.only_on_esp32)

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
    cg.add(var.set_security_enabled(security_enabled))

    for conf in config.get(CONF_ON_SHOW_PASS_KEY, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        yield automation.build_automation(trigger, [(cg.std_string, 'pass_key')], conf)

    for conf in config.get(CONF_ON_AUTHENTICATION_COMPLETE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        yield automation.build_automation(trigger, [(cg.bool_, 'success')], conf)
