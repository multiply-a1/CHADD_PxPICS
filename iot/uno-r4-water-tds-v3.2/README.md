# Arduino UNO R4 WiFi TDS/EC Meter

Ein kompakter WLAN-fähiger TDS/EC-Meter auf Basis des **Arduino UNO R4 WiFi** und eines analogen **TDS Board V1.0**. Das Projekt misst den Analogausgang des Boards, berechnet daraus einen EC-Wert, zeigt den Wert auf der integrierten LED-Matrix an und veröffentlicht die Daten per **MQTT Discovery** direkt an Home Assistant.[cite:167][cite:45][cite:106]

## Überblick

Das Projekt ist für einfache Hydroponik-, Wasser- oder Nährlösungsüberwachung gedacht. Die Messung läuft lokal auf dem UNO R4 WiFi, die Werte werden zyklisch als JSON per MQTT gesendet, und Home Assistant kann das Gerät dank MQTT Discovery automatisch als Sensorgerät anlegen.[cite:45][cite:15]

### Funktionen

- EC-Messung über analoges TDS/EC-Frontend.[cite:167]
- WLAN-Anbindung direkt über den UNO R4 WiFi.[cite:1]
- MQTT Discovery für Home Assistant.[cite:45]
- Retained Discovery- und State-Topics für sofortige Sichtbarkeit in Home Assistant.[cite:45]
- Anzeige des EC-Werts auf der integrierten 12x8 LED-Matrix des UNO R4 WiFi.[cite:106]
- Feste Temperaturannahme, solange noch kein Wassertemperatursensor verbaut ist.[cite:214]
- Kalibrierung über eine 1.413 mS/cm Eichlösung.[cite:160][cite:164]

## Hardware

### Benötigte Komponenten

| Komponente | Zweck |
|---|---|
| Arduino UNO R4 WiFi | Hauptcontroller, WLAN, LED-Matrix.[cite:1][cite:106] |
| TDS Board V1.0 / analoges TDS-Board | Analoges Frontend für Leitfähigkeits-/TDS-Messung.[cite:167] |
| TDS-/EC-Sonde | Messfühler im Wasser. |
| 5V-Versorgung über USB | Stromversorgung für Arduino und Sensorboard. |
| 1.413 mS/cm Kalibrierlösung | Referenzpunkt für die Kalibrierung.[cite:160][cite:164] |

### Verdrahtung

Nach dem erfolgreichen Test hat sich gezeigt, dass bei diesem Board der **richtige Analogausgang am Pin `A0` des TDS-Boards** abgegriffen werden muss. Der zuvor getestete `T`-Pin lieferte kein brauchbares Messsignal im praktischen Betrieb.

**Empfohlene Verdrahtung:**

| TDS-Board | Arduino UNO R4 WiFi |
|---|---|
| `A0` | `A0` |
| `+` | `5V` |
| `-` | `GND` |
| Sondenstecker | An den Sondenanschluss des TDS-Boards |

### Wichtiger Hinweis zum Board

Bei dem getesteten TDS Board V1.0 war der Anschluss über `T` zwar naheliegend, lieferte aber praktisch einen fast festen High-Pegel. Erst der Anschluss des Arduino an **`A0` des TDS-Boards** ergab plausible Messwerte zwischen Luft, Wasser und Eichlösung. Dieses Verhalten zeigt, dass bei solchen Klon- oder No-Name-Boards die Beschriftung nicht immer so eindeutig ist, wie sie auf den ersten Blick wirkt.

## Messprinzip

Das TDS-Board gibt ein analoges Spannungssignal aus, das mit dem ADC des UNO R4 WiFi eingelesen wird. Der UNO R4 unterstützt erhöhte ADC-Auflösung, was für geglättete Analogmessungen hilfreich ist.[cite:15]

Die Messkette wurde praktisch mit folgenden Beispielwerten verifiziert:

| Zustand | Median | Spannung |
|---|---:|---:|
| Sensor in Luft | 460 | 0.125 V |
| Sensor in Wasser | 1671 | 0.510 V |
| Sensor in 1.413 mS/cm Eichlösung | 5963 | 1.820 V |

