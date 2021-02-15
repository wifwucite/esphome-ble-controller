#include "ble_maintenance_handler.h"

#include <regex>

#include <BLE2902.h>

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

void BLEMaintenanceHandler::setup(BLEServer* bleServer) {
  ESP_LOGCONFIG(TAG, "Setting up maintenance service");

  BLEService* service = bleServer->createService(SERVICE_UUID);

  bleModeCharacteristic = service->createCharacteristic(CHARACTERISTIC_UUID_MODE,
      BLECharacteristic::PROPERTY_READ //
      | BLECharacteristic::PROPERTY_NOTIFY
      | BLECharacteristic::PROPERTY_WRITE
  );
  uint16_t mode = (uint16_t) global_ble_controller->get_ble_mode();
  bleModeCharacteristic->setValue(mode);
  bleModeCharacteristic->setCallbacks(this);

  BLEDescriptor* modeDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  modeDescriptor->setValue("BLE Mode (0=BLE, 1=mixed, 2=WiFi)");
  bleModeCharacteristic->addDescriptor(modeDescriptor);
  bleModeCharacteristic->addDescriptor(new BLE2902());

#ifdef USE_LOGGER
  log_level = ESPHOME_LOG_LEVEL;
  if (global_ble_controller->get_ble_mode() != BLEMaintenanceMode::BLE_ONLY) {
    log_level = ESPHOME_LOG_LEVEL_CONFIG;
  }

  loggingCharacteristic = service->createCharacteristic(CHARACTERISTIC_UUID_LOGGING,
      BLECharacteristic::PROPERTY_READ //
      | BLECharacteristic::PROPERTY_NOTIFY
  );

  BLEDescriptor* loggingDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  loggingDescriptor->setValue("Log messages");
  loggingCharacteristic->addDescriptor(loggingDescriptor);
  loggingCharacteristic->addDescriptor(new BLE2902());
  
  logLevelCharacteristic = service->createCharacteristic(CHARACTERISTIC_UUID_LOG_LEVEL,
      BLECharacteristic::PROPERTY_READ //
      | BLECharacteristic::PROPERTY_NOTIFY //
      | BLECharacteristic::PROPERTY_WRITE
  );
  logLevelCharacteristic->setValue(log_level);
  logLevelCharacteristic->setCallbacks(this);

  BLEDescriptor* logLevelDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  logLevelDescriptor->setValue("Log level (0=None, 4=Config, 5=Debug)");
  logLevelCharacteristic->addDescriptor(logLevelDescriptor);
  logLevelCharacteristic->addDescriptor(new BLE2902());
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
  if (characteristic == bleModeCharacteristic) {
    on_mode_written();
#ifdef USE_LOGGER
  } else if (characteristic == logLevelCharacteristic) {
    on_log_level_written();
#endif
  } else {
    ESP_LOGW(TAG, "Unknown characteristic written!");
  }
}

void BLEMaintenanceHandler::on_mode_written() {
    std::string value = bleModeCharacteristic->getValue();
    if (value.length() == 1) {
      uint8_t mode = value[0];
      ESP_LOGD(TAG, "BLE mode chracteristic written: %d", mode);
      global_ble_controller->set_ble_mode(mode);
    }
}

#ifdef USE_LOGGER
void BLEMaintenanceHandler::on_log_level_written() {
    std::string value = logLevelCharacteristic->getValue();
    if (value.length() == 1) {
      uint8_t level = value[0];
      ESP_LOGD(TAG, "Log level chracteristic written: %d", level);
      set_log_level(level);
    }
}

/**
 * Removes magic logger symbols from the message, e.g., sequences that mark the start or the end, or a color.
 */
string removeLoggerMagic(const string& message) {
  // Note: We do not use regex replacement because it enlarges the binary by roughly 50kb!
  string result;
  boolean withinMagic = false;
  for (string::size_type i = 0; i < message.length() - 1; ++i) {
    if (message[i] == '\033' && message[i+1] == '[') { // log magis always starts with "\033[" see log.h
      withinMagic = true;
      ++i;
    } else if (withinMagic) {
      withinMagic = (message[i] != 'm');
    } else {
      result.push_back(message[i]);
    }
  }
  return result;
}

void BLEMaintenanceHandler::send_log_message(int level, const char *tag, const char *message) {
  if (level <= this->log_level) {
    loggingCharacteristic->setValue(removeLoggerMagic(message));
    loggingCharacteristic->notify();
  }
}
#endif

} // namespace esp32_ble_controller
} // namespace esphome
