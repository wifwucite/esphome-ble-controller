#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <BLEServer.h>

#include "esphome/core/component.h"
#include "esphome/core/controller.h"
#include "esphome/core/defines.h"
#include "esphome/core/preferences.h"

#include "ble_component_handler_base.h"
#include "ble_maintenance_handler.h"
#include "thread_safe_bounded_queue.h"

using std::string;
using std::unordered_map;
using std::vector;

namespace esphome {
namespace esp32_ble_controller {

class BLEControllerCommandExecutionTrigger;

/**
 * Bluetooth Low Energy controller for ESP32.
 * It provides a BLE server that can BLE clients like mobile phones can connect to and access components (like reading sensor values and control switches).
 * In addition it provides maintenance feature like a BLE mode and logging over BLE.
 * <para>
 * Besides the generic maintenance, this controller only exposes components over BLE that have been registered before (i.e. configured explicitly in the yaml configuration).
 * @brief BLE controller for ESP32
 */
class ESP32BLEController : public Component, public Controller, private BLESecurityCallbacks {
public:
  ESP32BLEController();
  virtual ~ESP32BLEController() {}

  void register_command(const string& name, const string& description, BLEControllerCommandExecutionTrigger* trigger);
  const vector<BLECommand*>& get_commands() const;

  void register_component(Nameable* component, const string& service_UUID, const string& characteristic_UUID, bool use_BLE2902 = true);

  float get_setup_priority() const override { return setup_priority::PROCESSOR; }

  void setup() override;

  void dump_config() override;

  void loop() override;

  inline BLEMaintenanceMode get_ble_mode() const { return ble_mode; }
  void set_ble_mode(BLEMaintenanceMode mode);
  void set_ble_mode(uint8_t mode);

  void set_security_enabled(bool enabled);
  inline bool get_security_enabled() const { return security_enabled; }

  void set_command_result(string result_message);

#ifdef USE_LOGGER
  int get_log_level() { return maintenance_handler->get_log_level(); }
  void set_log_level(int level) { maintenance_handler->set_log_level(level); }
#endif

  // Controller methods:
#ifdef USE_BINARY_SENSOR
  void on_binary_sensor_update(binary_sensor::BinarySensor *obj, bool state) override;
#endif
#ifdef USE_COVER
  void on_cover_update(cover::Cover *obj) override;
#endif
#ifdef USE_FAN
  void on_fan_update(fan::FanState *obj) override;
#endif
#ifdef USE_LIGHT
  void on_light_update(light::LightState *obj) override;
#endif
#ifdef USE_SENSOR
  void on_sensor_update(sensor::Sensor *obj, float state) override;
#endif
#ifdef USE_SWITCH
  void on_switch_update(switch_::Switch *obj, bool state) override;
#endif
#ifdef USE_TEXT_SENSOR
  void on_text_sensor_update(text_sensor::TextSensor *obj, std::string state) override;
#endif
#ifdef USE_CLIMATE
  void on_climate_update(climate::Climate *obj) override;
#endif

  void add_on_show_pass_key_callback(std::function<void(string)>&& trigger_function);
  void add_on_authentication_complete_callback(std::function<void(boolean)>&& trigger_function);

  /// Executes a given function in the main loop of the app. (Can be called from another RTOS task.)
  void execute_in_loop(std::function<void()>&& deferred_function);

private:
  void initialize_ble_mode();

  bool setup_ble();
  void setup_ble_services();
  void setup_ble_services_for_components();
  template <typename C> void setup_ble_services_for_components(const vector<C*>& components, BLEComponentHandlerBase* (*handler_creator)(C*, const BLECharacteristicInfoForHandler&));
  template <typename C> void setup_ble_service_for_component(C* component, BLEComponentHandlerBase* (*handler_creator)(C*, const BLECharacteristicInfoForHandler&));
  template <typename C, typename S> void update_component_state(C* component, S state);

  void enable_ble_security();
  virtual uint32_t onPassKeyRequest(); // inherited from BLESecurityCallbacks
  virtual void onPassKeyNotify(uint32_t pass_key); // inherited from BLESecurityCallbacks
  virtual bool onSecurityRequest(); // inherited from BLESecurityCallbacks
  virtual void onAuthenticationComplete(esp_ble_auth_cmpl_t); // inherited from BLESecurityCallbacks
  virtual bool onConfirmPIN(uint32_t pin); // inherited from BLESecurityCallbacks
  
private:
  BLEServer* bleServer;

  BLEMaintenanceMode ble_mode;
  ESPPreferenceObject ble_mode_preference;

  bool security_enabled{true};

  BLEMaintenanceHandler* maintenance_handler;

  unordered_map<string, BLECharacteristicInfoForHandler> info_for_component;
  unordered_map<string, BLEComponentHandlerBase*> handler_for_component;

  ThreadSafeBoundedQueue<std::function<void()>> deferred_functions_for_loop{16};

  CallbackManager<void(string)> on_show_pass_key_callbacks;
  CallbackManager<void(boolean)> on_authentication_complete_callbacks;
};

/// The BLE controller singleton.
extern ESP32BLEController* global_ble_controller;

} // namespace esp32_ble_controller
} // namespace esphome
