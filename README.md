This component provides a Bluetooth Low Energy (BLE) controller for [ESPHome](https://esphome.io). It allows to monitor sensor data and control switches and other components via BLE connections (for example from a smart phone):

![BLE connection from phone](BLE-Services-and-Characteristics.png)

In addition, there is a command channel, which allows to configure the WiFi credentials for the ESP32 over BLE (among other things).

⚠️ **Note**: This controller only works with ESP32 micro-controllers, not with ESP8266 chips because they do not offer built-in BLE support.

## Installation
This component is compatible with ESPHome 2022.12.0 or later. (For earlier ESPHome releases, please try an earlier version of this component.) To install this component you do not need to download or copy anything, you can just refer to this external component from the yaml file as shown below.

## Configuration

### Getting started

The following configuration shows how to make a (template) switch accessible via BLE:
```yaml
external_components:
  - source: github://wifwucite/esphome-ble-controller

switch:
  - platform: template
    name: "Template Switch"
    optimistic: true
    id: template_switch

esp32_ble_controller:
  services:
  - service: "4fafc201-1fb5-459e-8fcc-c5c9c331915d"
    characteristics:
      - characteristic: "beb5483e-36e1-4688-b7f5-ea07361b26ab"
        exposes: template_switch
```

You define your sensors, switches and other compoments as usual (like the [template switch](https://esphome.io/components/switch/template.html) in the example). 
Make sure to assign an id to each component you want to expose via bluetooth. 
Then you add `esp32_ble_controller` to include the controller itself. 
In order to make a component available you need to define a corresponding BLE characteristic that is contained in a BLE service. If you are not familiar with BLE you do need to worry much. For each characteristic and each service you simply need a different UUID, which you could generate [here](https://www.uuidgenerator.net). A service is basically used for grouping characteristics, so it can contain multiple characteristics. Each characteristic exposes a component, which is configured via the `exposes` property specifying the id of the respective component.

If you flash this example configuration and connect to your ESP32 device from your phone or tablet, you can see device information similar to the data displayed in the image above. Note how the service UUID and characteristic UUID provided in the characteristic configuration of the template switch now show up. Besides the switch that was configured explicitly there is also a so-called maintenance service which is provided by the controller automatically. It allows you to send commands to your device and access some logging related characteristics, which will be explained below.

### Configuration options

```yaml
esp32_ble_controller:
  services:
  - service: <service 1 UUID>
    characteristics:
      - characteristic: <characteristic 1.1 UUID>
        exposes: <id of component>
      - characteristic: <characteristic 1.2 UUID>
        exposes: <id of component>
  - service: <service 2 UUID>
    characteristics:
      - characteristic: <characteristic 2.1 UUID>
        exposes: <id of component>

  # you can add your own custom commands
  # The description is shown when the user sends "help test-cmd" as command.
  commands:
  - command: test-cmd
    description: just a test
    on_execute:
    - logger.log: "test command executed"

  # allows to enable or disable security, default is 'secure'
  # Options:
  # - none: 
  #     disables security
  # - bond:
  #     no real security, but devices do some bonding (pairing) upon first connect
  # - secure:
  #     enables secure connections and man-in-the-middle protection
  # If the "on_show_pass_key" automation is present, then upon first pairing the other device (your phone) 
  # sends a 6-digit pass key to the ESP and the ESP is supposed to display it so that it can be entered on the other device.
  # This automation is not available for the "none" mode, optional for the "bond" mode, and required for the "secure" mode.
  security_mode: secure

  # allows to disable the maintenance service, default is 'true'
  # When 'false', the maintenance service is not exposed, which provides at least some protection when security mode is "none".
  # Note: Writeable characteristics like those for switches or fans may still be written by basically anyone.
  maintenance: true

  # automation that is invoked when the pass key should be displayed, the pass key is available in the automation as "pass_key" variable of type std::string (not available if security mode is "none")
  # the example below just logs the pass keys
  on_show_pass_key:
  - logger.log:
      format: "pass key is %s"
      args: 'pass_key.c_str()'
  # automation that is invoked when the authentication is complete, the boolean "success" indicates success or failure (not available if security mode is "none")
  on_authentication_complete:
  - logger.log:
      format: "BLE authentication complete %d" # shows 1 on success, 0 on failure
      args: 'success'
  # automations that are invoked when the device is connected to / disconnected from a client (phone or tablet for example)
  on_connected:
  - logger.log: "I am connected. :-)"
  on_disconnected:
  - logger.log: "I am disconnected. :-("
```

## Features

### BLE security

By default security is switched on, which means that the ESP32 has to be bonded (paired) when it is used for the first time with a new device. (This feature can be switched off via configuration.) Secure connections and protection against man-in-the-middle attacks are enabled. The device to be bonded sends a pass key (a 6 digit PIN) to the ESP32. Via the `on_show_pass_key` automation you can log the pass key or even show it on a display. (At the bottom you can find an example that makes use of a display to show the pass key until pairing is complete.)

Note: On some computers (like the MacBook Pro for example) the very first bonding process seems to fail if the security is enabled. In that case you can change the security mode to "bond" for the very first encounter (without an `on_show_pass_key` automation). After that succeeded you may change the mode back to "secure". Even if you delete the bonding information from both devices later on, secure bonding attempts will work and recreate the bonding.

### Maintenance service

The maintenance BLE service is provided implicitly when you include `esp32_ble_controller` in your yaml configuration unless you disable it explicitly via the `maintenance` property. It provides two characteristics:

* Command channel (UTF-8 string, read-write):
Allows to send commands to the ESP32 and receives answers back from it. A command is a string which consists of the name of the command and (possibly) arguments, separated by spaces.
You can define your own custom commands in yaml as described below in detail.
There are also some built-in commands, which are always available:
  * help [&lt;command>]:
    Without argument, it lists all available commands. When the name of a command is given like in "help log-level" it displays a specific description for this command.
  * ble-maintenance [off]:
    Switches the maintenance service off and boots the device. After the boot the maintenance service will **not be availble anymore until you flash your device again**. Thus you can set up your device with the maintenance service enabled and disable that service as soon as everything is running (if you are operating your device in an insecure mode).
  * ble-services [on|off]:
    Switches the component related (non-maintenance) BLE services on or off and boots the device. You may wonder why one should switch off these services. On most ESP32 boards both BLE and WiFi share the same physical 2,4 GHz antenna on the ESP32. So, too much traffic on both of them can cause it to crash and reboot. Short-lived WiFi connections for sending MQTT messages work fine with services enabled. However, when connecting to the [web server](https://esphome.io/components/web_server.html) or for [OTA updates](https://esphome.io/components/ota.html) services should be disabled. (Note that ESPHome permits configurations without the WiFi component, so if you encounter problems with BLE you could try disabling WiFi completely.)
  * wifi-config &lt;ssid> &lt;password> [hidden]:
    Sets the SSID and the password to use for connecting to WiFi. The optional 'hidden' argument marks the network as hidden network. It is recommended to use this command only when security is enabled. You can also use "wifi-config clear" to clear the WiFi configuration; then the default credentials (compiled into the firmware) will be used. (This command is only available if the WiFi component has been configured at all.)
  * parings [clear]:
    Lists the addresses of all paired devices, or clears all paired devices.
  * version:
    Shows the version of the device. (Currently this displays the compilation time.)
  * log-level [level]: 
    If no argument is provided, it queries the current log level for logging over BLE. When a level argument is provided like in "log-level 0" the log level is adjusted. Currently the levels have to be specified as integer number between 0 (= no logging) and 7 (= very verbose).  
      ⚠️ **Note**: You cannot get finer logging than the overall log level specified for the [logger component](https://esphome.io/components/logger.html).
* Log messages (UTF-8 string, read-only):  
Provides the latest log message that matches the configured log level.

#### Custom commands

 A custom commmand consists of three parts: name, description (shown by help) and the `on_execute` automation that is executed when the command runs. A custom command can have arguments which are passed to the automation as a vector of strings named `arguments`. In addition a custom command send a result, which can be defined by assigning a string to the `result` argument or via the `ble_cmd.send_result` automation (similar to [`logger.log`](https://esphome.io/components/logger.html)). Both variants are shown below.
 
 ```yaml
esp32_ble_controller:
  commands:
  - command: test-cmd
    description: just a test
    on_execute:
    - if:
        condition:
          lambda: 'return arguments.empty();'
        then:
          - lambda: |-
              result = "test command executed without arguments";
        else:
          - ble_cmd.send_result:
              format: "test command executed with argument %s"
              args: 'arguments[0].c_str()'
```

### Supported components

* [Binary sensor](https://esphome.io/components/binary_sensor/index.html) (read-only, 2-byte unsigned little-endian integer): The characteristic stores the boolean sensor value as integer (0 or 1).
* [Sensor](https://esphome.io/components/sensor/index.html) (read-only, 4-byte little-endian float): The characteristic stores the floating point sensor value (without unit).
* [Text sensor](https://esphome.io/components/text_sensor/index.html) (read-only, UTF-8 string): The characteristic stores the string sensor value.
* [Switch](https://esphome.io/components/switch/index.html) (read-write, 2-byte unsigned little-endian integer): The characteristic represents the on-off state of the switch as integer value (0 or 1). Writing a 0 or 1 can be used to turn the switch on or off.
* [Fan](https://esphome.io/components/fan/index.html) (read-write, UTF-8 string): The characteristic represents the complete state of the fan (not only on-off, also speed, oscillating, and direction). Writing a string option can be used to change the on-off state ("on"/"off"), the speed (an integer value), the oscillating flag ("yes"/"no"), or the direction ("forward"/"reverse"). You can set more than one option at a time: "on 45 no" would turn the fan on set its speed to 45 and switch oscillation off.

# Examples

## Show pass key on display during authentication

Configuration to show the 6-digit pass key during authentication on a display:
![BLE pass key on display](BLE-PassKey.png)

```yaml
display:
  - platform: ...
    ...
    pages:
      - id: page_standard
        lambda: |-
          // print standard stuff
      - id: page_ble_pass_key
        lambda: |-
          it.print(0, 0, id(my_font), TextAlign::TOP_LEFT, "Bluetooth");
          it.print(0, 20, id(my_font), TextAlign::TOP_LEFT, "Key");
          it.print(0, 40, id(my_font), TextAlign::TOP_LEFT, id(ble_pass_key).c_str());

globals:
  - id: ble_pass_key
    type: std::string

esp32_ble_controller:
  services:
  - service: "4fafc201-1fb5-459e-8fcc-c5c9c331915d"
    characteristics:
      - characteristic: "beb5483e-36e1-4688-b7f5-ea07361b26ab"
        exposes: template_switch
  on_show_pass_key:
    then:
      - lambda: |-
          id(ble_pass_key) = pass_key;
      - display.page.show: page_ble_pass_key
      - component.update: my_display
  on_authentication_complete:
    then:
      - lambda: |-
          id(ble_pass_key) = "";
      - display.page.show: page_standard
      - component.update: my_display
```

## Integration with ble_client (light and switch example)

This example shows how to integrate with [ble_client](https://esphome.io/components/ble_client.html) (a standard component of ESPHome). In the example we toggle a light with a momentary swith. Why do we need bluetooth? Because in our case the switch and the light are connected to two different ESP32 boards, and we use BLE to "tunnel" the switch press event from one ESP32 (the BLE client) to the other ESP32 (the server).

So, our setup looks like this:
* We have a physical momentary switch connected to a GPIO pin of our first ESP32 board, which will be the BLE client. The physical light is connected to a GPIO pin of our second ESP32 board, which will be the BLE server.
* On the BLE client, we use a binary sensor compoment to detect when the momentary switch is pressed by a human. We use the [ble_client](https://esphome.io/components/ble_client.html) component to send a value from the client to the server when the switch is pressed.
* On BLE server, we expose a template switch via this BLE controller component as a characteristic. The BLE client writes to this charactericstic and thus informs the server about button press. The template switch on the server controls the actual light compoment, which turns the physical light on or off.
* To summarize, the sequence of events is this:
  * Human presses button of momentary switch connected to the BLE client board.
  * Binary sensor component detects change of GPIO level and writes to a characteristic on the BLE server (using ``ble_client.ble_write``).
  * The BLE controller component on the server observes this write and toggles the template switch.
  * Template switch toggles the light component.
  * Light component sets the GPIO level on the server side, which controls the physical light.

The example has been provided [evlo](https://github.com/evlo). Pins in example are based on M5stack AtomS3 devices.

### Switch configuration (BLE client)

⚠️ **Note**: BLE MAC address can be different than WiFi MAC address, make sure you are using correct one. The [ble_client](https://esphome.io/components/ble_client.html) component only supports unsafe communication!

```yaml
substitutions:
  hostname: demo-switch-ble
  device_id: demo_switch_ble
  comment: ESPHome ble demo switch
  # You have to put your server's BLE address here, see server's startup log
  mac_light: "f4:12:fa:61:00:f5"
  switch_button_id: button0
  light_peer_id: demo_light
  light_virtual_switch_uuid: 32f40d3a-d24d-11ed-afa1-0242ac120002
  light_control_service_uuid: 2dc01d40-d24d-11ed-afa1-0242ac120002

ble_client:
  - mac_address: ${mac_light}
    id: ${light_peer_id}_client
    on_connect:
      then:
        - lambda: |-
            ESP_LOGD("ble_client", "Connected to ${light_peer_id}");

binary_sensor:
  - platform: gpio
    pin:
      number: GPIO41
    id: ${switch_button_id}
    name: ${switch_button_id}
    icon: "mdi:gesture-tap-button"
    on_release:
      then:
        - logger.log: "Toggling the light using BLE"
        - ble_client.ble_write:
            id: ${light_peer_id}_client
            service_uuid: ${light_control_service_uuid}
            characteristic_uuid: ${light_virtual_switch_uuid}
            value: 1
```

### Light configuration (BLE server)

```yaml
substitutions:
  hostname: demo-light-ble
  device_id: demo_light_ble
  mac_light: "f4:12:fa:61:00:f5"
  light_virtual_switch_uuid: 32f40d3a-d24d-11ed-afa1-0242ac120002
  light_control_service_uuid: 2dc01d40-d24d-11ed-afa1-0242ac120002
  light_toggle_command: demo-toggle
  light_virtual_switch_id: light_virtual_switch

esp32_ble_controller:
  security_mode: none
  services:
    - service: ${light_control_service_uuid}
      characteristics:
        - characteristic: ${light_virtual_switch_uuid}
          exposes: ${light_virtual_switch_id}
  # The commands and the events below are for debugging purposes.
  commands:
    - command: ${light_toggle_command}
      description: Toggle the light demo
      on_execute:
        - logger.log: "Toggle executed"
        - light.toggle: status_led
  on_connected:
    - logger.log: "Connected."
  on_disconnected:
    - logger.log: "Disconnected."

switch:
  - platform: template
    id: ${light_virtual_switch_id}
    turn_on_action:
      - logger.log: "Switch ON"
      - light.toggle: status_led
    turn_off_action:
      - logger.log: "Switch OFF"
      - light.toggle: status_led
               
light: 
  - platform: neopixelbus
    num_leds: 1
    variant: WS2812
    pin: GPIO35
    id: status_led
    name: ${hostname} RGB LED
    default_transition_length: 
      milliseconds: 0
```