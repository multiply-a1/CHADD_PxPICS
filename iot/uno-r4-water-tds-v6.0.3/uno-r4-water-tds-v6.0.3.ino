#include <WiFiS3.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Arduino_LED_Matrix.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>
#include <EEPROM.h>

// =====================================================
// USER CONFIG
// =====================================================
const char* ssid = "your_ssid";
const char* password = "your_password";
const char* mqtt_server = "192.168.178.176";
const int mqtt_port = 1883;
const char* mqtt_username = "";
const char* mqtt_password = "";
const char* hostname = "uno-r4-water-ec-6003";
const char* client_id = "uno-r4-water-ec-6003";
const char* discovery_prefix = "homeassistant";
const char* availability_topic = "sensors/uno-r4-water-ec-6003/status";
const char* ph_calibration_topic = "sensors/uno-r4-water-ec-6003/ph/calibrate";
const char* tank_calibration_topic = "sensors/uno-r4-water-ec-6003/tank/calibrate";
const char* matrix_enabled_command_topic = "sensors/uno-r4-water-ec-6003/matrix/enabled/set";
const char* matrix_enabled_state_topic = "sensors/uno-r4-water-ec-6003/matrix/enabled/state";
const char* matrix_mode_command_topic = "sensors/uno-r4-water-ec-6003/matrix/mode/set";
const char* matrix_mode_state_topic = "sensors/uno-r4-water-ec-6003/matrix/mode/state";

// =====================================================
// DEVICE CONFIG
// =====================================================
const char* DEVICE_MODEL = "UNO R4 WiFi + TDS + pH + Rain + Tanks + Matrix";
const char* DEVICE_MANUFACTURER = "Arduino / Custom";

// =====================================================
// HARDWARE
// =====================================================
const int EC_SENSOR_PIN = A0;
const int PH_SENSOR_PIN = A5;
const int RAIN_SENSOR_ANALOG_PIN = A1;
const int RAIN_SENSOR_DIGITAL_PIN = 5;
const int DS18B20_PIN = 2;
const int RO_TRIG_PIN = 6;
const int RO_ECHO_PIN = 7;
const int MIX_TRIG_PIN = 8;
const int MIX_ECHO_PIN = 9;
const int DRAIN_TRIG_PIN = 10;
const int DRAIN_ECHO_PIN = 11;
const float VREF = 5.0f;
const int ADC_MAX = 16383;

// =====================================================
// TIMING
// =====================================================
const unsigned long MQTT_PUBLISH_INTERVAL = 10000;
const unsigned long MATRIX_REFRESH_INTERVAL = 1000;
const unsigned long SERIAL_STATUS_INTERVAL = 30000;
const unsigned long LEVEL_MEASURE_INTERVAL = 5000;

unsigned long lastPublishTime = 0;
unsigned long lastMatrixRefreshTime = 0;
unsigned long lastSerialStatusTime = 0;
unsigned long lastLevelMeasureTime = 0;

// =====================================================
// CALIBRATION / STATE
// =====================================================
float ecCalibrationFactor = 1.0f;
const float TDS_FACTOR = 500.0f;
float phV7 = 2.500f, phV4 = 3.000f, phV10 = 2.000f;
bool phHasV7 = false, phHasV4 = false, phHasV10 = false;
float roFullCm = NAN, roEmptyCm = NAN, roCapacityL = 120.0f;
float mixFullCm = NAN, mixEmptyCm = NAN, mixCapacityL = 120.0f;
float drainFullCm = NAN, drainEmptyCm = NAN, drainCapacityMl = 500.0f;
float roDistanceCm = NAN, mixDistanceCm = NAN, drainDistanceCm = NAN;
float roPercent = NAN, mixPercent = NAN, drainPercent = NAN;
float roLiters = NAN, mixLiters = NAN, drainMl = NAN;
float lastValidTempC = 20.0f;
bool matrixEnabled = true;
String matrixMode = "ec";

const uint32_t EEPROM_MAGIC = 0x39454E52;
struct PersistData {
  uint32_t magic;
  float roFullCm;
  float roEmptyCm;
  float roCapacityL;
  float mixFullCm;
  float mixEmptyCm;
  float mixCapacityL;
  float drainFullCm;
  float drainEmptyCm;
  float drainCapacityMl;
  float phV7;
  float phV4;
  float phV10;
  bool phHasV7;
  bool phHasV4;
  bool phHasV10;
};

// =====================================================
// GLOBALS
// =====================================================
OneWire oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
ArduinoLEDMatrix matrix;
uint8_t frame[8][12];

struct SensorData {
  float ec_voltage;
  int ec_raw;
  float temp_c;
  float ec_raw_ms_cm;
  float ec_ms_cm;
  float tds_ppm;
  float ph_voltage;
  float ph_value;
  float rain_analog_v;
  int rain_analog_raw;
  bool rain_digital;
  float ro_distance_cm;
  float mix_distance_cm;
  float drain_distance_cm;
  float ro_percent;
  float mix_percent;
  float drain_percent;
  float ro_liters;
  float mix_liters;
  float drain_ml;
  long wifi_rssi;
  unsigned long uptime_s;
  bool valid;
};

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

