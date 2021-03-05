#pragma once

#include "esphome/core/defines.h"

#include "ble_component_handler_base.h"

#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_COVER
#endif
#ifdef USE_FAN
#endif
#ifdef USE_LIGHT
#endif
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif
#ifdef USE_CLIMATE
#endif

using std::string;

namespace esphome {
namespace esp32_ble_controller {

class BLEComponentHandlerFactory {
public:
  static BLEComponentHandlerBase* create_component_handler(Nameable* component, const BLECharacteristicInfoForHandler& characteristicInfo);

#ifdef USE_BINARY_SENSOR
  static BLEComponentHandlerBase* create_binary_sensor_handler(binary_sensor::BinarySensor* component, const BLECharacteristicInfoForHandler& characteristicInfo);
#endif

#ifdef USE_COVER
#endif

#ifdef USE_FAN
#endif

#ifdef USE_LIGHT
#endif

#ifdef USE_SENSOR
  static BLEComponentHandlerBase* create_sensor_handler(sensor::Sensor* component, const BLECharacteristicInfoForHandler& characteristicInfo);
#endif

#ifdef USE_SWITCH
  static BLEComponentHandlerBase* create_switch_handler(switch_::Switch* component, const BLECharacteristicInfoForHandler& characteristicInfo);
#endif

#ifdef USE_TEXT_SENSOR
  static BLEComponentHandlerBase* create_text_sensor_handler(text_sensor::TextSensor* component, const BLECharacteristicInfoForHandler& characteristicInfo);
#endif

#ifdef USE_CLIMATE
#endif
};

} // namespace esp32_ble_controller
} // namespace esphome
