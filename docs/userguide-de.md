# CHADD PxPICS – Vollautomatisches Precision Irrigation System

## Coolste Features – Warum CHADD rockt

CHADD PxPICS ist nicht dein Standard-Timer – hier die **Killer-Features**:

1. **10-Stufen Intent-Steering**
Von GEN 1 (hartes generatives Dryback) bis VEG 10 (stabile Veg-Führung) – präziser als manuelle Thresholds.[^1]
2. **Dryback + Shot-Size-Kontrolle**
Misst echten Wasserverlust, dosiert ml-genau (P1/P2/P3-Logik) – Crop-Steering auf Profi-Niveau.[^1]
3. **Tages- \& Sicherheits-Limits**
Max. Shots/Tag, Sensor-Timeouts, Fallback-Modi – verhindert Overwatering oder Pumpen-Katastrophen.[^1]
4. **MQTT-Logging \& Home Assistant UI**
Vollständige Diagnose: `last_blocker_reason`, Profile-Namen, Shot-History – sieh, *warum* es gießt.[^1]
5. **Modular \& Erweiterbar**
Wireless-Sensoren, Multi-Zone-ready, KI-Roadmap – von DIY zu kommerziellem Scale.[^1]

**Unique Selling Point:** K.I.S.S. trifft Precision – lazy Grower werden Profis, ohne stundenlanges Babysitten.[^1]

## Vorwort

Ich habe jahrelange Erfahrung mit diversen hydroponischen Kleinstanlagen. Dazu zählen NFT, DWC, RDWC, Aeroponic, Coco und vor allem auch DIY-Anlagen.

Während des Baus von CHADD war ich überzeugt, dass solch eine Anlage im privaten Hausgebrauch nicht mehr als aufwendige und teure Spielerei ist.

Dennoch reizte mich der Gedanke, den großen, professionellen und exklusiven Clubs mal auf den Zahn zu fühlen.

Ich gehöre eher zu den Lazy Growern. Das altbewährte Motto K.I.S.S. habe ich schon früh verinnerlicht. Für alle, die es nicht kennen: Es heißt „Keep it simple, stupid“.

Vom Laziness-Faktor eines Autopot-Systems über wöchentliche Wasserwechsel bis hin zur täglichen Handbewässerung – Wege zu growen gibt es wie Sand am Meer.

Man sollte Prioritäten setzen, KISS ist da ganz klar:

- Halte die Kosten niedrig
- Arbeite so wenig wie möglich
- Verlasse dich nicht auf Technik Dritter

CHADD, wenn einmal eingestellt, ist KISS auf Steroiden – aber mit anderen Prioritäten.

Bei CHADD nehmen Zeit, Aufwand, Ertrag und Qualität die Top-Plätze der Prioritätenliste ein. Für jede kommerzielle Einrichtung sehr wichtige Faktoren.

Dabei steigen die Kosten, man arbeitet mehr und man verlässt sich zu 100 % auf Technik Dritter.

Wie unterscheiden sich die Kosten zu klassischen Systemen?

- Erhöhter Wasserverbrauch, somit auch mehr Verbrauch von Salzen
- Erhöhter Energieverbrauch durch Überwachungs- und Kontrolltechnik
- Rockwool-Cubes sind Single-Use

Erhöhtes Risiko eines Wasserschadens durch die Druckleitung.

An alle Mieter: Passt auf, dass ihr nur Qualitätsfittings kauft und alles richtig und fest zusammenbaut.

Wasser findet immer die Schwachstelle. Ist nicht cool, wenn die Polizei eure Pflanzen retten muss, weil euer nährstoffreiches Wasser nicht im Medium gelandet ist, sondern beim Nachbarn.

Wenn der Text jetzt schon zu viel war, dann ist CHADD wahrscheinlich nichts für dich. Man sollte in allen nötigen Bereichen wenigstens Grundkenntnisse mitbringen oder bereit sein, sich das Wissen anzueignen.

Künstliche Intelligenz ist dabei ein wertvolles Werkzeug.

Einfach 1000 oder 2000 € für ein funktionierendes „Set and Forget“-System wie Growlink oder GroSense auszugeben, ist es wahrscheinlich wert, wenn man nicht gerne schlaflose Nächte mit Troubleshooting verbringen will xD

## Features

Hier **alle Features** von CHADD PxPICS (aktuell + geplant), sortiert nach Kategorie:

### Kern-Steuerung