// =====================================================
// MATRIX HELPERS
// =====================================================
void clearFrame(){ for(int r=0;r<8;r++) for(int c=0;c<12;c++) frame[r][c]=0; }
void renderFrame(){ matrix.renderBitmap(frame,8,12); }
void drawDigit(int d,int x,int y){ if(d<0||d>9) return; for(int r=0;r<5;r++) for(int c=0;c<3;c++){ int rr=y+r, cc=x+c; if(rr>=0&&rr<8&&cc>=0&&cc<12) frame[rr][cc]=DIGITS[d][r][c]; } }
void drawDot(int x,int y){ if(y>=0&&y<8&&x>=0&&x<12) frame[y][x]=1; }
void drawMinus(int x,int y){ if(y>=0&&y<8&&x>=0&&x+2<12){ frame[y][x]=1; frame[y][x+1]=1; frame[y][x+2]=1; } }
void showBootIcon(){ clearFrame(); for(int i=2;i<10;i++) frame[1][i]=1; for(int i=4;i<8;i++) frame[2][i]=1; for(int i=5;i<7;i++) frame[3][i]=1; for(int i=5;i<7;i++) frame[4][i]=1; renderFrame(); }
void showEcNumber(float ec){ if(ec<0) ec=0; if(ec>9.99f) ec=9.99f; int ec100=(int)(ec*100.0f+0.5f); int d1=ec100/100; int d2=(ec100/10)%10; int d3=ec100%10; clearFrame(); drawDigit(d1,0,1); drawDot(3,5); drawDigit(d2,4,1); drawDigit(d3,8,1); renderFrame(); }
void showTempNumber(float t){ bool neg=t<0; if(t<-9.9f) t=-9.9f; if(t>99.9f) t=99.9f; int v=(int)(fabs(t)*10.0f+0.5f); int d1=v/100; int d2=(v/10)%10; int d3=v%10; clearFrame(); if(neg) drawMinus(0,3); if(t>=10.0f){ drawDigit(d1,4,1); drawDot(7,5); drawDigit(d2,8,1); } else { if(!neg) drawDigit(d2,0,1); drawDot(3,5); drawDigit(d3,4,1); drawDigit(0,8,1); } renderFrame(); }
void showValueNumber(float value){ showEcNumber(value); }

// =====================================================
// HELPERS
// =====================================================
bool mqtt_publish(const char* topic, const char* payload, bool retained){ return mqtt.publish(topic, payload, retained); }
String getStateTopic(){ return String("sensors/uno-r4-water-ec/")+hostname+"/state"; }
String getPhCommandTopic(){ return String(ph_calibration_topic); }
String getTankCommandTopic(){ return String(tank_calibration_topic); }
void publishMatrixState(){ mqtt.publish(matrix_enabled_state_topic, matrixEnabled ? "ON" : "OFF", true); mqtt.publish(matrix_mode_state_topic, matrixMode.c_str(), true); }
void clearMatrix(){ clearFrame(); renderFrame(); }

float readAnalogVoltageAvg(int pin,int samples,int delayMs,int &rawAvg){ long sum=0; for(int i=0;i<samples;i++){ sum+=analogRead(pin); delay(delayMs);} rawAvg=sum/samples; return (rawAvg/(float)ADC_MAX)*VREF; }
float readEcVoltage(int &rawAvg){ return readAnalogVoltageAvg(EC_SENSOR_PIN,30,10,rawAvg); }
float readPhVoltage(int &rawAvg){ return readAnalogVoltageAvg(PH_SENSOR_PIN,20,10,rawAvg); }
float readRainAnalog(int &rawAvg){ return readAnalogVoltageAvg(RAIN_SENSOR_ANALOG_PIN,10,5,rawAvg); }
float calculateRawEcMsCm(float voltage,float tempC){ float cc=1.0f+0.02f*(tempC-25.0f); float cv=voltage/cc; float ecUsCm=(133.42f*cv*cv*cv-255.86f*cv*cv+857.39f*cv); if(ecUsCm<0) ecUsCm=0; return ecUsCm/1000.0f; }
float readWaterTemperature(){ ds18b20.requestTemperatures(); float t=ds18b20.getTempCByIndex(0); if(t==DEVICE_DISCONNECTED_C||t<-50.0f||t>125.0f||t==85.0f) return lastValidTempC; lastValidTempC=t; return t; }
float calculatePhFrom3Point(float voltage){ if(phHasV7&&phHasV4&&phHasV10){ if(voltage>=phV7) return 7.0f + (voltage-phV7)*((7.0f-10.0f)/(phV7-phV10)); else return 7.0f + (voltage-phV7)*((7.0f-4.0f)/(phV7-phV4)); } if(phHasV7&&phHasV4) return 7.0f + (voltage-phV7)*((7.0f-4.0f)/(phV7-phV4)); if(phHasV7&&phHasV10) return 7.0f + (voltage-phV7)*((7.0f-10.0f)/(phV7-phV10)); return 7.0f + (2.50f-voltage)*3.50f; }
float calcPercent(float measuredCm,float fullCm,float emptyCm){ if(isnan(fullCm)||isnan(emptyCm)||fabs(emptyCm-fullCm)<0.0001f) return NAN; float pct=(emptyCm-measuredCm)*100.0f/(emptyCm-fullCm); if(pct<0)pct=0; if(pct>100)pct=100; return pct; }
bool measureDistance(int trigPin,int echoPin,float &distanceCm){ digitalWrite(trigPin,LOW); delayMicroseconds(2); digitalWrite(trigPin,HIGH); delayMicroseconds(10); digitalWrite(trigPin,LOW); unsigned long duration=pulseIn(echoPin,HIGH,30000UL); if(duration==0) return false; distanceCm=duration/58.0f; return true; }

