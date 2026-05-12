# CHADD PXPICS Precision Irrigation

CHADD PXPICS ist ein Home-Assistant- und Node-RED-basiertes Bewässerungsprojekt für präzise Substratsteuerung nach Crop-Steering-Prinzipien.
Der Aufbau orientiert sich an Dryback, Field Capacity, Shot Size, Tageslogik, Sicherheitsgrenzen und einem 10-Stufen-Intent-System.

## Einleitung

Ziel dieses Projekts ist eine Bewässerungssteuerung, die nicht nur nach einem einfachen Feuchtigkeitsschwellwert arbeitet.
Stattdessen kombiniert das System Sensorwerte, Tageszähler, letzte Bewässerungszeit, Shot-Größe, Zeitfenster und Intent-Profile, um eine deutlich feinere Steuerung zu ermöglichen.

Das System ist so aufgebaut, dass es in mehreren Reifegraden betrieben werden kann:

- Manuell, mit Logging und Sicherheits-Helpern.
- Halbautomatisch, mit Dryback- und Zeitfensterlogik.
- Vollautomatisch, mit Intent-Profilen für generatives und vegetatives Steering.

Die Architektur nutzt Home Assistant als zentrale Entitäten- und UI-Ebene und Node-RED als Entscheidungs- und Automationslogik.

## Benötigte Materialien

### Hardware

- Home-Assistant-System, zum Beispiel Home Assistant OS oder Supervised.
- Node-RED-Installation, idealerweise mit Home-Assistant-Websocket-Integration.
- MQTT-Broker, zum Beispiel Mosquitto.
- Pumpe oder Magnetventil mit schaltbarem Relais oder Smart Plug.
- Ein oder mehrere Tropfer oder Drip Stakes.
- Ein messbarer Wasserweg, damit Shot Size und Laufzeit kalibriert werden können.
- Substratsensoren für mindestens VWC; ideal zusätzlich EC und Temperatur.
- Optional Auffangbecher oder Catch Cups zur Kalibrierung von Volumen, EC und Gleichmäßigkeit.

### Software / Integrationen

- Home Assistant.
- Node-RED.
- `node-red-contrib-home-assistant-websocket`.
- MQTT-Integration in Home Assistant.
- Optional Dashboard / Lovelace-Karten für Helper und Diagnose.

## Funktionsprinzip

Das Projekt orientiert sich an fünf Kernideen:

1. **Field Capacity als Referenzpunkt** – P1 soll das Substrat bis zur Ziel-Sättigung auffüllen.
2. **Dryback als Steuergröße** – die Differenz zwischen Sättigung und nächstem Tiefpunkt bestimmt, wie generativ oder vegetativ gefahren wird.
3. **Shot Size und Rest Period** – nicht nur ob bewässert wird, sondern wie viel und wie oft.
4. **Tageslogik** – `last_irrigation_of_day`, Tageszähler und Tageslimits verhindern unkontrolliertes Nachgießen.
5. **Intent-Profilsteuerung** – 10 Stufen von GEN 1 bis VEG 10 definieren das Steuerverhalten.

## Intent-Logik

Der Intent-Slider läuft von 1 bis 10.
Diese Werte sind nicht nur eine Skala, sondern Profile:

- **GEN 1–5** = generatives Steering.
- **VEG 6–10** = vegetatives Steering.

Typisch gilt:

- Niedrigere GEN-Stufen bedeuten stärkeres generatives Steering, größere Drybacks und restriktivere Wasserführung.
- Höhere VEG-Stufen bedeuten vegetativere Steuerung, mehr Stabilität im Wassergehalt und bei VEG 9–10 eine stärkere P2-Nutzung.

## Benötigte Sensoren und Aktoren

Vor dem produktiven Betrieb sollten diese Entitäten vorhanden oder an dein Setup angepasst sein:

### Sensoren

- `sensor.chadd_pxpics_vwc`
- `sensor.chadd_pxpics_ec`
- `sensor.chadd_pxpics_temp`

### Aktoren

- `switch.shelly_plug_s` oder dein tatsächlicher Pumpenschalter

Diese Namen sind Platzhalter aus dem aktuellen Flow und müssen auf die realen Entitäten gemappt werden.

## Home-Assistant-Helper

Die aktuelle Helper-Datei ist `/packages/chadd_helpers.yaml`.
Sie enthält mehrere Gruppen von Helfern.

### Steuerung

- `input_select.chadd_pxpics_control_mode`
- `input_select.chadd_pxpics_fallback_mode`
- `input_select.chadd_pxpics_primary_control`
- `input_select.chadd_pxpics_phase_mode`
- `input_select.chadd_pxpics_growth_stage`

