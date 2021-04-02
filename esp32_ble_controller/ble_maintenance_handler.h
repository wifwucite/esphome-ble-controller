#pragma once

#include <BLEServer.h>
#include <BLECharacteristic.h>

#include "esphome/core/defines.h"
#include "esphome/core/log.h"

#include "esp32_ble_controller.h"

namespace esphome {
namespace esp32_ble_controller {

/**
 * Provides standard maintenance support for the BLE controller like logging over BLE and controlling BLE mode.
 * It does not control individual ESPHome components (like sensors, switches, ...), but rather provides generic global functionality.
 * It provides a special BLE service with its own characteristics.
 * @brief Provides maintenance support for BLE clients (like controlling the BLE mode and logging over BLE)
 */
class BLEMaintenanceHandler : private BLECharacteristicCallbacks {
public:
  virtual ~BLEMaintenanceHandler() {}

  void setup(BLEServer* ble_server);

  void set_ble_mode(BLEMaintenanceMode mode);

#ifdef USE_LOGGER
  void set_log_level(int level) { log_level = level; }

  void send_log_message(int level, const char *tag, const char *message);
#endif

private:
  virtual void onWrite(BLECharacteristic *characteristic) override;
  void on_mode_written();
#ifdef USE_LOGGER
  void on_log_level_written();
#endif

  bool is_security_enabled();
  
private:
  BLEService* maintenance_service;

  BLECharacteristic* ble_mode_characteristic;

#ifdef USE_LOGGER
  int log_level;

  BLECharacteristic* logging_characteristic;
  BLECharacteristic* log_level_characteristic;
#endif

};

} // namespace esp32_ble_controller
} // namespace esphome