Diese Werte zeigen ein plausibles, sich änderndes Analogsignal und bestätigen, dass die Messung grundsätzlich funktioniert.

## Temperaturkompensation

Die elektrische Leitfähigkeit ist temperaturabhängig. Typisch sind etwa **1.5 bis 2.2 % Änderung pro °C**, weshalb schon wenige Grad Temperaturabweichung den EC-Wert merklich verschieben können.[cite:214][cite:217]

Für dieses Projekt wird zunächst eine **feste Wassertemperatur von 20.0 °C** angenommen. Das ist für einen einfachen Trend- und Praxisbetrieb ausreichend, solange sich die reale Wassertemperatur nur ungefähr im Bereich von 18 bis 22 °C bewegt. In diesem Bereich ist eine gewisse Abweichung zu erwarten, aber für eine erste Hydroponik-Überwachung ist das oft noch akzeptabel.[cite:214][cite:212]

### Empfehlung

- Für grobe Trends: feste 20.0 °C reichen zunächst aus.
- Für saubere Vergleichbarkeit und exakte EC-Führung: später einen Wassertemperatursensor ergänzen.
- Ein wasserdichter DS18B20 ist dafür oft die pragmatischste Wahl.

## Kalibrierung

Die Kalibrierung erfolgt mit einer **1.413 mS/cm**-Lösung, einem gängigen Referenzpunkt für EC-/Leitfähigkeitsmessungen.[cite:160][cite:164]

### Vorgehensweise

1. Sensor in die 1.413 mS/cm Eichlösung stellen.
2. Spannung oder Rohwert stabilisieren lassen.
3. Roh-EC mit der verwendeten Formel berechnen.
4. Kalibrierfaktor so anpassen, dass aus dem Rohwert genau **1.413 mS/cm** wird.

### Verwendeter Kalibrierpunkt

- Spannung in Eichlösung: **1.820 V**
- Referenzwert: **1.413 mS/cm**

### Grundidee der Berechnung

Zunächst wird aus der Sensorspannung ein unkalibrierter Roh-EC berechnet. Anschließend wird dieser mit einem Kalibrierfaktor multipliziert:

```cpp
float ec_ms_cm = ec_raw_ms_cm * ecCalibrationFactor;
```

Der Faktor wird aus dem bekannten Referenzpunkt abgeleitet:

```cpp
float rawEcAtCalibration = calculateRawEcMsCm(1.820f, 20.0f);
ecCalibrationFactor = 1.413f / rawEcAtCalibration;
```

## MQTT und Home Assistant

Das Projekt verwendet ein MQTT-Schema, das sich in der Praxis bewährt hat und sofort in Home Assistant sichtbar wird, wenn MQTT Discovery aktiv ist. Die zentrale Idee ist:

- ein **retained Availability-Topic**,
- **retained Discovery-Konfigurationen**,
- ein gemeinsames **JSON-State-Topic**,
- und ein **retained letzter State**, damit Home Assistant direkt einen gültigen Wert vorfindet.[cite:45][cite:206]

### Warum diese Variante zuverlässig funktioniert

Die bewährte Struktur orientiert sich an einem bereits funktionierenden Emulator-Setup mit **PubSubClient**. Dort werden nach erfolgreichem MQTT-Connect zuerst `online` auf dem Availability-Topic veröffentlicht, danach die Discovery-Configs retained gesendet und anschließend der eigentliche Sensorzustand ebenfalls retained publiziert.[cite:206]

Dieses Muster ist für Home Assistant robust, weil die Discovery-Daten persistent im Broker liegen und neue oder neu startende HA-Instanzen das Gerät sofort wiederfinden können.[cite:45]

### MQTT-Topik-Struktur

Beispielhaft verwendete Topics:

