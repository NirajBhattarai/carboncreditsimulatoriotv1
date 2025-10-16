# Carbon Credit IoT MQTT Setup Guide

This guide will help you set up a local MQTT broker and configure your ESP32 device to send carbon credit sensor data via MQTT.

## Prerequisites

- Docker and Docker Compose installed
- Python 3.x with pip
- PlatformIO CLI (for ESP32 development)

## Quick Start

### 1. Start the MQTT Broker

```bash
# Start the MQTT broker and web interface
docker-compose up -d

# Check if services are running
docker-compose ps
```

This will start:
- **Mosquitto MQTT Broker** on port 1883
- **MQTT Explorer** web interface on port 4000

### 2. Install Python MQTT Client

```bash
# Install required Python packages
pip install paho-mqtt

# Make the test client executable
chmod +x mqtt_test_client.py
```

### 3. Test MQTT Connection

```bash
# Run the test client to monitor MQTT messages
python3 mqtt_test_client.py
```

### 4. Configure ESP32 Device

1. Update `src/secrets.h` with your MQTT broker IP address:
   ```cpp
   #define MQTT_SERVER "YOUR_MQTT_BROKER_IP"  // Replace with actual IP
   ```

2. Build and upload to ESP32:
   ```bash
   pio run --target upload
   ```

## MQTT Topics

The device publishes sensor data to these topics:

- `carbon_credit/sensor_data` - Main sensor readings (CO2, humidity, credits, emissions, offset)
- `carbon_credit/commands` - Command topic (for future remote control)

## Message Format

Sensor data is published as JSON:

```json
{
  "co2": 450,
  "humidity": 65,
  "credits": 225.0,
  "emissions": 13.0,
  "offset": true,
  "timestamp": 1234567890
}
```

## Web Interface

Access the MQTT Explorer web interface at: http://localhost:4000

- Connect to broker: `localhost:1883`
- Subscribe to topic: `carbon_credit/#`

## Troubleshooting

### MQTT Broker Issues

```bash
# Check broker logs
docker-compose logs mosquitto

# Restart services
docker-compose restart
```

### ESP32 Connection Issues

1. Verify WiFi credentials in `secrets.h`
2. Check MQTT broker IP address
3. Ensure broker is running and accessible
4. Check serial monitor for connection status

### Python Client Issues

```bash
# Install missing dependencies
pip install --upgrade paho-mqtt

# Test broker connectivity
telnet localhost 1883
```

## Configuration Files

- `docker-compose.yml` - MQTT broker and web interface setup
- `mqtt/config/mosquitto.conf` - MQTT broker configuration
- `src/secrets.h` - ESP32 WiFi and MQTT settings
- `mqtt_test_client.py` - Python MQTT test client

## Security Notes

- Current setup allows anonymous connections (development only)
- For production, enable authentication in `mosquitto.conf`
- Use TLS/SSL for encrypted connections
- Implement proper access control lists (ACLs)

## Next Steps

1. Set up authentication and encryption
2. Add data persistence to MQTT broker
3. Implement remote device control via MQTT commands
4. Add data visualization dashboard
5. Set up MQTT data forwarding to cloud services
