#include <BLE2902.h>

#include "ble_maintenance_handler.h"

#ifdef USE_LOGGER
#include "esphome/components/logger/logger.h"
#endif

// https://www.uuidgenerator.net
#define SERVICE_UUID                  "7b691dff-9062-4192-b46a-692e0da81d91"
#define CHARACTERISTIC_UUID_MODE      "9484a6ab-54c9-4432-bff9-13bada528ab7"
#define CHARACTERISTIC_UUID_LOGGING   "a1083f3b-0ad6-49e0-8a9d-56eb5bf462ca"
#define CHARACTERISTIC_UUID_LOG_LEVEL "d2af61d2-5086-4a99-94e9-6638edc3d14c"

namespace esphome {
namespace esp32_ble_controller {

static const char *TAG = "ble_maintenance_handler";

void BLEMaintenanceHandler::setup(BLEServer* ble_server) {
  ESP_LOGCONFIG(TAG, "Setting up maintenance service");

  BLEService* service = ble_server->createService(SERVICE_UUID);

  ble_mode_characteristic = service->createCharacteristic(CHARACTERISTIC_UUID_MODE,
      BLECharacteristic::PROPERTY_READ //
      | BLECharacteristic::PROPERTY_NOTIFY
      | BLECharacteristic::PROPERTY_WRITE
  );
  uint16_t mode = (uint16_t) global_ble_controller->get_ble_mode();
  ble_mode_characteristic->setValue(mode);
  ble_mode_characteristic->setCallbacks(this);

  esp_gatt_perm_t access_permissions;
  if (is_security_enabled()) {
    access_permissions = ESP_GATT_PERM_READ_ENC_MITM | ESP_GATT_PERM_WRITE_ENC_MITM; // signing (ESP_GATT_PERM_WRITE_SIGNED_MITM) did not work with iPhone
  } else {
    access_permissions = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE;
  }
  ble_mode_characteristic->setAccessPermissions(access_permissions);

  BLEDescriptor* mode_descriptor_2901 = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  mode_descriptor_2901->setAccessPermissions(access_permissions);
  mode_descriptor_2901->setValue("BLE Mode (0=BLE, 1=mixed, 2=WiFi)");
  ble_mode_characteristic->addDescriptor(mode_descriptor_2901);
  BLEDescriptor* mode_descriptor_2902 = new BLE2902();
  mode_descriptor_2902->setAccessPermissions(access_permissions);
  ble_mode_characteristic->addDescriptor(mode_descriptor_2902);

#ifdef USE_LOGGER
  log_level = ESPHOME_LOG_LEVEL;
  if (global_ble_controller->get_ble_mode() != BLEMaintenanceMode::BLE_ONLY) {
    log_level = ESPHOME_LOG_LEVEL_CONFIG;
  }

  logging_characteristic = service->createCharacteristic(CHARACTERISTIC_UUID_LOGGING,
      BLECharacteristic::PROPERTY_READ //
      | BLECharacteristic::PROPERTY_NOTIFY
  );
  logging_characteristic->setAccessPermissions(access_permissions);

  BLEDescriptor* logging_descriptor_2901 = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  logging_descriptor_2901->setAccessPermissions(access_permissions);
  logging_descriptor_2901->setValue("Log messages");
  logging_characteristic->addDescriptor(logging_descriptor_2901);
  BLE2902* logging_descriptor_2902 = new BLE2902();
  logging_descriptor_2902->setAccessPermissions(access_permissions);
  logging_characteristic->addDescriptor(logging_descriptor_2902);
  
  log_level_characteristic = service->createCharacteristic(CHARACTERISTIC_UUID_LOG_LEVEL,
      BLECharacteristic::PROPERTY_READ //
      | BLECharacteristic::PROPERTY_NOTIFY //
      | BLECharacteristic::PROPERTY_WRITE
  );
  log_level_characteristic->setAccessPermissions(access_permissions);
  log_level_characteristic->setValue(log_level);
  log_level_characteristic->setCallbacks(this);

  BLEDescriptor* log_level_descriptor_2901 = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  log_level_descriptor_2901->setAccessPermissions(access_permissions);
  log_level_descriptor_2901->setValue("Log level (0=None, 4=Config, 5=Debug)");
  log_level_characteristic->addDescriptor(log_level_descriptor_2901);
  BLE2902* log_level_descriptor_2902 = new BLE2902();
  log_level_descriptor_2902->setAccessPermissions(access_permissions);
  log_level_characteristic->addDescriptor(log_level_descriptor_2902);
#endif

  service->start();

#ifdef USE_LOGGER
  // NOTE: We register the callback after the service has been started!
  if (logger::global_logger != nullptr) {
    logger::global_logger->add_on_log_callback([this](int level, const char *tag, const char *message) {
      // publish log message
      this->send_log_message(level, tag, message);
    });
  }
#endif    
}

void BLEMaintenanceHandler::onWrite(BLECharacteristic *characteristic) {
  if (characteristic == ble_mode_characteristic) {
    global_ble_controller->execute_in_loop([this](){ on_mode_written(); });
#ifdef USE_LOGGER
  } else if (characteristic == log_level_characteristic) {
    global_ble_controller->execute_in_loop([this](){ on_log_level_written(); });
#endif
  } else {
    ESP_LOGW(TAG, "Unknown characteristic written!");
  }
}

void BLEMaintenanceHandler::on_mode_written() {
    std::string value = ble_mode_characteristic->getValue();
    if (value.length() == 1) {
      uint8_t mode = value[0];
      ESP_LOGD(TAG, "BLE mode chracteristic written: %d", mode);
      global_ble_controller->set_ble_mode(mode);
    }
}

#ifdef USE_LOGGER
void BLEMaintenanceHandler::on_log_level_written() {
    std::string value = log_level_characteristic->getValue();
    if (value.length() == 1) {
      uint8_t level = value[0];
      ESP_LOGD(TAG, "Log level chracteristic written: %d", level);
      set_log_level(level);
    }
}

/**
 * Removes magic logger symbols from the message, e.g., sequences that mark the start or the end, or a color.
 */
string remove_logger_magic(const string& message) {
  // Note: We do not use regex replacement because it enlarges the binary by roughly 50kb!
  string result;
  boolean within_magic = false;
  for (string::size_type i = 0; i < message.length() - 1; ++i) {
    if (message[i] == '\033' && message[i+1] == '[') { // log magis always starts with "\033[" see log.h
      within_magic = true;
      ++i;
    } else if (within_magic) {
      within_magic = (message[i] != 'm');
    } else {
      result.push_back(message[i]);
    }
  }
  return result;
}

void BLEMaintenanceHandler::send_log_message(int level, const char *tag, const char *message) {
  if (level <= this->log_level) {
    logging_characteristic->setValue(remove_logger_magic(message));
    logging_characteristic->notify();
  }
}
#endif

bool BLEMaintenanceHandler::is_security_enabled() {
  return global_ble_controller->get_security_enabled();
}

} // namespace esp32_ble_controller
} // namespace esphome
