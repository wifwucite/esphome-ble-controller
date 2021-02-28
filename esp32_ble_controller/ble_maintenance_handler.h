#pragma once

#include <BLEServer.h>
#include <BLECharacteristic.h>

#include "esphome/core/defines.h"
#include "esphome/core/log.h"

#include "esp32_ble_controller.h"

namespace esphome {
namespace esp32_ble_controller {

/**
 * Provides standard BLE functionality that does not depend on particular component configuration. This includes logging over BLE and controlling BLE mode.
 * This handler exposes a BLE maintenance service with characteristics for the mode and for logging.
 * @brief BLE mode characteristic and logging over BLE
 */
class BLEMaintenanceHandler : private BLECharacteristicCallbacks {
public:
  virtual ~BLEMaintenanceHandler() {}

  void setup(BLEServer* bleServer);

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
  BLEService* maintenanceService;

  BLECharacteristic* bleModeCharacteristic;

#ifdef USE_LOGGER
  int log_level;

  BLECharacteristic* loggingCharacteristic;
  BLECharacteristic* logLevelCharacteristic;
#endif

};

} // namespace esp32_ble_controller
} // namespace esphome
