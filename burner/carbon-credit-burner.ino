/**
 * Carbon Credit Burner IoT Simulation
 * Compatible with Wokwi Arduino Simulator
 * 
 * This simulation represents a device that monitors CO2 consumption activities
 * such as industrial processes, transportation, or energy consumption.
 * It burns carbon credits to offset CO2 emissions.
 */

#include <WiFi.h>
#include <HTTPClient.h>

// WiFi credentials (for simulation)
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// Backend API endpoints
const char* burnerApiUrl = "http://localhost:3000/api/carbon-burner";
const char* creditBurnUrl = "http://localhost:3000/api/burn-credits";

// Sensor pins
const int CO2_EMISSION_SENSOR_PIN = A0;  // Simulates CO2 emission measurement
const int ENERGY_CONSUMPTION_PIN = A1;   // Simulates energy consumption
const int ACTIVITY_LED_PIN = 2;          // Red LED for active CO2 emission
const int CREDIT_LED_PIN = 3;            // Yellow LED for credit burning
const int BUZZER_PIN = 4;                // Audio feedback for high emissions

// Carbon credit burning constants
const float CO2_EMISSION_BASELINE = 0.0;  // Baseline CO2 emission (kg)
const float ENERGY_EMISSION_FACTOR = 0.8; // kg CO2 per kWh consumed
const float CREDIT_BURN_RATE = 0.001;      // CO2 kg to carbon credits

// Data transmission interval (milliseconds)
const unsigned long TRANSMISSION_INTERVAL = 30000; // 30 seconds
const unsigned long CREDIT_BURN_INTERVAL = 45000;  // 45 seconds
unsigned long lastTransmission = 0;
unsigned long lastCreditBurn = 0;

// Global variables for sensor data
float co2Emission = 0.0;          // CO2 emission in kg
float energyConsumed = 0.0;       // Energy consumed in kWh
float temperature = 22.0;
float humidity = 45.0;
String deviceId = "CARBON_BURNER_" + String(random(1000, 9999));
String activityType = "Industrial Process";

// Credit burning tracking
float totalCreditsBurned = 0.0;
float currentSessionCreditsBurned = 0.0;
float availableCredits = 100.0;  // Simulated available credits
bool isActive = false;
bool autoBurnEnabled = true;

