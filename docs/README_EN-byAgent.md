# CHADD PXPICS

**CHADD PXPICS** is a Node-RED and Home Assistant based irrigation control system for greenhouse/substrate watering. It uses Home Assistant helpers as the source of truth, reads live sensor data, and drives irrigation decisions through a structured decision core.

[![Node-RED](https://img.shields.io/badge/Node--RED-Flow-red)](https://nodered.org/)
[![Home Assistant](https://img.shields.io/badge/Home%20Assistant-Integration-41BDF5)](https://www.home-assistant.io/)
[![License](https://img.shields.io/badge/License-Unlicensed-lightgrey)](#license)

## Overview

CHADD PXPICS combines Home Assistant helper entities with Node-RED logic to manage irrigation by VWC, dryback, phase windows, intent profiles, fallback rules, and safety limits.

The project is designed to be easy to tune from Home Assistant while keeping the decision logic in Node-RED.

## Features

- Reads VWC, EC, and temperature from Home Assistant sensors.
- Uses Home Assistant helpers as the runtime source of truth.
- Supports auto, assist, and manual control modes.
- Supports dryback-first, VWC-first, and hybrid steering.
- Applies phase windows and phase auto mode.
- Supports intent profiles for different growth stages.
- Tracks irrigation counts, fallback counts, and last irrigation timestamps.
- Provides fallback watering and manual override behavior.
- Exposes decision and context data for debugging and MQTT logging.

## Architecture

The system is split into three parts:

1. **Home Assistant helpers** store configuration, counters, and timestamps.
2. **Node-RED decision flows** read the current state and calculate the next action.
3. **Outputs and logs** update helpers, publish MQTT data, and make the runtime visible.

## Core concepts

### Control mode

- `auto`: the decision core is in full control.
- `assist`: the core may water when conditions are met.
- `manual`: manual behavior takes priority.

### Primary control

- `dryback_first`
- `vwc_first`
- `hybrid`

### Field capacity reference

`vwcFcRef` is the field-capacity reference used to calculate dryback and operating thresholds.

### Operational PAW

The project can display an operational PAW percentage based on `vwcFcRef` and a lower reference value. This is a practical display metric, not a strict physiological PWP value.

## Repository contents

Typical files in this project include:

- Node-RED flow exports.
- Home Assistant helper YAML.
- Phase window flows.
- Decision core references.
- Experimental or development flow versions.
- README documentation.

## Requirements

- Home Assistant.
- Node-RED.
- Working sensor entities for VWC, EC, and temperature.
- The CHADD PXPICS Home Assistant helper set.

## Installation

### 1. Import Home Assistant helpers

Import the helper YAML into Home Assistant and verify that all entity IDs exist exactly as expected.

### 2. Import Node-RED flows

Import the Node-RED flow export into your Node-RED instance.

### 3. Verify entity IDs

Check that the sensor IDs, helper IDs, and switch IDs in the flow match your setup.

### 4. Test the reset path

Run the daily reset and confirm that both counters and irrigation timestamps are reset correctly.

### 5. Test manual watering

Trigger a manual run and verify the pump output, timestamps, and counters.

## Home Assistant helpers

The project relies on helper entities such as:

- `input_select` for control mode, fallback mode, primary control, phase mode, and growth stage.
- `input_number` for intent, dryback targets, field capacity reference, shot size, pump time, counters, and limits.
- `input_boolean` for manual override, dry-run mode, skip-next-shot, fallback alert, decision logging, and phase auto mode.
- `input_text` for blocker reason, MQTT base topic, zone name, and current profile name.
- `input_datetime` for last irrigation timestamps and calibration dates.

## Decision flow

The decision core reads the current sensor and helper state, then calculates:

- Whether irrigation is allowed.
- Whether the system should water normally.
- Whether fallback watering is needed.
- Whether the action should be blocked.
- The expected shot size and pump time.

## Troubleshooting

### Missing helper entity

If a helper is missing, the flow can fail or fall back to invalid values. Verify the YAML import and the exact entity ID.

### Reset does not clear cooldown

If timestamps are not reset together with counters, cooldown logic may still block watering. Reset all related irrigation helpers, not only the counters.

### Wrong dryback behavior

If dryback does not match your expectations, verify `vwcFcRef` and the lower reference value used for your operational PAW display.

### Unexpected block reason

Check the blocker reason helper, irrigation count, last irrigation timestamp, sensor timeout, and min-gap settings.

## Calibration workflow

1. Define a stable field-capacity reference.
2. Set `vwcFcRef` in Home Assistant.
3. Choose a lower reference value for operational PAW.
4. Observe drain, dryback, and refill response.
5. Tune shot size and min-gap settings.
6. Re-test after changes to substrate, sensors, or phase logic.

## Development notes

This project is evolving. Keep helper IDs, sensor IDs, and flow versions aligned.

When changing logic, prefer explicit state fields and small function nodes so debugging stays simple.

## License

No license has been defined yet.
