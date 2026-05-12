#include <WiFiS3.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Arduino_LED_Matrix.h>

// =====================================================
// USER CONFIG
// =====================================================
const char* ssid = "your_ssid";
const char* password = "your_password";

const char* mqtt_server = "192.168.178.176";
const int mqtt_port = 1883;
const char* mqtt_username = "";
const char* mqtt_password = "";

const char* hostname = "uno-r4-water-ec";
const char* discovery_prefix = "homeassistant";
const char* availability_topic = "sensors/uno-r4-water-ec/status";

// =====================================================
// DEVICE CONFIG
// =====================================================
const char* DEVICE_MODEL = "UNO R4 WiFi + TDS Board V1";
const char* DEVICE_MANUFACTURER = "Arduino / Custom";

// =====================================================
// HARDWARE
// =====================================================
const int SENSOR_PIN = A0;
const float VREF = 5.0f;
const int ADC_MAX = 16383;
const float FIXED_TEMP_C = 20.0f;

// =====================================================
// TIMING
// =====================================================
const unsigned long MQTT_PUBLISH_INTERVAL = 10000;
unsigned long lastPublishTime = 0;
unsigned long readingNumber = 0;

// =====================================================
// EC CALIBRATION
// =====================================================
// Messpunkt von dir:
// Eichlösung 1.413 mS/cm -> 1.820 V

float ecCalibrationFactor = 1.0f; // wird in setup() aus Kalibrierpunkt berechnet

// =====================================================
// GLOBALS
// =====================================================
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
ArduinoLEDMatrix matrix;

// =====================================================
// MATRIX FONT 3x5
// =====================================================
const uint8_t DIGITS[10][5][3] = {
  {{1,1,1},{1,0,1},{1,0,1},{1,0,1},{1,1,1}},
  {{0,1,0},{1,1,0},{0,1,0},{0,1,0},{1,1,1}},
  {{1,1,1},{0,0,1},{1,1,1},{1,0,0},{1,1,1}},
  {{1,1,1},{0,0,1},{1,1,1},{0,0,1},{1,1,1}},
  {{1,0,1},{1,0,1},{1,1,1},{0,0,1},{0,0,1}},
  {{1,1,1},{1,0,0},{1,1,1},{0,0,1},{1,1,1}},
  {{1,1,1},{1,0,0},{1,1,1},{1,0,1},{1,1,1}},
  {{1,1,1},{0,0,1},{0,0,1},{0,0,1},{0,0,1}},
  {{1,1,1},{1,0,1},{1,1,1},{1,0,1},{1,1,1}},
  {{1,1,1},{1,0,1},{1,1,1},{0,0,1},{1,1,1}}
};

uint8_t frame[8][12];

struct SensorData {
  float voltage;
  int raw;
  float ec_raw_ms_cm;
  float ec_ms_cm;
  float water_temp_c;
  long wifi_rssi;
  unsigned long uptime_s;
  bool valid;
};

void clearFrame() {
  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 12; c++) {
      frame[r][c] = 0;
    }
  }
}

void renderFrame() {
  matrix.renderBitmap(frame, 8, 12);
}

void drawDigit(int digit, int x, int y) {
  if (digit < 0 || digit > 9) return;
  for (int r = 0; r < 5; r++) {
    for (int c = 0; c < 3; c++) {
      int rr = y + r;
      int cc = x + c;
      if (rr >= 0 && rr < 8 && cc >= 0 && cc < 12) {
        frame[rr][cc] = DIGITS[digit][r][c];
      }
    }
  }
}

void drawDot(int x, int y) {
  if (y >= 0 && y < 8 && x >= 0 && x < 12) frame[y][x] = 1;
}

void showEcNumber(float ec) {
  if (ec < 0) ec = 0;
  if (ec > 9.99f) ec = 9.99f;

  int ec100 = (int)(ec * 100.0f + 0.5f);
  int d1 = ec100 / 100;
  int d2 = (ec100 / 10) % 10;
  int d3 = ec100 % 10;

  clearFrame();
  drawDigit(d1, 0, 1);
  drawDot(3, 5);
  drawDigit(d2, 4, 1);
  drawDigit(d3, 8, 1);
  renderFrame();
}

