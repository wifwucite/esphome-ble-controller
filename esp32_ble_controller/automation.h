#pragma once

//#include "esphome/core/component.h"
#include "esphome/core/automation.h"

#include "esp32_ble_controller.h"

namespace esphome {
namespace esp32_ble_controller {

class BLEControllerShowPassKeyTrigger : public Trigger<std::string> {
public:
  BLEControllerShowPassKeyTrigger(ESP32BLEController* controller) {
    controller->add_on_show_pass_key_callback([this](string pass_key) {
      this->trigger(pass_key);
    });
  }
};

} // namespace esp32_ble_controller
} // namespace esphome
