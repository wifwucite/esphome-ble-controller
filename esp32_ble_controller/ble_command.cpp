#include "ble_command.h"

#include "esp32_ble_controller.h"
#include "automation.h"

namespace esphome {
namespace esp32_ble_controller {

BLECommandHelp::BLECommandHelp() : BLECommand("help", "show help for commands") {}

void BLECommandHelp::execute(const vector<string>& arguments) const {
  if (arguments.empty()) {
    string help("Availabe:");
    for (const auto& command : global_ble_controller->get_commands()) {
      help += ' ';
      help += command->get_name();
    }
    help += ", 'help <cmd>' for more.";
    global_ble_controller->set_command_result(help);
  } else {
    string command_name = arguments[0];
    for (const auto& command : global_ble_controller->get_commands()) {
      if (command->get_name() == command_name) {
        global_ble_controller->set_command_result(command->get_name() + ": " + command->get_description());
        return;
      }
      global_ble_controller->set_command_result("Unknown BLE command '" + command_name + "'");
    }
  }
}

BLECommandSwitchServicesOnOrOff::BLECommandSwitchServicesOnOrOff() : BLECommand("ble-services", "ble-services on|off enables or disables the non-maintenance BLE services") {}

void BLECommandSwitchServicesOnOrOff::execute(const vector<string>& arguments) const {
  if (!arguments.empty()) {
    string on_or_off = arguments[0];
    if (on_or_off == "off") {
      global_ble_controller->set_ble_mode(BLEMaintenanceMode::WIFI_ONLY);
    } else {
      global_ble_controller->set_ble_mode(BLEMaintenanceMode::MIXED);
    }
  }
  BLEMaintenanceMode mode = global_ble_controller->get_ble_mode();
  string enabled_or_disabled = mode == BLEMaintenanceMode::WIFI_ONLY ? "disabled" : "enabled";
  global_ble_controller->set_command_result("Non-maintenance services are " + enabled_or_disabled +".");
}


BLECommandCustom::BLECommandCustom(const string& name, const string& description, BLEControllerCommandExecutionTrigger* trigger) : BLECommand(name, description), trigger(trigger) {}

void BLECommandCustom::execute(const vector<string>& arguments) const {
  trigger->trigger(arguments);
}

#ifdef USE_LOGGER
BLECommandLogLevel::BLECommandLogLevel() : BLECommand("log-level", "get or set log level (0=None, 4=Config, 5=Debug)") {}

void BLECommandLogLevel::execute(const vector<string>& arguments) const {
  if (!arguments.empty()) {
    string log_level = arguments[0];
    int level = std::strtol(log_level.c_str(), nullptr, 10);
    global_ble_controller->set_log_level(level);
  }
  global_ble_controller->set_command_result("Log level is " + to_string(global_ble_controller->get_log_level())+".");
}
#endif

} // namespace esp32_ble_controller
} // namespace esphome
