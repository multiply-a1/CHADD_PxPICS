/*
 * SDI-12 Sensor Emulator V4.2
 * - 8h profile rotation (Veg / Early Flower / Late Flower)
 * - Within each 8h block: realistic 24h-style curves mapped into the block
 * - MQTT Home Assistant discovery
 * - Existing substrate + air sensors kept
 * - Additional virtual sensors added
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <math.h>
#include <Adafruit_NeoPixel.h>

#define DEBUG true
#define MQTT_MAX_PACKET_SIZE 2048
#define EMULATE_SENSOR true
#define EMULATE_NOISE true
#define ENABLE_STATUS_LED true

#define HOSTNAME_PREFIX "sdi12sensor-"
#define HOSTNAME_SUFFIX "emulator8h-1"
#define EC_SIMPLE_FACTOR 500.0f
#define FC_VWC 42.0f
#define PWP_VWC 18.0f
#define PI_F 3.14159265f

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
const unsigned long PROFILE_INTERVAL = 8UL * 60UL * 60UL * 1000UL;
unsigned long lastPublishTime = 0;
unsigned long readingNumber = 0;
unsigned long profileTimer = 0;
int currentProfile = 0;

const long GMT_OFFSET_SEC = 3600;
const int DAYLIGHT_OFFSET_SEC = 3600;
const int DAY_START_MIN = 5 * 60 + 30;
const int DAY_END_MIN = 21 * 60;

const float TEMP_NOISE_AMPL = 0.12f;
const float MOISTURE_NOISE_AMPL = 0.45f;
const float EC_NOISE_AMPL = 20.0f;
const float AIR_TEMP_NOISE_AMPL = 0.20f;
const float AIR_HUMIDITY_NOISE_AMPL = 1.2f;
const float CO2_NOISE_AMPL = 15.0f;
const float PH_NOISE_AMPL = 0.03f;
const float ORP_NOISE_AMPL = 6.0f;
const float PAR_NOISE_AMPL = 18.0f;
const float WATER_LEVEL_NOISE_AMPL = 0.8f;

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
  float vpd;
  float co2;
  float tank_ph;
  float tank_temperature;
  float tank_ec;
  float tank_orp;
  float substrate_fc;
  float dryback;
  float paw_percent;
  float runoff_ph;
  float runoff_ec;
  float runoff_temperature;
  float osmotic_potential;
  float leaf_temperature;
  float par;
  float dli;
  float dew_point;
  float delta_t;
  float water_level;
  int wifi_rssi;
  char stage[16];
  bool valid;
  unsigned long timestamp;
};

struct GrowthProfile {
  const char* name;
  float airTempMin, airTempMax;
  float rhMin, rhMax;
  float co2Base;
  float tankPhBase;
  float tankEcBase;
  float tankOrpBase;
  float vwcBase;
  float vwcSwing;
  float substrateTempBase;
  float substrateEcBase;
  float runoffPhBase;
  float runoffEcBase;
  float waterLevelBase;
};

const GrowthProfile profiles[] = {
  {"veg",         19.0f, 27.0f, 55.0f, 75.0f,  700.0f, 5.85f, 1.35f, 715.0f, 40.0f,  7.0f, 22.0f, 1200.0f, 5.95f, 1450.0f, 78.0f},
  {"early_flower",20.0f, 28.0f, 45.0f, 65.0f,  900.0f, 5.95f, 1.75f, 705.0f, 37.0f,  7.5f, 23.0f, 1300.0f, 6.00f, 1420.0f, 76.0f},
  {"late_flower", 20.0f, 28.0f, 40.0f, 60.0f, 1000.0f, 6.05f, 2.05f, 700.0f, 34.0f,  8.0f, 24.0f, 1400.0f, 6.10f, 1380.0f, 74.0f}
};
const int NUM_PROFILES = sizeof(profiles) / sizeof(profiles[0]);

WiFiClient espClient;
PubSubClient mqtt(espClient);
Adafruit_NeoPixel pixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

uint32_t COLOR_OFF, COLOR_BLUE, COLOR_YELLOW, COLOR_GREEN, COLOR_RED, COLOR_WHITE;

void debugPrint(String message, bool forceOutput = false) { if (DEBUG || forceOutput) Serial.println("[DEBUG] " + message); }
float clampf(float value, float minVal, float maxVal) { if (value < minVal) return minVal; if (value > maxVal) return maxVal; return value; }
float addNoise(float value, float amplitude) {
#if EMULATE_NOISE
  float r = (float)random(-1000, 1001) / 1000.0f;
  return value + (r * amplitude);
#else
  return value;
#endif
}
float calculateVWCNonSoil(float raw) { float result = (6.771e-10 * pow(raw, 3) - 5.105e-6 * pow(raw, 2) + 1.302e-2 * raw - 10.848) * 100.0f; return constrain(result, 0, 100); }
float calculateECSimple(float raw_ec) { return raw_ec / EC_SIMPLE_FACTOR; }
float calculateECEpsilon(float raw) { float epsilon = 2.887e-9 * pow(raw, 3) - 2.080e-5 * pow(raw, 2) + 5.276e-2 * raw - 43.39; if (epsilon < 0) epsilon = 0; return pow(epsilon, 2); }
float pawFromVWC(float vwc) { return clampf(((vwc - PWP_VWC) / (FC_VWC - PWP_VWC)) * 100.0f, 0.0f, 100.0f); }
float estimateRawForTargetVWC(float targetVWC) { float bestRaw = 1000.0f, bestDiff = 999999.0f; for (float raw = 800.0f; raw <= 5000.0f; raw += 1.0f) { float vwc = calculateVWCNonSoil(raw); float diff = fabs(vwc - targetVWC); if (diff < bestDiff) { bestDiff = diff; bestRaw = raw; } } return bestRaw; }
float smoothStep(float x) { x = clampf(x, 0.0f, 1.0f); return x * x * (3.0f - 2.0f * x); }
float dayWave(float x) { return 0.5f - 0.5f * cosf(2.0f * PI_F * x); }
float wrap24(float x) { while (x < 0) x += 1.0f; while (x >= 1.0f) x -= 1.0f; return x; }
int getMinuteOfDay() { struct tm ti; if (!getLocalTime(&ti)) return -1; return ti.tm_hour * 60 + ti.tm_min; }
String formatTime(unsigned long ms) { unsigned long s = ms / 1000; unsigned long m = s / 60; s %= 60; char t[10]; sprintf(t, "%02lu:%02lu", m, s); return String(t); }
String getStateTopic() { return String("sensors/sdi12sensor/") + hostname + "/state"; }

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
  pixel.setPixelColor(0, COLOR_OFF);
  pixel.show();
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

void debugTable(const SensorData& d) {
  if (!DEBUG) return;
  Serial.println("\n─────────────────────────────────────");
  Serial.printf("Time: %s Reading #%lu\n", formatTime(d.timestamp).c_str(), readingNumber);
  Serial.printf("Profile: %s Stage: %s\n", d.stage, profiles[currentProfile].name);
  Serial.println("─────────────────────────────────────");
  Serial.printf("Air Temp: %.2f°C  RH: %.2f%%  VPD: %.2f kPa  CO2: %.0f ppm\n", d.air_temperature, d.air_humidity, d.vpd, d.co2);
  Serial.printf("Tank pH: %.2f  Tank Temp: %.2f°C  Tank EC: %.2f  ORP: %.0f mV\n", d.tank_ph, d.tank_temperature, d.tank_ec, d.tank_orp);
  Serial.printf("VWC: %.2f%%  FC: %.2f%%  Dryback: %.2f%%  PAW: %.2f%%\n", d.vwc_calibrated, d.substrate_fc, d.dryback, d.paw_percent);
  Serial.printf("Sub Temp: %.2f°C  Sub EC: %.2f  Runoff pH: %.2f  Runoff EC: %.2f\n", d.temperature, d.ec_raw, d.runoff_ph, d.runoff_ec);
  Serial.printf("LeafTemp: %.2f°C  PAR: %.0f  DLI: %.2f  DewPoint: %.2f°C  DeltaT: %.2f°C\n", d.leaf_temperature, d.par, d.dli, d.dew_point, d.delta_t);
  Serial.printf("WaterLevel: %.2f%%  WiFi RSSI: %d dBm  Osmotic: %.2f\n", d.water_level, d.wifi_rssi, d.osmotic_potential);
  Serial.println("─────────────────────────────────────\n");
}

void setupWiFi() {
  debugPrint("Connecting to WiFi network: " + String(ssid), true);
  setStatusLed(COLOR_YELLOW);
  WiFi.begin(ssid, password);
  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (millis() - startAttempt > 30000) { WiFi.disconnect(true, true); delay(1000); WiFi.begin(ssid, password); startAttempt = millis(); }
  }
}

void ensureTimeSync() { configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, "pool.ntp.org", "time.nist.gov"); struct tm timeinfo; int tries = 0; while (!getLocalTime(&timeinfo) && tries < 30) { delay(500); tries++; } }

void publishDiscoveryConfigs() {
  String state_topic = getStateTopic();
  String device_config = "{\"identifiers\":[\"" + String(hostname) + "\"],\"name\":\"" + String(hostname) + "\",\"model\":\"SDI-12 Sensor Emulator V4.2\",\"manufacturer\":\"Chill Division / Custom\"}";
  struct SensorConfig { const char* name; const char* id; const char* device_class; const char* unit; const char* value_template; };
  SensorConfig configs[] = {
    {"WiFi RSSI", "wifi_rssi", "signal_strength", "dBm", "{{ value_json.wifi_rssi | float }}"},
    {"Air Temperature", "air_temperature", "temperature", "°C", "{{ value_json.air_temperature | float }}"},
    {"Air Humidity", "air_humidity", "humidity", "%", "{{ value_json.air_humidity | float }}"},
    {"VPD", "vpd", "", "kPa", "{{ value_json.vpd | float }}"},
    {"CO2", "co2", "carbon_dioxide", "ppm", "{{ value_json.co2 | float }}"},
    {"Tank pH", "tank_ph", "", "pH", "{{ value_json.tank_ph | float }}"},
    {"Tank Temperature", "tank_temperature", "temperature", "°C", "{{ value_json.tank_temperature | float }}"},
    {"Tank EC", "tank_ec", "conductivity", "mS/cm", "{{ value_json.tank_ec | float }}"},
    {"Tank ORP", "tank_orp", "", "mV", "{{ value_json.tank_orp | float }}"},
    {"Substrate VWC", "vwc_calibrated", "moisture", "%", "{{ value_json.vwc_calibrated | float }}"},
    {"Substrate FC", "substrate_fc", "", "%", "{{ value_json.substrate_fc | float }}"},
    {"Substrate Temperature", "temperature", "temperature", "°C", "{{ value_json.temperature | float }}"},
    {"Substrate EC", "ec_raw", "conductivity", "µS/cm", "{{ value_json.ec_raw | float }}"},
    {"Dryback", "dryback", "", "%", "{{ value_json.dryback | float }}"},
    {"PAW", "paw_percent", "", "%", "{{ value_json.paw_percent | float }}"},
    {"RunOff pH", "runoff_ph", "", "pH", "{{ value_json.runoff_ph | float }}"},
    {"RunOff EC", "runoff_ec", "conductivity", "µS/cm", "{{ value_json.runoff_ec | float }}"},
    {"RunOff Temperature", "runoff_temperature", "temperature", "°C", "{{ value_json.runoff_temperature | float }}"},
    {"Klima Osmotic Potential", "osmotic_potential", "", "", "{{ value_json.osmotic_potential | float }}"},
    {"LeafTemp", "leaf_temperature", "temperature", "°C", "{{ value_json.leaf_temperature | float }}"},
    {"PAR", "par", "illuminance", "µmol/m²/s", "{{ value_json.par | float }}"},
    {"DLI", "dli", "", "mol/m²/d", "{{ value_json.dli | float }}"},
    {"Dew Point", "dew_point", "temperature", "°C", "{{ value_json.dew_point | float }}"},
    {"DeltaT", "delta_t", "temperature", "°C", "{{ value_json.delta_t | float }}"},
    {"Water Level", "water_level", "water", "%", "{{ value_json.water_level | float }}"}
  };
  for (const auto& sensor : configs) {
    String config_topic = String(discovery_prefix) + "/sensor/" + hostname + "/" + sensor.id + "/config";
    String config = "{\"name\":\"" + String(sensor.name) + "\",\"object_id\":\"" + String(hostname) + "_" + sensor.id + "\",\"unique_id\":\"" + String(hostname) + "_" + sensor.id + "\"";
    if (strlen(sensor.device_class) > 0) config += ",\"device_class\":\"" + String(sensor.device_class) + "\"";
    config += ",\"state_class\":\"measurement\",\"unit_of_measurement\":\"" + String(sensor.unit) + "\",\"state_topic\":\"" + state_topic + "\",\"value_template\":\"" + String(sensor.value_template) + "\",\"availability_topic\":\"" + String(availability_topic) + "\",\"payload_available\":\"online\",\"payload_not_available\":\"offline\",\"device\":" + device_config + "}";
    mqtt.publish(config_topic.c_str(), config.c_str(), true);
    delay(30);
  }
  mqtt.publish(availability_topic, "online", true);
}

void reconnectMQTT() { while (!mqtt.connected()) { if (mqtt.connect(client_id, mqtt_username, mqtt_password, availability_topic, 1, true, "offline")) { mqtt.publish(availability_topic, "online", true); publishDiscoveryConfigs(); } else { delay(5000); } } }

float stagePosition(float elapsedFrac, float phaseShift = 0.0f) { return wrap24(elapsedFrac + phaseShift); }

void generateDayCycleData(SensorData &d) {
  d.valid = true; d.timestamp = millis();
  const GrowthProfile& gp = profiles[currentProfile];
  unsigned long phaseNow = millis() % PROFILE_INTERVAL;
  float phase = (float)phaseNow / (float)PROFILE_INTERVAL;
  float light = dayWave(phase);
  float night = 1.0f - light;

  float airTemp = gp.airTempMin + light * (gp.airTempMax - gp.airTempMin);
  float airRh = gp.rhMax - light * (gp.rhMax - gp.rhMin);
  float vpd = clampf((0.6108f * expf((17.27f * airTemp) / (airTemp + 237.3f))) * (1.0f - airRh / 100.0f), 0.0f, 6.0f);
  float co2 = 470.0f + night * 220.0f + light * gp.co2Base;
  float tankPh = gp.tankPhBase + 0.03f * sinf(phase * 2.0f * PI_F);
  float tankTemp = 19.0f + light * 2.2f + 0.2f * sinf((phase + 0.2f) * 2.0f * PI_F);
  float tankEc = gp.tankEcBase + light * 0.12f;
  float tankOrp = gp.tankOrpBase + night * 10.0f - light * 8.0f;

  float vwcTarget = gp.vwcBase + night * gp.vwcSwing + light * (gp.vwcSwing * 0.35f);
  float substrateTemp = gp.substrateTempBase + light * 1.2f + 0.5f * sinf((phase - 0.1f) * 2.0f * PI_F);
  float substrateEc = gp.substrateEcBase + night * 180.0f + light * 120.0f;

  float dryback = clampf(((FC_VWC - vwcTarget) / FC_VWC) * 100.0f, 0.0f, 100.0f);
  float paw = pawFromVWC(vwcTarget);

  float runoffPh = gp.runoffPhBase + 0.05f * light;
  float runoffEc = gp.runoffEcBase + light * 160.0f;
  float runoffTemp = tankTemp - 0.4f + 0.1f * light;
  float osmotic = -0.00125f * substrateEc;
  float leafTemp = airTemp - (0.7f + light * 0.6f) + 0.12f * sinf((phase + 0.35f) * 2.0f * PI_F);
  float par = light > 0.02f ? (light * 1100.0f) : 0.0f;
  float dli = (par * 8.0f) / 1000.0f / 60.0f;
  float dew = airTemp - ((100.0f - airRh) / 5.0f);
  float deltaT = leafTemp - airTemp;
  float waterLevel = gp.waterLevelBase - night * 2.0f - light * 1.0f;

  d.temperature = clampf(addNoise(substrateTemp, TEMP_NOISE_AMPL), 16.0f, 28.0f);
  d.vwc_raw = estimateRawForTargetVWC(clampf(addNoise(vwcTarget, MOISTURE_NOISE_AMPL), 18.0f, 80.0f));
  d.vwc_calibrated = calculateVWCNonSoil(d.vwc_raw);
  d.ec_raw = clampf(addNoise(substrateEc, EC_NOISE_AMPL), 500.0f, 2500.0f);
  d.ec_simple = calculateECSimple(d.ec_raw);
  d.ec_epsilon = calculateECEpsilon(d.vwc_raw);
  d.air_temperature = clampf(addNoise(airTemp, AIR_TEMP_NOISE_AMPL), 15.0f, 35.0f);
  d.air_humidity = clampf(addNoise(airRh, AIR_HUMIDITY_NOISE_AMPL), 30.0f, 90.0f);
  d.vpd = clampf(addNoise(vpd, 0.03f), 0.0f, 6.0f);
  d.co2 = clampf(addNoise(co2, CO2_NOISE_AMPL), 380.0f, 1300.0f);
  d.tank_ph = clampf(addNoise(tankPh, PH_NOISE_AMPL), 5.2f, 6.6f);
  d.tank_temperature = clampf(addNoise(tankTemp, 0.15f), 18.0f, 28.0f);
  d.tank_ec = clampf(addNoise(tankEc, EC_NOISE_AMPL), 700.0f, 2500.0f);
  d.tank_orp = clampf(addNoise(tankOrp, ORP_NOISE_AMPL), 620.0f, 780.0f);
  d.substrate_fc = FC_VWC;
  d.dryback = dryback;
  d.paw_percent = paw;
  d.runoff_ph = clampf(addNoise(runoffPh, PH_NOISE_AMPL), 5.2f, 6.8f);
  d.runoff_ec = clampf(addNoise(runoffEc, EC_NOISE_AMPL), 900.0f, 2800.0f);
  d.runoff_temperature = clampf(addNoise(runoffTemp, 0.15f), 16.0f, 28.0f);
  d.osmotic_potential = clampf(addNoise(osmotic, 0.01f), -4.0f, 0.0f);
  d.leaf_temperature = clampf(addNoise(leafTemp, 0.18f), 16.0f, 35.0f);
  d.par = clampf(addNoise(par, PAR_NOISE_AMPL), 0.0f, 1500.0f);
  d.dli = clampf(dli, 0.0f, 45.0f);
  d.dew_point = dew;
  d.delta_t = deltaT;
  d.water_level = clampf(addNoise(waterLevel, WATER_LEVEL_NOISE_AMPL), 0.0f, 100.0f);
  d.wifi_rssi = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : -127;
  snprintf(d.stage, sizeof(d.stage), "%s", gp.name);
}

SensorData readSensor() { SensorData d = {}; generateDayCycleData(d); return d; }

void publishData(const SensorData& d) {
  if (!d.valid) return;
  StaticJsonDocument<1536> doc;
  doc["stage"] = d.stage;
  doc["profile_index"] = currentProfile;
  doc["wifi_rssi"] = d.wifi_rssi;
  doc["air_temperature"] = roundf(d.air_temperature * 100.0f) / 100.0f;
  doc["air_humidity"] = roundf(d.air_humidity * 100.0f) / 100.0f;
  doc["vpd"] = roundf(d.vpd * 100.0f) / 100.0f;
  doc["co2"] = roundf(d.co2);
  doc["tank_ph"] = roundf(d.tank_ph * 100.0f) / 100.0f;
  doc["tank_temperature"] = roundf(d.tank_temperature * 100.0f) / 100.0f;
  doc["tank_ec"] = roundf(d.tank_ec * 100.0f) / 100.0f;
  doc["tank_orp"] = roundf(d.tank_orp);
  doc["vwc_raw"] = roundf(d.vwc_raw * 100.0f) / 100.0f;
  doc["vwc_calibrated"] = roundf(d.vwc_calibrated * 100.0f) / 100.0f;
  doc["substrate_fc"] = roundf(d.substrate_fc * 100.0f) / 100.0f;
  doc["temperature"] = roundf(d.temperature * 100.0f) / 100.0f;
  doc["ec_raw"] = roundf(d.ec_raw * 100.0f) / 100.0f;
  doc["ec_simple"] = roundf(d.ec_simple * 100.0f) / 100.0f;
  doc["ec_epsilon"] = roundf(d.ec_epsilon * 100.0f) / 100.0f;
  doc["dryback"] = roundf(d.dryback * 100.0f) / 100.0f;
  doc["paw_percent"] = roundf(d.paw_percent * 100.0f) / 100.0f;
  doc["runoff_ph"] = roundf(d.runoff_ph * 100.0f) / 100.0f;
  doc["runoff_ec"] = roundf(d.runoff_ec * 100.0f) / 100.0f;
  doc["runoff_temperature"] = roundf(d.runoff_temperature * 100.0f) / 100.0f;
  doc["osmotic_potential"] = roundf(d.osmotic_potential * 100.0f) / 100.0f;
  doc["leaf_temperature"] = roundf(d.leaf_temperature * 100.0f) / 100.0f;
  doc["par"] = roundf(d.par * 100.0f) / 100.0f;
  doc["dli"] = roundf(d.dli * 100.0f) / 100.0f;
  doc["dew_point"] = roundf(d.dew_point * 100.0f) / 100.0f;
  doc["delta_t"] = roundf(d.delta_t * 100.0f) / 100.0f;
  doc["water_level"] = roundf(d.water_level * 100.0f) / 100.0f;
  char payload[1536]; serializeJson(doc, payload, sizeof(payload)); mqtt.publish(getStateTopic().c_str(), payload, true); debugTable(d); readingNumber++;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  initStatusLed();
  setStatusLed(COLOR_BLUE);
  randomSeed((uint32_t)esp_random());
  WiFi.setHostname(hostname);
  setupWiFi();
  ensureTimeSync();
  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setBufferSize(MQTT_MAX_PACKET_SIZE);
  profileTimer = millis();
  updateIdleLedState();
}

void loop() {
  if (!mqtt.connected()) reconnectMQTT();
  mqtt.loop();
  updateIdleLedState();
  unsigned long now = millis();
  if (now - profileTimer >= PROFILE_INTERVAL) { currentProfile = (currentProfile + 1) % NUM_PROFILES; profileTimer = now; flashStatusLed(COLOR_WHITE, 250); }
  if (now - lastPublishTime >= MQTT_PUBLISH_INTERVAL) { publishData(readSensor()); lastPublishTime = now; }
}