| Topic | Zweck |
|---|---|
| `sensors/uno-r4-water-ec/status` | Availability-Topic (`online` / `offline`) |
| `sensors/uno-r4-water-ec/<hostname>/state` | Gemeinsames JSON-State-Topic |
| `homeassistant/sensor/<hostname>/ec_ms_cm/config` | Discovery für EC |
| `homeassistant/sensor/<hostname>/ec_raw_ms_cm/config` | Discovery für Roh-EC |
| `homeassistant/sensor/<hostname>/ec_voltage/config` | Discovery für Spannung |
| `homeassistant/sensor/<hostname>/ec_raw/config` | Discovery für ADC-Rohwert |
| `homeassistant/sensor/<hostname>/water_temp_c/config` | Discovery für Wassertemperatur |
| `homeassistant/sensor/<hostname>/wifi_rssi/config` | Discovery für WLAN-Diagnose |
| `homeassistant/sensor/<hostname>/uptime_s/config` | Discovery für Laufzeit |

### Beispiel-State-Payload

```json
{
  "ec_ms_cm": 1.413,
  "ec_raw_ms_cm": 1.652,
  "ec_voltage": 1.820,
  "ec_raw": 5963,
  "water_temp_c": 20.0,
  "wifi_rssi": -45,
  "uptime_s": 175
}
```

### MQTT Discovery-Anforderungen

Damit Home Assistant das Gerät sofort erkennt, sollten folgende Punkte erfüllt sein:

- MQTT Discovery in Home Assistant aktivieren.[cite:45]
- Discovery-Topics retained veröffentlichen.[cite:45]
- State-Topic nach Möglichkeit retained senden.[cite:45][cite:206]
- Pro Entity eine eindeutige `unique_id` verwenden.[cite:45]
- Einen sauberen `device`-Block mit `identifiers`, `name`, `model` und `manufacturer` setzen.[cite:45][cite:206]
- Availability mit Last Will (`offline`) und Online-Meldung (`online`) nutzen.[cite:45][cite:206]

## LED-Matrix-Anzeige

Der UNO R4 WiFi besitzt eine integrierte **12x8 LED-Matrix**, die sich für kompakte numerische Anzeigen eignet.[cite:106] In diesem Projekt wird der EC-Wert als **`x.xx`** dargestellt, also zum Beispiel `1.41`.

Da die Matrix klein ist, werden dafür einfache 3x5-Ziffern verwendet. Für kurze numerische Werte ist das gut lesbar, für komplexere Texte dagegen weniger geeignet.[cite:106][cite:125]

## Softwarearchitektur

### Verwendete Bibliotheken

| Bibliothek | Zweck |
|---|---|
| `WiFiS3` | WLAN auf dem UNO R4 WiFi.[cite:1] |
| `PubSubClient` | MQTT-Kommunikation im robusten retained-Discovery-Stil.[cite:206] |
| `ArduinoJson` | Erzeugen des JSON-State-Payloads. |
| `Arduino_LED_Matrix` | Ausgabe auf der internen LED-Matrix.[cite:106] |

### Kernfunktionen der Firmware

- WLAN verbinden
- MQTT verbinden
- Availability publizieren
- Discovery-Konfigurationen retained veröffentlichen
- Analogwert mitteln
- Spannung berechnen
- Roh-EC berechnen
- Kalibrierfaktor anwenden
- JSON-State retained senden
- EC-Wert auf Matrix anzeigen

## Beispiel-Firmwarelogik

### Analoge Mittelwertbildung

```cpp
float readAveragedVoltage(int &rawAvg) {
  const int samples = 30;
  long sum = 0;

  for (int i = 0; i < samples; i++) {
    sum += analogRead(SENSOR_PIN);
    delay(10);
  }

  rawAvg = sum / samples;
  return (rawAvg / (float)ADC_MAX) * VREF;
}
```

### Roh-EC-Berechnung

```cpp
float calculateRawEcMsCm(float voltage, float tempC) {
  float compensationCoefficient = 1.0f + 0.02f * (tempC - 25.0f);
  float compensatedVoltage = voltage / compensationCoefficient;

  float ecUsCm = (133.42f * compensatedVoltage * compensatedVoltage * compensatedVoltage
                - 255.86f * compensatedVoltage * compensatedVoltage
                + 857.39f * compensatedVoltage);

  if (ecUsCm < 0) ecUsCm = 0;
  return ecUsCm / 1000.0f;
}
```

