#include "ble_component_handler_base.h"

#include <BLE2902.h>

#include "esphome/core/log.h"

#include "esp32_ble_controller.h"

namespace esphome {
namespace esp32_ble_controller {

static const char *TAG = "ble_component_handler_base";

/* class MyCallbacks: public BLECharacteristicCallbacks {
public:
    MyCallbacks(BLEComponentHandler& my_handler) : my_handler(my_handler) {}

    void onWrite(BLECharacteristic *characteristic) {
      my_handler.on_characteristic_written();
      std::string value = characteristic->getValue();

      if (value.length() > 0) {
        ESP_LOGD(TAG, "Value = %s", value.c_str());
      }
    }

private:
  BLEComponentHandler& my_handler;
};
 */

BLEComponentHandlerBase::BLEComponentHandlerBase(Nameable* component, const BLECharacteristicInfoForHandler& characteristicInfo) 
  : component(component), characteristicInfo(characteristicInfo)
{}

BLEComponentHandlerBase::~BLEComponentHandlerBase() 
{}

void BLEComponentHandlerBase::setup(BLEServer* bleServer) {
  auto object_id = component->get_object_id();

  ESP_LOGCONFIG(TAG, "Setting up BLE characteristic for device %s", object_id.c_str());

  const string& serviceUUID = characteristicInfo.serviceUUID;
  ESP_LOGCONFIG(TAG, "Setting up service %s", serviceUUID.c_str());
  BLEService* service = bleServer->getServiceByUUID(serviceUUID);
  if (service == nullptr) {
    service = bleServer->createService(serviceUUID);
  }

  const string& characteristicUUID = characteristicInfo.characteristicUUID;
  ESP_LOGCONFIG(TAG, "Setting up char %s", characteristicUUID.c_str());

  uint32_t properties = BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY;
  if (this->can_receive_writes()) {
    properties |= BLECharacteristic::PROPERTY_WRITE;
  }

  characteristic = service->createCharacteristic(characteristicUUID, properties);
  // characteristic = service->createCharacteristic(characteristicUUID,
  //     BLECharacteristic::PROPERTY_READ //
  //     | BLECharacteristic::PROPERTY_NOTIFY
  //     | BLECharacteristic::PROPERTY_WRITE
  //     //| BLECharacteristic::PROPERTY_INDICATE
  // );
  if (this->can_receive_writes()) {
    characteristic->setCallbacks(this);
  }

  if (global_ble_controller->get_security_enabled()) {
    esp_gatt_perm_t permissions = ESP_GATT_PERM_READ_ENC_MITM;
    if (this->can_receive_writes()) {
      permissions |= ESP_GATT_PERM_WRITE_ENC_MITM; // signing (ESP_GATT_PERM_WRITE_SIGNED_MITM) did not work with iPhone
    }
    characteristic->setAccessPermissions(permissions);
  }

  BLEDescriptor* descriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  descriptor->setValue(component->get_name());
  characteristic->addDescriptor(descriptor);

  if (characteristicInfo.useBLE2902)
  {
    // With this descriptor clients can switch notifications on and off, but we want to send notifications anyway as long as we are connected. The homebridge plug-in cannot turn notifications on and off.
    // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
    characteristic->addDescriptor(new BLE2902());
  }

  service->start();

  ESP_LOGCONFIG(TAG, "%s: SRV %s - CHAR %s", object_id.c_str(), serviceUUID.c_str(), characteristicUUID.c_str());
}

void BLEComponentHandlerBase::send_value(float value) {
  auto object_id = component->get_object_id();
  ESP_LOGD(TAG, "Update component %s to %f", object_id.c_str(), value);

  characteristic->setValue(value);
  characteristic->notify();
}

void BLEComponentHandlerBase::send_value(string value) {
  auto object_id = component->get_object_id();
  ESP_LOGD(TAG, "Update component %s to %s", object_id.c_str(), value.c_str());

  characteristic->setValue(value);
  characteristic->notify();
}

void BLEComponentHandlerBase::send_value(bool raw_value) {
  auto object_id = component->get_object_id();
  ESP_LOGD(TAG, "Updating component %s to %d", object_id.c_str(), raw_value);

  uint16_t value = raw_value;
  characteristic->setValue(value);
  characteristic->notify();
}

void BLEComponentHandlerBase::onWrite(BLECharacteristic *characteristic) {
  ESP_LOGD(TAG, "onWrite called");
  on_characteristic_written();
}

} // namespace esp32_ble_controller
} // namespace esphome