void setup() {
  Serial.begin(115200);
  
  // Initialize pins
  pinMode(ACTIVITY_LED_PIN, OUTPUT);
  pinMode(CREDIT_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(CO2_EMISSION_SENSOR_PIN, INPUT);
  pinMode(ENERGY_CONSUMPTION_PIN, INPUT);
  
  // Connect to WiFi
  connectToWiFi();
  
  // Initialize sensors
  initializeSensors();
  
  Serial.println("=== Carbon Credit Burner Simulation Started ===");
  Serial.println("Device ID: " + deviceId);
  Serial.println("Activity Type: " + activityType);
  Serial.println("Available Credits: " + String(availableCredits));
  Serial.println("==============================================");
}

void loop() {
  // Read sensor data
  readSensorData();
  
  // Process and validate data
  processSensorData();
  
  // Calculate CO2 emission based on energy consumption
  calculateCO2Emission();
  
  // Display data on serial monitor
  displaySensorData();
  
  // Check for high emission alerts
  checkEmissionAlerts();
  
  // Send data to backend
  if (millis() - lastTransmission >= TRANSMISSION_INTERVAL) {
    sendDataToBackend();
    lastTransmission = millis();
  }
  
  // Burn credits to offset emissions
  if (millis() - lastCreditBurn >= CREDIT_BURN_INTERVAL && autoBurnEnabled) {
    burnCarbonCredits();
    lastCreditBurn = millis();
  }
  
  delay(1000); // Wait 1 second before next reading
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi connected!");
  Serial.println("IP address: " + WiFi.localIP().toString());
}

void initializeSensors() {
  Serial.println("Initializing Carbon Credit Burner sensors...");
  delay(2000);
  
  // Simulate different activity types
  int activityTypeNum = random(1, 5);
  switch(activityTypeNum) {
    case 1:
      activityType = "Manufacturing Process";
      break;
    case 2:
      activityType = "Transportation";
      break;
    case 3:
      activityType = "Energy Consumption";
      break;
    case 4:
      activityType = "Data Center Operations";
      break;
    default:
      activityType = "Industrial Process";
      break;
  }
  
  Serial.println("Activity Type: " + activityType);
  Serial.println("Sensors initialized successfully");
  
  // Blink LEDs to indicate initialization
  for (int i = 0; i < 3; i++) {
    digitalWrite(ACTIVITY_LED_PIN, HIGH);
    digitalWrite(CREDIT_LED_PIN, HIGH);
    delay(200);
    digitalWrite(ACTIVITY_LED_PIN, LOW);
    digitalWrite(CREDIT_LED_PIN, LOW);
    delay(200);
  }
}

void readSensorData() {
  // Simulate CO2 emission sensor reading
  int co2RawValue = analogRead(CO2_EMISSION_SENSOR_PIN);
  co2Emission = map(co2RawValue, 0, 1023, 0, 100); // 0-100 kg CO2 emission
  
  // Simulate energy consumption sensor reading
  int energyRawValue = analogRead(ENERGY_CONSUMPTION_PIN);
  energyConsumed = map(energyRawValue, 0, 1023, 0, 150); // 0-150 kWh
  
  // Add realistic variation
  co2Emission += random(-10, 10);
  energyConsumed += random(-15, 15);
  
  // Simulate environmental conditions
  temperature = 20.0 + random(-5, 15); // 15-35°C
  humidity = 30.0 + random(-10, 40);   // 20-70%
  
  // Determine if device is actively emitting CO2
  isActive = (co2Emission > 10.0 && energyConsumed > 20.0);
}

void processSensorData() {
  // Ensure values are within valid ranges
  if (co2Emission < 0) co2Emission = 0;
  if (energyConsumed < 0) energyConsumed = 0;
  if (co2Emission > 100) co2Emission = 100;
  if (energyConsumed > 150) energyConsumed = 150;
  
  // Round values to 2 decimal places
  co2Emission = round(co2Emission * 100) / 100.0;
  energyConsumed = round(energyConsumed * 100) / 100.0;
  temperature = round(temperature * 100) / 100.0;
  humidity = round(humidity * 100) / 100.0;
}

void calculateCO2Emission() {
  // Calculate CO2 emission based on energy consumption
  // Different activities have different CO2 emission rates
  float emissionRate = 0.0;
  
  if (activityType == "Manufacturing Process") {
    emissionRate = 1.2; // kg CO2 per kWh
  } else if (activityType == "Transportation") {
    emissionRate = 0.9; // kg CO2 per kWh
  } else if (activityType == "Energy Consumption") {
    emissionRate = 0.6; // kg CO2 per kWh
  } else if (activityType == "Data Center Operations") {
    emissionRate = 0.4; // kg CO2 per kWh
  } else {
    emissionRate = ENERGY_EMISSION_FACTOR;
  }
  
  // Update CO2 emission based on energy consumed
  co2Emission = energyConsumed * emissionRate;
}

void displaySensorData() {
  Serial.println("=== Carbon Credit Burner Data ===");
  Serial.println("Device ID: " + deviceId);
  Serial.println("Activity Type: " + activityType);
  Serial.println("CO2 Emission: " + String(co2Emission) + " kg");
  Serial.println("Energy Consumed: " + String(energyConsumed) + " kWh");
  Serial.println("Temperature: " + String(temperature) + " °C");
  Serial.println("Humidity: " + String(humidity) + " %");
  Serial.println("Status: " + String(isActive ? "ACTIVE" : "INACTIVE"));
  Serial.println("Available Credits: " + String(availableCredits));
  Serial.println("Total Credits Burned: " + String(totalCreditsBurned));
  Serial.println("Session Credits Burned: " + String(currentSessionCreditsBurned));
  Serial.println("Auto Burn: " + String(autoBurnEnabled ? "ENABLED" : "DISABLED"));
  Serial.println("=================================");
}

void checkEmissionAlerts() {
  // Visual indicators for emission status
  if (isActive) {
    digitalWrite(ACTIVITY_LED_PIN, HIGH);
  } else {
    digitalWrite(ACTIVITY_LED_PIN, LOW);
  }
  
  // Alert for high CO2 emission
  if (co2Emission > 50) {
    Serial.println("HIGH EMISSION ALERT: Significant CO2 emission detected!");
    tone(BUZZER_PIN, 1000, 500);
  }
  
  // Alert for critically high emissions
  if (co2Emission > 80) {
    Serial.println("CRITICAL EMISSION ALERT: Dangerous CO2 emission level!");
    for (int i = 0; i < 3; i++) {
      tone(BUZZER_PIN, 1500, 300);
      delay(300);
    }
  }
  
  // Alert for low credit balance
  if (availableCredits < 10.0) {
    Serial.println("LOW CREDIT ALERT: Running low on carbon credits!");
    tone(BUZZER_PIN, 800, 1000);
  }
}

void burnCarbonCredits() {
  if (isActive && co2Emission > 0 && availableCredits > 0) {
    // Calculate credits needed to offset emissions
    float creditsToBurn = co2Emission * CREDIT_BURN_RATE;
    
    // Don't burn more credits than available
    if (creditsToBurn > availableCredits) {
      creditsToBurn = availableCredits;
    }
    
    if (creditsToBurn > 0.01) { // Only burn if significant
      availableCredits -= creditsToBurn;
      currentSessionCreditsBurned += creditsToBurn;
      totalCreditsBurned += creditsToBurn;
      
      Serial.println("=== CARBON CREDITS BURNED ===");
      Serial.println("Credits Burned: " + String(creditsToBurn, 4));
      Serial.println("CO2 Offset: " + String(co2Emission) + " kg");
      Serial.println("Remaining Credits: " + String(availableCredits));
      Serial.println("Session Total Burned: " + String(currentSessionCreditsBurned, 4));
      Serial.println("Lifetime Total Burned: " + String(totalCreditsBurned, 4));
      Serial.println("=============================");
      
      // Visual and audio feedback for credit burning
      digitalWrite(CREDIT_LED_PIN, HIGH);
      tone(BUZZER_PIN, 1200, 300);
      delay(100);
      digitalWrite(CREDIT_LED_PIN, LOW);
      
      // Send credit burning data to backend
      sendCreditBurnDataToBackend(creditsToBurn);
    }
  }
}

void sendDataToBackend() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(burnerApiUrl);
    http.addHeader("Content-Type", "application/json");
    
    // Create JSON payload for burner data
    String jsonPayload = "{";
    jsonPayload += "\"deviceId\":\"" + deviceId + "\",";
    jsonPayload += "\"deviceType\":\"carbon_burner\",";
    jsonPayload += "\"activityType\":\"" + activityType + "\",";
    jsonPayload += "\"co2Emission\":" + String(co2Emission) + ",";
    jsonPayload += "\"energyConsumed\":" + String(energyConsumed) + ",";
    jsonPayload += "\"temperature\":" + String(temperature) + ",";
    jsonPayload += "\"humidity\":" + String(humidity) + ",";
    jsonPayload += "\"isActive\":" + String(isActive ? "true" : "false") + ",";
    jsonPayload += "\"availableCredits\":" + String(availableCredits) + ",";
    jsonPayload += "\"totalCreditsBurned\":" + String(totalCreditsBurned) + ",";
    jsonPayload += "\"autoBurnEnabled\":" + String(autoBurnEnabled ? "true" : "false") + ",";
    jsonPayload += "\"timestamp\":" + String(millis()) + ",";
    jsonPayload += "\"location\":\"Burner Simulation Environment\"";
    jsonPayload += "}";
    
    Serial.println("Sending burner data to backend...");
    Serial.println("Payload: " + jsonPayload);
    
    int httpResponseCode = http.POST(jsonPayload);
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("HTTP Response Code: " + String(httpResponseCode));
      Serial.println("Response: " + response);
    } else {
      Serial.println("Error sending data. HTTP Code: " + String(httpResponseCode));
    }
    
    http.end();
  } else {
    Serial.println("WiFi not connected. Cannot send data.");
  }
}

