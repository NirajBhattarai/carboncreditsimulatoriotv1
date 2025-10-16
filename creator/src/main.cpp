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
  
  Serial.print("Attempting MQTT connection...");
  
  // Attempt to connect
  if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
    Serial.println(" connected");
    mqttConnected = true;
    
    // Subscribe to topics if needed
    String subscribeTopic = String(MQTT_TOPIC_PREFIX) + "/commands";
    mqttClient.subscribe(subscribeTopic.c_str());
    
    return true;
  } else {
    Serial.print(" failed, rc=");
    Serial.print(mqttClient.state());
    Serial.println(" try again in 5 seconds");
    mqttConnected = false;
    return false;
  }
}

/**
 * @brief Publish sensor data to MQTT (optimized)
 * @param co2 CO2 reading
 * @param humidity Humidity reading
 * @param credits Carbon credits
 * @param emissions Emissions value
 * @param offset Offset status
 */
void publishToMqtt(int co2, int humidity, float credits, float emissions, bool offset) {
  if (!mqttClient.connected()) {
    return;
  }
  
  // Create compact JSON payload
  char payload[128];
  snprintf(payload, sizeof(payload), 
    "{\"c\":%d,\"h\":%d,\"cr\":%.1f,\"e\":%.1f,\"o\":%s,\"t\":%lu}",
    co2, humidity, credits, emissions, offset ? "true" : "false", millis());
  
  // Publish to main topic
  char topic[32];
  snprintf(topic, sizeof(topic), "%s/sensor_data", MQTT_TOPIC_PREFIX);
  
  bool result = mqttClient.publish(topic, payload);
  
  if (result) {
    Serial.println("  ✅ Published to MQTT");
  } else {
    Serial.println("  ❌ MQTT publish failed");
  }
}

/**
 * @brief Generate random sensor data within realistic ranges
 */
void generateRandomSensorData() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastDataUpdate >= dataUpdateInterval) {
    lastDataUpdate = currentTime;
    
    // Generate random CO2 reading (300-2000 ppm)
    co2Reading = random(CO2_MIN, CO2_MAX + 1);
    
    // Generate random humidity reading (20-80%)
    humidityReading = random(HUMIDITY_MIN, HUMIDITY_MAX + 1);
    
    // Calculate carbon credits and emissions
    carbonCredits = co2Reading * 0.5;
    emissions = humidityReading * 0.2;
    offset = (carbonCredits >= emissions);
    
    Serial.printf("🔄 Generated new data - CO2:%d Hum:%d Credits:%.1f Offset:%s\n",
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
  display.println("Carbon Credit Monitor");
  
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
  connectToMqtt();

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
  display.println("Carbon Credit");
  display.setCursor(0, 15);
  display.println("Monitor");
  display.setCursor(0, 35);
  display.println("Initializing...");
  display.display();
  delay(2000);
  
  // Initialize random seed
  randomSeed(analogRead(0));
  
  Serial.println("✅ Setup Complete!");
}

void loop() {
  // Handle MQTT connection
  if (!mqttClient.connected()) {
    unsigned long currentTime = millis();
    if (currentTime - lastMqttAttempt >= mqttRetryInterval) {
      lastMqttAttempt = currentTime;
      connectToMqtt();
    }
  } else {
    mqttClient.loop();
  }

  // Generate random sensor data
  generateRandomSensorData();
  
  // Update OLED display
  updateOLEDDisplay();

  // Publish to MQTT
  publishToMqtt(co2Reading, humidityReading, carbonCredits, emissions, offset);

  delay(1000); // Faster update for better display experience
}