### Kernparameter

- `input_number.chadd_pxpics_intent`
- `input_number.chadd_pxpics_intent_profile_index`
- `input_number.chadd_pxpics_dryback_target`
- `input_number.chadd_pxpics_vwc_target`
- `input_number.chadd_pxpics_ec_target`
- `input_number.chadd_pxpics_shot_size_ml`
- `input_number.chadd_pxpics_pump_time`
- `input_number.chadd_pxpics_dripper_count`
- `input_number.chadd_pxpics_dripper_flowrate`
- `input_number.chadd_pxpics_field_capacity_estimate`

### Zeit- und Fensterlogik

- `input_number.chadd_pxpics_min_time_between_shots`
- `input_number.chadd_pxpics_window_start_minutes`
- `input_number.chadd_pxpics_window_end_minutes`
- `input_number.chadd_pxpics_p2_hold_minutes`

### Sicherheits- und Tageslogik

- `input_number.chadd_pxpics_sensor_timeout_minutes`
- `input_number.chadd_pxpics_max_pump_time`
- `input_number.chadd_pxpics_lockout_after_manual_minutes`
- `input_number.chadd_pxpics_fallback_shots_today_limit`
- `input_number.chadd_pxpics_fallback_shots_today`
- `input_number.chadd_pxpics_irrigation_count_today`
- `input_number.chadd_pxpics_max_irrigations_per_day`

### GEN/VEG-Profilwerte

- `input_number.chadd_pxpics_gen_dryback_target`
- `input_number.chadd_pxpics_veg_dryback_target`
- `input_number.chadd_pxpics_gen_shot_size_ml`
- `input_number.chadd_pxpics_veg_shot_size_ml`
- `input_number.chadd_pxpics_gen_max_irrigations_per_day`
- `input_number.chadd_pxpics_veg_max_irrigations_per_day`

### Diagnose und Status

- `input_boolean.chadd_pxpics_dry_run_mode`
- `input_boolean.chadd_pxpics_manual_override_enabled`
- `input_boolean.chadd_pxpics_skip_next_shot`
- `input_boolean.chadd_pxpics_fallback_alert_enabled`
- `input_boolean.chadd_pxpics_decision_log_enabled`
- `input_boolean.chadd_pxpics_phase_auto_mode`
- `input_text.chadd_pxpics_last_blocker_reason`
- `input_text.chadd_pxpics_current_profile_name`
- `input_datetime.chadd_pxpics_last_irrigation`
- `input_datetime.chadd_pxpics_last_irrigation_of_day`

## Installation

### 1. Dateien bereitstellen

lade den packages ordner in da ha home verzeichniss

### 2. Helper in Home Assistant einbinden

Referenziere den Ordner mit den YAML-Dateien über deine `configuration.yaml`:

```yaml
homeassistant:
  packages: !include_dir_named packages
```

### 3. Home Assistant neu laden

- Home Assistant neu starten, oder
- Helper / YAML-Konfiguration neu laden, sofern dein Setup das sauber unterstützt.

### 4. Entitäten prüfen

Prüfe in Home Assistant, ob alle Helper angelegt wurden.


### 5. Node-RED-Flow importieren

In Node-RED:

- Menü öffnen.
- `Import` wählen.
- `chadd_pxpics_nodered_flow_v0_8.json` importieren.
- Deploy noch nicht sofort final freigeben, sondern erst die Entity-IDs prüfen.

### 6. Server- und MQTT-Knoten prüfen

Im Flow sind ein Home-Assistant-Serverknoten und ein MQTT-Broker-Knoten enthalten.
Prüfe:

- ist Node-RED mit Home Assistant verbunden,
- stimmt der MQTT-Broker,
- stimmen Host, Port und Authentifizierung,
- passen die Topic-Strukturen zu deinem Setup.

## Konfiguration

### Sensor-Mapping

Passe diese Flow-Entitäten auf deine echten Sensoren an:

- `sensor.chadd_pxpics_vwc`
- `sensor.chadd_pxpics_ec`
- `sensor.chadd_pxpics_temp`

### Pumpen-Mapping

Ersetze bei Bedarf:

- `switch.shelly_plug_s`

mit deinem tatsächlichen Pumpenaktor.

### MQTT-Basis

Der Standardwert ist:

```text
grow/CHADD_PXPICS
```

Der Zonenname ist standardmäßig:

```text
zone_1
```

Daraus entstehen Topics wie:

- `grow/CHADD_PXPICS/zone_1/decision`
- `grow/CHADD_PXPICS/zone_1/fallback`
- `grow/CHADD_PXPICS/zone_1/context`

### Empfohlene Startwerte