- 10-Stufen Intent-System (GEN 1–5, VEG 6–10)
- Dryback-Berechnung als primäre Steuergröße
- Field Capacity als Referenzpunkt (P1/P2/P3-Logik)
- Shot-Size-Dosierung (ml-genau via Pumpenlaufzeit)
- Rest-Perioden \& Min-Time-Between-Shots[^1]

### Tages- \& Limit-Management

- Tages-Shot-Zähler (`irrigation_count_today`)
- Max-Irrigations-per-Day (pro Intent-Profil)
- Fallback-Shots-Limit
- Last-Irrigation-of-Day-Tracking
- Lockout nach Manual-Override[^1]

### Sensorik \& Monitoring

- VWC, EC, Temp (BGT SZ, TEROS, etc.)
- Wireless via M5Stack ATOM Lite
- Sensor-Timeout-Erkennung
- Runoff-Detektion (Ultraschall/IR)
- TDS/pH in Drip \& Drain[^1]

### Modi \& Sicherheit

- Control-Modes: Manual, Auto, Fallback
- Dry-Run-Modus (Pumpen-Simulation)
- Manual-Override mit Skip-Next-Shot
- Blocker-Reason-Logging (`last_blocker_reason`)
- Phase-Auto-Mode (Veg/Gen-Wechsel)[^1]

### UI \& Diagnose

- Home Assistant Helper (Input-Number/Slider)
- Live Profile-Name-Anzeige
- MQTT-Context/Decision-Logs
- Zeitfenster-Logik (Window-Start/End)
- Kalibrierungs-Helper (Shot-Size, Dripper-Flow)[^1]

### Hardware-Integration

- Macvlan-Docker (standalone Services)
- Gardena-Pumpe \& Tavlit-Fittings
- RO/Mix-Tank-Monitoring
- AC Infinity AI Kompatibilität (Klima)[^1]

### Roadmap-Features (zukünftig)

- pH/ORP-Sensorik \& Nährstoff-Steering
- Langzeit-DB (InfluxDB/Grafana)
- KI-Auswertung \& Auto-Profil-Tuning
- Kalender/Crop-Phase-Integration
- Osmotic Steering (EC-basiert)
- Auto-Tank-Befüllung \& Mixing[^1]

**Status:** 70+ Features in Alpha – von Basis bis Profi-Tools. Erweiterbar ohne Rewrite.[^1]

## LFG

Was brauchen wir?

- Osmoseanlage
- DI-Filter
- RO-Tank (HDPE, UV-stabil, lebensmittelecht und lichtdicht – gibt’s nicht? Doch, danke Bauhaus!)
- RO-Tank-Pumpe
- RO-Tank-Zubehör
- Mix-Tank (wie RO-Tank)
- Mix-Tank-Zubehör
- Diverse HDPE- oder LDPE-Rohre und Fittings
- Raspi 4 oder höher inkl. Zubehör (Netzteil, Kühlung, SSD, MicroSD, Gehäuse)
- 1 Substratsensor (besser 2 oder 3)
- 1 M5Stack ATOM Lite
- 1 WLAN-Router
- Internet
- Hobbock-Eimer für die DXD-Station
- pH-, EC- und diverse andere Sensoren
- Schläuche, Kabel, Werkzeug und einen freien Willen
- Pro-Versionen von diversen Chatbots sind hilfreich (ich nutze Perplexity)

Dabei ist noch nicht das Growzelt oder die Raumausstattung enthalten.

## Pi-Setup 

01 – **Installiere headless Raspberry Pi OS, Docker \& Portainer auf deinem Pi.**

Dieser Schritt ist unkompliziert, es gibt viele gute Anleitungen online.

02 – **Konfiguriere Macvlan in Portainer.**

Ich habe dafür eine Anleitung verwendet.

Durch Macvlan verhält sich jeder Docker-Service wie ein eigenständiger Server. Es ist nicht zwingend notwendig, aber empfohlen.[^1]

03 – **Verwende meine Stacks für:**

- **Home Assistant**
- **Eclipse Mosquitto (MQTT-Broker)**
- **Node-RED**

Du musst einige Werte konfigurieren wie deine Macvlan-IPs, Zeitzone und Ordnerstruktur. Die Verwendung einer SSD (über HAT oder USB) für alle Daten ist dringend empfohlen.[^1]
Falls Node-RED oder Mosquitto Probleme haben, stelle sicher, dass die Berechtigungen deiner Ordner korrekt sind.

**In Node-RED:** Lade und installiere das **Home Assistant Webhook**-Plugin herunter.

**In Home Assistant:**

