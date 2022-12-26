import re

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.automation import LambdaAction
from esphome.const import CONF_ID, CONF_TRIGGER_ID, CONF_FORMAT, CONF_ARGS
from esphome import automation
from esphome.core import coroutine, Lambda
from esphome.cpp_generator import MockObj

CODEOWNERS = ['@wifwucite']

### Shared stuff ########################################################################################################

esp32_ble_controller_ns = cg.esphome_ns.namespace('esp32_ble_controller')
ESP32BLEController = esp32_ble_controller_ns.class_('ESP32BLEController', cg.Component, cg.Controller)

### Configuration validation ############################################################################################

# BLE component services and characteristics #####
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
    cv.GenerateID(CONF_EXPOSES_COMPONENT): cv.use_id(cg.EntityBase), # TASK validate that only supported EntityBase instances are referenced
    cv.Optional(CONF_BLE_USE_2902, default=True): cv.boolean,
})

BLE_SERVICE = cv.Schema({
    cv.Required(CONF_BLE_SERVICE): validate_UUID,
    cv.Required(CONF_BLE_CHARACTERISTICS): cv.ensure_list(BLE_CHARACTERISTIC),
})

# custom commands #####
CONF_BLE_COMMANDS = "commands"
CONF_BLE_CMD_ID = "command"
CONF_BLE_CMD_DESCRIPTION = "description"
CONF_BLE_CMD_ON_EXECUTE = "on_execute"
BLEControllerCustomCommandExecutionTrigger = esp32_ble_controller_ns.class_('BLEControllerCustomCommandExecutionTrigger', automation.Trigger.template())

BUILTIN_CMD_IDS = ['help', 'ble-services', 'wifi-config', 'pairings', 'version', 'log-level']
CMD_ID_CHARACTERS = "abcdefghijklmnopqrstuvwxyz0123456789-"
def validate_command_id(value):
    """Validate that this value is a valid command id.
    """
    value = cv.string_strict(value).lower()
    if value in BUILTIN_CMD_IDS:
        raise cv.Invalid(f"{value} is a built-in command")
    for c in value:
        if c not in CMD_ID_CHARACTERS:
            raise cv.Invalid(f"Invalid character for command id: {c}")
    return value

BLE_COMMAND = cv.Schema({
    cv.Required(CONF_BLE_CMD_ID): validate_command_id,
    cv.Required(CONF_BLE_CMD_DESCRIPTION): cv.string_strict,
    cv.Required(CONF_BLE_CMD_ON_EXECUTE): automation.validate_automation({
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(BLEControllerCustomCommandExecutionTrigger),
    }),
})

# BLE maintenance services #####
CONF_EXPOSE_MAINTENANCE_SERVICE = "maintenance"

# security mode enumeration #####
CONF_SECURITY_MODE = 'security_mode'
BLESecurityMode = esp32_ble_controller_ns.enum("BLESecurityMode", is_class = True)
CONF_SECURITY_MODE_NONE = 'none' # no security, no bonding
CONF_SECURITY_MODE_BOND = 'bond' # no secure connection, no man-in-the-middle protection, just bonding (pairing)
CONF_SECURITY_MODE_SECURE = 'secure' # default: bonding, secure connection, MITM protection
SECURTY_MODE_OPTIONS = {
    CONF_SECURITY_MODE_NONE: BLESecurityMode.NONE,
    CONF_SECURITY_MODE_BOND: BLESecurityMode.BOND,
    CONF_SECURITY_MODE_SECURE: BLESecurityMode.SECURE,
}

# authetication and (dis)connected automations #####
CONF_ON_SHOW_PASS_KEY = "on_show_pass_key"
BLEControllerShowPassKeyTrigger = esp32_ble_controller_ns.class_('BLEControllerShowPassKeyTrigger', automation.Trigger.template())

CONF_ON_AUTHENTICATION_COMPLETE = "on_authentication_complete"
BLEControllerAuthenticationCompleteTrigger = esp32_ble_controller_ns.class_('BLEControllerAuthenticationCompleteTrigger', automation.Trigger.template())

def forbid_config_setting_for_automation(automation_id, setting_key, forbidden_setting_value, config):
    """Validates that a given automation is only present if a given setting does not have a given value."""
    if automation_id in config and config[setting_key] == forbidden_setting_value:
        raise cv.Invalid("Automation '" + automation_id + "' not available if " + setting_key + " = " + forbidden_setting_value)

