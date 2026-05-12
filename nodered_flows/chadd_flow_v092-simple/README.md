# CHADD PXPICS Irrigation v0.9

Node-RED flow for Home Assistant based irrigation control using substrate sensor data, Home Assistant helpers as configuration storage, and a small decision engine that calculates whether irrigation should run, how large a shot should be, and how long the pump should stay on.[cite:444][cite:389]

## Overview

This flow is built around the tab **CHADD PXPICS Irrigation v0.9** and is described in the flow metadata as a refactored version that uses Home Assistant helpers as the source of truth.[cite:444] A repeating inject node starts the decision cycle every 3 minutes, while a separate daily reset chain clears counters and status helpers at 00:05.[cite:444]

The flow reads live substrate and control inputs from Home Assistant, assembles them into `msg.payload`, loads an irrigation profile from an internal profile map, writes the active profile name back to Home Assistant, and then passes the payload into the decision core.[cite:444] The result is routed into action branches such as `auto_water`, `fallback_water`, `manual`, `block`, and hold-style outcomes, with corresponding helper updates and MQTT logging.[cite:444]

## Main logic

The data collection stage reads VWC, EC, temperature, control mode, primary control, phase settings, growth stage, intent, dryback target, EC target, shot size, dripper count, dripper flowrate, field-capacity reference, and several safety or counter helpers via `api-current-state` nodes that write directly into `msg.payload.*` output properties.[cite:444] This structure keeps the runtime payload flat and makes later nodes simpler because the decision core can read values directly from `msg.payload` without extra conversion layers.[cite:444]

The `Load intent profile` function maps the selected intent or explicit profile index to one of ten predefined profiles ranging from **VEG 1** to **FLOWER 10**.[cite:444] Each profile defines steering metadata and irrigation defaults such as dryback target, dryback band, shot size, daily limit, P2 hold, VWC floor percentage, and irrigation window settings.[cite:444]

The decision core calculates dryback, applies sensor sanity checks, checks cooldown and daily limits, determines whether main logic is allowed, and computes shot size and pump time from `shot_size_ml`, `dripper_count`, and `dripper_flowrate`.[cite:444] The current helper definitions declare `chadd_pxpics_dripper_flowrate` with the unit `ml/s`, so the pump-time math assumes flow per dripper in milliliters per second unless the code is changed.[cite:389][cite:444]

## Actions and outputs

When irrigation is allowed, the route branch for `auto_water` writes helper values such as shot size, last irrigation timestamps, and irrigation counters, then publishes a retained MQTT decision message containing action, reason, profile, dryback, shot size, pump time, counters, and timestamps.[cite:444] The MQTT topic is built from the configured base topic and zone name, with defaults like `grow/CHADD_PXPICS` and `zone_1` if no custom values are present.[cite:444][cite:389]

Fallback handling is implemented as a separate route that can increment fallback counters, set a blocker or fallback reason, raise the fallback alert boolean, and publish a dedicated fallback log to MQTT.[cite:444] A daily reset chain resets fallback shots today, irrigation count today, fallback alert state, and the last blocker text to a clean state for the next cycle.[cite:444]

The flow also contains helper conversion nodes for writing Home Assistant services in the exact payload format those services expect, such as `{ "value": ... }` for `input_text.set_value` or `input_number.set_value`.[cite:444] This is an important design detail because the service nodes are strict about incoming JSON structure even when the decision payload itself is already correct.[cite:444]

## Home Assistant helpers

The supplied helper package defines the configuration and state entities the flow expects in Home Assistant, including control modes, profile settings, dryback targets, shot size, pump time, dripper count, dripper flowrate, field capacity, daily limits, fallback settings, MQTT topic settings, status text fields, booleans, and timestamps.[cite:389] Several of these helpers are not just inputs but also writeback targets for the flow, such as the current profile name, last blocker reason, fallback alert, and irrigation counters.[cite:389][cite:444]

Important helper examples include `input_number.chadd_pxpics_shot_size_ml`, `input_number.chadd_pxpics_dripper_flowrate`, `input_number.chadd_pxpics_max_pump_time`, `input_text.chadd_pxpics_last_blocker_reason`, `input_text.chadd_pxpics_current_profile_name`, and `input_datetime.chadd_pxpics_last_irrigation`.[cite:389] Sensor inputs in the cleaned flow are currently wired to `sensor.sdi12sensor_greenhouse1_sdi12_vwc`, `sensor.sdi12sensor_greenhouse1_sdi12_raw_ec`, and `sensor.sdi12sensor_greenhouse1_sdi12_temperature`, so imports into a different Home Assistant instance may require entity-id changes before the flow works unchanged.[cite:444]

## Setup

1. Import `chadd_nodered_v091_clean.json` into Node-RED and confirm that the tab **CHADD PXPICS Irrigation v0.9** appears.[cite:444]
2. Create the required Home Assistant helpers from the YAML package or recreate them manually with matching entity IDs and data types.[cite:389]
3. Configure the Home Assistant websocket server used by the flow and verify the MQTT broker settings if decision logging is needed.[cite:444]
4. Check entity IDs for VWC, EC, temperature, pump switch, helper entities, and any dashboard cards before enabling the repeating inject node.[cite:444][cite:389]
5. Validate dripper calibration carefully because pump time depends directly on `shot_size_ml / (dripper_count * dripper_flowrate)` in the flow logic.[cite:444]

## Calibration notes

The helper YAML declares the dripper flowrate in `ml/s`, with a box input and a 0.001 step size.[cite:389] For example, a 2 L/h dripper corresponds to about 0.556 ml/s, which matches the default-like values used in the flow test payloads and earlier function examples.[cite:444]

If a more human-friendly entry style is preferred, the helper could be changed to `ml/min`, but then the decision code must convert that value to `ml/s` before calculating `pumpTime`.[cite:389][cite:444] Without that code change, changing only the helper label would introduce a factor-of-60 error in pump runtime.[cite:389][cite:444]

## Testing and troubleshooting

The cleaned flow includes disabled test inject nodes and several debug-oriented helper functions, which makes it easier to validate the decision engine without immediately changing live hardware state.[cite:444] A safe bring-up path is to verify sensor payload assembly first, then profile writeback, then decision output, then helper writeback, and only after that the final pump branch.[cite:444]

Common issues are usually caused by mismatched entity IDs, incorrect Home Assistant service payload shape, dashboard templates using the wrong property path, or inconsistent flow-rate units.[cite:444][cite:389] If a Home Assistant service node throws JSON or type errors, the fix is usually to send a minimal payload object for that node rather than the entire decision payload.[cite:444]

## Files

- `chadd_nodered_v091_clean.json` contains the cleaned Node-RED flow definition for version 0.9.[cite:444]
- `chadd_pxpics_homeassistant_helpers_v0_5.yaml` contains the Home Assistant helper entities expected by the broader project and still matches the main helper model used by the flow.[cite:389]
