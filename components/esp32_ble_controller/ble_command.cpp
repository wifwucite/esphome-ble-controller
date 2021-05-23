#include "ble_command.h"

#include "esp32_ble_controller.h"
#include "automation.h"

namespace esphome {
namespace esp32_ble_controller {

// help ///////////////////////////////////////////////////////////////////////////////////////////////

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

// ble-services ///////////////////////////////////////////////////////////////////////////////////////////////

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

// log-level ///////////////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_LOGGER
BLECommandLogLevel::BLECommandLogLevel() : BLECommand("log-level", "get or set log level (0=None, 4=Config, 5=Debug)") {}

void BLECommandLogLevel::execute(const vector<string>& arguments) const {
  if (!arguments.empty()) {
    string log_level = arguments[0];
    const optional<int> level = parse_int(log_level);
    if (level.has_value()) {
      global_ble_controller->set_log_level(level.value());
    }
  }
  global_ble_controller->set_command_result("Log level is " + to_string(global_ble_controller->get_log_level())+".");
}
#endif

// custom ///////////////////////////////////////////////////////////////////////////////////////////////

BLECustomCommand::BLECustomCommand(const string& name, const string& description, BLEControllerCustomCommandExecutionTrigger* trigger) : BLECommand(name, description), trigger(trigger) {}

void BLECustomCommand::execute(const vector<string>& arguments) const {
  BLECustomCommandResultHolder result_holder;
  trigger->trigger(arguments, result_holder);
  optional<string> result = result_holder.get_result();
  if (result.has_value()) {
    ESP_LOGI(TAG, "Setting result %s", result.value().c_str());
    global_ble_controller->set_command_result(result.value());
  }
}

} // namespace esp32_ble_controller
} // namespace esphome
