#pragma once

#include "ble_component_handler_base.h"

using std::string;

namespace esphome {
namespace esp32_ble_controller {

/**
 * Provides a BLE characteristic that corresponds to a component (like switch, sensor, ...) to expose the current sensor state via BLE.
 */
template <typename C>
class BLEComponentHandler : public BLEComponentHandlerBase {
public:
  BLEComponentHandler(C* component, const BLECharacteristicInfoForHandler& characteristicInfo) : BLEComponentHandlerBase(component, characteristicInfo) {}
  virtual ~BLEComponentHandler() {}

protected:
  virtual C* get_component() override { return static_cast<C*>(BLEComponentHandlerBase::get_component()); }
  
};

} // namespace esp32_ble_controller
} // namespace esphome