Für den ersten Testlauf sind diese Werte sinnvoll:

- `control_mode = auto`
- `fallback_mode = timed_survival`
- `primary_control = dryback_first`
- `dry_run_mode = on`
- `phase_auto_mode = on`
- `intent = 4` oder `7`
- `min_time_between_shots = 15` oder höher
- `sensor_timeout_minutes = 15`
- `max_pump_time = 60`
- `fallback_shots_today_limit = 2`
- `max_irrigations_per_day = 4`

## Wie der Flow arbeitet

Der aktuelle v0.8-Flow läuft standardmäßig alle 3 Minuten.
Die Hauptlogik ist:

1. Sensoren und Helper lesen.
2. Phase- und Intent-Status lesen.
3. Profil aus dem Intent ableiten.
4. Profilnamen nach Home Assistant zurückschreiben.
5. Dryback, Shot Size, Limits und Fallback prüfen.
6. Entscheidung treffen: `auto_water`, `fallback_water`, `manual` oder `block`.
7. Tageswerte und Zeitstempel aktualisieren.
8. MQTT-Logs schreiben.

Zusätzlich läuft ein täglicher Reset für Tageszähler.

## Wichtige Begriffe

### Field Capacity

Field Capacity ist der Referenzpunkt, an dem das Substrat gesättigt ist und zusätzliches Wasser als Leachate / Runoff austritt.

### Shot Size

Shot Size ist die Wassermenge pro Bewässerungsereignis.
Sie kann über Dripperzahl, Durchfluss und Laufzeit berechnet werden.

### Dryback

Dryback beschreibt den Wasserverlust zwischen dem letzten Peak nach Bewässerung und dem nächsten Tiefpunkt vor der nächsten Bewässerung.
Er ist eine der wichtigsten Größen für vegetatives oder generatives Steering.

### P1 / P2 / P3

- **P1**: erste Wiederauffüllung Richtung Field Capacity.
- **P2**: Halte- oder Pflegephase nach P1, vor allem bei vegetativeren Strategien.
- **P3**: spätere Tagessteuerung oder Endfenster-Logik, je nach Profil.

Die aktuelle Flow-Basis ist schon auf P1/P2-orientierte Profile vorbereitet, insbesondere über Fensterlogik, Profile und `p2_hold_minutes`.

## Dry-Run empfohlen

Vor echtem Pumpenbetrieb sollte immer ein Dry-Run erfolgen.
Dazu:

1. `input_boolean.chadd_pxpics_dry_run_mode` aktivieren.
2. Flow deployen.
3. Prüfen, ob `current_profile_name` korrekt wechselt.
4. Prüfen, ob `last_blocker_reason` sinnvoll gesetzt wird.
5. Prüfen, ob MQTT-Logs ankommen.
6. Sicherstellen, dass die Pumpe nicht real schaltet.

## Troubleshooting

### Keine Reaktion im Flow

- Home-Assistant-Verbindung prüfen.
- Entity-IDs prüfen.
- `dry_run_mode`, `control_mode` und `phase_auto_mode` prüfen.
- Node-RED-Debug nutzen.

### Pumpe läuft zu häufig

- `min_time_between_shots` erhöhen.
- `max_irrigations_per_day` senken.
- Shot Size verkleinern.
- Profil und Intent prüfen.

### Fallback wird zu oft aktiv

- Sensorqualität prüfen.
- Timeout prüfen.
- Sanity-Limits prüfen.
- Dryback-Target und Profil-Mapping prüfen.

### Profilname wird nicht geschrieben

- `input_text.chadd_pxpics_current_profile_name` prüfen.
- Schreibrechte / Service-Call im HA-Knoten prüfen.

### Tageszähler stimmen nicht

- Täglichen Reset prüfen.
- Schreibpfade für `irrigation_count_today` und `fallback_shots_today` prüfen.
- Testen, ob echte oder nur Dry-Run-Aktionen gezählt werden sollen.

## Weiterentwicklung

Sinnvolle nächste Schritte wären:

- konsolidierte finale Intent-Matrix v2,
- exaktere P1/P2/P3-Zeitfensterlogik,
- bessere `last_updated`-basierte Sensor-Timeout-Erkennung,
- Dashboard-Karten für Diagnose,
- automatische Kalibrierung für Shot Size,
- mehrzonige Unterstützung.

## Hinweise

Dieses Projekt ist eine technische Basis für präzise Bewässerung und sollte immer zuerst im Dry-Run und dann unter Aufsicht getestet werden.
Jede Anlage reagiert anders auf Substrat, Topfgröße, Tropfer, Klima und Pflanzenphase.
Die Profile müssen daher an dein reales System angepasst und schrittweise validiert werden.