void showBootIcon() {
  clearFrame();
  for (int i = 2; i < 10; i++) frame[1][i] = 1;
  for (int i = 4; i < 8; i++) frame[2][i] = 1;
  for (int i = 5; i < 7; i++) frame[3][i] = 1;
  for (int i = 5; i < 7; i++) frame[4][i] = 1;
  renderFrame();
}

void debugPrint(const String& message, bool forceOutput = false) {
  Serial.println("[DEBUG] " + message);
}

String getStateTopic() {
  return String("sensors/uno-r4-water-ec/") + hostname + "/state";
}

bool mqtt_publish(const char* topic, const char* payload, bool retained) {
  debugPrint("Publishing to: " + String(topic));
  debugPrint("Payload: " + String(payload));
  bool success = mqtt.publish(topic, payload, retained);
  debugPrint(String("Publish ") + (success ? "successful" : "failed"));
  return success;
}

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

float calculateRawEcMsCm(float voltage, float tempC) {
  float compensationCoefficient = 1.0f + 0.02f * (tempC - 25.0f);
  float compensatedVoltage = voltage / compensationCoefficient;

  float ecUsCm = (133.42f * compensatedVoltage * compensatedVoltage * compensatedVoltage
                - 255.86f * compensatedVoltage * compensatedVoltage
                + 857.39f * compensatedVoltage);

  if (ecUsCm < 0) ecUsCm = 0;
  return ecUsCm / 1000.0f;
}

SensorData readSensor() {
  SensorData data;
  data.raw = 0;
  data.voltage = readAveragedVoltage(data.raw);
  data.water_temp_c = FIXED_TEMP_C;
  data.ec_raw_ms_cm = calculateRawEcMsCm(data.voltage, data.water_temp_c);
  data.ec_ms_cm = data.ec_raw_ms_cm * ecCalibrationFactor;
  data.wifi_rssi = WiFi.RSSI();
  data.uptime_s = millis() / 1000UL;
  data.valid = true;
  return data;
}

void setupWiFi() {
  debugPrint("Connecting to WiFi network: " + String(ssid), true);
  WiFi.begin(ssid, password);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - startAttempt > 30000) {
      Serial.println("\nWiFi connect timeout, retrying...");
      WiFi.disconnect();
      delay(1000);
      WiFi.begin(ssid, password);
      startAttempt = millis();
    }
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void publishDiscoveryConfigs() {
  debugPrint("Publishing discovery configurations...");

  String state_topic = getStateTopic();
  String device_config =
    "{\"identifiers\":[\"" + String(hostname) + "\"],"
    "\"name\":\"" + String(hostname) + "\","
    "\"model\":\"" + String(DEVICE_MODEL) + "\","
    "\"manufacturer\":\"" + String(DEVICE_MANUFACTURER) + "\"}";

  struct SensorConfig {
    const char* name;
    const char* id;
    const char* device_class;
    const char* unit;
    const char* value_template;
    const char* icon;
  };

  SensorConfig configs[] = {
    {"Water EC", "ec_ms_cm", "", "mS/cm", "{{ value_json.ec_ms_cm | float }}", "mdi:flash"},
    {"Water EC Raw", "ec_raw_ms_cm", "", "mS/cm", "{{ value_json.ec_raw_ms_cm | float }}", "mdi:flash-outline"},
    {"Water Voltage", "ec_voltage", "", "V", "{{ value_json.ec_voltage | float }}", "mdi:sine-wave"},
    {"Water Raw ADC", "ec_raw", "", "RAW", "{{ value_json.ec_raw | float }}", "mdi:counter"},
    {"Water Temperature", "water_temp_c", "temperature", "°C", "{{ value_json.water_temp_c | float }}", "mdi:thermometer"},
    {"WiFi RSSI", "wifi_rssi", "", "dBm", "{{ value_json.wifi_rssi | float }}", "mdi:wifi"},
    {"Uptime", "uptime_s", "", "s", "{{ value_json.uptime_s | float }}", "mdi:timer-outline"}
  };

  for (const auto& sensor : configs) {
    String config_topic = String(discovery_prefix) + "/sensor/" + hostname + "/" + sensor.id + "/config";

    String config = "{\"name\":\"" + String(sensor.name) + "\","
                    "\"object_id\":\"" + String(hostname) + "_" + sensor.id + "\","
                    "\"unique_id\":\"" + String(hostname) + "_" + sensor.id + "\"";

    if (strlen(sensor.device_class) > 0) {
      config += ",\"device_class\":\"" + String(sensor.device_class) + "\"";
    }

    if (strlen(sensor.icon) > 0) {
      config += ",\"icon\":\"" + String(sensor.icon) + "\"";
    }

    config += ",\"state_class\":\"measurement\","
              "\"unit_of_measurement\":\"" + String(sensor.unit) + "\","
              "\"state_topic\":\"" + state_topic + "\","
              "\"value_template\":\"" + String(sensor.value_template) + "\","
              "\"availability_topic\":\"" + String(availability_topic) + "\","
              "\"payload_available\":\"online\","
              "\"payload_not_available\":\"offline\","
              "\"device\":" + device_config + "}";

    mqtt_publish(config_topic.c_str(), config.c_str(), true);
    delay(50);
  }

  mqtt_publish(availability_topic, "online", true);
}