// =====================================================
bool saveTankCalibration() {
  PersistData d;
  d.magic = EEPROM_MAGIC;
  d.roFullCm = roFullCm;
  d.roEmptyCm = roEmptyCm;
  d.roCapacityL = roCapacityL;
  d.mixFullCm = mixFullCm;
  d.mixEmptyCm = mixEmptyCm;
  d.mixCapacityL = mixCapacityL;
  d.drainFullCm = drainFullCm;
  d.drainEmptyCm = drainEmptyCm;
  d.drainCapacityMl = drainCapacityMl;
  d.phV7 = phV7; d.phV4 = phV4; d.phV10 = phV10;
  d.phHasV7 = phHasV7; d.phHasV4 = phHasV4; d.phHasV10 = phHasV10;
  EEPROM.put(0, d);
}

void loadTankCalibration() {
  PersistData d;
  EEPROM.get(0, d);
  if (d.magic == EEPROM_MAGIC) {
    roFullCm = d.roFullCm; roEmptyCm = d.roEmptyCm; roCapacityL = d.roCapacityL;
    mixFullCm = d.mixFullCm; mixEmptyCm = d.mixEmptyCm; mixCapacityL = d.mixCapacityL;
    drainFullCm = d.drainFullCm; drainEmptyCm = d.drainEmptyCm; drainCapacityMl = d.drainCapacityMl;
    phV7 = d.phV7; phV4 = d.phV4; phV10 = d.phV10;
    phHasV7 = d.phHasV7; phHasV4 = d.phHasV4; phHasV10 = d.phHasV10;
  }
}

void resetTankCalibrationDefaults() {
  roFullCm = NAN; roEmptyCm = NAN; roCapacityL = 120.0f;
  mixFullCm = NAN; mixEmptyCm = NAN; mixCapacityL = 120.0f;
  drainFullCm = NAN; drainEmptyCm = NAN; drainCapacityMl = 500.0f;
  saveTankCalibration();
}

