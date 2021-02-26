This component provides a Bluetooth Low Energy (BLE) controller for [ESPHome](https://esphome.io). It allows to monitor sensor data and control switches and other components via BLE connections (for example from a smart phone):

![BLE connection from phone](BLE-Services-and-Characteristics.png)

⚠️ **Note**: This controller only works with ESP32 micro-controllers, not with ESP8266 chips because they do not offer built-in BLE support.

## Installation
Copy the `esp32_ble_controller` directory into your ESPHome `custom_components` directory. (If you do not have such a directory it, then create it in your ESPHome configuration directory, i.e. the directory storing your yaml files.)

## Configuration

### Getting started

The following configuration shows how to make a (template) switch accessible via BLE:
```yaml
switch:
  - platform: template
    name: "Template Switch"
    optimistic: true
    id: template_switch

esp32_ble_controller:
  id: ble_controller

custom_component:
- lambda: |-
    id(ble_controller)->register_component(id(template_switch), "4fafc201-1fb5-459e-8fcc-c5c9c331915d", "beb5483e-36e1-4688-b7f5-ea07361b26ab");
    return {};
```

You define your sensors, switches ane other compoments as usual (like the [template switch](https://esphome.io/components/switch/template.html) in the example). Then you add `esp32_ble_controller`to include the controller itself and assign an id to it, which you will need later. After that you need to register every component that you want expose via BLE. Currently this is done in a `custom_component` section, which actually does not create any components; it is just a way to add the registration code to `main.cpp`. (And I plan to make registration more elegant in the future.)
For the registration you provide a unique service UUID and a unique characteristic UUID, which you could generate [here](https://www.uuidgenerator.net). If you are not familiar with BLE you do need to worry much, the only thing you must ensure is that all characteristic UUIDs (the final ones in the `register_component` calls) are really unique. Service UUIDs can be reused, then the corresponding characteristics will be placed in the same BLE service.

If you flash this example configuration and connect to your ESP32 device from your phone, you can see device information similar to the data displayed in the image above. Note how the service UUID and characteristic UUID provided in the registration of the template switch now show up. Besides the switch that was configured explicitly there is also a so-called maintenance service which is provided by the controller automatically. It allows you to set BLE mode and access some logging related characteristics, which will be explained below.

### Configuration options

```yaml
esp32_ble_controller:
  id: ble_controller
  security_mode: show_pin # default is 'show_pin', 'none' is also possible
```

## Features

### BLE security

By default security is switched on, which means that the ESP32 has to paired (bonded) when it is used for the first time with a new device. (This feature can be switched off via configuration.) Protection against man-in-the-middle attacks is enabled. The device to be paired sends a pass key (a 6 digit PIN) to the ESP32, which is logged (on info level.) (As a future extension I would like to add an automation that allows showing the PIN on a display until the paring is complete.)

### Maintenance service

The maintenance BLE service is provided implicitly when you include `esp32_ble_controller` in your yaml configuration. It provides three characteristics:

* BLE Mode (read-write):  
This mode controls the balance between BLE and WiFi. Note that both BLE and WiFi share the same physical 2,4 GHz antenna on the ESP32. So too much traffic on both of them can cause it to crash and reboot. Here the modes come in handy. Switching the mode triggers a safe reboot of the ESP32.
  * 0 = BLE only: This is supposed to switch WiFi off completely and allow only BLE traffic.  
  ⚠️ **Note**: This not working yet!
  * 1 = mixed mode: This allows both BLE and WiFi traffic, but it may crash if there is too much traffic. (Short-lived WiFi connections for sending MQTT messages work fine, but for connecting to the web server `WiFi only` mode is recommended.)
  * 2 = WiFi only: This disables all non-maintenance BLE characteristics in order to reduce BLE traffic to the minimum. It also sets the log level to `configuration`. You can use this mode for [OTA updates](https://esphome.io/components/ota.html).
* Log level (read-write):  
Sets the log level for logging over BLE. Currently the levels have to be specified as integer number between 0 (= no logging) and 7 (= very verbose).  
  ⚠️ **Note**: You cannot get finer logging than the overall log level specified for the [logger component](https://esphome.io/components/logger.html).
* Log messages (read-only):  
Provides the latest log message that matches the configured log level.

### Supported components

* [Binary sensor](https://esphome.io/components/binary_sensor/index.html) (read-only)
* [Sensor](https://esphome.io/components/sensor/index.html) (read-only)
* [Text sensor](https://esphome.io/components/text_sensor/index.html) (read-only)
* [Switch](https://esphome.io/components/switch/index.html) (read-write)