void sendCreditBurnDataToBackend(float creditsBurned) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(creditBurnUrl);
    http.addHeader("Content-Type", "application/json");
    
    // Create JSON payload for credit burning
    String jsonPayload = "{";
    jsonPayload += "\"deviceId\":\"" + deviceId + "\",";
    jsonPayload += "\"deviceType\":\"carbon_burner\",";
    jsonPayload += "\"creditsBurned\":" + String(creditsBurned, 4) + ",";
    jsonPayload += "\"co2Emission\":" + String(co2Emission) + ",";
    jsonPayload += "\"co2Offset\":" + String(co2Emission) + ",";
    jsonPayload += "\"activityType\":\"" + activityType + "\",";
    jsonPayload += "\"remainingCredits\":" + String(availableCredits) + ",";
    jsonPayload += "\"timestamp\":" + String(millis()) + ",";
    jsonPayload += "\"location\":\"Burner Simulation Environment\"";
    jsonPayload += "}";
    
    Serial.println("Sending credit burn data to backend...");
    Serial.println("Payload: " + jsonPayload);
    
    int httpResponseCode = http.POST(jsonPayload);
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Credit Burn Response Code: " + String(httpResponseCode));
      Serial.println("Response: " + response);
    } else {
      Serial.println("Error sending credit burn data. HTTP Code: " + String(httpResponseCode));
    }
    
    http.end();
  }
}

// Utility functions
void addCredits(float credits) {
  availableCredits += credits;
  Serial.println("Added " + String(credits) + " credits. Total: " + String(availableCredits));
}

void resetSessionCredits() {
  currentSessionCreditsBurned = 0.0;
  Serial.println("Session credits burned reset");
}

void toggleAutoBurn() {
  autoBurnEnabled = !autoBurnEnabled;
  Serial.println("Auto burn " + String(autoBurnEnabled ? "enabled" : "disabled"));
}

void calibrateSensors() {
  Serial.println("Starting sensor calibration...");
  delay(5000); // Wait for sensors to stabilize
  Serial.println("Calibration complete");
}

float getEmissionIntensity() {
  // Calculate emission intensity based on CO2 per energy consumed
  if (energyConsumed > 0) {
    return co2Emission / energyConsumed;
  }
  return 0.0;
}

void emergencyCreditPurchase() {
  // Simulate emergency credit purchase
  float emergencyCredits = 50.0;
  addCredits(emergencyCredits);
  Serial.println("EMERGENCY: Purchased " + String(emergencyCredits) + " credits!");
}
