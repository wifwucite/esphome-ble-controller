/*
  Lines to be added to main:
  esp32_ble_controller::esp32_ble_controller* ble_controller = new esp32_ble_controller::esp32_ble_controller();
  App.register_component(ble_controller);
*/

#include <BLEDevice.h>
#include <BLEServer.h>

#include "esphome/core/application.h"
#include "esphome/core/log.h"

#include <esp_bt_main.h>

#include "esp32_ble_controller.h"
#include "ble_maintenance_handler.h"
#include "ble_component_handler_factory.h"

namespace esphome {
namespace esp32_ble_controller {

static const char *TAG = "esp32_ble_controller";

void ESP32BLEController::register_component(Nameable* component, const string& serviceUUID, const string& characteristicUUID, bool useBLE2902) {
  BLECharacteristicInfoForHandler info;
  info.serviceUUID = serviceUUID;
  info.characteristicUUID = characteristicUUID;
  info.useBLE2902 = useBLE2902;

  infoForComponent[component->get_object_id()] = info;
}

void ESP32BLEController::setup() {
  ESP_LOGCONFIG(TAG, "Setting up BLE controller...");

  initialize_ble_mode();

  if (!setup_ble()) {
    return;
  }

  if (global_ble_controller == nullptr) {
    global_ble_controller = this;
  } else {
    ESP_LOGE(TAG, "Already have an instance of the BLE controller");
  }

  // Create the BLE Device
  BLEDevice::init(App.get_name());

  setup_ble_services();

  setup_controller();

  // Start advertising
  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  // advertising->setScanResponse(false);
  // advertising->setMinPreferred(0x6);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
}

bool ESP32BLEController::setup_ble() {
  if (btStarted()) {
    ESP_LOGI(TAG, "BLE already started");
    return true;
  }

  ESP_LOGI(TAG, "  Setting up BLE ...");

  esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

  // Initialize the bluetooth controller with the default configuration
  if (!btStart()) {
    ESP_LOGE(TAG, "btStart failed: %d", esp_bt_controller_get_status());
    mark_failed();
    return false;
  }

  esp_err_t err = esp_bluedroid_init();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_bluedroid_init failed: %d", err);
    mark_failed();
    return false;
  }
  err = esp_bluedroid_enable();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_bluedroid_enable failed: %d", err);
    mark_failed();
    return false;
  }

  return true;
}

void ESP32BLEController::setup_ble_services() {
  bleServer = BLEDevice::createServer();

  maintenanceHandler = new BLEMaintenanceHandler();
  maintenanceHandler->setup(bleServer);

  if (get_ble_mode() != BLEMaintenanceMode::WIFI_ONLY) {
    setup_ble_services_for_components();
  }
}

void ESP32BLEController::setup_ble_services_for_components() {
#ifdef USE_BINARY_SENSOR
  setup_ble_services_for_components(App.get_binary_sensors(), BLEComponentHandlerFactory::create_binary_sensor_handler);
#endif
#ifdef USE_COVER
  setup_ble_services_for_components(App.get_covers());
#endif
#ifdef USE_FAN
  setup_ble_services_for_components(App.get_fans());
#endif
#ifdef USE_LIGHT
  setup_ble_services_for_components(App.get_lights());
#endif
#ifdef USE_SENSOR
  setup_ble_services_for_components(App.get_sensors(), BLEComponentHandlerFactory::create_sensor_handler);
#endif
#ifdef USE_SWITCH
  setup_ble_services_for_components(App.get_switches(), BLEComponentHandlerFactory::create_switch_handler);
#endif
#ifdef USE_TEXT_SENSOR
  setup_ble_services_for_components(App.get_text_sensors(), BLEComponentHandlerFactory::create_text_sensor_handler);
#endif
#ifdef USE_CLIMATE
  setup_ble_services_for_components(App.get_climates());
#endif

  for (auto const& entry : handlerForComponent) {
    entry.second->setup(bleServer);
  }
}