- Verbinde dich mit dem **MQTT-Broker-Server**.[^1]
- Installiere **Node-RED Companion** (via HACS oder Add-on, Integration).
- Installiere **HACS** (Home Assistant Community Store).
- Via HACS: Lade **HACS von dem Screenshot** (z. B. Mushroom Cards, Custom Button Card) für professionelle Dashboards.
  Diese erweitern die UI mit dynamischen Karten für Intent-Slider, Dryback-Charts und Logs.[^2]

## Sensorik

Es gibt eine breite Palette an Sensoren. Jeder sollte zumindest einen groben Richtwert liefern, aber aktuell gilt noch: Je teurer, desto genauer. Und bei P.I.S.S. wollen wir so präzise wie möglich arbeiten.

Natürlich ist das immer eine Frage des Budgets. Selbst die günstigsten Sensoren können zum Erfolg führen – vorausgesetzt, man ist in der Lage, die Pflanze richtig zu „lesen“.

Hier ein kleiner Überblick:

- BGT SZ (SDI-12)
- TEROS 12 (SDI-12)
- TEROS ONE (SDI-12, vermutlich noch nicht kompatibel)
- Andratek (RS485)
- SlapSense

Für RS485-Sensoren benötigt ihr einen Waveshare USB-zu-RS485-Konverter (ca. 10 €), um den Sensor per Kabel mit dem Raspberry Pi zu verbinden.

Für dieses Guide verwenden wir den BGT SZ in Kombination mit einem M5Stack ATOM Lite, um den Sensor drahtlos mit dem Pi zu verbinden.

Ich nutze AC Infinity AI zur Steuerung des Klimas, daher sind keine weiteren Umgebungssensoren notwendig.

R&D umfasst jedoch verschiedene zusätzliche Sensoren, um das System weiter zu verbessern, zum Beispiel:

- Wasser-/Regensensoren zur sofortigen Erkennung von Runoff (Pre-Alpha; möglich via Ultraschall, IR oder kapazitiv)
- TDS-Meter für die Drip- und Drain-Station
- Runoff-pH-Messung in der Drain-Station
- pH-Sensor in der Drip-Station (optional; bei größeren Anlagen sinnvoll, um pH-Veränderungen in der Leitung zu erkennen)
- Ultraschall-Füllstandsmessung

## Wassertanks

Das Tank-System ist simpel: Sammle 120 l RO-Wasser, fülle es in den 120 l Mix-Tank, bewässere vom Mix-Tank. Bestenfalls haben beide Tanks dasselbe Volumen. Bei weniger als 60 l besteht das Risiko, dass die Pumpe zu schnell trockenläuft.

**RO-Tank:**
**Mix-Tank:**

[  ] 01 – 1× Tavlit Nutlock 20 mm × 3/4" AG (€ 1,31)
[  ] 02 – 3× Tavlit T-Stück 3/4" IG × 3 (€ 4,03)
[  ] 03 – 2× Tavlit Mini-Kugelhahn 3/4" AG (€ 3,14)
[  ] 04 – 2× Tavlit Doppelnippel 3/4" AG (€ 0,68)
[  ] 05 – 1× Tavlit Reduzierstück 1" IG × 3/4" AG (€ 2,63)
[  ] 06 – 1× Tavlit Nutlock 16 mm × 3/4" AG (€ 1,29)
[  ] 07 – 1× Tavlit Nutlock Winkel 90° 16 mm × 16 mm (€ 3,00)
[  ] 08 – 1× Tavlit Endkappe 3/4" IG (€ 0,86)
[  ] 09 – 1× Gardena QuickConnect 3/4" IG (€ 2,00)
[  ] 10 – 1× Netafim K10 Vakuumbrecher (€ 29,78)
[  ] 11 – 1× PG-Verschraubung M32 (€ 0,20)
[  ] 12 – 2× PG-Verschraubung M25 (€ 0,20)
[  ] 13 – 1× PG-Verschraubung M10 (€ 0,20)
[  ] 14 – 1× Gardena 4700/2 Pumpe (€ 100,00)
[  ] 15 – 1 m Netafim 20 mm Schwarz-Weiß LDPE-Rohr
[  ] 16 – 1 m Netafim 16 mm Schwarz-Weiß LDPE-Rohr

## Brain

# CHADD PxPICS Precision Irrigation

**CHADD PxPICS** ist ein Home-Assistant- und Node-RED-basiertes Bewässerungsprojekt für präzise Substratsteuerung nach Crop-Steering-Prinzipien.
Der Aufbau orientiert sich an Dryback, Field Capacity, Shot Size, Tageslogik, Sicherheitsgrenzen und einem 10-Stufen-Intent-System.

