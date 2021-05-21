#pragma once

#include "esphome/core/automation.h"

#include "esp32_ble_controller.h"

namespace esphome {
namespace esp32_ble_controller {

/// Trigger for showing the pass key during authentication with a client.
class BLEControllerShowPassKeyTrigger : public Trigger<std::string> {
public:
  BLEControllerShowPassKeyTrigger(ESP32BLEController* controller) {
    controller->add_on_show_pass_key_callback([this](string pass_key) {
      this->trigger(pass_key);
    });
  }
};

/// Trigger that is fired when authentication with a client is completed (either with success or failure).
class BLEControllerAuthenticationCompleteTrigger : public Trigger<boolean> {
public:
  BLEControllerAuthenticationCompleteTrigger(ESP32BLEController* controller) {
    controller->add_on_authentication_complete_callback([this](boolean success) {
      this->trigger(success);
    });
  }
};

/// Trigger that is fired when the BLE server has connected to a client.
class BLEControllerServerConnectedTrigger : public Trigger<> {
public:
  BLEControllerServerConnectedTrigger(ESP32BLEController* controller) {
    controller->add_on_connected_callback([this]() {
      this->trigger();
    });
  }
};

/// Trigger that is fired when the BLE server has disconnected from a client.
class BLEControllerServerDisconnectedTrigger : public Trigger<> {
public:
  BLEControllerServerDisconnectedTrigger(ESP32BLEController* controller) {
    controller->add_on_disconnected_callback([this]() {
      this->trigger();
    });
  }
};

/// Trigger that is fired when a command is executed.
class BLEControllerCommandExecutionTrigger : public Trigger<std::vector<std::string>> {
public:
  BLEControllerCommandExecutionTrigger(ESP32BLEController* controller) {}
};

} // namespace esp32_ble_controller
} // namespace esphome
