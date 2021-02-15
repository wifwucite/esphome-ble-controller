#pragma once

#include "esphome/core/defines.h"
#ifdef USE_SWITCH

#include <BLEServer.h>
#include <BLECharacteristic.h>

#include "esphome/components/switch/switch.h"

#include "ble_component_handler.h"

#include <string>
using std::string;

namespace esphome {
namespace esp32_ble_controller {

using switch_::Switch;

class BLESwitchHandler : public BLEComponentHandler<Switch> {
public:
  BLESwitchHandler(Switch* component, const BLECharacteristicInfoForHandler& characteristicInfo) : BLEComponentHandler(component, characteristicInfo) {}
  virtual ~BLESwitchHandler() {}

protected:
  virtual bool can_receive_writes() { return true; }
  virtual void on_characteristic_written() override;
};

} // namespace esp32_ble_controller
} // namespace esphome

#endif