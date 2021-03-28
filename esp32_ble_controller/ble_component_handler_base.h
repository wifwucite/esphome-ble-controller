#pragma once

#include <string>

#include <BLEServer.h>
#include <BLECharacteristic.h>

#include "esphome/core/component.h"
#include "esphome/core/controller.h"
#include "esphome/core/defines.h"

using std::string;

namespace esphome {
namespace esp32_ble_controller {

struct BLECharacteristicInfoForHandler {
  string service_UUID;
  string characteristic_UUID;
  boolean use_BLE2902;
};

class BLEComponentHandlerBase : private BLECharacteristicCallbacks {
public:
  BLEComponentHandlerBase(Nameable* component, const BLECharacteristicInfoForHandler& characteristic_info);
  virtual ~BLEComponentHandlerBase();

  void setup(BLEServer* bleServer);

  void send_value(float value);
  void send_value(string value);
  void send_value(bool value);

protected:
  virtual Nameable* get_component() { return component; }
  BLECharacteristic* get_characteristic() { return characteristic; }

  virtual bool can_receive_writes() { return false; }
  virtual void on_characteristic_written() {}

  bool is_security_enabled();
  
private:
  virtual void onWrite(BLECharacteristic *characteristic); // inherited from BLECharacteristicCallbacks

  Nameable* component;
  BLECharacteristicInfoForHandler characteristic_info;

  BLECharacteristic* characteristic;
};

} // namespace esp32_ble_controller
} // namespace esphome
