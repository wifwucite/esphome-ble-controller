#include "wifi_configuration_handler.h"

#include "esphome/core/application.h"
#include "esphome/core/log.h"
#include "esphome/components/wifi/wifi_component.h"

namespace esphome {
namespace esp32_ble_controller {

static const char *TAG = "wifi_configuration_handler";

void WifiSettingsHandler::setup() {
  // Hash with compilation time
  // This ensures the AP override is not applied for OTA
  uint32_t hash = fnv1_hash("wifi_settings#" + App.get_compilation_time());
  wifi_settings_preference = global_preferences.make_preference<WifiSettings>(hash, true);

  WifiSettings settings;
  if (load_settings(settings)) {
    ESP_LOGI(TAG, "Overriding WIFI settings with stored preferences");
    override_sta(settings);
  }
}

void WifiSettingsHandler::set_credentials(const std::string &ssid, const std::string &password, bool hidden_network) {
  ESP_LOGI(TAG, "Updating WIFI settings");

  WifiSettings settings;

  strncpy(settings.ssid, ssid.c_str(), WIFI_SSID_LEN);
  strncpy(settings.password, password.c_str(), WIFI_PASSWORD_LEN);
  settings.hidden_network = hidden_network;

  if (!save_settings(settings)) {
    ESP_LOGE(TAG, "Could not save new WIFI settings");
  }

  override_sta(settings);
}

void WifiSettingsHandler::clear_credentials() {
  ESP_LOGI(TAG, "Clearing WIFI settings");
    
  WifiSettings settings;
  settings.ssid[0] = 0;

  if (!save_settings(settings)) {
    ESP_LOGE(TAG, "Could not clear WIFI settings");
  }
}

optional<std::string> WifiSettingsHandler::get_current_ssid() const {
  WifiSettings settings;
  if (load_settings(settings)) {
    return make_optional<std::string>(settings.ssid);
  } else {
    return optional<std::string>();
  }
}

bool WifiSettingsHandler::load_settings(WifiSettings& settings) const {
  return const_cast<WifiSettingsHandler*>(this)->wifi_settings_preference.load(&settings) && strlen(settings.ssid);
}

bool WifiSettingsHandler::save_settings(const WifiSettings& settings) {
  return wifi_settings_preference.save(&settings);
}

void WifiSettingsHandler::override_sta(const WifiSettings& settings) {
  wifi::WiFiAP sta;

  sta.set_ssid(settings.ssid);
  sta.set_password(settings.password);
  sta.set_hidden(settings.hidden_network);

  wifi::global_wifi_component->set_sta(sta);
}

} // namespace esp32_ble_controller
} // namespace esphome
