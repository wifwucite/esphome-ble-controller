#pragma once

#include "esphome/core/defines.h"
#ifdef USE_FAN

#include <string>

#include "esphome/components/fan/fan.h"

#include "ble_component_handler.h"

using std::string;

namespace esphome {
namespace esp32_ble_controller {

using fan::Fan;

/**
 * Special component handler for fans, which allows turning the fan on and off from a BLE client.
 */
class BLEFanHandler : public BLEComponentHandler<Fan> {
public:
  BLEFanHandler(Fan* component, const BLECharacteristicInfoForHandler& characteristic_info) : BLEComponentHandler(component, characteristic_info) {}
  virtual ~BLEFanHandler() {}

  virtual void send_value(bool value) override;

protected:
  virtual bool can_receive_writes() { return true; }
  virtual void on_characteristic_written() override;
};

} // namespace esp32_ble_controller
} // namespace esphome

#endif
