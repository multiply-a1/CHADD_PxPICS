/*
 * SDI-12 Sensor Emulator V3.5 with MQTT Publishing for Home Assistant
 *
 * Changes in V3.5:
 * - Keeps V3.4 LED status support
 * - Adds 2 extra HA sensors: air_temperature and air_humidity
 * - Keeps original substrate derivation path intact
 * - Adds extra ambient values into MQTT state payload and discovery
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <math.h>
#include <Adafruit_NeoPixel.h>

#define DEBUG true
#define MQTT_MAX_PACKET_SIZE 1024

#define EMULATE_SENSOR true
#define EMULATE_NOISE true
#define ENABLE_STATUS_LED true

#define HOSTNAME_PREFIX "sdi12sensor-"
#define HOSTNAME_SUFFIX "greenhouse1"
#define EC_SIMPLE_FACTOR 500.0f

const char* ssid = "your_ssid";
const char* password = "your_password";
const char* hostname = HOSTNAME_PREFIX HOSTNAME_SUFFIX;

const char* mqtt_server = "192.168.178.176";
const int mqtt_port = 1883;
const char* mqtt_username = "";
const char* mqtt_password = "";
const char* client_id = hostname;
const char* discovery_prefix = "homeassistant";
const char* availability_topic = "sensors/sdi12sensor/status";

const unsigned long MQTT_PUBLISH_INTERVAL = 10000;
unsigned long lastPublishTime = 0;
unsigned long readingNumber = 0;

const long GMT_OFFSET_SEC = 3600;
const int DAYLIGHT_OFFSET_SEC = 3600;

const int DAY_START_MIN = 5 * 60 + 30;
const int DAY_END_MIN = 21 * 60;

const float TEMP_NIGHT = 18.4f;
const float TEMP_DAY = 22.6f;

const float VWC_MORNING_HIGH = 74.0f;
const float VWC_DAY_LOW = 36.0f;
const float VWC_NIGHT_HOLD = 42.0f;

const float EC_MORNING_LOW = 780.0f;
const float EC_DAY_HIGH = 1850.0f;
const float EC_NIGHT_HOLD = 1100.0f;

const float AIR_TEMP_NIGHT = 19.2f;
const float AIR_TEMP_DAY = 27.4f;
const float AIR_HUMIDITY_NIGHT = 71.0f;
const float AIR_HUMIDITY_DAY = 49.0f;

const float TEMP_NOISE_AMPL = 0.12f;
const float MOISTURE_NOISE_AMPL = 0.45f;
const float EC_NOISE_AMPL = 20.0f;
const float AIR_TEMP_NOISE_AMPL = 0.20f;
const float AIR_HUMIDITY_NOISE_AMPL = 1.2f;

const int LED_PIN = 35;
const int LED_COUNT = 1;
const uint8_t LED_BRIGHTNESS = 32;

struct SensorData {
  float vwc_raw;
  float vwc_calibrated;
  float temperature;
  float ec_raw;
  float ec_simple;
  float ec_epsilon;
  float air_temperature;
  float air_humidity;
  bool valid;
  unsigned long timestamp;
};

WiFiClient espClient;
PubSubClient mqtt(espClient);
Adafruit_NeoPixel pixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void debugPrint(String message, bool forceOutput = false);
void debugTable(const SensorData& data);
String formatTime(unsigned long ms);
String getStateTopic();
bool mqtt_publish(const char* topic, const char* payload, bool retained);
void setupWiFi();
void ensureTimeSync();
void publishDiscoveryConfigs();
void reconnectMQTT();
float calculateVWCNonSoil(float raw);
float calculateECSimple(float raw_ec);
float calculateECEpsilon(float raw);
SensorData readSensor();
void publishData(const SensorData& data);
float clampf(float value, float minVal, float maxVal);
float addNoise(float value, float amplitude);
float estimateRawForTargetVWC(float targetVWC);
void generateDayCycleData(SensorData &data);
int getMinuteOfDay();
float smoothStep(float x);
void initStatusLed();
void setStatusLed(uint32_t color);
void flashStatusLed(uint32_t color, int durationMs);
void updateIdleLedState();

uint32_t COLOR_OFF;
uint32_t COLOR_BLUE;
uint32_t COLOR_YELLOW;
uint32_t COLOR_GREEN;
uint32_t COLOR_RED;
uint32_t COLOR_WHITE;

void debugPrint(String message, bool forceOutput) {
  if (DEBUG || forceOutput) {
    Serial.println("[DEBUG] " + message);
  }
}

void initStatusLed() {
#if ENABLE_STATUS_LED
  pixel.begin();
  pixel.setBrightness(LED_BRIGHTNESS);
  COLOR_OFF = pixel.Color(0, 0, 0);
  COLOR_BLUE = pixel.Color(0, 0, 255);
  COLOR_YELLOW = pixel.Color(255, 180, 0);
  COLOR_GREEN = pixel.Color(0, 255, 0);
  COLOR_RED = pixel.Color(255, 0, 0);
  COLOR_WHITE = pixel.Color(180, 180, 180);
  setStatusLed(COLOR_OFF);
#endif
}

void setStatusLed(uint32_t color) {
#if ENABLE_STATUS_LED
  pixel.setPixelColor(0, color);
  pixel.show();
#endif
}

void flashStatusLed(uint32_t color, int durationMs) {
#if ENABLE_STATUS_LED
  uint32_t previous = COLOR_OFF;
  if (WiFi.status() == WL_CONNECTED && mqtt.connected()) previous = COLOR_GREEN;
  else if (WiFi.status() == WL_CONNECTED) previous = COLOR_YELLOW;
  else previous = COLOR_RED;
  setStatusLed(color);
  delay(durationMs);
  setStatusLed(previous);
#endif
}

void updateIdleLedState() {
#if ENABLE_STATUS_LED
  if (WiFi.status() == WL_CONNECTED && mqtt.connected()) setStatusLed(COLOR_GREEN);
  else if (WiFi.status() == WL_CONNECTED) setStatusLed(COLOR_YELLOW);
  else setStatusLed(COLOR_RED);
#endif
}

String formatTime(unsigned long ms) {
  unsigned long seconds = ms / 1000;
  unsigned long minutes = seconds / 60;
  seconds = seconds % 60;
  char timeStr[10];
  sprintf(timeStr, "%02lu:%02lu", minutes, seconds);
  return String(timeStr);
}

void debugTable(const SensorData& data) {
  if (!DEBUG) return;

  Serial.println("\n─────────────────────────────────────");
  Serial.printf("Time: %s Reading #%lu\n", formatTime(data.timestamp).c_str(), readingNumber);
  Serial.println("─────────────────────────────────────");
  Serial.println("├─ Substrate");
  Serial.printf("│ ├─ VWC Raw: %.2f\n", data.vwc_raw);
  Serial.printf("│ ├─ VWC Cal: %.2f%%\n", data.vwc_calibrated);
  Serial.printf("│ ├─ Temp: %.2f°C\n", data.temperature);
  Serial.printf("│ ├─ EC Raw: %.2f µS/cm\n", data.ec_raw);
  Serial.printf("│ ├─ EC Simple: %.2f\n", data.ec_simple);
  Serial.printf("│ └─ EC Epsilon: %.2f\n", data.ec_epsilon);
  Serial.println("└─ Air");
  Serial.printf("  ├─ Temp: %.2f°C\n", data.air_temperature);
  Serial.printf("  └─ RH: %.2f%%\n", data.air_humidity);
  Serial.println("─────────────────────────────────────\n");
}

float calculateVWCNonSoil(float raw) {
  float result = (6.771e-10 * pow(raw, 3) -
                  5.105e-6 * pow(raw, 2) +
                  1.302e-2 * raw -
                  10.848) * 100.0f;
  return constrain(result, 0, 100);
}

float calculateECSimple(float raw_ec) {
  return raw_ec / EC_SIMPLE_FACTOR;
}

float calculateECEpsilon(float raw) {
  float epsilon = 2.887e-9 * pow(raw, 3) -
                  2.080e-5 * pow(raw, 2) +
                  5.276e-2 * raw -
                  43.39;
  if (epsilon < 0) epsilon = 0;
  return pow(epsilon, 2);
}

float clampf(float value, float minVal, float maxVal) {
  if (value < minVal) return minVal;
  if (value > maxVal) return maxVal;
  return value;
}

float addNoise(float value, float amplitude) {
#if EMULATE_NOISE
  float r = (float)random(-1000, 1001) / 1000.0f;
  return value + (r * amplitude);
#else
  return value;
#endif
}

float estimateRawForTargetVWC(float targetVWC) {
  float bestRaw = 1000.0f;
  float bestDiff = 999999.0f;
  for (float raw = 800.0f; raw <= 5000.0f; raw += 1.0f) {
    float vwc = calculateVWCNonSoil(raw);
    float diff = fabs(vwc - targetVWC);
    if (diff < bestDiff) {
      bestDiff = diff;
      bestRaw = raw;
    }
  }
  return bestRaw;
}

String getStateTopic() {
  return String("sensors/sdi12sensor/") + hostname + "/state";
}

float smoothStep(float x) {
  x = clampf(x, 0.0f, 1.0f);
  return x * x * (3.0f - 2.0f * x);
}

int getMinuteOfDay() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return -1;
  return timeinfo.tm_hour * 60 + timeinfo.tm_min;
}

void generateDayCycleData(SensorData &data) {
  data.valid = true;
  data.timestamp = millis();

  int minuteOfDay = getMinuteOfDay();
  if (minuteOfDay < 0) minuteOfDay = 12 * 60;

  float targetTemp;
  float targetVWC;
  float targetECraw;
  float targetAirTemp;
  float targetAirHumidity;

  if (minuteOfDay < DAY_START_MIN) {
    targetTemp = TEMP_NIGHT;
    targetVWC = VWC_NIGHT_HOLD;
    targetECraw = EC_NIGHT_HOLD;
    targetAirTemp = AIR_TEMP_NIGHT;
    targetAirHumidity = AIR_HUMIDITY_NIGHT;
  } else if (minuteOfDay <= DAY_END_MIN) {
    float p = (float)(minuteOfDay - DAY_START_MIN) / (float)(DAY_END_MIN - DAY_START_MIN);
    float s = smoothStep(p);
    targetTemp = TEMP_NIGHT + sinf(p * PI) * (TEMP_DAY - TEMP_NIGHT);
    targetVWC = VWC_MORNING_HIGH - s * (VWC_MORNING_HIGH - VWC_DAY_LOW);
    targetECraw = EC_MORNING_LOW + s * (EC_DAY_HIGH - EC_MORNING_LOW);
    targetAirTemp = AIR_TEMP_NIGHT + sinf(p * PI) * (AIR_TEMP_DAY - AIR_TEMP_NIGHT);
    targetAirHumidity = AIR_HUMIDITY_NIGHT - s * (AIR_HUMIDITY_NIGHT - AIR_HUMIDITY_DAY);
  } else {
    float nightSpan = (24 * 60) - DAY_END_MIN;
    float p = (float)(minuteOfDay - DAY_END_MIN) / nightSpan;
    float s = smoothStep(p);
    targetTemp = TEMP_DAY - s * (TEMP_DAY - TEMP_NIGHT);
    targetVWC = VWC_DAY_LOW + s * (VWC_NIGHT_HOLD - VWC_DAY_LOW);
    targetECraw = EC_DAY_HIGH - s * (EC_DAY_HIGH - EC_NIGHT_HOLD);
    targetAirTemp = AIR_TEMP_DAY - s * (AIR_TEMP_DAY - AIR_TEMP_NIGHT);
    targetAirHumidity = AIR_HUMIDITY_DAY + s * (AIR_HUMIDITY_NIGHT - AIR_HUMIDITY_DAY);
  }

  targetTemp = clampf(addNoise(targetTemp, TEMP_NOISE_AMPL), 16.0f, 28.0f);
  targetVWC = clampf(addNoise(targetVWC, MOISTURE_NOISE_AMPL), 30.0f, 80.0f);
  targetECraw = clampf(addNoise(targetECraw, EC_NOISE_AMPL), 500.0f, 2500.0f);
  targetAirTemp = clampf(addNoise(targetAirTemp, AIR_TEMP_NOISE_AMPL), 15.0f, 35.0f);
  targetAirHumidity = clampf(addNoise(targetAirHumidity, AIR_HUMIDITY_NOISE_AMPL), 30.0f, 90.0f);

  data.temperature = targetTemp;
  data.vwc_raw = estimateRawForTargetVWC(targetVWC);
  data.vwc_calibrated = calculateVWCNonSoil(data.vwc_raw);
  data.ec_raw = targetECraw;
  data.ec_simple = calculateECSimple(data.ec_raw);
  data.ec_epsilon = calculateECEpsilon(data.vwc_raw);
  data.air_temperature = targetAirTemp;
  data.air_humidity = targetAirHumidity;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  initStatusLed();
  setStatusLed(COLOR_BLUE);

  Serial.println("\nSDI-12 Sensor Emulator V3.5 Starting Up");
  Serial.println("Debug output is " + String(DEBUG ? "enabled" : "disabled"));
  Serial.println("Emulation mode is enabled");
  Serial.println("Noise is " + String(EMULATE_NOISE ? "enabled" : "disabled"));
  Serial.println("Status LED is " + String(ENABLE_STATUS_LED ? "enabled" : "disabled"));

  randomSeed((uint32_t)esp_random());
  WiFi.setHostname(hostname);
  debugPrint("Device hostname: " + String(hostname), true);

  setupWiFi();
  ensureTimeSync();

  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setBufferSize(MQTT_MAX_PACKET_SIZE);
  updateIdleLedState();
  debugPrint("Setup complete", true);
}

void setupWiFi() {
  debugPrint("Connecting to WiFi network: " + String(ssid), true);
  setStatusLed(COLOR_YELLOW);
  WiFi.begin(ssid, password);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - startAttempt > 30000) {
      Serial.println("\nWiFi connect timeout, retrying...");
      WiFi.disconnect(true, true);
      delay(1000);
      WiFi.begin(ssid, password);
      startAttempt = millis();
      setStatusLed(COLOR_RED);
      delay(300);
      setStatusLed(COLOR_YELLOW);
    }
  }

  Serial.println("\nWiFi connected");
  Serial.printf("Hostname: %s\n", WiFi.getHostname());
  Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
}

void ensureTimeSync() {
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, "pool.ntp.org", "time.nist.gov");
  debugPrint("Syncing time with NTP...", true);
  setStatusLed(COLOR_YELLOW);

  struct tm timeinfo;
  int tries = 0;
  while (!getLocalTime(&timeinfo) && tries < 30) {
    delay(500);
    Serial.print("*");
    tries++;
  }

  if (getLocalTime(&timeinfo)) {
    Serial.println("\nTime synced");
  } else {
    Serial.println("\nTime sync failed, using fallback midday cycle");
  }
}

void publishDiscoveryConfigs() {
  debugPrint("Publishing discovery configurations...");

  String state_topic = getStateTopic();
  String device_config = "{\"identifiers\":[\"" + String(hostname) + "\"],"
                         "\"name\":\"" + String(hostname) + "\","
                         "\"model\":\"SDI-12 Sensor Emulator V3.5\","
                         "\"manufacturer\":\"Chill Division / Custom\"}";

  struct SensorConfig {
    const char* name;
    const char* id;
    const char* device_class;
    const char* unit;
    const char* value_template;
  };

  SensorConfig configs[] = {
    {"SDI12 VWC", "vwc_calibrated", "moisture", "%", "{{ value_json.vwc_calibrated | float }}"},
    {"SDI12 Raw VWC", "vwc_raw", "", "RAW", "{{ value_json.vwc_raw | float }}"},
    {"SDI12 Substrate Temperature", "temperature", "temperature", "°C", "{{ value_json.temperature | float }}"},
    {"SDI12 Raw EC", "ec_raw", "", "µS/cm", "{{ value_json.ec_raw | float }}"},
    {"SDI12 EC ppm500", "ec_simple", "", "ppm500", "{{ value_json.ec_simple | float }}"},
    {"SDI12 EC Epsilon", "ec_epsilon", "", "pW EC", "{{ value_json.ec_epsilon | float }}"},
    {"SDI12 Air Temperature", "air_temperature", "temperature", "°C", "{{ value_json.air_temperature | float }}"},
    {"SDI12 Air Humidity", "air_humidity", "humidity", "%", "{{ value_json.air_humidity | float }}"}
  };

  for (const auto& sensor : configs) {
    String config_topic = String(discovery_prefix) + "/sensor/" + hostname + "/" + sensor.id + "/config";

    String config = "{\"name\":\"" + String(sensor.name) + "\"," 
                    "\"object_id\":\"" + String(hostname) + "_" + sensor.id + "\"," 
                    "\"unique_id\":\"" + String(hostname) + "_" + sensor.id + "\"";

    if (strlen(sensor.device_class) > 0) {
      config += ",\"device_class\":\"" + String(sensor.device_class) + "\"";
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

bool mqtt_publish(const char* topic, const char* payload, bool retained) {
  debugPrint("Publishing to: " + String(topic));
  debugPrint("Payload: " + String(payload));
  bool success = mqtt.publish(topic, payload, retained);
  debugPrint("Publish " + String(success ? "successful" : "failed"));
  return success;
}

void reconnectMQTT() {
  while (!mqtt.connected()) {
    debugPrint("Attempting MQTT connection...");
    setStatusLed(COLOR_YELLOW);

    if (mqtt.connect(client_id, mqtt_username, mqtt_password, availability_topic, 1, true, "offline")) {
      debugPrint("MQTT Connected", true);
      mqtt.publish(availability_topic, "online", true);
      publishDiscoveryConfigs();
      setStatusLed(COLOR_GREEN);
    } else {
      Serial.printf("MQTT connection failed, rc=%d", mqtt.state());
      Serial.println(" retrying in 5 seconds");
      setStatusLed(COLOR_RED);
      delay(1000);
      setStatusLed(COLOR_YELLOW);
      delay(4000);
    }
  }
}

SensorData readSensor() {
#if EMULATE_SENSOR
  debugPrint("Generating emulated values and deriving sensor fields...");
  SensorData data;
  generateDayCycleData(data);
  return data;
#else
  SensorData data = {0, 0, 0, 0, 0, 0, 0, 0, false, millis()};
  return data;
#endif
}

void publishData(const SensorData& data) {
  if (!data.valid) return;

  StaticJsonDocument<384> doc;
  doc["vwc_raw"] = roundf(data.vwc_raw * 100.0f) / 100.0f;
  doc["vwc_calibrated"] = roundf(data.vwc_calibrated * 100.0f) / 100.0f;
  doc["temperature"] = roundf(data.temperature * 100.0f) / 100.0f;
  doc["ec_raw"] = roundf(data.ec_raw * 100.0f) / 100.0f;
  doc["ec_simple"] = roundf(data.ec_simple * 100.0f) / 100.0f;
  doc["ec_epsilon"] = roundf(data.ec_epsilon * 100.0f) / 100.0f;
  doc["air_temperature"] = roundf(data.air_temperature * 100.0f) / 100.0f;
  doc["air_humidity"] = roundf(data.air_humidity * 100.0f) / 100.0f;

  char state_payload[384];
  serializeJson(doc, state_payload, sizeof(state_payload));

  String state_topic = getStateTopic();
  bool success = mqtt_publish(state_topic.c_str(), state_payload, true);

  if (success) {
    flashStatusLed(COLOR_WHITE, 80);
    debugTable(data);
    readingNumber++;
  } else {
    setStatusLed(COLOR_RED);
  }
}

void loop() {
  if (!mqtt.connected()) {
    reconnectMQTT();
  }

  mqtt.loop();
  updateIdleLedState();

  unsigned long currentTime = millis();
  if (currentTime - lastPublishTime >= MQTT_PUBLISH_INTERVAL) {
    SensorData data = readSensor();
    if (data.valid) {
      publishData(data);
    } else {
      debugPrint("Failed to create sensor data", true);
      setStatusLed(COLOR_RED);
    }
    lastPublishTime = currentTime;
  }
}
