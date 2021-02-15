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
  string serviceUUID;
  string characteristicUUID;
  boolean useBLE2902;
};

class BLEComponentHandlerBase : private BLECharacteristicCallbacks {
public:
  BLEComponentHandlerBase(Nameable* component, const BLECharacteristicInfoForHandler& characteristicInfo);
  virtual ~BLEComponentHandlerBase();

  void setup(BLEServer* bleServer);

  void send_value(float value);
  void send_value(string value);

  void set_value(bool value);
  void send_value(bool value);

protected:
  virtual Nameable* get_component() { return component; }
  BLECharacteristic* get_characteristic() { return characteristic; }

  virtual bool can_receive_writes() { return false; }
  virtual void on_characteristic_written() {}
  
private:
  virtual void onWrite(BLECharacteristic *characteristic);

  Nameable* component;
  BLECharacteristicInfoForHandler characteristicInfo;

  BLECharacteristic* characteristic;
};

} // namespace esp32_ble_controller
} // namespace esphome