template <typename C> 
void ESP32BLEController::setup_ble_services_for_components(const vector<C*>& components, BLEComponentHandlerBase* (*handler_creator)(C*, const BLECharacteristicInfoForHandler&)) {
  for (C* component: components) {
    setup_ble_service_for_component(component, handler_creator);
  }
}

template <typename C> 
void ESP32BLEController::setup_ble_service_for_component(C* component, BLEComponentHandlerBase* (*handler_creator)(C*, const BLECharacteristicInfoForHandler&)) {
  static_assert(std::is_base_of<Nameable, C>::value, "Nameable subclasses expected");

  auto object_id = component->get_object_id();
  if (infoForComponent.count(object_id)) {
    auto info = infoForComponent[object_id];
    handlerForComponent[object_id] = handler_creator(component, info);
  }
}

void ESP32BLEController::initialize_ble_mode() {
  bleModePreference = global_preferences.make_preference<uint8_t>(fnv1_hash("BLEMaintenanceMode"));

  uint8_t mode;
  if (!bleModePreference.load(&mode)) {
    mode = (uint8_t) BLEMaintenanceMode::BLE_ONLY;
  }

  bleMode = static_cast<BLEMaintenanceMode>(mode);
  
  ESP_LOGCONFIG(TAG, "BLE mode: %d", mode);
}


void ESP32BLEController::set_ble_mode(BLEMaintenanceMode mode) {
  set_ble_mode((uint8_t) mode);
}

void ESP32BLEController::set_ble_mode(uint8_t newMode) {
  if (newMode > (uint8_t) BLEMaintenanceMode::WIFI_ONLY) {
    ESP_LOGI(TAG, "Ignoring unsupported BLE mode %d", newMode);
    return;
  }

  ESP_LOGI(TAG, "Updating BLE mode to %d", newMode);
  BLEMaintenanceMode newBleMode = static_cast<BLEMaintenanceMode>(newMode);
  if (bleMode != newBleMode) {
    bleMode = newBleMode;
    bleModePreference.save(&bleMode);

    App.safe_reboot();
  }
}

void ESP32BLEController::dump_config() {
  ESP_LOGCONFIG(TAG, "Bluetooth Low Energy Controller:");
  ESP_LOGCONFIG(TAG, "  BLE mode: %d", (uint8_t) bleMode);
}

#ifdef USE_BINARY_SENSOR
  void ESP32BLEController::on_binary_sensor_update(binary_sensor::BinarySensor *obj, bool state) { update_component_state(obj, state); }
#endif
#ifdef USE_COVER
  void ESP32BLEController::on_cover_update(cover::Cover *obj) {}
#endif
#ifdef USE_FAN
  void ESP32BLEController::on_fan_update(fan::FanState *obj) {}
#endif
#ifdef USE_LIGHT
  void ESP32BLEController::on_light_update(light::LightState *obj) {}
#endif
#ifdef USE_SENSOR
  void ESP32BLEController::on_sensor_update(sensor::Sensor *component, float state) { update_component_state(component, state); }
#endif
#ifdef USE_SWITCH
  void ESP32BLEController::on_switch_update(switch_::Switch *obj, bool state) { update_component_state(obj, state); }
#endif
#ifdef USE_TEXT_SENSOR
  void ESP32BLEController::on_text_sensor_update(text_sensor::TextSensor *obj, std::string state) { update_component_state(obj, state); }
#endif
#ifdef USE_CLIMATE
  void ESP32BLEController::on_climate_update(climate::Climate *obj) {}
#endif

template <typename C, typename S> 
void ESP32BLEController::update_component_state(C* component, S state) {
  static_assert(std::is_base_of<Nameable, C>::value, "Nameable subclasses expected");

  auto object_id = component->get_object_id();
  BLEComponentHandlerBase* handler = handlerForComponent[object_id];
  if (handler != nullptr) {
    handler->send_value(state);
  }
}

ESP32BLEController* global_ble_controller = nullptr;

} // namespace esp32_ble_controller
} // namespace esphome
