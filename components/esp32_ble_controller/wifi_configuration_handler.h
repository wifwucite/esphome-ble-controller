#pragma once

#include <string>

#include "esphome/core/defines.h"
#include "esphome/core/preferences.h"

namespace esphome {
namespace esp32_ble_controller {

#define WIFI_SSID_LEN 33
#define WIFI_PASSWORD_LEN 65

struct WifiSettings {
  char ssid[WIFI_SSID_LEN];
  char password[WIFI_PASSWORD_LEN];
  bool hidden_network;
} PACKED;  // NOLINT

class WifiSettingsHandler {
public:
  void setup();

  void set_credentials(const std::string& ssid, const std::string& password, bool hidden_network);

private:
  void override_sta(const WifiSettings& settings);

private:
  ESPPreferenceObject wifi_settings_preference;
};

} // namespace esp32_ble_controller
} // namespace esphome