## Einleitung

Ziel dieses Projekts ist eine Bewässerungssteuerung, die nicht nur nach einem einfachen Feuchtigkeitsschwellwert arbeitet.
Stattdessen kombiniert das System Sensorwerte, Tageszähler, letzte Bewässerungszeit, Shot-Größe, Zeitfenster und Intent-Profile, um eine deutlich feinere Steuerung zu ermöglichen.

Das System ist so aufgebaut, dass es in mehreren Reifegraden betrieben werden kann:

- Manuell, mit Logging und Sicherheits-Helfern.
- Halbautomatisch, mit Dryback- und Zeitfensterlogik.
- Vollautomatisch, mit Intent-Profilen für generatives und vegetatives Steering.

Die Architektur nutzt **Home Assistant** als zentrale Entitäten- und UI-Ebene und **Node-RED** als Entscheidungs- und Automationslogik.[^1]

## Benötigte Materialien

### Hardware

- Home-Assistant-System (z. B. Home Assistant OS oder Supervised).
- Node-RED-Installation (idealerweise mit Home-Assistant-Websocket-Integration).
- MQTT-Broker (z. B. Mosquitto).
- Pumpe oder Magnetventil mit schaltbarem Relais oder Smart Plug.
- Ein oder mehrere Tropfer oder Drip Stakes.
- Ein messbarer Wasserweg für Kalibrierung von Shot Size und Laufzeit.
- Substratsensoren für mindestens VWC (ideal zusätzlich EC und Temperatur).
- Optional: Auffangbecher oder Catch Cups zur Kalibrierung von Volumen, EC und Gleichmäßigkeit.[^1]


### Software / Integrationen

- Home Assistant.
- Node-RED.
- `node-red-contrib-home-assistant-websocket`.
- MQTT-Integration in Home Assistant.
- Optional: Dashboard / Lovelace-Karten für Helper und Diagnose.[^1]


## Funktionsprinzip

Das Projekt orientiert sich an fünf Kernideen:

1. **Field Capacity als Referenzpunkt** – P1 soll das Substrat bis zur Ziel-Sättigung auffüllen.
2. **Dryback als Steuergröße** – Die Differenz zwischen Sättigung und nächstem Tiefpunkt bestimmt generatives oder vegetatives Steering.
3. **Shot Size und Rest Period** – Nicht nur *ob* bewässert wird, sondern *wie viel* und *wie oft*.
4. **Tageslogik** – `last_irrigation_of_day`, Tageszähler und Limits verhindern unkontrolliertes Nachgießen.
5. **Intent-Profilsteuerung** – 10 Stufen von GEN 1 bis VEG 10 definieren das Steuerverhalten.[^1]

## Intent-Logik

Der Intent-Slider läuft von 1 bis 10. Diese Werte sind keine bloße Skala, sondern Profile:

- **GEN 1–5** = generatives Steering (stärkere Drybacks, restriktivere Führung).
- **VEG 6–10** = vegetatives Steering (mehr Stabilität, stärkere P2-Nutzung bei 9–10).[^1]


## Benötigte Sensoren und Aktoren

### Sensoren

- `sensor.chadd_pxpics_vwc`
- `sensor.chadd_pxpics_ec`
- `sensor.chadd_pxpics_temp`[^1]


### Aktoren

- `switch.shelly_plug_s` (oder dein tatsächlicher Pumpenschalter)[^1]


## Home-Assistant-Helper

Die Helper sind in `/packages/chadd_helpers.yaml` definiert. Wichtige Gruppen:

**Steuerung:**

- `input_select.chadd_pxpics_control_mode`
- `input_select.chadd_pxpics_fallback_mode`
- `input_select.chadd_pxpics_intent` (1–10)[^1]

**Kernparameter:**

- `input_number.chadd_pxpics_dryback_target`
- `input_number.chadd_pxpics_shot_size_ml`
- `input_number.chadd_pxpics_pump_time`[^1]

**Sicherheit \& Diagnose:**

- `input_boolean.chadd_pxpics_dry_run_mode`
- `input_text.chadd_pxpics_last_blocker_reason`
- `input_datetime.chadd_pxpics_last_irrigation`[^1]


## Installation (Schritt-für-Schritt)

1. **Dateien bereitstellen:** Lade den `packages`-Ordner in dein HA-Home-Verzeichnis.
2. **Helper einbinden:** In `configuration.yaml`:

