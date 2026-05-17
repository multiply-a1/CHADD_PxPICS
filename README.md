<div align="center">

# 🚀 CHADD PxPICS

> Centralized, home automated, data-driven pressure x precision irrigation control system

<p>
  <img src="https://img.shields.io/badge/Status-Alpha-orange.svg" alt="Status" />
  <img src="https://img.shields.io/badge/Home%20Assistant-Integration-blue.svg" alt="Home Assistant" />
  <img src="https://img.shields.io/badge/Node--RED-Automation-green.svg" alt="Node-RED" />
  <img src="https://img.shields.io/badge/MQTT-Mosquitto-lightgrey.svg" alt="MQTT" />
  <img src="https://img.shields.io/badge/License-MIT-yellow.svg" alt="License" />
</p>

<p>
  <img src="https://github.com/multiply-a1/CHADD_PxPICS/blob/main/images/banner.jpg" alt="CHADD PxPICS Banner" />
</p>

**K.I.S.S. on steroids** — from lazy grower to precision steering with Home Assistant and Node-RED.

</div>

## 📸 Screenshots

<p align="center">
  <img src="https://github.com/multiply-a1/CHADD_PxPICS/blob/main/images/dashboard-v095.jpg" alt="Dashboard Screenshot 1" />
</p>

<p align="center">
  <img src="https://github.com/multiply-a1/CHADD_PxPICS/blob/main/images/nodered_flow093.jpg" alt="Node-RED Flow Screenshot" />
</p>

<p align="center">
  <img src="https://github.com/multiply-a1/CHADD_PxPICS/blob/main/images/dual_tank_picture.jpg" alt="Tank and Sensor Screenshot" width="30%" />
  <img src="https://github.com/multiply-a1/CHADD_PxPICS/blob/main/images/dxd-station-01.jpg" alt="dxd_prototype" width="30%" />
</p>


# [Hier gehts zum deutschen Guide](https://github.com/multiply-a1/CHADD_PxPICS/blob/main/docs/userguide-de.md)


## 💡 Table of Contents

