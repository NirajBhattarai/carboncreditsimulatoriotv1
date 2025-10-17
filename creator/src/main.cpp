#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "secrets.h"

// OLED settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// MQTT client
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Sensor pins
#define CO2_PIN 34
#define HUMIDITY_PIN 35

// Sensor data variables
int co2Reading = 0;
int humidityReading = 0;
float carbonCredits = 0;
float emissions = 0;
bool offset = false;

// Random data generation
unsigned long lastDataUpdate = 0;
const unsigned long dataUpdateInterval = 2000; // 2 seconds

// MQTT transmission timing
unsigned long lastMqttPublish = 0;
const unsigned long mqttPublishInterval = 15000; // 15 seconds aggregated data
unsigned long lastHeartbeat = 0;
const unsigned long heartbeatInterval = 300000; // 5 minutes heartbeat
unsigned long lastCriticalAlert = 0;
const unsigned long criticalAlertCooldown = 30000; // 30 seconds cooldown

// Data aggregation arrays
int co2Readings[15]; // Store 15 readings (30 seconds worth)
int humidityReadings[15];
int readingIndex = 0;
int readingsCount = 0;

// Critical thresholds
const int CRITICAL_CO2_THRESHOLD = 1800; // High CO2 level for sequester
const float CRITICAL_CREDITS_THRESHOLD = 2.0; // Critical low credits

// MQTT connection status
bool mqttConnected = false;
unsigned long lastMqttAttempt = 0;
const unsigned long mqttRetryInterval = 5000; // 5 seconds

// Sensor data ranges
const int CO2_MIN = 300;    // Normal outdoor CO2 level
const int CO2_MAX = 2000;   // High indoor CO2 level
const int HUMIDITY_MIN = 20; // Dry environment
const int HUMIDITY_MAX = 80; // Humid environment

/**
 * @brief Callback function for MQTT messages
 * @param topic The topic the message was received on
 * @param payload The message payload
 * @param length The length of the payload
 */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);
}

/**
 * @brief Connect to MQTT broker
 * @return true if connection successful, false otherwise
 */
bool connectToMqtt() {
  if (mqttClient.connected()) {
    return true;
  }
  
  Serial.printf("Attempting MQTT connection to %s:%d...", MQTT_SERVER, MQTT_PORT);
  
  // Set keep alive and timeout
  mqttClient.setKeepAlive(60);
  
  // Attempt to connect with will message
  if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
    Serial.println(" ✅ CONNECTED");
    mqttConnected = true;
    
    // Subscribe to topics with API key
    char subscribeTopic[100];
    snprintf(subscribeTopic, sizeof(subscribeTopic), "%s/%s/commands", MQTT_TOPIC_PREFIX, API_KEY);
    mqttClient.subscribe(subscribeTopic);
    Serial.printf("📡 Subscribed to: %s\n", subscribeTopic);
    
    return true;
  } else {
    Serial.printf(" ❌ FAILED, rc=%d\n", mqttClient.state());
    mqttConnected = false;
    
    // Print detailed error information
    switch (mqttClient.state()) {
      case -4: Serial.println("  Error: Connection timeout"); break;
      case -3: Serial.println("  Error: Connection lost"); break;
      case -2: Serial.println("  Error: Connect failed"); break;
      case -1: Serial.println("  Error: Disconnected"); break;
      case 1: Serial.println("  Error: Bad protocol"); break;
      case 2: Serial.println("  Error: Bad client ID"); break;
      case 3: Serial.println("  Error: Unavailable"); break;
      case 4: Serial.println("  Error: Bad credentials"); break;
      case 5: Serial.println("  Error: Unauthorized"); break;
      default: Serial.printf("  Error: Unknown state %d\n", mqttClient.state()); break;
    }
    
    return false;
  }
}

