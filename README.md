# ESP32 SNMP Project 🌐

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## Overview 📝

This project implements SNMP (Simple Network Management Protocol) v2 functionality on an ESP32 microcontroller with dual network connectivity support (Ethernet and WiFi). Perfect for IoT monitoring and management applications! 

## Features ⭐

- 🔄 Dual Network Support:
  - ENC28J60 Ethernet Module Support
  - WiFi Connectivity with Fallback
- 📊 SNMP v2c Implementation:
  - GET/SET Operations
  - SNMP Traps
  - Custom OIDs
- 🎯 Hardware Integration:
  - DHT11 Temperature & Humidity Sensor
  - WS2812B RGB LED
  - Relay Control
  - Touch Sensor
- 🌐 Web Interface:
  - Real-time Sensor Data
  - Remote Relay Control
  - LED Mode Control

## Hardware Requirements 🛠️

- ESP32 Development Board
- ENC28J60 Ethernet Module
- DHT11 Temperature & Humidity Sensor
- WS2812B RGB LED
- Relay Module
- Touch Sensor
- Jumper Wires

## Pin Configuration 📌

```
LED_PIN     -> GPIO15
DHTPIN      -> GPIO16
RELAY_PIN   -> GPIO12
TOUCH_PIN   -> GPIO17

ENC28J60 SPI Pins:
CS_PIN      -> GPIO5
SO_PIN      -> GPIO19
SI_PIN      -> GPIO23
SCK_PIN     -> GPIO18
```

## Required Libraries 📚

- UIPEthernet
- WiFi
- WebServer
- SPIFFS
- FastLED
- DHT
- ArduinoJson
- SNMP_Agent
- SNMPTrap

## Installation & Setup 🔧

1. Clone this repository:
   ```bash
   git clone https://github.com/Esmail-sarhadi/esp32_snmp.git
   ```

2. Install required libraries through Arduino IDE Library Manager

3. Configure your network settings in the code:
   ```cpp
   const char* ssid = "your_wifi_ssid";
   const char* password = "your_wifi_password";
   ```

4. Upload the code to your ESP32

## SNMP Configuration 🔐

Default SNMP configuration:
- Community String (Read): "public"
- Community String (Write): "private"
- Default Trap Destination: 192.168.1.243

Custom OIDs:
- Temperature: .1.3.6.1.4.1.5.2
- Humidity: .1.3.6.1.4.1.5.3
- Relay State: .1.3.6.1.4.1.5.4

## Features in Detail 🎯

### Network Redundancy
The system first attempts to connect via Ethernet. If Ethernet connection fails, it automatically switches to WiFi mode.

### LED Modes
- Mode 0: Rainbow Effect
- Mode 1: Breathing Blue
- Mode 2: Color Changing Sequence

### Web Interface
Access the web interface through:
- Ethernet Mode: http://<ethernet_ip>
- WiFi Mode: http://<wifi_ip>

## Contributing 🤝

Contributions are welcome! Please feel free to submit a Pull Request.

## License 📄

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Author ✍️

**Esmail Sarhadi**
- GitHub: [@Esmail-sarhadi](https://github.com/Esmail-sarhadi)

## Acknowledgments 🙏

- Thanks to all contributors and the open-source community
- Special thanks to the ESP32 and SNMP library developers

## Support 💬

If you have any questions or run into issues, please open an issue in the GitHub repository.

---
⭐ Don't forget to star this repo if you find it helpful!