- [Overview](#-overview)
- [Why This Project](#-why-this-project)
- [✨ Coolest Features](#-coolest-features)
- [📋 All Features](#-all-features)
- [🏗 Architecture](#-architecture)
- [🧰 Requirements](#-requirements)
- [⚙️ Installation](#-installation)
- [🌱 Sensor Stack](#-sensor-stack)
- [💧 Tank System](#-tank-system)
- [🔩 Irrigation Hardware](#-irrigation-hardware)
- [🧪 Drip & Drain Station](#-drip--drain-station)
- [🧠 Software Stack](#-software-stack)
- [📶 Dashboard](#-dashboard)
- [📟 System Status](#-system-status)
- [🐞 Known Bugs](#-known-bugs)
- [🗺 Roadmap](#-roadmap)
- [💸 Donation](#-donation)
- [🤝 Contributing](#-contributing)
- [📄 License](#-license)
- [🗂️ Helpfiles](#-helpfiles)
- [🔗 Linklist](#-linklist)

## 🌿 Overview

CHADD PxPICS is a Home Assistant and Node-RED based irrigation project for precise substrate control following crop-steering principles. The system combines sensor data, daily counters, last irrigation time, shot size, time windows, and intent profiles to control irrigation much more precisely than a simple threshold-based controller. The architecture uses Home Assistant as the central entities and UI layer, Node-RED as the decision and automation layer, and MQTT for communication and logging.

<p align="center">
  <img src="https://github.com/multiply-a1/CHADD_PxPICS/blob/main/images/system_flow.png" alt="system flow" width="60%" />
</p>

## 🎯 Why This Project

This project is aimed at growers who value precision, repeatability, and diagnostics more than a cheap timer solution. Instead of watering blindly, the system works with Field Capacity, Dryback, Shot Size, and safety limits. The goal is a system that can evolve from manual assistance to a largely autonomous set-and-forget operation.

## ✨ Coolest Features

- 10-level intent steering from strongly generative to strongly vegetative. 🌱➡️🌳
- Dryback and shot-size control instead of simple on/off thresholds.
- Home Assistant dashboard with helpers, profile names, blocker reasons, and MQTT logs.
- Modular setup with Raspberry Pi, Docker, Portainer, MQTT, Home Assistant, and Node-RED.
- Extensible with pH, ORP, long-term databases, AI analysis, and automatic tank functions.

## 📋 All Features

| Area | Features |
|---|---|
| Core Control | 10-level intent system, Dryback as primary control variable, Field Capacity as reference point, shot-size dosing via pump runtime, P1/P2-oriented profile control, minimum interval between irrigations. |
| Daily and Safety Logic | Daily irrigation counter, maximum irrigations per day, fallback shot limit, sensor timeout protection, lockout after manual intervention, max pump time, skip-next-shot, manual override. |
| Sensors and Monitoring | VWC, EC, and temperature sensors, MQTT decision and context logs, profile name and blocker reason as diagnostic values, extensible runoff and level sensing. |
| Platform and UI | Home Assistant, Node-RED, MQTT broker, Docker stacks on Raspberry Pi, Macvlan, HACS, Node-RED Companion, dashboard extensions. |
| Roadmap Features | pH and ORP sensing, long-term databases, AI analysis, calendar and crop data, set-and-forget runs, osmotic steering, automatic fill and mixing functions. |

## 🏗 Architecture

The architecture cleanly separates infrastructure, logic, and actuation. Home Assistant manages entities, helpers, and dashboards, while Node-RED handles irrigation decisions and MQTT is used for logging and optional sensor communication. On the Raspberry Pi, the core components typically run as Docker containers under Portainer; Macvlan is recommended so each service behaves like an independent server.

## 🧰 Requirements

### Hardware

- Raspberry Pi 4 or higher including power supply, cooling, SSD, and enclosure.
- Reverse osmosis system, optional DI filter.
- RO tank and mix tank, ideally with the same volume, HDPE, lightproof and food safe.
- Pumps, pipes, fittings, sensors, and tools.

### Software

- Headless Raspberry Pi OS. [Guide](https://www.raspberrypi.com/documentation/computers/getting-started.html)
- Docker and Portainer. [Guide](https://www.heise.de/news/Wie-man-Docker-auf-dem-Raspberry-Pi-in-15-Minuten-einrichtet-7524692.html)
- Home Assistant.
- Node-RED.
- `node-red-contrib-home-assistant-websocket`.
- Eclipse Mosquitto as MQTT broker.
- HACS and Node-RED Companion for full dashboard functionality.

## ⚙️ Installation

### 1. Prepare the Raspberry Pi

Install headless Raspberry Pi OS, Docker, and Portainer on your Pi. This step is straightforward; the detailed guides should be easy to follow.

### 2. Configure Macvlan

Configure Macvlan in Portainer. This makes each Docker service behave like an independent server; it is not strictly required, but recommended. [Guide](https://ugreen-forum.de/forum/thread/238-tut-macvlan-konfigurieren-schritt-fuer-schritt-anleitung/)

### 3. Deploy the stacks

[/portainer_stacks](https://github.com/multiply-a1/CHADD_PxPICS/tree/main/portainer_stacks)<br>

Use the stacks for Home Assistant, Eclipse Mosquitto (MQTT broker), and Node-RED. Adjust Macvlan IPs, timezone, and folder structure to match your setup. An SSD via HAT or USB is recommended for persistent data.

### 4. Extend Home Assistant

Install Node-RED Companion, HACS, and the required HACS dashboard extensions in Home Assistant for full dashboard functionality. This makes it easier to build visual cards, controls, and status views.

<p align="center">
  <img src="https://github.com/multiply-a1/CHADD_PxPICS/blob/main/dashboard-dev/hacs.jpg" alt="HA_HACS" />
</p>

### 5. Check MQTT and plugins

Connect Home Assistant to the MQTT broker. In Node-RED, install the Home Assistant websocket or webhook setup that matches your flow. If Node-RED or Mosquitto have issues, check folder permissions first.

### 6. Import helpers and flow

[/packages](https://github.com/multiply-a1/CHADD_PxPICS/tree/main/packages)<br>

Include the `packages` folder in Home Assistant via `configuration.yaml`, then import the Node-RED flow. Verify all entity IDs, MQTT topics, and server nodes before switching to production.

```yaml
homeassistant:
  packages: !include_dir_named packages
```

## 🌱 Sensor Stack

[/iot](https://github.com/multiply-a1/CHADD_PxPICS/tree/main/iot)<br>

### Substrate Sensors

There is a wide range of sensors available; in general, every sensor should at least provide a rough reference value, but as of now the rule is often still: the more expensive, the more accurate. For this project, <br>[BGT SZ (SDI-12)](https://www.alibaba.com/x/1lAXPBk?ck=pdp)<br>[TEROS 12 (SDI-12)](https://metergroup.com/products/teros-12/)<br>[Andratek (RS485)](https://www.antratek.de/moisture-ec-temperature-sensor-for-soil-substrate-rockwool-cocopeat-modbus-rtu-rs485)<br> were identified as relevant options. RS485 sensors require a Waveshare USB-to-RS485 converter;<br> in the described setup, a BGT SZ (SDI-12) is connected wirelessly to the Pi via an [M5Stack ATOM Lite](https://docs.m5stack.com/en/core/ATOM%20Lite) and [ChillDivision Sketch](https://github.com/Chill-Division/sdi12-substrate-sensor?tab=readme-ov-file).

### Sensor Emulator / Simulator (MQTT)

For startes i also included a <br>[5in1](https://github.com/multiply-a1/CHADD_PxPICS/tree/main/iot/AtomLiteS3-Sensor-Emulator_5in1_v3.5) and a <br>[22in1 Sensor Emulator](https://github.com/multiply-a1/CHADD_PxPICS/tree/main/iot/AtomLiteS3_Sensor_Emulator_22in1_8h_v4.2)<br>based on ChillDivisions Sketch ([Backup](https://github.com/multiply-a1/CHADD_PxPICS/tree/main/iot/AtomLite-SDI12-Teros12_Wifi_Sensor_Original_2026-ChillDivision)) to setup CHADD while waiting on the real life sensor. Those were tested on the AtomLite S3. i still have some ideas to increase usefullness for them so stay tunes...

<p align="center">
  <img src="https://github.com/multiply-a1/CHADD_PxPICS/blob/main/iot/AtomLiteS3_Sensor_Emulator_22in1_8h_v4.2/22in1-emu-v4.2.jpg" alt="22in1" width="30%" />
  <img src="https://github.com/multiply-a1/CHADD_PxPICS/blob/main/iot/AtomLiteS3-Sensor-Emulator_5in1_v3.5/5in1emu-v3.5.jpg" alt="5in1" width="30%" />
</p>

### Arduino UNO R4 Wifi Sensor Suite

An arduino uno r4 wifi sketch for EC/TDS is also ready and working. you can find version 3.2 [here](https://github.com/multiply-a1/CHADD_PxPICS/tree/main/iot/uno-r4-water-tds-v3.2) this version is very basic, please check the latest version: 

[arduino uno r4 water tds v6.0.3](https://github.com/multiply-a1/CHADD_PxPICS/tree/main/iot/uno-r4-water-tds-v6.0.3) now comes with a big sensor suite: TDS, EC, TEMP, PH, TANK Level .. ORP very soon....

<p align="center">
  <img src="https://github.com/multiply-a1/CHADD_PxPICS/blob/main/images/dxd-station-03-tds_closeup.jpg" alt="dxd_prototype" width="30%" />
  <img src="https://github.com/multiply-a1/CHADD_PxPICS/blob/main/iot/uno-r4-water-tds-v6.0.3/sensor.jpg" alt="603" width="50%" />
</p>

## 💧 Tank System

The tank system is intentionally simple: collect RO water, transfer it into the mix tank, and irrigate from the mix tank. A practical starting point is a 120 l RO tank and a 120 l mix tank; ideally, both tanks should have the same volume. Below 60 l tank content, there is a risk that the pump runs dry too quickly. Please check the /watertanks folder for more information.

Typical parts mentioned in the bill of materials include Tavlit fittings, ball valves, double nipples, PG cable glands, Netafim LDPE pipe, a Netafim K10 vacuum breaker, and a Gardena 4700/2 pump.

## 🔩 Irrigation Hardware

Details soon...
The irrigation hardware uses pressure-stable components and a calibratable delivery path so shot size can be dosed reproducibly. The described setup includes Netafim LDPE pipes, Tavlit fittings, Gardena quick connectors, ball valves, a vacuum breaker, and a delivery pump. Clean assembly, high-quality fittings, and careful leak testing are important because pressurized lines increase the risk of water damage.

## 🧪 Drip & Drain Station

The drip-and-drain station is used to monitor input and output and expands the system with runoff information. Proposed extensions include TDS measurement for drip and drain, runoff pH, ultrasonic level measurement, and other sensors for immediate runoff detection. A 500ml Hobbock bucket for the DXD station was mentioned as part of the hardware setup.

## 🧠 Software Stack

[/portainer_stacks](https://github.com/multiply-a1/CHADD_PxPICS/tree/main/portainer_stacks)<br>
[/nodered_flows](https://github.com/multiply-a1/CHADD_PxPICS/tree/main/nodered_flows)<br>
[/packages](https://github.com/multiply-a1/CHADD_PxPICS/tree/main/packages)<br>

The software stack is the brain section of the system and is based on Home Assistant, Node-RED, and MQTT. The logic combines sensor values, daily counters, last irrigation time, shot size, time windows, and intent profiles to control irrigation far more precisely than a simple threshold controller. The current flow runs every three minutes by default, reads sensors and helpers, derives a profile, checks Dryback, Shot Size, limits, and fallback conditions, and then makes decisions such as `auto_water`, `fallback_water`, `manual`, or `block`.

Important concepts are:

- Field Capacity as the saturation reference point.
- Dryback as the central control variable.
- Shot Size as the amount of water per irrigation event.
- P1/P2/P3 as phase logic.
- Dry-run for safe commissioning without activating the pump.

## 📶 Dashboard

[/dashboard-dev](https://github.com/multiply-a1/CHADD_PxPICS/tree/main/dashboard-dev)<br>
The dashboard is still under heavy construction as im still learning by doing.

## 📟 System Status

The current status is Alpha. The system has not yet been validated in a productive live environment and is planned to be tested live for the first time in September 2026. It is therefore currently best suited for experimental, supervised setups rather than unattended production use.

## 🐞 Known Bugs

There are already some known bugs, and it is realistic that many more remain unknown. Typical problem areas include MQTT timeouts, inaccurate Dryback calculation when calibration is weak, profile or intent issues after restarts, and edge cases in daily or P2 logic. For this reason, a dry run before real pump operation is strongly recommended.

## 🗺 Roadmap

Planned or desired next steps include pH and ORP sensing, long-term databases, AI analysis, calendar or crop data, set-and-forget runs, osmotic steering, and automatic fill and mixing functions. The existing project documentation also names multi-zone support, better sensor timeout detection, dashboard diagnostic cards, and automatic shot-size calibration as useful future improvements.

## 💸 Donation

If you like the project and want to support it, you can donate via Bitcoin or Lightning.

### Bitcoin

`3NR2sqaP8p1CDYjjc4VjuFuAZAZ8m1VJo7`

### Lightning

`needycabbage98@walletofsatoshi.com`

<p align="left">
  <img src="https://github.com/multiply-a1/CHADD_PxPICS/blob/main/images/lightning_btc.jpg" alt="lightning_btc" width="20%" />
</p>

## 🤝 Contributing

Pull requests and issues are welcome. Please test changes in dry-run mode first and adapt entity IDs, MQTT topics, and helper values to your setup.

## 📄 License

This project is released under the MIT License.

## 🗂️ Helpfiles

[/docs](https://github.com/multiply-a1/CHADD_PxPICS/tree/main/docs)<br><br>
[User Guide Deutsch](https://github.com/multiply-a1/CHADD_PxPICS/blob/main/docs/userguide-de.md)<br>
[Readme-DE-byAgent](https://github.com/multiply-a1/CHADD_PxPICS/blob/main/docs/README-DE-byAgent.md)<br>
[Readme-EN-byAgent](https://github.com/multiply-a1/CHADD_PxPICS/blob/main/docs/README_EN-byAgent.md)<br>

## 🔗 Linklist

[https://cropsteering.xyz/](https://cropsteering.xyz/)<br>
[Home Assistant Grow Room (HAGR)](https://github.com/JakeTheRabbit/HAGR)<br>
[https://github.com/Intergalactic-XYZ/awesome-cropsteering](https://github.com/Intergalactic-XYZ/awesome-cropsteering)<br>
[opensalts](http://opensalts.wikidot.com/)<br>
[Grodan Grow Guide 2024 V2](https://www.grodan101.com/siteassets/downloads/grow-guide/grow-guide---cannabis-edition.pdf)<br>
[Agrowtek Crop Steering Guide](https://www.agrowtek.com/doc/an/AN_CropSteering.pdf)<br>
[Athena Handbook](https://info.athenaag.com/homegrower/homegrower-handbook)<br>
[CCI Black Book Crop Steering Super System](https://ccibook.com/pages/crop-steering-super-system-e-book)<br>
[AROYA Cultivation Guide](https://aroya.helpdocs.io/article/21hyiezmgu-cultivation-quick-start-guide)<br>
[Netafim Katalog](https://www.netafim.de/globalassets/local/germany/downloads/fur-webseite/netafim-katalog-2022-web-2.pdf)<br>