/**
 * @brief Publish aggregated sensor data to MQTT (optimized for carbon monitoring)
 * @param co2 CO2 reading
 * @param humidity Humidity reading
 * @param credits Carbon credits
 * @param emissions Emissions value
 * @param offset Offset status
 */
void publishAggregatedDataToMqtt() {
  // Double-check MQTT connection status
  if (!mqttClient.connected() || !mqttConnected) {
    Serial.printf("❌ MQTT not connected - skipping publish (Client: %s, Status: %s)\n", 
                  mqttClient.connected() ? "connected" : "disconnected",
                  mqttConnected ? "true" : "false");
    return;
  }
  
  if (readingsCount == 0) {
    Serial.println("❌ No readings to publish");
    return;
  }
  
  // Calculate aggregated statistics
  float avgCO2 = 0, avgHumidity = 0;
  int maxCO2 = 0, minCO2 = 9999;
  int maxHumidity = 0, minHumidity = 9999;
  
  for (int i = 0; i < readingsCount; i++) {
    avgCO2 += co2Readings[i];
    avgHumidity += humidityReadings[i];
    maxCO2 = max(maxCO2, co2Readings[i]);
    minCO2 = min(minCO2, co2Readings[i]);
    maxHumidity = max(maxHumidity, humidityReadings[i]);
    minHumidity = min(minHumidity, humidityReadings[i]);
  }
  
  avgCO2 /= readingsCount;
  avgHumidity /= readingsCount;
  
  // Get IP and MAC address
  IPAddress ip = WiFi.localIP();
  String macAddress = WiFi.macAddress();
  
  // Create comprehensive JSON payload with larger buffer
  char payload[600];
  int payloadLen = snprintf(payload, sizeof(payload), 
    "{\"ip\":\"%d.%d.%d.%d\",\"mac\":\"%s\",\"avg_c\":%.1f,\"max_c\":%d,\"min_c\":%d,\"avg_h\":%.1f,\"max_h\":%d,\"min_h\":%d,\"cr\":%.1f,\"e\":%.1f,\"o\":%s,\"t\":%lu,\"type\":\"sequester\",\"samples\":%d}",
    ip[0], ip[1], ip[2], ip[3], macAddress.c_str(), 
    avgCO2, maxCO2, minCO2, avgHumidity, maxHumidity, minHumidity,
    carbonCredits, emissions, offset ? "true" : "false", millis(), readingsCount);
  
  // Check if payload was truncated
  if (payloadLen >= sizeof(payload) - 1) {
    Serial.println("❌ Payload too large - truncated");
    return;
  }
  
  // Publish to topic with API key
  char topic[100];
  snprintf(topic, sizeof(topic), "%s/%s/sensor_data", MQTT_TOPIC_PREFIX, API_KEY);
  
  Serial.printf("📤 Publishing to topic: %s\n", topic);
  Serial.printf("📤 Payload length: %d\n", payloadLen);
  
  bool result = mqttClient.publish(topic, payload);
  
  if (result) {
    Serial.printf("📊 Published aggregated data to MQTT topic: %s (samples: %d)\n", topic, readingsCount);
    readingsCount = 0; // Reset for next aggregation
  } else {
    Serial.printf("❌ MQTT aggregated publish failed - State: %d\n", mqttClient.state());
  }
}

/**
 * @brief Send critical alert for dangerous conditions
 */
