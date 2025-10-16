# Carbon Credit Simulator IoT v1

A comprehensive IoT simulation project for carbon credit marketplace operations, featuring both **Carbon Credit Creators** (devices that generate credits through CO2 reduction) and **Carbon Credit Burners** (devices that consume credits to offset CO2 emissions).

## Project Structure

```
carboncreditsimulatoriotv1/
├── burner/                    # Carbon Credit Burner Simulation
│   ├── carbon-credit-burner-production.ino
│   ├── carbon-credit-burner.ino
│   ├── diagram.json
│   ├── platformio.ini
│   ├── src/
│   │   ├── main.cpp
│   │   └── secrets.h
│   ├── wokwi.toml
│   └── ...
├── creator/                   # Carbon Credit Creator Simulation
│   ├── diagram.json
│   ├── docker-compose.yml
│   ├── platformio.ini
│   ├── README_MQTT.md
│   ├── src/
│   │   ├── main.cpp
│   │   └── secrets.h
│   ├── wokwi.toml
│   ├── mqtt/                  # MQTT broker configuration
│   └── ...
└── README.md                  # This file
```

## Overview

The IoT simulations mimic real-world environmental monitoring devices that:
- **Creators**: Monitor CO2 reduction activities (renewable energy, carbon capture, etc.) and generate carbon credits
- **Burners**: Monitor CO2 emission activities (industrial processes, transportation, etc.) and burn carbon credits to offset emissions

## Hardware Simulation

Both simulations include the following components:

### ESP32 DevKit V1
- Main microcontroller
- WiFi connectivity for data transmission
- Analog and digital I/O pins

### MQ135 CO2 Sensor
- **Creator**: Simulates CO2 reduction measurement
- **Burner**: Simulates CO2 emission measurement
- Connected to analog pin A0
- Measures CO2 levels from 0-100 kg

### Activity LEDs
- **Creator**: Green LED (GPIO2) for active CO2 reduction
- **Burner**: Red LED (GPIO2) for active CO2 emission
- Indicates device activity status

### Credit LEDs
- **Creator**: Blue LED (GPIO3) for credit generation
- **Burner**: Yellow LED (GPIO3) for credit burning
- Provides visual feedback for credit operations

### Buzzer
- Connected to GPIO4
- Provides audio alerts for various conditions
- Different tones for different alert levels

### DHT22 Temperature & Humidity Sensor
- Connected to GPIO5/GPIO6
- Measures temperature and humidity
- Provides environmental context for readings

### Energy Sensor (Potentiometer)
- **Creator**: Simulates renewable energy generation (A1)
- **Burner**: Simulates energy consumption (A1)
- Used for calculating CO2 impact

## Features

### Carbon Credit Creator Features

#### Data Collection
- **CO2 Reduction**: Continuous monitoring of CO2 reduction activities
- **Energy Generation**: Renewable energy production measurement
- **Temperature & Humidity**: Environmental context for readings
- **Activity Types**: Solar, Wind, Carbon Capture, or Renewable Energy

#### Credit Generation
- **Automatic Credit Calculation**: Based on CO2 reduction and energy generation
- **Activity-Specific Rates**: Different conversion rates for different activities
- **Real-time Processing**: Immediate credit generation when thresholds are met
- **Session Tracking**: Tracks credits generated per session and lifetime

#### Alert System
- **High Activity Alert**: Triggers when CO2 reduction > 30 kg
- **Visual Indicators**: Green LED for activity, Blue LED for credit generation
- **Audio Alerts**: Buzzer notifications for significant activities

### Carbon Credit Burner Features

#### Data Collection
- **CO2 Emission**: Continuous monitoring of CO2 emission activities
- **Energy Consumption**: Energy usage measurement
- **Temperature & Humidity**: Environmental context for readings
- **Activity Types**: Manufacturing, Transportation, Energy Consumption, or Data Center Operations

