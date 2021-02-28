#include "ble_component_handler_base.h"

#include <BLE2902.h>

#include "esphome/core/log.h"

#include "esp32_ble_controller.h"

namespace esphome {
namespace esp32_ble_controller {

static const char *TAG = "ble_component_handler_base";

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

  esp_gatt_perm_t access_permissions;
  if (is_security_enabled()) {
    access_permissions = ESP_GATT_PERM_READ_ENC_MITM | ESP_GATT_PERM_WRITE_ENC_MITM; // signing (ESP_GATT_PERM_WRITE_SIGNED_MITM) did not work with iPhone
  } else {
    access_permissions = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE;
  }
  characteristic->setAccessPermissions(access_permissions);

  BLEDescriptor* descriptor_2901 = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  descriptor_2901->setValue(component->get_name());
  descriptor_2901->setAccessPermissions(access_permissions);
  characteristic->addDescriptor(descriptor_2901);

  if (characteristicInfo.useBLE2902)
  {
    // With this descriptor clients can switch notifications on and off, but we want to send notifications anyway as long as we are connected. The homebridge plug-in cannot turn notifications on and off.
    // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
    BLE2902* descriptor_2902 = new BLE2902();
    descriptor_2902->setAccessPermissions(access_permissions);
    characteristic->addDescriptor(descriptor_2902);
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

bool BLEComponentHandlerBase::is_security_enabled() {
  return global_ble_controller->get_security_enabled();
}

} // namespace esp32_ble_controller
} // namespace esphome