```yaml
homeassistant:
  packages: !include_dir_named packages
```

3. **HA neu laden:** Starte HA neu oder lade YAML-Konfig.
4. **Node-RED-Flow importieren:** Importiere `chadd_pxpics_nodered_flow_v0_8.json` und passe Entity-IDs an.
5. **MQTT prüfen:** Stelle sicher, dass Topics wie `grow/CHADD_PXPICS/zone_1/decision` passen.[^1]

## Wichtige Hinweise

- **Dry-Run zuerst!** Aktiviere `dry_run_mode` für Tests.
- Passe alle Entity-Namen an dein Setup an.
- Starte mit Intent 4 oder 7, `min_time_between_shots = 15 min`.
- Troubleshooting: Überprüfe HA-Verbindung, Sensor-Timeouts und Profile.[^1]

## Irrigation Hardware

Die Irrigation-Hardware basiert auf druckstabilen Komponenten für präzise Dosierung:

- **Hauptleitung:** Netafim LDPE-Rohr (16/20 mm) mit Tavlit-Fittings und Kugelhähnen für RO- und Mix-Tank.
- **Druckregelung:** Vakuumbrecher (Netafim K10), PG-Verschraubungen und Mini-Kugelhähne zur Vermeidung von Druckschwankungen.
- **Pumpe:** Gardena 4700/2 (100 €) für zuverlässigen Transport aus dem Mix-Tank – kalibriere Laufzeit auf Shot-Size (z. B. 5–20 ml pro Tropfer).[^1][^2]

**Tipps:** Nutze nur Qualitätsfittings (Tavlit/Gardena), baue fest an und teste auf Lecks. Risiko von Wasserschäden minimiert sich durch Vakuumbrecher und Endkappen.[^3]

Hier ein typisches Layout für Drip-Irrigation – passe es an deinen Tank-Setup an.

## Drip x Drain Station

Die Drip-and-Drain-Station misst Input vs. Output für EC/Runoff-Kontrolle:

- **Drip-Seite:** Tropfer (z. B. 2 l/h) pro Pflanze, mit TDS/pH-Option am Ende der Leitung (bei großen Setups essenziell für pH-Stabilität).
- **Drain-Seite:** Hobbock-Eimer oder Auffangwanne mit Sensoren für Runoff-pH, EC und Volumen (Ultraschall für Füllstand).
- **Integration:** Verbinde mit CHADD via MQTT – erkennt Leachate sofort und passt Intent an.[^3]

**Vorteil:** Ermöglicht Crop-Steering durch Dryback- und EC-Monitoring. Starte mit kapazitiven Runoff-Sensoren (Pre-Alpha).[^4]

## Bekannte Bugs

Das System ist noch in der Frühphase – hier einige **bekannte Bugs** (und wahrscheinlich viele unbekannte):

- MQTT-Timeouts bei schwachem WLAN (Sensor-Updates verzögert).
- Intent-Profil-Switching kann bei schnellen HA-Neustarts hängen bleiben.
- Dryback-Berechnung ungenau bei unkalibrierten Field-Capacity-Werten.
- P2-Hold-Logik überspringt manchmal Shot-Limits bei Edge-Cases.

**Workaround:** Immer Dry-Run aktiv lassen und Logs in Node-RED überwachen. Melde Issues via GitHub oder Forum.[^1][^2]

## Systemstatus

**Alpha-Stage** – Ungetestet in Live-Environment.

- Funktioniert im Lab (Dry-Run + kleine Tests).
- Keine Langzeitstabilität bewiesen.
- **Erster Live-Test geplant:** September 2026 (Veg-Phase, 4–6 Pflanzen).

**Warnung:** Nur für Experimentierfreudige. Keine Garantie auf Ertragserhalt oder Hardware-Sicherheit![^3]

## Roadmap

Geplante Features (Priorität hoch → niedrig):

- pH/ORP-Sensorik für automatisierte Nährstoffanpassung.
- Langzeitdatenbanken (InfluxDB/Grafana) für Trend-Analyse.
- KI-Auswertung (Crop-Steering via ML-Modelle).
- Kalender/Crop-Daten-Integration (Auto-Phasenwechsel).
- Set-\&-Forget-Run (volle Automatisierung ohne tägliches Tuning).
- Osmotic Steering (EC-basiertes präzises Salz-Management).
- Automatische Befüll-/Mischfunktionen (Tank-Refill via Osmose).[^2]

**Zeitrahmen:** Erste Erweiterungen Q4 2026, KI bis 2027.
