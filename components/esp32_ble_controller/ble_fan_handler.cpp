#include "ble_fan_handler.h"

#ifdef USE_FAN

#include "ble_utils.h"

namespace esphome {
namespace esp32_ble_controller {

static const char *TAG = "ble_fan_handler";

void BLEFanHandler::send_value(bool on_off) {
  string state_as_string;

  state_as_string = "fan=";
  state_as_string += on_off ? "on" : "off";

  const FanState* fan_state = get_component();
  const auto& traits = fan_state->get_traits();

  if (traits.supports_speed()) {
    state_as_string += " speed=";
    state_as_string += to_string(fan_state->speed);
    const int max_speed = traits.supported_speed_count();
    if (max_speed != 100) {
      state_as_string += "/";
      state_as_string += to_string(max_speed);
    }
  }
  
  if (traits.supports_oscillation()) {
    state_as_string += " oscillating=";
    state_as_string += fan_state->oscillating ? "yes" : "no";
  }
  
  if (traits.supports_direction()) {
    state_as_string += " direction=";
    state_as_string += fan_state->direction == fan::FAN_DIRECTION_FORWARD ? "forward" : "reverse";
  }

  BLEComponentHandlerBase::send_value(state_as_string);
}

void BLEFanHandler::on_characteristic_written() {
  std::string value = get_characteristic()->getValue();

  FanState* fan_state = get_component();

  // for backward compatibility
  if (value.length() == 1) {
    uint8_t on = value[0];
    ESP_LOGD(TAG, "Fan chracteristic written: %d", on);
    if (on)
      fan_state->turn_on().perform();
    else
      fan_state->turn_off().perform();
    return;
  }

  auto call = fan_state->make_call();
  const auto& traits = fan_state->get_traits();
  for (const string& option : split(value)) {
    if (option == "on") {
      call.set_state(true);
      continue;
    }
    if (option == "off") {
      call.set_state(false);
      continue;
    }
    
    if (traits.supports_speed()) {
      const optional<int> opt_speed = parse_int(option);
      if (opt_speed.has_value()) {
        const int speed = opt_speed.value();
        if (speed >= 0 && speed <= traits.supported_speed_count()) {
          call.set_speed(speed);
          continue;
        }
      }
    }

    if (traits.supports_oscillation()) {
      if (option == "yes") {
        call.set_oscillating(true);
        continue;
      }
      if (option == "no") {
        call.set_oscillating(false);
        continue;
      }
    }
    
    if (traits.supports_direction()) {
      if (option == "forward") {
        call.set_direction(fan::FAN_DIRECTION_FORWARD);
        continue;
      } 
      if (option == "reverse") {
        call.set_direction(fan::FAN_DIRECTION_REVERSE);
        continue;
      }
    }

    ESP_LOGW(TAG, "Unknown fan option: %s", option.c_str());
  }
  call.perform();
}

} // namespace esp32_ble_controller
} // namespace esphome

#endif