void reconnectMQTT() {
  while (!mqtt.connected()) {
    debugPrint("Attempting MQTT connection...");

    if (mqtt.connect(hostname, mqtt_username, mqtt_password, availability_topic, 1, true, "offline")) {
      debugPrint("MQTT connected", true);
      mqtt.publish(availability_topic, "online", true);
      publishDiscoveryConfigs();
    } else {
      Serial.print("MQTT connection failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

void publishData(const SensorData& data) {
  if (!data.valid) return;

  StaticJsonDocument<256> doc;
  doc["ec_ms_cm"] = roundf(data.ec_ms_cm * 1000.0f) / 1000.0f;
  doc["ec_raw_ms_cm"] = roundf(data.ec_raw_ms_cm * 1000.0f) / 1000.0f;
  doc["ec_voltage"] = roundf(data.voltage * 1000.0f) / 1000.0f;
  doc["ec_raw"] = data.raw;
  doc["water_temp_c"] = roundf(data.water_temp_c * 10.0f) / 10.0f;
  doc["wifi_rssi"] = data.wifi_rssi;
  doc["uptime_s"] = data.uptime_s;

  char state_payload[256];
  serializeJson(doc, state_payload, sizeof(state_payload));

  String state_topic = getStateTopic();
  bool success = mqtt_publish(state_topic.c_str(), state_payload, true);

  if (success) {
    Serial.println("[SENSOR] ------------------------");
    Serial.print("[SENSOR] raw: ");
    Serial.println(data.raw);
    Serial.print("[SENSOR] voltage: ");
    Serial.println(data.voltage, 3);
    Serial.print("[SENSOR] tempC: ");
    Serial.println(data.water_temp_c, 1);
    Serial.print("[SENSOR] raw EC mS/cm: ");
    Serial.println(data.ec_raw_ms_cm, 3);
    Serial.print("[SENSOR] calibrated EC mS/cm: ");
    Serial.println(data.ec_ms_cm, 3);
    Serial.print("[MQTT] payload: ");
    Serial.println(state_payload);

    showEcNumber(data.ec_ms_cm);
    readingNumber++;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(14);
  pinMode(SENSOR_PIN, INPUT);

  matrix.begin();
  showBootIcon();

  Serial.println("\nUNO R4 Water EC starting");

  // Kalibrierpunkt aus deinem Messwert:
  // 1.820 V in 1.413 mS/cm Lösung bei 20C
  float rawEcAtCalibration = calculateRawEcMsCm(1.820f, FIXED_TEMP_C);
  ecCalibrationFactor = 1.413f / rawEcAtCalibration;

  Serial.print("[CAL] rawEcAt1.820V = ");
  Serial.println(rawEcAtCalibration, 3);
  Serial.print("[CAL] ecCalibrationFactor = ");
  Serial.println(ecCalibrationFactor, 4);

  setupWiFi();

  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setBufferSize(1024);

  reconnectMQTT();

  SensorData firstData = readSensor();
  publishData(firstData);

  lastPublishTime = millis();
}

void loop() {
  if (!mqtt.connected()) {
    reconnectMQTT();
  }

  mqtt.loop();

  unsigned long currentTime = millis();
  if (currentTime - lastPublishTime >= MQTT_PUBLISH_INTERVAL) {
    SensorData data = readSensor();
    if (data.valid) {
      publishData(data);
    }
    lastPublishTime = currentTime;
  }
}