void sendCriticalAlert(const char* alertType, const char* message) {
  if (!mqttClient.connected()) {
    Serial.println("❌ MQTT not connected - cannot send alert");
    return;
  }
  
  // Get IP and MAC address
  IPAddress ip = WiFi.localIP();
  String macAddress = WiFi.macAddress();
  
  char payload[500];
  int payloadLen = snprintf(payload, sizeof(payload), 
    "{\"ip\":\"%d.%d.%d.%d\",\"mac\":\"%s\",\"alert_type\":\"%s\",\"message\":\"%s\",\"co2\":%d,\"credits\":%.1f,\"t\":%lu,\"type\":\"alert\"}",
    ip[0], ip[1], ip[2], ip[3], macAddress.c_str(), 
    alertType, message, co2Reading, carbonCredits, millis());
  
  // Check if payload was truncated
  if (payloadLen >= sizeof(payload) - 1) {
    Serial.println("❌ Alert payload too large - truncated");
    return;
  }
  
  char topic[100];
  snprintf(topic, sizeof(topic), "%s/%s/alerts", MQTT_TOPIC_PREFIX, API_KEY);
  
  Serial.printf("🚨 Sending critical alert to topic: %s\n", topic);
  
  bool result = mqttClient.publish(topic, payload);
  
  if (result) {
    Serial.printf("🚨 CRITICAL ALERT sent: %s - %s\n", alertType, message);
  } else {
    Serial.printf("❌ Critical alert publish failed - State: %d\n", mqttClient.state());
  }
}

/**
 * @brief Send heartbeat status every 5 minutes
 */
void sendHeartbeat() {
  if (!mqttClient.connected()) {
    Serial.println("❌ MQTT not connected - cannot send heartbeat");
    return;
  }
  
  // Get IP and MAC address
  IPAddress ip = WiFi.localIP();
  String macAddress = WiFi.macAddress();
  
  char payload[400];
  int payloadLen = snprintf(payload, sizeof(payload), 
    "{\"ip\":\"%d.%d.%d.%d\",\"mac\":\"%s\",\"status\":\"online\",\"uptime\":%lu,\"rssi\":%d,\"t\":%lu,\"type\":\"heartbeat\"}",
    ip[0], ip[1], ip[2], ip[3], macAddress.c_str(), 
    millis(), WiFi.RSSI(), millis());
  
  // Check if payload was truncated
  if (payloadLen >= sizeof(payload) - 1) {
    Serial.println("❌ Heartbeat payload too large - truncated");
    return;
  }
  
  char topic[100];
  snprintf(topic, sizeof(topic), "%s/%s/heartbeat", MQTT_TOPIC_PREFIX, API_KEY);
  
  Serial.printf("💓 Sending heartbeat to topic: %s\n", topic);
  
  bool result = mqttClient.publish(topic, payload);
  
  if (result) {
    Serial.println("💓 Heartbeat sent successfully");
  } else {
    Serial.printf("❌ Heartbeat publish failed - State: %d\n", mqttClient.state());
  }
}

/**
 * @brief Generate carbon sequestration sensor data and store for aggregation
 */
void generateCarbonSequestrationData() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastDataUpdate >= dataUpdateInterval) {
    lastDataUpdate = currentTime;
    
    // Generate CO2 reading (300-2000 ppm) - sequestering carbon
    co2Reading = random(CO2_MIN, CO2_MAX + 1);
    
    // Generate humidity reading (20-80%)
    humidityReading = random(HUMIDITY_MIN, HUMIDITY_MAX + 1);
    
    // Store readings for aggregation
    co2Readings[readingIndex] = co2Reading;
    humidityReadings[readingIndex] = humidityReading;
    readingIndex = (readingIndex + 1) % 15;
    if (readingsCount < 15) readingsCount++;
    
    // Calculate carbon credits generated and emissions offset
    carbonCredits = co2Reading * 0.5;  // Credits generated from sequestration
    emissions = humidityReading * 0.2; // Emissions offset
    offset = (carbonCredits >= emissions);
    
    Serial.printf("🌱 CARBON SEQUESTRATION - CO2:%d Hum:%d Credits Generated:%.1f Offset:%s\n",
                  co2Reading, humidityReading, carbonCredits,
                  offset ? "YES" : "NO");
  }
}

/**
 * @brief Update OLED display with current sensor data
 */
void updateOLEDDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Title
  display.setCursor(0, 0);
  display.println("Carbon Sequester");
  
  // CO2 reading
  display.setCursor(0, 12);
  display.print("CO2: ");
  display.print(co2Reading);
  display.println(" ppm");
  
  // Humidity reading
  display.setCursor(0, 24);
  display.print("Humidity: ");
  display.print(humidityReading);
  display.println("%");
  
  // Carbon credits
  display.setCursor(0, 36);
  display.print("Credits: ");
  display.print(carbonCredits, 1);
  
  // Offset status
  display.setCursor(0, 48);
  display.print("Offset: ");
  display.println(offset ? "YES" : "NO");
  
  // MQTT status
  display.setCursor(0, 56);
  display.print("MQTT: ");
  display.println(mqttConnected ? "OK" : "ERR");
  
  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // WiFi with Google DNS
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  
  // CRITICAL: Set Google DNS to fix DNS resolution
  WiFi.config(WiFi.localIP(), WiFi.gatewayIP(), WiFi.subnetMask(), 
              IPAddress(8, 8, 8, 8), IPAddress(8, 8, 4, 4));
  
  Serial.println("\n✅ WiFi Connected!");
  Serial.print("IP: "); Serial.println(WiFi.localIP());
  Serial.print("DNS: "); Serial.println(WiFi.dnsIP());

  // MQTT setup
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(1024); // Increase buffer size for larger payloads
  
  // Test MQTT connection
  Serial.println("🔌 Testing MQTT connection...");
  if (connectToMqtt()) {
    Serial.println("✅ MQTT connection test successful");
  } else {
    Serial.println("❌ MQTT connection test failed - will retry in loop");
  }

  // OLED setup
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED failed");
    for (;;);
  }
  
  // Initialize display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Carbon Sequester");
  display.setCursor(0, 15);
  display.println("Carbon Capture");
  display.setCursor(0, 35);
  display.println("Initializing...");
  display.display();
  delay(2000);
  
  // Initialize random seed
  randomSeed(analogRead(0));
  
  Serial.println("✅ Carbon Sequester Setup Complete!");
  Serial.println("🌱 CARBON SEQUESTRATION MODE ACTIVATED");
}

void loop() {
  // Handle MQTT connection with better debugging
  if (!mqttClient.connected()) {
    mqttConnected = false;
    unsigned long currentTime = millis();
    if (currentTime - lastMqttAttempt >= mqttRetryInterval) {
      lastMqttAttempt = currentTime;
      Serial.printf("🔄 Attempting MQTT reconnection... (State: %d)\n", mqttClient.state());
      connectToMqtt();
    }
  } else {
    mqttClient.loop();
    // Update connection status
    if (!mqttConnected) {
      mqttConnected = true;
      Serial.println("✅ MQTT connection restored");
    }
  }

  // Generate carbon sequestration data
  generateCarbonSequestrationData();
  
  // Update OLED display
  updateOLEDDisplay();

  // Hybrid MQTT transmission system
  unsigned long currentTime = millis();
  
  // 1. Send aggregated data every 15 seconds
  if (currentTime - lastMqttPublish >= mqttPublishInterval) {
    publishAggregatedDataToMqtt();
    lastMqttPublish = currentTime;
  }
  
  // 2. Send critical alerts immediately (with cooldown)
  if (currentTime - lastCriticalAlert >= criticalAlertCooldown) {
    if (co2Reading > CRITICAL_CO2_THRESHOLD) {
      sendCriticalAlert("HIGH_CO2", "High CO2 levels detected - sequestration needed!");
      lastCriticalAlert = currentTime;
    } else if (carbonCredits < CRITICAL_CREDITS_THRESHOLD) {
      sendCriticalAlert("LOW_CREDITS", "Low carbon credit generation!");
      lastCriticalAlert = currentTime;
    }
  }
  
  // 3. Send heartbeat every 5 minutes
  if (currentTime - lastHeartbeat >= heartbeatInterval) {
    sendHeartbeat();
    lastHeartbeat = currentTime;
  }

  delay(1000); // Faster update for better display experience
}