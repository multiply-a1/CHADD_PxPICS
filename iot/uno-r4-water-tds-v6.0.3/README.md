# UNO R4 Water EC + pH + Rain + Tanks + Matrix v6.0.3

Firmware for an Arduino UNO R4 WiFi-based water monitoring node with MQTT auto-discovery for Home Assistant, covering EC, pH, DS18B20 water temperature, analog and digital rain sensing, three ultrasonic tank level channels, and the onboard 12x8 LED matrix aswell as persistent configuration data[cite:495][cite:479][cite:449]

## Features

### Sensors

- EC measurement on `A0`, including raw ADC, voltage, raw EC and calibrated EC output.[cite:495][cite:449]
- pH measurement on `A5` with 3-point calibration support for pH 4, 7 and 10.[cite:495][cite:449]
- DS18B20 water temperature on digital pin `2`.[cite:495][cite:449]
- Rain sensor with analog input on `A1` and digital input on pin `5`.[cite:495][cite:479]
- Three ultrasonic level channels for RO, Mix and Drain tanks on pins `6-11`.[cite:495][cite:449]
- Wi-Fi RSSI and uptime telemetry via MQTT state payloads.[cite:495]

### Home Assistant integration

- MQTT auto-discovery for sensor entities, binary sensor, matrix switch/select controls, calibration buttons and tank-volume number entities.[cite:495][cite:479][cite:480]
- Tank volume inputs are exposed as MQTT Number entities with numeric entry boxes, min/max/step handling and retained state feedback through the JSON state topic.[cite:495][cite:480]
- Calibration actions for tank geometry are exposed as MQTT Button entities for direct use in dashboard cards.[cite:495][cite:479]

### Matrix control

- Built-in UNO R4 LED matrix can display `ec`, `temp`, `ph`, `tank` or `off`.[cite:495]
- `matrix_enabled = OFF` and `matrix_mode = off` both clear the display immediately.[cite:495]
- Matrix state is published as separate MQTT state topics for dashboard control consistency.[cite:495]

## Pin mapping

| Function | Pin |
|---|---|
| EC sensor | `A0` |
| pH sensor | `A5` |
| Rain analog | `A1` |
| Rain digital | `5` |
| DS18B20 | `2` |
| RO tank trig / echo | `6` / `7` |
| Mix tank trig / echo | `8` / `9` |
| Drain tank trig / echo | `10` / `11` |

Pins and assignments are defined directly in the sketch constants for v3.9.4.[cite:495]

## MQTT entities

### Telemetry

The main JSON state topic publishes EC, pH, temperature, rain, tank distances, tank percentages, tank fill volumes, Wi-Fi RSSI, uptime, matrix state and configured tank capacities.[cite:495]

### Control topics

| Entity type | Purpose |
|---|---|
| Switch | Matrix enabled on/off.[cite:495] |
| Select | Matrix mode: `ec`, `temp`, `ph`, `tank`, `off`.[cite:495] |
| Button | Tank calibration actions like `set-ro-tank-full` and `set-drain-tank-empty`.[cite:495] |
| Number | Tank volume inputs for RO, Mix and Drain capacity values.[cite:495][cite:480] |

## Serial commands

The firmware also supports direct serial commands for setup and calibration.[cite:495][cite:449]

```text
ph4
ph7
ph10
set-ro-tank-full
set-ro-tank-empty
set-ro-tank-volume
set-mix-tank-full
set-mix-tank-empty
set-mix-tank-volume
set-drain-tank-full
set-drain-tank-empty
set-drain-tank-volume
rocap <L>
mixcap <L>
draincap <ml>
matrix-on
matrix-off
matrix-ec
matrix-temp
matrix-ph
matrix-tank
show
help
load
save
```

## Tank calibration workflow

1. Install each ultrasonic sensor in its final position.[cite:449][cite:495]
2. Trigger the corresponding `set-*-tank-full` command when the tank is physically full.[cite:449][cite:495]
3. Trigger `set-*-tank-empty` when the same tank is physically empty.[cite:449][cite:495]
4. Set the tank capacity using the Home Assistant number field or serial command, for example `rocap 120`.[cite:495][cite:480]
5. The firmware then calculates percentage and fill volume from measured distance, full distance, empty distance and configured capacity.[cite:495][cite:449]
6. Press Save Calibration in HA or use Serial Monitor
7. After Power-Loss use Load Calibration

## Home Assistant card layout

A practical dashboard layout for this firmware is:

- **Sensors** card for EC, pH, temperature, rain and tank fill readings.[cite:495][cite:479]
- **Steuerelemente** card for `matrix_enabled` and `matrix_mode`.[cite:495][cite:479]
- **Kalibrierung** card for tank calibration buttons and tank-volume number fields.[cite:495][cite:480]

This separation matches the current discovery layout in v3.9.4, where matrix controls and calibration controls are exposed as different entity types for cleaner dashboard composition.[cite:495][cite:479]

## Dependencies

Install these libraries in the Arduino IDE before compiling:

- `WiFiS3`
- `PubSubClient`
- `ArduinoJson`
- `Arduino_LED_Matrix`
- `OneWire`
- `DallasTemperature`

These headers are included directly in the firmware source for v3.9.4.[cite:495]

## Build notes

- Target board: **Arduino UNO R4 WiFi**.[cite:495]
- ADC resolution is set to 14-bit with `analogReadResolution(14)`.[cite:495]
- MQTT buffer size is increased to `2048` to fit discovery payloads and state JSON.[cite:495]
- Tank-volume MQTT number commands are parsed as `name: value` strings on the tank calibration topic.[cite:494][cite:495]

## Version

`v6.0.3` is the current working snapshot used as the base for this README.[cite:495]