def automations_available(config):
    """Validates that the security related automations are only present if the security mode is not none."""
    forbid_config_setting_for_automation(CONF_ON_SHOW_PASS_KEY, CONF_SECURITY_MODE, CONF_SECURITY_MODE_NONE, config)
    forbid_config_setting_for_automation(CONF_ON_AUTHENTICATION_COMPLETE, CONF_SECURITY_MODE, CONF_SECURITY_MODE_NONE, config)
    return config

def require_automation_for_config_setting(automation_id, setting_key, requiring_setting_value, config):
    """Validates that a given automation is only present if a given setting does not have a given value."""
    if config[setting_key] == requiring_setting_value and not automation_id in config:
        raise cv.Invalid("Automation '" + automation_id + "' required if " + setting_key + " = " + requiring_setting_value)

def required_automations_present(config):
    """Validates that the pass key related automation is present if the security mode is set to secure."""
    require_automation_for_config_setting(CONF_ON_SHOW_PASS_KEY, CONF_SECURITY_MODE, CONF_SECURITY_MODE_SECURE, config)
    return config

CONF_ON_SERVER_CONNECTED = "on_connected"
BLEControllerServerConnectedTrigger = esp32_ble_controller_ns.class_('BLEControllerServerConnectedTrigger', automation.Trigger.template())

CONF_ON_SERVER_DISCONNECTED = "on_disconnected"
BLEControllerServerDisconnectedTrigger = esp32_ble_controller_ns.class_('BLEControllerServerDisconnectedTrigger', automation.Trigger.template())

# Schema for the controller (incl. validation) #####
CONFIG_SCHEMA = cv.All(cv.only_on_esp32, cv.only_with_arduino, cv.Schema({
    cv.GenerateID(): cv.declare_id(ESP32BLEController),

    cv.Optional(CONF_BLE_SERVICES): cv.ensure_list(BLE_SERVICE),

    cv.Optional(CONF_BLE_COMMANDS): cv.ensure_list(BLE_COMMAND),

    cv.Optional(CONF_EXPOSE_MAINTENANCE_SERVICE, default=True): cv.boolean,

    cv.Optional(CONF_SECURITY_MODE, default=CONF_SECURITY_MODE_SECURE): cv.enum(SECURTY_MODE_OPTIONS),

    cv.Optional(CONF_ON_SHOW_PASS_KEY): automation.validate_automation({
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(BLEControllerShowPassKeyTrigger),
    }),
    cv.Optional(CONF_ON_AUTHENTICATION_COMPLETE): automation.validate_automation({
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(BLEControllerAuthenticationCompleteTrigger),
    }),

    cv.Optional(CONF_ON_SERVER_CONNECTED): automation.validate_automation({
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(BLEControllerServerConnectedTrigger),
    }),
    cv.Optional(CONF_ON_SERVER_DISCONNECTED): automation.validate_automation({
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(BLEControllerServerDisconnectedTrigger),
    }),

    }), automations_available, required_automations_present)

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

@coroutine
def to_code_command(ble_controller_var, cmd):
    """Coroutine that registers all BLE commands with BLE controller"""
    id = cmd[CONF_BLE_CMD_ID]
    description = cmd[CONF_BLE_CMD_DESCRIPTION]
    trigger_conf = cmd[CONF_BLE_CMD_ON_EXECUTE][0]
    trigger = cg.new_Pvariable(trigger_conf[CONF_TRIGGER_ID], ble_controller_var)
    yield automation.build_automation(trigger, [(cg.std_ns.class_("vector<std::string>"), 'arguments'), (esp32_ble_controller_ns.class_("BLECustomCommandResultSender"), 'result')], trigger_conf)
    cg.add(ble_controller_var.register_command(id, description, trigger))

