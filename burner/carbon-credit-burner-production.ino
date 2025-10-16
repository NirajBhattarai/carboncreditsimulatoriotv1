/**
 * Carbon Credit Burner IoT Device - Production Version
 * Compatible with ESP32 and real sensors
 * 
 * This device monitors CO2 emission activities and automatically
 * triggers carbon credit burning when thresholds are reached.
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// WiFi Configuration
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Backend API Configuration
const char* apiBaseUrl = "http://your-backend-url:3001";
const char* deviceId = "CARBON_BURNER_001";
const char* deviceType = "BURNER";
const char* location = "Manufacturing Plant Beta";
const char* projectName = "Industrial Process Monitoring";

// Sensor Pins
const int CO2_SENSOR_PIN = A0;        // MQ135 CO2 sensor
const int ENERGY_SENSOR_PIN = A1;     // Energy consumption sensor
const int TEMP_HUMIDITY_PIN = 2;      // DHT22 sensor
const int ACTIVITY_LED_PIN = 4;       // Red LED for activity
const int CREDIT_LED_PIN = 5;         // Yellow LED for credit burning
const int STATUS_LED_PIN = 6;         // Status LED
const int BUZZER_PIN = 7;             // Buzzer for alerts

// Sensor Objects
DHT dht(TEMP_HUMIDITY_PIN, DHT22);

// Device Configuration
const float CO2_THRESHOLD = 1000.0;      // CO2 emission threshold
const float ENERGY_THRESHOLD = 500.0;    // Energy consumption threshold
const unsigned long DATA_INTERVAL = 30000; // Send data every 30 seconds
const unsigned long STATUS_INTERVAL = 5000; // Status check every 5 seconds

// Global Variables
unsigned long lastDataTransmission = 0;
unsigned long lastStatusCheck = 0;
float accumulatedCO2 = 0.0;
float accumulatedEnergy = 0.0;
unsigned long lastReset = 0;
bool thresholdReached = false;
bool deviceRegistered = false;
int connectionAttempts = 0;
const int MAX_CONNECTION_ATTEMPTS = 5;

// Device Status
struct DeviceStatus {
  bool isConnected;
  bool isActive;
  float currentCO2;
  float currentEnergy;
  float temperature;
  float humidity;
  unsigned long uptime;
  int dataPointsSent;
  String lastError;
};

DeviceStatus deviceStatus = {
  false, false, 0.0, 0.0, 0.0, 0.0, 0, 0, ""
};

void setup() {
  Serial.begin(115200);
  
  // Initialize pins
  pinMode(ACTIVITY_LED_PIN, OUTPUT);
  pinMode(CREDIT_LED_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(CO2_SENSOR_PIN, INPUT);
  pinMode(ENERGY_SENSOR_PIN, INPUT);
  
  // Initialize sensors
  dht.begin();
  
  // Initialize status
  deviceStatus.uptime = millis();
  lastReset = millis();
  
  Serial.println("=== Carbon Credit Burner Device Starting ===");
  Serial.println("Device ID: " + String(deviceId));
  Serial.println("Location: " + String(location));
  Serial.println("Project: " + String(projectName));
  Serial.println("=============================================");
  
  // Connect to WiFi
  connectToWiFi();
  
  // Register device with backend
  registerDevice();
  
  // Initial status LED pattern
  blinkStatusLED(3, 200);
}

void loop() {
  unsigned long currentTime = millis();
  
  // Update device status
  deviceStatus.uptime = currentTime;
  
  // Read sensor data
  readSensorData();
  
  // Process accumulated data
  processAccumulatedData();
  
  // Check thresholds
  checkThresholds();
  
  // Send data to backend
  if (currentTime - lastDataTransmission >= DATA_INTERVAL) {
    sendDataToBackend();
    lastDataTransmission = currentTime;
  }
  
  // Status check and health monitoring
  if (currentTime - lastStatusCheck >= STATUS_INTERVAL) {
    performStatusCheck();
    lastStatusCheck = currentTime;
  }
  
  // Handle LED indicators
  updateLEDs();
  
  // Handle buzzer alerts
  handleBuzzerAlerts();
  
  delay(1000); // Wait 1 second before next iteration
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  
  Serial.print("Connecting to WiFi");
  int attempts = 0;
  
  while (WiFi.status() != WL_CONNECTED && attempts < MAX_CONNECTION_ATTEMPTS) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connected!");
    Serial.println("IP address: " + WiFi.localIP().toString());
    deviceStatus.isConnected = true;
    blinkStatusLED(2, 100);
  } else {
    Serial.println();
    Serial.println("WiFi connection failed!");
    deviceStatus.isConnected = false;
    deviceStatus.lastError = "WiFi connection failed";
    blinkStatusLED(5, 100);
  }
}

void registerDevice() {
  if (!deviceStatus.isConnected) {
    Serial.println("Cannot register device - no WiFi connection");
    return;
  }
  
  HTTPClient http;
  http.begin(String(apiBaseUrl) + "/iot/devices");
  http.addHeader("Content-Type", "application/json");
  
  // Create device registration payload
  DynamicJsonDocument doc(1024);
  doc["deviceId"] = deviceId;
  doc["deviceType"] = deviceType;
  doc["location"] = location;
  doc["projectName"] = projectName;
  doc["description"] = "ESP32 Carbon Credit Burner Device";
  
  String payload;
  serializeJson(doc, payload);
  
  int httpResponseCode = http.POST(payload);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Device registration response: " + response);
    
    if (httpResponseCode == 201) {
      deviceRegistered = true;
      Serial.println("Device registered successfully!");
      blinkStatusLED(1, 500);
    } else {
      deviceStatus.lastError = "Device registration failed: " + String(httpResponseCode);
    }
  } else {
    deviceStatus.lastError = "Device registration error: " + String(httpResponseCode);
    Serial.println("Device registration error: " + String(httpResponseCode));
  }
  
  http.end();
}

void readSensorData() {
  // Read CO2 sensor (simulated - replace with actual sensor reading)
  int co2Raw = analogRead(CO2_SENSOR_PIN);
  deviceStatus.currentCO2 = map(co2Raw, 0, 4095, 0, 2000); // Map to 0-2000 ppm
  
  // Read energy consumption sensor (simulated)
  int energyRaw = analogRead(ENERGY_SENSOR_PIN);
  deviceStatus.currentEnergy = map(energyRaw, 0, 4095, 0, 1000); // Map to 0-1000 kWh
  
  // Read temperature and humidity
  deviceStatus.temperature = dht.readTemperature();
  deviceStatus.humidity = dht.readHumidity();
  
  // Validate sensor readings
  if (isnan(deviceStatus.temperature) || isnan(deviceStatus.humidity)) {
    deviceStatus.temperature = 25.0; // Default values
    deviceStatus.humidity = 50.0;
  }
  
  // Update accumulated values
  accumulatedCO2 += deviceStatus.currentCO2;
  accumulatedEnergy += deviceStatus.currentEnergy;
}

void processAccumulatedData() {
  unsigned long currentTime = millis();
  
  // Reset accumulated data every hour
  if (currentTime - lastReset >= 3600000) { // 1 hour = 3600000 ms
    accumulatedCO2 = 0.0;
    accumulatedEnergy = 0.0;
    thresholdReached = false;
    lastReset = currentTime;
    
    Serial.println("Accumulated data reset");
  }
}

void checkThresholds() {
  if (!thresholdReached && 
      accumulatedCO2 >= CO2_THRESHOLD && 
      accumulatedEnergy >= ENERGY_THRESHOLD) {
    
    thresholdReached = true;
    deviceStatus.isActive = true;
    
    Serial.println("=== THRESHOLD REACHED ===");
    Serial.println("CO2 Emissions: " + String(accumulatedCO2));
    Serial.println("Energy Consumed: " + String(accumulatedEnergy));
    Serial.println("========================");
    
    // Trigger credit burning
    triggerCreditBurning();
    
    // Alert with buzzer
    playAlertTone();
  }
}

void triggerCreditBurning() {
  if (!deviceStatus.isConnected || !deviceRegistered) {
    Serial.println("Cannot trigger burning - device not properly connected");
    return;
  }
  
  HTTPClient http;
  http.begin(String(apiBaseUrl) + "/iot/data");
  http.addHeader("Content-Type", "application/json");
  
  // Create data payload
  DynamicJsonDocument doc(1024);
  doc["deviceId"] = deviceId;
  doc["timestamp"] = millis();
  doc["co2Value"] = accumulatedCO2;
  doc["energyValue"] = accumulatedEnergy;
  doc["temperature"] = deviceStatus.temperature;
  doc["humidity"] = deviceStatus.humidity;
  doc["deviceType"] = deviceType;
  doc["location"] = location;
  doc["projectName"] = projectName;
  
  String payload;
  serializeJson(doc, payload);
  
  int httpResponseCode = http.POST(payload);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Burning trigger response: " + response);
    
    if (httpResponseCode == 201) {
      Serial.println("Credit burning triggered successfully!");
      blinkCreditLED(5, 200);
    } else {
      deviceStatus.lastError = "Burning trigger failed: " + String(httpResponseCode);
    }
  } else {
    deviceStatus.lastError = "Burning trigger error: " + String(httpResponseCode);
    Serial.println("Burning trigger error: " + String(httpResponseCode));
  }
  
  http.end();
}

void sendDataToBackend() {
  if (!deviceStatus.isConnected || !deviceRegistered) {
    return;
  }
  
  HTTPClient http;
  http.begin(String(apiBaseUrl) + "/iot/data");
  http.addHeader("Content-Type", "application/json");
  
  // Create data payload
  DynamicJsonDocument doc(1024);
  doc["deviceId"] = deviceId;
  doc["timestamp"] = millis();
  doc["co2Value"] = deviceStatus.currentCO2;
  doc["energyValue"] = deviceStatus.currentEnergy;
  doc["temperature"] = deviceStatus.temperature;
  doc["humidity"] = deviceStatus.humidity;
  doc["deviceType"] = deviceType;
  doc["location"] = location;
  doc["projectName"] = projectName;
  
  String payload;
  serializeJson(doc, payload);
  
  int httpResponseCode = http.POST(payload);
  
  if (httpResponseCode > 0) {
    deviceStatus.dataPointsSent++;
    Serial.println("Data sent successfully. Total points: " + String(deviceStatus.dataPointsSent));
  } else {
    deviceStatus.lastError = "Data transmission error: " + String(httpResponseCode);
    Serial.println("Data transmission error: " + String(httpResponseCode));
  }
  
  http.end();
}

void performStatusCheck() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    deviceStatus.isConnected = false;
    Serial.println("WiFi connection lost, attempting reconnection...");
    connectToWiFi();
  } else {
    deviceStatus.isConnected = true;
  }
  
  // Print status information
  Serial.println("=== Device Status ===");
  Serial.println("Uptime: " + String(deviceStatus.uptime / 1000) + " seconds");
  Serial.println("Connected: " + String(deviceStatus.isConnected ? "Yes" : "No"));
  Serial.println("Registered: " + String(deviceRegistered ? "Yes" : "No"));
  Serial.println("Active: " + String(deviceStatus.isActive ? "Yes" : "No"));
  Serial.println("CO2: " + String(deviceStatus.currentCO2) + " ppm");
  Serial.println("Energy: " + String(deviceStatus.currentEnergy) + " kWh");
  Serial.println("Temp: " + String(deviceStatus.temperature) + "°C");
  Serial.println("Humidity: " + String(deviceStatus.humidity) + "%");
  Serial.println("Accumulated CO2: " + String(accumulatedCO2));
  Serial.println("Accumulated Energy: " + String(accumulatedEnergy));
  Serial.println("Threshold Reached: " + String(thresholdReached ? "Yes" : "No"));
  Serial.println("Data Points Sent: " + String(deviceStatus.dataPointsSent));
  if (deviceStatus.lastError.length() > 0) {
    Serial.println("Last Error: " + deviceStatus.lastError);
  }
  Serial.println("===================");
}

void updateLEDs() {
  // Status LED - blink based on connection status
  if (deviceStatus.isConnected) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(50);
    digitalWrite(STATUS_LED_PIN, LOW);
  } else {
    digitalWrite(STATUS_LED_PIN, LOW);
  }
  
  // Activity LED - solid when active
  digitalWrite(ACTIVITY_LED_PIN, deviceStatus.isActive ? HIGH : LOW);
  
  // Credit LED - blink when threshold reached
  if (thresholdReached) {
    digitalWrite(CREDIT_LED_PIN, HIGH);
    delay(100);
    digitalWrite(CREDIT_LED_PIN, LOW);
  } else {
    digitalWrite(CREDIT_LED_PIN, LOW);
  }
}

void handleBuzzerAlerts() {
  if (thresholdReached) {
    playAlertTone();
  }
}

void playAlertTone() {
  // Play warning tone for threshold reached
  tone(BUZZER_PIN, 800, 200);
  delay(300);
  tone(BUZZER_PIN, 600, 200);
  delay(300);
  tone(BUZZER_PIN, 400, 200);
}

void blinkStatusLED(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(delayMs);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(delayMs);
  }
}

void blinkCreditLED(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(CREDIT_LED_PIN, HIGH);
    delay(delayMs);
    digitalWrite(CREDIT_LED_PIN, LOW);
    delay(delayMs);
  }
}
