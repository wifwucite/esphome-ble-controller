#pragma once

#include <string>

#include "esphome/core/defines.h"
#include "esphome/core/preferences.h"
#include "esphome/core/optional.h"

#ifdef USE_WIFI

namespace esphome {
namespace esp32_ble_controller {

static const uint8_t WIFI_SSID_LEN = 33;
static const uint8_t WIFI_PASSWORD_LEN = 65;

struct WifiConfiguration {
  char ssid[WIFI_SSID_LEN];
  char password[WIFI_PASSWORD_LEN];
  bool hidden_network;
} PACKED;  // NOLINT

class WifiConfigurationHandler {
public:
  void setup();

  void set_credentials(const std::string& ssid, const std::string& password, bool hidden_network);
  void clear_credentials();

  const optional<std::string> get_current_ssid() const;

private:
  bool load_configuration(WifiConfiguration& configuration) const;
  bool save_configuration(const WifiConfiguration& configuration);
  void override_sta(const WifiConfiguration& configuration);

private:
  ESPPreferenceObject wifi_configuration_preference;
};

} // namespace esp32_ble_controller
} // namespace esphome


#endif