#### Credit Burning
- **Automatic Credit Burning**: Burns credits to offset CO2 emissions
- **Activity-Specific Rates**: Different emission rates for different activities
- **Credit Balance Management**: Tracks available credits and prevents over-burning
- **Session Tracking**: Tracks credits burned per session and lifetime

#### Alert System
- **High Emission Alert**: Triggers when CO2 emission > 50 kg
- **Critical Emission Alert**: Triggers when CO2 emission > 80 kg
- **Low Credit Alert**: Triggers when available credits < 10
- **Visual Indicators**: Red LED for activity, Yellow LED for credit burning

### Data Transmission
- **WiFi Connectivity**: Connects to Wokwi-GUEST network
- **HTTP API**: Sends data to backend server
- **JSON Format**: Structured data transmission
- **Multiple Endpoints**: Separate APIs for creator and burner data
- **Real-time Updates**: Regular data transmission intervals

### Carbon Footprint Calculation
- **Creator**: Converts CO2 reduction to carbon credits
- **Burner**: Converts CO2 emissions to credit requirements
- **Activity-Specific Rates**: Different conversion factors for different activities
- **Real-time Processing**: Immediate calculations and credit operations

## Quick Start

### 1. Wokwi Setup

#### For Carbon Credit Creator:
1. Go to [Wokwi.com](https://wokwi.com)
2. Create a new project
3. Import the `creator/diagram.json` file
4. Copy the Arduino code from `creator/src/main.cpp`
5. Use `creator/wokwi.toml` for configuration

#### For Carbon Credit Burner:
1. Go to [Wokwi.com](https://wokwi.com)
2. Create a new project
3. Import the `burner/diagram.json` file
4. Copy the Arduino code from `burner/src/main.cpp`
5. Use `burner/wokwi.toml` for configuration

### 2. MQTT Setup (Creator)

The creator simulation includes MQTT support:

```bash
# Start the MQTT broker and web interface
cd creator
docker-compose up -d

# Check if services are running
docker-compose ps
```

This will start:
- **Mosquitto MQTT Broker** on port 1883
- **MQTT Explorer** web interface on port 4000

### 3. Backend Integration
1. Ensure your backend server is running on `localhost:3000`
2. Create the following API endpoints:
   - `/api/carbon-creator` - For creator data
   - `/api/carbon-burner` - For burner data
   - `/api/generate-credits` - For credit generation
   - `/api/burn-credits` - For credit burning

## API Data Formats

### Creator Data Format:
```json
{
  "deviceId": "CARBON_CREATOR_1234",
  "deviceType": "carbon_creator",
  "activityType": "Solar Energy Generation",
  "co2Reduction": 25.5,
  "energyGenerated": 50.0,
  "temperature": 22.5,
  "humidity": 45.0,
  "isActive": true,
  "totalCreditsGenerated": 12.5,
  "timestamp": 1234567890,
  "location": "Creator Simulation Environment"
}
```

### Burner Data Format:
```json
{
  "deviceId": "CARBON_BURNER_5678",
  "deviceType": "carbon_burner",
  "activityType": "Manufacturing Process",
  "co2Emission": 35.2,
  "energyConsumed": 45.0,
  "temperature": 24.0,
  "humidity": 50.0,
  "isActive": true,
  "availableCredits": 75.5,
  "totalCreditsBurned": 25.0,
  "autoBurnEnabled": true,
  "timestamp": 1234567890,
  "location": "Burner Simulation Environment"
}
```

## Configuration

### Creator Sensor Calibration
- **CO2 Reduction Range**: 0-50 kg
- **Energy Generation Range**: 0-100 kWh
- **Temperature Range**: 15-35°C
- **Humidity Range**: 20-70%
- **Credit Conversion Rate**: 0.001 credits per kg CO2 reduced

### Burner Sensor Calibration
- **CO2 Emission Range**: 0-100 kg
- **Energy Consumption Range**: 0-150 kWh
- **Temperature Range**: 15-35°C
- **Humidity Range**: 20-70%
- **Credit Burn Rate**: 0.001 credits per kg CO2 emitted

### Activity-Specific Rates

#### Creator Activities:
- **Solar Energy Generation**: 0.4 kg CO2 per kWh
- **Wind Energy Generation**: 0.3 kg CO2 per kWh
- **Carbon Capture Technology**: 1.0 kg CO2 direct capture
- **Renewable Energy Generation**: 0.5 kg CO2 per kWh (default)

#### Burner Activities:
- **Manufacturing Process**: 1.2 kg CO2 per kWh
- **Transportation**: 0.9 kg CO2 per kWh
- **Energy Consumption**: 0.6 kg CO2 per kWh
- **Data Center Operations**: 0.4 kg CO2 per kWh

## Usage

### Running the Simulations

#### Carbon Credit Creator:
1. Upload the `creator/src/main.cpp` code to Wokwi
2. Start the simulation
3. Monitor serial output for CO2 reduction readings
4. Observe Green LED for activity, Blue LED for credit generation
5. Check backend for received creator data and credit generation

#### Carbon Credit Burner:
1. Upload the `burner/src/main.cpp` code to Wokwi
2. Start the simulation
3. Monitor serial output for CO2 emission readings
4. Observe Red LED for activity, Yellow LED for credit burning
5. Check backend for received burner data and credit burning

### Monitoring Data
- **Serial Monitor**: Real-time sensor readings and credit operations
- **LED Indicators**: Visual status updates for activity and credit operations
- **Audio Alerts**: Immediate notifications for various conditions
- **Backend Logs**: Data transmission and credit operation records

## Integration with Carbon Credit Marketplace

These IoT simulations provide the foundation for:

1. **Real-time Monitoring**: Continuous environmental data collection from both creators and burners
2. **Carbon Credit Lifecycle**: Complete credit generation, trading, and burning workflow
3. **Automated Credit Operations**: Automatic credit generation and burning based on real-time data
4. **Alert Systems**: Immediate notification of environmental issues and credit status
5. **Data Analytics**: Historical data for trend analysis and optimization
6. **Marketplace Integration**: Direct connection to trading platform with real-time data
7. **Carbon Footprint Tracking**: Comprehensive monitoring of both positive and negative environmental impact

## Troubleshooting

### Common Issues
1. **WiFi Connection**: Ensure Wokwi-GUEST network is available
2. **Backend Connection**: Verify server is running and accessible
3. **Sensor Readings**: Check analog pin connections
4. **Alert System**: Verify LED and buzzer connections
5. **Credit Operations**: Check API endpoints are properly configured

### Debug Information
- Serial output provides detailed logging for both simulations
- HTTP response codes indicate transmission status
- Error messages help identify issues
- Credit operation logs show generation and burning activities

## Future Enhancements

- **Multiple Sensors**: Support for additional environmental sensors
- **Machine Learning**: Predictive analytics for CO2 trends and credit optimization
- **Blockchain Integration**: Direct integration with carbon credit blockchain
- **Mobile App**: Real-time monitoring via mobile application
- **Historical Data**: Long-term data storage and analysis
- **Credit Trading**: Direct peer-to-peer credit trading between devices
- **Smart Contracts**: Automated credit transactions based on IoT data
- **Carbon Footprint Scoring**: Comprehensive environmental impact scoring
- **Multi-Device Networks**: Support for multiple creators and burners in a network
- **Advanced Analytics**: AI-powered insights for carbon credit optimization

## Support

For issues or questions regarding the IoT simulations:
1. Check the serial monitor output for both creator and burner devices
2. Verify hardware connections in Wokwi diagrams
3. Ensure backend APIs are properly configured for all endpoints
4. Review error messages and debug information
5. Check credit operation logs for generation and burning activities
6. Verify WiFi connectivity and API endpoint accessibility