// SERIAL CALIBRATION
// =====================================================
void printCalibrationMenu(){
  Serial.println();
  Serial.println("===== Calibration =====");
  Serial.println("pH: ph7 / ph4 / ph10 / show");
  Serial.println("Tanks: set-ro-tank-full / set-ro-tank-empty / set-ro-tank-volume / load / save");
  Serial.println("       set-mix-tank-full / set-mix-tank-empty / set-mix-tank-volume");
  Serial.println("       set-drain-tank-full / set-drain-tank-empty / set-drain-tank-volume");
  Serial.println("       rocap <L> / mixcap <L> / draincap <ml>");
  Serial.println("Control: matrix-on / matrix-off / matrix-ec / matrix-temp / matrix-ph / matrix-tank");
  Serial.println("Help: show / help / save / load");
  Serial.println("=======================");
  Serial.println();
}
void printCalibrationState(){
  Serial.println("--- Calibration State ---");
  Serial.print("phV4="); Serial.println(phV4,4);
  Serial.print("phV7="); Serial.println(phV7,4);
  Serial.print("phV10="); Serial.println(phV10,4);
  Serial.print("Ro full="); Serial.print(roFullCm,2); Serial.print(" empty="); Serial.print(roEmptyCm,2); Serial.print(" cap="); Serial.println(roCapacityL,1);
  Serial.print("Mix full="); Serial.print(mixFullCm,2); Serial.print(" empty="); Serial.print(mixEmptyCm,2); Serial.print(" cap="); Serial.println(mixCapacityL,1);
  Serial.print("Drain full="); Serial.print(drainFullCm,2); Serial.print(" empty="); Serial.print(drainEmptyCm,2); Serial.print(" cap="); Serial.println(drainCapacityMl,1);
  Serial.print("Matrix enabled="); Serial.println(matrixEnabled);
  Serial.print("Matrix mode="); Serial.println(matrixMode);
}
void handleSerialCalibration(){
  if(!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim(); cmd.toLowerCase();
  if(cmd=="help"){ printCalibrationMenu(); return; }
  if(cmd=="show"){ printCalibrationState(); return; }
  if(cmd=="save"){
    saveTankCalibration();
    Serial.println(F(">>> EEPROM SAVED <<<"));
    Serial.print(F("RO full cm  : ")); Serial.println(roFullCm,2);
    Serial.print(F("RO empty cm : ")); Serial.println(roEmptyCm,2);
    Serial.print(F("RO cap L    : ")); Serial.println(roCapacityL,2);
    Serial.print(F("Mix full cm : ")); Serial.println(mixFullCm,2);
    Serial.print(F("Mix empty cm: ")); Serial.println(mixEmptyCm,2);
    Serial.print(F("Mix cap L   : ")); Serial.println(mixCapacityL,2);
    Serial.print(F("Drain full  : ")); Serial.println(drainFullCm,2);
    Serial.print(F("Drain empty : ")); Serial.println(drainEmptyCm,2);
    Serial.print(F("Drain cap ml: ")); Serial.println(drainCapacityMl,2);
    Serial.print(F("pH V7       : ")); Serial.println(phV7,4);
    Serial.print(F("pH V4       : ")); Serial.println(phV4,4);
    Serial.print(F("pH V10      : ")); Serial.println(phV10,4);
    Serial.print(F("pH hasV7/4/10: ")); Serial.print(phHasV7); Serial.print(F("/")); Serial.print(phHasV4); Serial.print(F("/")); Serial.println(phHasV10);
    return;
  }
  if(cmd=="load"){
    loadTankCalibration();
    Serial.println(F(">>> EEPROM LOADED <<<"));
    Serial.print(F("RO full cm  : ")); Serial.println(roFullCm,2);
    Serial.print(F("RO empty cm : ")); Serial.println(roEmptyCm,2);
    Serial.print(F("RO cap L    : ")); Serial.println(roCapacityL,2);
    Serial.print(F("Mix full cm : ")); Serial.println(mixFullCm,2);
    Serial.print(F("Mix empty cm: ")); Serial.println(mixEmptyCm,2);
    Serial.print(F("Mix cap L   : ")); Serial.println(mixCapacityL,2);
    Serial.print(F("Drain full  : ")); Serial.println(drainFullCm,2);
    Serial.print(F("Drain empty : ")); Serial.println(drainEmptyCm,2);
    Serial.print(F("Drain cap ml: ")); Serial.println(drainCapacityMl,2);
    Serial.print(F("pH V7       : ")); Serial.println(phV7,4);
    Serial.print(F("pH V4       : ")); Serial.println(phV4,4);
    Serial.print(F("pH V10      : ")); Serial.println(phV10,4);
    Serial.print(F("pH hasV7/4/10: ")); Serial.print(phHasV7); Serial.print(F("/")); Serial.print(phHasV4); Serial.print(F("/")); Serial.println(phHasV10);
    return;
  }
  if(cmd=="ph7"){ int r=0; phV7=readPhVoltage(r); phHasV7=true; return; }
  if(cmd=="ph4"){ int r=0; phV4=readPhVoltage(r); phHasV4=true; return; }
  if(cmd=="ph10"){ int r=0; phV10=readPhVoltage(r); phHasV10=true; return; }
  if(cmd=="set-ro-tank-full"){ float d=0; if(measureDistance(RO_TRIG_PIN,RO_ECHO_PIN,d)) roFullCm=d; return; }
  if(cmd=="set-ro-tank-empty"){ float d=0; if(measureDistance(RO_TRIG_PIN,RO_ECHO_PIN,d)) roEmptyCm=d; return; }
  if(cmd=="set-ro-tank-volume"){ roCapacityL=120.0f; return; }
  if(cmd=="set-mix-tank-full"){ float d=0; if(measureDistance(MIX_TRIG_PIN,MIX_ECHO_PIN,d)) mixFullCm=d; return; }
  if(cmd=="set-mix-tank-empty"){ float d=0; if(measureDistance(MIX_TRIG_PIN,MIX_ECHO_PIN,d)) mixEmptyCm=d; return; }
  if(cmd=="set-mix-tank-volume"){ mixCapacityL=120.0f; return; }
  if(cmd=="set-drain-tank-full"){ float d=0; if(measureDistance(DRAIN_TRIG_PIN,DRAIN_ECHO_PIN,d)) drainFullCm=d; return; }
  if(cmd=="set-drain-tank-empty"){ float d=0; if(measureDistance(DRAIN_TRIG_PIN,DRAIN_ECHO_PIN,d)) drainEmptyCm=d; return; }
  if(cmd=="set-drain-tank-volume"){ drainCapacityMl=500.0f; return; }
  if(cmd.startsWith("rocap ")){ roCapacityL=max(0.1f, cmd.substring(6).toFloat()); return; }
  if(cmd.startsWith("mixcap ")){ mixCapacityL=max(0.1f, cmd.substring(7).toFloat()); return; }
  if(cmd.startsWith("draincap ")){ drainCapacityMl=max(1.0f, cmd.substring(9).toFloat()); return; }
  if(cmd=="matrix-on"){ matrixEnabled=true; publishMatrixState(); return; }
  if(cmd=="matrix-off"){ matrixEnabled=false; clearMatrix(); publishMatrixState(); return; }
  if(cmd=="matrix-ec"){ matrixEnabled=true; matrixMode="ec"; publishMatrixState(); return; }
  if(cmd=="matrix-temp"){ matrixEnabled=true; matrixMode="temp"; publishMatrixState(); return; }
  if(cmd=="matrix-ph"){ matrixEnabled=true; matrixMode="ph"; publishMatrixState(); return; }
  if(cmd=="matrix-tank"){ matrixEnabled=true; matrixMode="tank"; publishMatrixState(); return; }
  Serial.println("Unknown command.");
}

// =====================================================
// MQTT DISCOVERY
// =====================================================
void publishDiscoveryConfigs(){
  String state_topic=getStateTopic();
  String device_config="{\"identifiers\":[\""+String(hostname)+"\"],\"name\":\""+String(hostname)+"\",\"model\":\""+String(DEVICE_MODEL)+"\",\"manufacturer\":\""+String(DEVICE_MANUFACTURER)+"\"}";
  struct SensorConfig{ const char* name; const char* id; const char* device_class; const char* unit; const char* value_template; const char* icon; };
  SensorConfig sensors[] = {
    {"Water EC","ec_ms_cm","","mS/cm","{{ value_json.ec_ms_cm | float }}","mdi:flash"},
    {"Water EC Raw","ec_raw_ms_cm","","mS/cm","{{ value_json.ec_raw_ms_cm | float }}","mdi:flash-outline"},
    {"Water TDS","tds_ppm","","ppm","{{ value_json.tds_ppm | float }}","mdi:water-opacity"},
    {"Water pH","ph_value","ph","pH","{{ value_json.ph_value | float }}","mdi:test-tube"},
    {"Water pH Voltage","ph_voltage","","V","{{ value_json.ph_voltage | float }}","mdi:sine-wave"},
    {"Water Voltage","ec_voltage","","V","{{ value_json.ec_voltage | float }}","mdi:sine-wave"},
    {"Water Raw ADC","ec_raw","","RAW","{{ value_json.ec_raw | int }}","mdi:counter"},
    {"Water Temperature","water_temp_c","temperature","°C","{{ value_json.water_temp_c | float }}","mdi:thermometer"},
    {"Rain Analog","rain_analog_v","","V","{{ value_json.rain_analog_v | float }}","mdi:water"},
    {"Rain Raw","rain_analog_raw","","RAW","{{ value_json.rain_analog_raw | int }}","mdi:counter"},
    {"Rain Digital","rain_digital","","","{{ value_json.rain_digital }}","mdi:weather-rainy"},
    {"Ro Tank Distance","ro_distance_cm","distance","cm","{{ value_json.ro_distance_cm | float }}","mdi:waves"},
    {"Mix Tank Distance","mix_distance_cm","distance","cm","{{ value_json.mix_distance_cm | float }}","mdi:waves"},
    {"Drain Tank Distance","drain_distance_cm","distance","cm","{{ value_json.drain_distance_cm | float }}","mdi:waves"},
    {"Ro Tank %","ro_percent","","%","{{ value_json.ro_percent | float }}","mdi:water-percent"},
    {"Mix Tank %","mix_percent","","%","{{ value_json.mix_percent | float }}","mdi:water-percent"},
    {"Drain Tank %","drain_percent","","%","{{ value_json.drain_percent | float }}","mdi:water-percent"},
    {"Ro Tank","ro_liters","","L","{{ value_json.ro_liters | float }}","mdi:cup-water"},
    {"Mix Tank","mix_liters","","L","{{ value_json.mix_liters | float }}","mdi:cup-water"},
    {"Drain Tank","drain_ml","","ml","{{ value_json.drain_ml | float }}","mdi:cup-water"},
    {"WiFi RSSI","wifi_rssi","","dBm","{{ value_json.wifi_rssi | float }}","mdi:wifi"},
    {"Uptime","uptime_s","","s","{{ value_json.uptime_s | float }}","mdi:timer-outline"}
  };
  for(const auto& s2 : sensors){
    String topic=String(discovery_prefix)+"/sensor/"+hostname+"/"+s2.id+"/config";
    String cfg="{\"name\":\""+String(s2.name)+"\",\"unique_id\":\""+String(hostname)+"_"+s2.id+"\",\"object_id\":\""+String(hostname)+"_"+s2.id+"\",\"state_topic\":\""+state_topic+"\",\"value_template\":\""+String(s2.value_template)+"\",\"unit_of_measurement\":\""+String(s2.unit)+"\",\"availability_topic\":\""+String(availability_topic)+"\",\"payload_available\":\"online\",\"payload_not_available\":\"offline\",\"device\":"+device_config;
    if(strlen(s2.device_class)) cfg += ",\"device_class\":\""+String(s2.device_class)+"\"";
    if(strlen(s2.icon)) cfg += ",\"icon\":\""+String(s2.icon)+"\"";
    if(String(s2.id)=="ec_ms_cm" || String(s2.id)=="ec_raw_ms_cm" || String(s2.id)=="tds_ppm" || String(s2.id)=="ph_value" || String(s2.id)=="water_temp_c" || String(s2.id)=="rain_analog_v" || String(s2.id)=="ro_percent" || String(s2.id)=="mix_percent" || String(s2.id)=="drain_percent" || String(s2.id)=="ro_liters" || String(s2.id)=="mix_liters" || String(s2.id)=="drain_ml" || String(s2.id)=="wifi_rssi" || String(s2.id)=="uptime_s") cfg += ",\"state_class\":\"measurement\"";
    cfg += "}";
    mqtt.publish(topic.c_str(), cfg.c_str(), true);
    delay(20);
  }
  String binTopic=String(discovery_prefix)+"/binary_sensor/"+hostname+"/rain_d0/config";
  String binCfg="{\"name\":\"Rain Digital\",\"unique_id\":\""+String(hostname)+"_rain_d0\",\"object_id\":\""+String(hostname)+"_rain_d0\",\"state_topic\":\""+state_topic+"\",\"value_template\":\"{{ value_json.rain_digital }}\",\"payload_on\":\"true\",\"payload_off\":\"false\",\"availability_topic\":\""+String(availability_topic)+"\",\"payload_available\":\"online\",\"payload_not_available\":\"offline\",\"device\":"+device_config+"}";
  mqtt.publish(binTopic.c_str(), binCfg.c_str(), true);
  String switchTopic=String(discovery_prefix)+"/switch/"+hostname+"/matrix_enabled/config";
  String switchCfg="{\"name\":\"Matrix Enabled\",\"unique_id\":\""+String(hostname)+"_matrix_enabled\",\"object_id\":\""+String(hostname)+"_matrix_enabled\",\"command_topic\":\""+String(matrix_enabled_command_topic)+"\",\"state_topic\":\""+String(matrix_enabled_state_topic)+"\",\"payload_on\":\"ON\",\"payload_off\":\"OFF\",\"availability_topic\":\""+String(availability_topic)+"\",\"payload_available\":\"online\",\"payload_not_available\":\"offline\",\"device\":"+device_config+"}";
  mqtt.publish(switchTopic.c_str(), switchCfg.c_str(), true);
  String selectTopic=String(discovery_prefix)+"/select/"+hostname+"/matrix_mode/config";
  String selectCfg="{\"name\":\"Matrix Mode\",\"unique_id\":\""+String(hostname)+"_matrix_mode\",\"object_id\":\""+String(hostname)+"_matrix_mode\",\"command_topic\":\""+String(matrix_mode_command_topic)+"\",\"state_topic\":\""+String(matrix_mode_state_topic)+"\",\"options\":[\"ec\",\"temp\",\"ph\",\"tank\",\"off\"],\"availability_topic\":\""+String(availability_topic)+"\",\"payload_available\":\"online\",\"payload_not_available\":\"offline\",\"device\":"+device_config+"}";
  mqtt.publish(selectTopic.c_str(), selectCfg.c_str(), true);
  const char* tankNumNames[] = {"Ro Tank Volume","Mix Tank Volume","Drain Tank Volume"};
  const char* tankNumIds[] = {"ro_tank_volume","mix_tank_volume","drain_tank_volume"};
  const char* tankNumCmds[] = {"ro_tank_volume: {{ value }}","mix_tank_volume: {{ value }}","drain_tank_volume: {{ value }}"};
  const char* tankNumStateTemplates[] = {"{{ value_json.ro_tank_volume | float }}","{{ value_json.mix_tank_volume | float }}","{{ value_json.drain_tank_volume | float }}"};
  const float tankMin[] = {0.0f,0.0f,0.0f};
  const float tankMax[] = {1000.0f,1000.0f,10000.0f};
  const float tankStep[] = {1.0f,1.0f,1.0f};
  for(int i=0;i<3;i++){
    String topic=String(discovery_prefix)+"/number/"+hostname+"/"+tankNumIds[i]+"/config";
    String cfg="{\"name\":\""+String(tankNumNames[i])+"\",\"unique_id\":\""+String(hostname)+"_"+tankNumIds[i]+"\",\"object_id\":\""+String(hostname)+"_"+tankNumIds[i]+"\",\"command_topic\":\""+String(tank_calibration_topic)+"\",\"state_topic\":\""+getStateTopic()+"\",\"value_template\":\""+String(tankNumStateTemplates[i])+"\",\"command_template\":\""+String(tankNumCmds[i])+"\",\"min\":\""+String(tankMin[i])+"\",\"max\":\""+String(tankMax[i])+"\",\"step\":\""+String(tankStep[i])+"\",\"mode\":\"box\",\"availability_topic\":\""+String(availability_topic)+"\",\"payload_available\":\"online\",\"payload_not_available\":\"offline\",\"device\":"+device_config+"}";
    mqtt.publish(topic.c_str(), cfg.c_str(), true);
    delay(20);
  }
  const char* btnNames[] = {"Set Ro Tank Full","Set Ro Tank Empty","Set Ro Tank Volume","Set Mix Tank Full","Set Mix Tank Empty","Set Mix Tank Volume","Set Drain Tank Full","Set Drain Tank Empty","Set Drain Tank Volume"};
  const char* btnIds[] = {"set_ro_tank_full","set_ro_tank_empty","set_ro_tank_volume","set_mix_tank_full","set_mix_tank_empty","set_mix_tank_volume","set_drain_tank_full","set_drain_tank_empty","set_drain_tank_volume"};
  const char* btnCmds[] = {"set-ro-tank-full","set-ro-tank-empty","set-ro-tank-volume","set-mix-tank-full","set-mix-tank-empty","set-mix-tank-volume","set-drain-tank-full","set-drain-tank-empty","set-drain-tank-volume"};
  // EEPROM save/load buttons
  const char* eepromBtnNames[] = {"Save Calibration","Load Calibration"};
  const char* eepromBtnIds[]   = {"eeprom_save","eeprom_load"};
  const char* eepromBtnCmds[]  = {"save","load"};
  const char* eepromBtnIcons[] = {"mdi:content-save","mdi:restore"};
  for(int i=0;i<2;i++){
    String etopic=String(discovery_prefix)+"/button/"+hostname+"/"+eepromBtnIds[i]+"/config";
    String ecfg="{\"name\":\""+String(eepromBtnNames[i])+"\",\"unique_id\":\""+String(hostname)+"_"+eepromBtnIds[i]+"\",\"object_id\":\""+String(hostname)+"_"+eepromBtnIds[i]+"\",\"command_topic\":\""+String(tank_calibration_topic)+"\",\"payload_press\":\""+String(eepromBtnCmds[i])+"\",\"icon\":\""+String(eepromBtnIcons[i])+"\",\"availability_topic\":\""+String(availability_topic)+"\",\"payload_available\":\"online\",\"payload_not_available\":\"offline\",\"device\":"+device_config+"}";
    mqtt.publish(etopic.c_str(), ecfg.c_str(), true);
    delay(20);
  }
  for(int i=0;i<9;i++){
    String topic=String(discovery_prefix)+"/button/"+hostname+"/"+btnIds[i]+"/config";
    String cfg="{\"name\":\""+String(btnNames[i])+"\",\"unique_id\":\""+String(hostname)+"_"+btnIds[i]+"\",\"object_id\":\""+String(hostname)+"_"+btnIds[i]+"\",\"command_topic\":\""+String(tank_calibration_topic)+"\",\"payload_press\":\""+String(btnCmds[i])+"\",\"availability_topic\":\""+String(availability_topic)+"\",\"payload_available\":\"online\",\"payload_not_available\":\"offline\",\"device\":"+device_config+"}";
    mqtt.publish(topic.c_str(), cfg.c_str(), true);
    delay(20);
  }
}
// =====================================================
// MQTT / NETWORK
// =====================================================
void onMqttMessage(char* topic, byte* payload, unsigned int length){
  String t(topic), msg; for(unsigned int i=0;i<length;i++) msg += (char)payload[i]; msg.trim(); String low=msg; low.toLowerCase();
  if(t==getPhCommandTopic()){ if(low=="ph4"){ int r=0; phV4=readPhVoltage(r); phHasV4=true; } else if(low=="ph7"){ int r=0; phV7=readPhVoltage(r); phHasV7=true; } else if(low=="ph10"){ int r=0; phV10=readPhVoltage(r); phHasV10=true; } }
  else if(t==getTankCommandTopic()){
    if(low=="save"){ saveTankCalibration(); return; }
    if(low=="load"){ loadTankCalibration(); return; }
    if(low=="set-ro-tank-full"){ float d=0; if(measureDistance(RO_TRIG_PIN,RO_ECHO_PIN,d)) roFullCm=d; }
    else if(low=="set-ro-tank-empty"){ float d=0; if(measureDistance(RO_TRIG_PIN,RO_ECHO_PIN,d)) roEmptyCm=d; }
    else if(low=="set-ro-tank-volume"){ roCapacityL=120.0f; }
    else if(low.startsWith("ro_tank_volume:")){ roCapacityL=max(0.1f, low.substring(15).toFloat()); }
    else if(low=="set-mix-tank-full"){ float d=0; if(measureDistance(MIX_TRIG_PIN,MIX_ECHO_PIN,d)) mixFullCm=d; }
    else if(low=="set-mix-tank-empty"){ float d=0; if(measureDistance(MIX_TRIG_PIN,MIX_ECHO_PIN,d)) mixEmptyCm=d; }
    else if(low=="set-mix-tank-volume"){ mixCapacityL=120.0f; }
    else if(low.startsWith("mix_tank_volume:")){ mixCapacityL=max(0.1f, low.substring(16).toFloat()); }
    else if(low=="set-drain-tank-full"){ float d=0; if(measureDistance(DRAIN_TRIG_PIN,DRAIN_ECHO_PIN,d)) drainFullCm=d; }
    else if(low=="set-drain-tank-empty"){ float d=0; if(measureDistance(DRAIN_TRIG_PIN,DRAIN_ECHO_PIN,d)) drainEmptyCm=d; }
    else if(low=="set-drain-tank-volume"){ drainCapacityMl=500.0f; }
    else if(low.startsWith("drain_tank_volume:")){ drainCapacityMl=max(1.0f, low.substring(18).toFloat()); }
  } else if(t==matrix_enabled_command_topic){ matrixEnabled = (low=="on" || low=="1" || low=="true"); if(!matrixEnabled) clearMatrix(); publishMatrixState(); }
  else if(t==matrix_mode_command_topic){ if(low=="ec" || low=="temp" || low=="ph" || low=="tank" || low=="off") matrixMode=low; if(!matrixEnabled || matrixMode=="off") clearMatrix(); else { SensorData d=readSensor(); applyMatrixMode(d); } publishMatrixState(); }
}
void setupWiFi(){ WiFi.begin(ssid,password); while(WiFi.status()!=WL_CONNECTED) delay(500); }
void reconnectMQTT(){ while(!mqtt.connected()){ if(mqtt.connect(client_id,mqtt_username,mqtt_password,availability_topic,1,true,"offline")){ mqtt.publish(availability_topic,"online",true); publishDiscoveryConfigs(); publishMatrixState(); mqtt.subscribe(getPhCommandTopic().c_str()); mqtt.subscribe(getTankCommandTopic().c_str()); mqtt.subscribe(matrix_enabled_command_topic); mqtt.subscribe(matrix_mode_command_topic); } else delay(5000); } }

// =====================================================
// SENSOR READ / PUBLISH
// =====================================================
SensorData readSensor(){ SensorData data; data.ec_raw=0; data.ec_voltage=readEcVoltage(data.ec_raw); data.temp_c=readWaterTemperature(); data.ec_raw_ms_cm=calculateRawEcMsCm(data.ec_voltage,data.temp_c); data.ec_ms_cm=data.ec_raw_ms_cm*ecCalibrationFactor; data.tds_ppm=data.ec_ms_cm*TDS_FACTOR; int phRaw=0; data.ph_voltage=readPhVoltage(phRaw); data.ph_value=calculatePhFrom3Point(data.ph_voltage); data.rain_analog_raw=0; data.rain_analog_v=readRainAnalog(data.rain_analog_raw); data.rain_digital=(digitalRead(RAIN_SENSOR_DIGITAL_PIN)==HIGH); data.ro_distance_cm=roDistanceCm; data.mix_distance_cm=mixDistanceCm; data.drain_distance_cm=drainDistanceCm; data.ro_percent=roPercent; data.mix_percent=mixPercent; data.drain_percent=drainPercent; data.ro_liters=roLiters; data.mix_liters=mixLiters; data.drain_ml=drainMl; data.wifi_rssi=WiFi.RSSI(); data.uptime_s=millis()/1000UL; data.valid=true; return data; }
void updateLevelMeasurements(){ float d=NAN; if(measureDistance(RO_TRIG_PIN,RO_ECHO_PIN,d)){ roDistanceCm=d; roPercent=calcPercent(d,roFullCm,roEmptyCm); if(!isnan(roPercent)) roLiters=roCapacityL*roPercent/100.0f; } if(measureDistance(MIX_TRIG_PIN,MIX_ECHO_PIN,d)){ mixDistanceCm=d; mixPercent=calcPercent(d,mixFullCm,mixEmptyCm); if(!isnan(mixPercent)) mixLiters=mixCapacityL*mixPercent/100.0f; } if(measureDistance(DRAIN_TRIG_PIN,DRAIN_ECHO_PIN,d)){ drainDistanceCm=d; drainPercent=calcPercent(d,drainFullCm,drainEmptyCm); if(!isnan(drainPercent)) drainMl=drainCapacityMl*drainPercent/100.0f; } }
void applyMatrixMode(const SensorData& data){ if(!matrixEnabled || matrixMode=="off"){ clearMatrix(); return; } if(matrixMode=="temp") showTempNumber(data.temp_c); else if(matrixMode=="ph") showValueNumber(data.ph_value); else if(matrixMode=="tank") showValueNumber(data.ro_percent); else showEcNumber(data.ec_ms_cm); }
void publishData(const SensorData& data){ StaticJsonDocument<1400> doc; doc["ec_ms_cm"]=roundf(data.ec_ms_cm*1000.0f)/1000.0f; doc["ec_raw_ms_cm"]=roundf(data.ec_raw_ms_cm*1000.0f)/1000.0f; doc["tds_ppm"]=roundf(data.tds_ppm); doc["ec_voltage"]=roundf(data.ec_voltage*1000.0f)/1000.0f; doc["ec_raw"]=data.ec_raw; doc["water_temp_c"]=roundf(data.temp_c*10.0f)/10.0f; doc["ph_voltage"]=roundf(data.ph_voltage*1000.0f)/1000.0f; doc["ph_value"]=roundf(data.ph_value*100.0f)/100.0f; doc["rain_analog_v"]=roundf(data.rain_analog_v*1000.0f)/1000.0f; doc["rain_analog_raw"]=data.rain_analog_raw; doc["rain_digital"]=data.rain_digital; if(!isnan(data.ro_distance_cm)) doc["ro_distance_cm"]=roundf(data.ro_distance_cm*100.0f)/100.0f; if(!isnan(data.mix_distance_cm)) doc["mix_distance_cm"]=roundf(data.mix_distance_cm*100.0f)/100.0f; if(!isnan(data.drain_distance_cm)) doc["drain_distance_cm"]=roundf(data.drain_distance_cm*100.0f)/100.0f; if(!isnan(data.ro_percent)) doc["ro_percent"]=roundf(data.ro_percent*100.0f)/100.0f; if(!isnan(data.mix_percent)) doc["mix_percent"]=roundf(data.mix_percent*100.0f)/100.0f; if(!isnan(data.drain_percent)) doc["drain_percent"]=roundf(data.drain_percent*100.0f)/100.0f; if(!isnan(data.ro_liters)) doc["ro_liters"]=roundf(data.ro_liters*10.0f)/10.0f; if(!isnan(data.mix_liters)) doc["mix_liters"]=roundf(data.mix_liters*10.0f)/10.0f; if(!isnan(data.drain_ml)) doc["drain_ml"]=roundf(data.drain_ml); doc["ro_tank_volume"]=roCapacityL; doc["mix_tank_volume"]=mixCapacityL; doc["drain_tank_volume"]=drainCapacityMl; doc["matrix_enabled"]=matrixEnabled; doc["matrix_mode"]=matrixMode; doc["wifi_rssi"]=data.wifi_rssi; doc["uptime_s"]=data.uptime_s; char payload[1400]; serializeJson(doc,payload,sizeof(payload)); mqtt_publish(getStateTopic().c_str(),payload,true); }

// =====================================================
// SETUP / LOOP
// =====================================================
void setup(){
  Serial.begin(115200); delay(1000); analogReadResolution(14);
  pinMode(EC_SENSOR_PIN,INPUT); pinMode(PH_SENSOR_PIN,INPUT); pinMode(RAIN_SENSOR_ANALOG_PIN,INPUT); pinMode(RAIN_SENSOR_DIGITAL_PIN,INPUT_PULLUP);
  pinMode(RO_TRIG_PIN,OUTPUT); pinMode(RO_ECHO_PIN,INPUT); pinMode(MIX_TRIG_PIN,OUTPUT); pinMode(MIX_ECHO_PIN,INPUT); pinMode(DRAIN_TRIG_PIN,OUTPUT); pinMode(DRAIN_ECHO_PIN,INPUT);
  digitalWrite(RO_TRIG_PIN,LOW); digitalWrite(MIX_TRIG_PIN,LOW); digitalWrite(DRAIN_TRIG_PIN,LOW);
  ds18b20.begin(); matrix.begin(); clearMatrix(); showBootIcon();
  Serial.println("\nUNO R4 Water EC + pH + Rain + Tanks + Matrix v6.0.3 starting");
  printCalibrationMenu();
  float rawEcAtCalibration = calculateRawEcMsCm(1.820f, 20.0f); ecCalibrationFactor = 1.413f / rawEcAtCalibration;
  setupWiFi(); mqtt.setServer(mqtt_server,mqtt_port); mqtt.setBufferSize(2048); mqtt.setCallback(onMqttMessage); reconnectMQTT();
  updateLevelMeasurements(); publishData(readSensor()); publishMatrixState(); if(matrixEnabled && matrixMode!="off") applyMatrixMode(readSensor());
  lastPublishTime=millis(); lastMatrixRefreshTime=millis(); lastSerialStatusTime=millis();
}
void loop(){
  handleSerialCalibration(); if(!mqtt.connected()) reconnectMQTT(); mqtt.loop(); unsigned long now=millis();
  if(now-lastLevelMeasureTime>=LEVEL_MEASURE_INTERVAL){ updateLevelMeasurements(); lastLevelMeasureTime=now; }
  if(now-lastMatrixRefreshTime>=MATRIX_REFRESH_INTERVAL){ if(matrixEnabled && matrixMode!="off") applyMatrixMode(readSensor()); else clearMatrix(); lastMatrixRefreshTime=now; }
  if(now-lastPublishTime>=MQTT_PUBLISH_INTERVAL){ SensorData d=readSensor(); publishData(d); if(matrixEnabled && matrixMode!="off") applyMatrixMode(d); else clearMatrix(); lastPublishTime=now; }
  if(now-lastSerialStatusTime>=SERIAL_STATUS_INTERVAL){ lastSerialStatusTime=now; Serial.println("[STATUS] MQTT running."); }
}