### MQTT-Reconnect mit Availability und Discovery

```cpp
void reconnectMQTT() {
  while (!mqtt.connected()) {
    if (mqtt.connect(hostname, mqtt_username, mqtt_password,
                     availability_topic, 1, true, "offline")) {
      mqtt.publish(availability_topic, "online", true);
      publishDiscoveryConfigs();
    } else {
      delay(5000);
    }
  }
}
```

## Inbetriebnahme

### 1. Hardware anschließen

- TDS-Board `A0` an Arduino `A0`
- TDS-Board `+` an `5V`
- TDS-Board `-` an `GND`
- Sonde an den Sondenanschluss des TDS-Boards

### 2. Firmware konfigurieren

In der `.ino`-Datei anpassen:

- WLAN-Zugangsdaten
- MQTT-Broker-Adresse
- MQTT-Port
- MQTT-Benutzername/Passwort
- Hostname
- Publish-Intervall

### 3. Firmware flashen

- Arduino UNO R4 WiFi in der IDE auswählen
- Benötigte Bibliotheken installieren
- Sketch kompilieren und hochladen
- Seriellen Monitor mit 115200 Baud öffnen

### 4. Funktion prüfen

Erwartete Prüfpunkte:

- WLAN verbindet erfolgreich.[cite:1]
- MQTT verbindet erfolgreich.
- Availability wird auf `online` gesetzt.[cite:206]
- Discovery-Topics erscheinen im Broker retained.[cite:45]
- Home Assistant legt das Gerät automatisch an.[cite:45]
- LED-Matrix zeigt den EC-Wert an.[cite:106]

## Fehlersuche

### Home Assistant findet das Gerät nicht

Mögliche Ursachen:

- MQTT Discovery ist in Home Assistant nicht aktiv.[cite:45]
- Discovery-Configs wurden nicht retained gesendet.[cite:45]
- Alte fehlerhafte Discovery-Topics liegen noch im Broker und stören die neue Struktur.[cite:45]
- `unique_id` oder `device.identifiers` sind inkonsistent.[cite:45]

### Werte sind unplausibel hoch oder ändern sich nicht

Mögliche Ursachen:

- Falscher Ausgang des TDS-Boards verwendet.
- `T` statt `A0` angeschlossen.
- Masseverbindung fehlt.
- Sonde oder TDS-Board defekt.
- Falsche Interpretation der Boardbeschriftung.

### LED-Matrix zeigt 999

Dann ist der berechnete EC-Wert größer als der darstellbare Bereich von `9.99`. Das deutet meist auf ein unplausibles Sensorsignal, falsche Kalibrierung oder einen falschen Signalausgang hin.

## Erweiterungen

Spätere sinnvolle Ausbaustufen:

- **DS18B20** als echter Wassertemperatursensor für präzisere Temperaturkompensation
- zusätzliche Statusanzeige auf der LED-Matrix
- Glättung über Medianfilter
- MQTT-Command-Topic für Kalibrierung oder Diagnose
- Logging in InfluxDB / Grafana über Home Assistant oder Node-RED

## Projektstatus

Der aktuelle Stand des Projekts ist funktional:

- Analogsignal verifiziert
- richtiger Ausgang (`A0` am TDS-Board) identifiziert
- MQTT-Übertragung funktional
- Home-Assistant-Discovery funktional
- LED-Matrix-Anzeige funktional
- fixe Temperaturannahme implementiert
- Kalibrierung über 1.413 mS/cm vorbereitet bzw. integriert

## Lizenz und Hinweise

Dieses Projekt kombiniert Standardbausteine aus Arduino-, MQTT- und Home-Assistant-Umfeld mit einer individuell angepassten Firmwarestruktur. Für Klon- oder No-Name-TDS-Boards sollte immer zuerst praktisch geprüft werden, welcher Ausgang tatsächlich das sinnvolle Analogsignal liefert, da Beschriftungen und Verhalten je nach Board stark variieren können.