def to_code(config):
    """Generates the C++ code for the BLE controller configuration"""
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)

    for cmd in config.get(CONF_BLE_SERVICES, []):
        yield to_code_service(var, cmd)

    for cmd in config.get(CONF_BLE_COMMANDS, []):
        yield to_code_command(var, cmd)

    cg.add(var.set_maintenance_service_exposed_after_flash(config[CONF_EXPOSE_MAINTENANCE_SERVICE]))

    security_enabled = SECURTY_MODE_OPTIONS[config[CONF_SECURITY_MODE]]
    cg.add(var.set_security_mode(config[CONF_SECURITY_MODE]))

    for conf in config.get(CONF_ON_SHOW_PASS_KEY, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        yield automation.build_automation(trigger, [(cg.std_string, 'pass_key')], conf)

    for conf in config.get(CONF_ON_AUTHENTICATION_COMPLETE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        yield automation.build_automation(trigger, [(cg.bool_, 'success')], conf)

    for conf in config.get(CONF_ON_SERVER_CONNECTED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        yield automation.build_automation(trigger, [], conf)

    for conf in config.get(CONF_ON_SERVER_DISCONNECTED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        yield automation.build_automation(trigger, [], conf)

    cg.add_library("ESP32 BLE Arduino", "2.0.0");

### Automation actions ############################################################################################

GLOBAL_BLE_CONTROLLER_VAR = MockObj(esp32_ble_controller_ns.global_ble_controller, "->")

### Automation action: ble_cmd.send_result ###

def maybe_simple_message(schema):
    def validator(value):
        if isinstance(value, dict):
            return cv.Schema(schema)(value)
        return cv.Schema(schema)({CONF_FORMAT: value})

    return validator

def validate_printf(value):
    # https://stackoverflow.com/questions/30011379/how-can-i-parse-a-c-format-string-in-python
    # pylint: disable=anomalous-backslash-in-string
    cfmt = """\
    (                                  # start of capture group 1
    %                                  # literal "%"
    (?:[-+0 #]{0,5})                   # optional flags
    (?:\d+|\*)?                        # width
    (?:\.(?:\d+|\*))?                  # precision
    (?:h|l|ll|w|I|I32|I64)?            # size
    [cCdiouxXeEfgGaAnpsSZ]             # type
    ) 
    """  # noqa
    matches = re.findall(cfmt, value[CONF_FORMAT], flags=re.X)
    if len(matches) != len(value[CONF_ARGS]):
        raise cv.Invalid(
            "Found {} printf-patterns ({}), but {} args were given!"
            "".format(len(matches), ", ".join(matches), len(value[CONF_ARGS]))
        )
    return value

BLE_CMD_SET_RESULT_ACTION_SCHEMA = cv.All(
    maybe_simple_message(
        {
            cv.Required(CONF_FORMAT): cv.string,
            cv.Optional(CONF_ARGS, default=list): cv.ensure_list(cv.lambda_),
        }
    ),
    validate_printf,
)

@automation.register_action("ble_cmd.send_result", LambdaAction, BLE_CMD_SET_RESULT_ACTION_SCHEMA)
async def ble_cmd_set_result_action_to_code(config, action_id, template_arg, args):
    args_ = [cg.RawExpression(str(x)) for x in config[CONF_ARGS]]

    text = str(cg.statement(GLOBAL_BLE_CONTROLLER_VAR.send_command_result(config[CONF_FORMAT], *args_)))

    lambda_ = await cg.process_lambda(Lambda(text), args, return_type=cg.void)
    return cg.new_Pvariable(action_id, template_arg, lambda_)

### Automation action: ble_maintenance.toggle/turn_on/turn_off ###

ToggleAction = esp32_ble_controller_ns.class_("ToggleMaintenanceServiceAction", automation.Action)
TurnOffAction = esp32_ble_controller_ns.class_("TurnOffMaintenanceServiceAction", automation.Action)
TurnOnAction = esp32_ble_controller_ns.class_("TurnOnMaintenanceServiceAction", automation.Action)

BLE_MAINTENANCE_ACTION_SCHEMA = cv.Schema({}) # no arguments required

@automation.register_action("ble_maintenance.toggle", ToggleAction, BLE_MAINTENANCE_ACTION_SCHEMA)
@automation.register_action("ble_maintenance.turn_off", TurnOffAction, BLE_MAINTENANCE_ACTION_SCHEMA)
@automation.register_action("ble_maintenance.turn_on", TurnOnAction, BLE_MAINTENANCE_ACTION_SCHEMA)
async def ble_maintenance_toggle_to_code(config, action_id, template_arg, args):
    print(config, action_id, template_arg, args)
    return cg.new_Pvariable(action_id, template_arg)
