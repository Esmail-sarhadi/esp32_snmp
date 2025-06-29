#include <UIPEthernet.h>
#include <WiFi.h>
#include <WebServer.h>   
#include <SPIFFS.h>    
#include <FastLED.h>
#include <DHT.h>   
#include <ArduinoJson.h> 
#include <WiFiUdp.h>  
#include <SNMP_Agent.h>  
#include <SNMPTrap.h>    

// Pin Definitions 
#define LED_PIN     15   
#define NUM_LEDS    1
#define DHTPIN      16
#define RELAY_PIN   12
#define TOUCH_PIN   17

// ENC28J60 pins for SPI connection
#define CS_PIN      5    
#define SO_PIN      19    
#define SI_PIN      23    
#define SCK_PIN     18   

// Network settings
uint8_t mac[6] = {0x74, 0x69, 0x69, 0x2D, 0x30, 0x31};

// WiFi Credentials
const char* ssid = "charon";
const char* password = "12121212";

// Initialize components
EthernetServer ethernetServer(80);
WebServer wifiServer(80); 
CRGB leds[NUM_LEDS];
DHT dht(DHTPIN, DHT11);

// UDP and SNMP objects
WiFiUDP wifiUdp;
EthernetUDP ethernetUdp;
SNMPAgent snmp = SNMPAgent("public", "private");

// SNMP Variables
int changingNumber = 1;
int settableNumber = 0;
uint32_t tensOfMillisCounter = 0;
uint8_t* stuff = 0;
ValueCallback* changingNumberOID;
ValueCallback* settableNumberOID;
ValueCallback* tempOID;
ValueCallback* humidityOID;
ValueCallback* relayOID;
TimestampCallback* timestampCallbackOID;
std::string staticString = "ESP32 SNMP Agent";
SNMPTrap* settableNumberTrap;
char* changingString;

// Global variables
bool relayState = false;
int currentLedMode = 0;
unsigned long lastTouchTime = 0;
const long debounceDelay = 500;
bool usingWiFi = false;

// SNMP sensor value variables
int currentTemp = 0;
int currentHumidity = 0;
int currentRelayState = 0;

// Ethernet stability variables
unsigned long lastEthernetCheck = 0;
const long ethernetCheckInterval = 10000; // Check every 10 seconds

void setup() {
  Serial.begin(115200);
  
  // Initialize SPIFFS
  if(!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }

  // Initialize pins
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(TOUCH_PIN, INPUT);
  digitalWrite(RELAY_PIN, LOW);
  
  // Initialize LED
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(50);
  
  // Initialize DHT sensor
  dht.begin();
  
  // Configure SPI for ENC28J60
  SPI.begin(SCK_PIN, SO_PIN, SI_PIN, CS_PIN);
  
  // Try Ethernet first
  initializeEthernet();
  
  // Initialize SNMP after network setup
  if(usingWiFi) {
    delay(1000); // Give WiFi time to stabilize 
    initializeSNMP(&wifiUdp);
  } else {
    delay(1000); // Give Ethernet time to stabilize
    initializeSNMP(&ethernetUdp);
  }
}

void initializeSNMP(UDP* udp) {
  // Setup SNMP
  snmp.setUDP(udp);
  snmp.begin();

  // Setup OPAQUE data
  stuff = (uint8_t*)malloc(4);
  stuff[0] = 1;
  stuff[1] = 2;
  stuff[2] = 24;
  stuff[3] = 67;
  
  // Add SNMP handlers
  changingNumberOID = snmp.addIntegerHandler(".1.3.6.1.4.1.5.0", &changingNumber);
  settableNumberOID = snmp.addIntegerHandler(".1.3.6.1.4.1.5.1", &settableNumber, true);
  
  // Initialize sensor values
  currentTemp = 0;
  currentHumidity = 0;
  currentRelayState = 0;
  
  // Add sensor and relay handlers
  tempOID = snmp.addIntegerHandler(".1.3.6.1.4.1.5.2", &currentTemp);
  humidityOID = snmp.addIntegerHandler(".1.3.6.1.4.1.5.3", &currentHumidity);
  relayOID = snmp.addIntegerHandler(".1.3.6.1.4.1.5.4", &currentRelayState, true);
  
  // Additional handlers
  snmp.addIntegerHandler(".1.3.6.1.4.1.4.0", &changingNumber);
  snmp.addOpaqueHandler(".1.3.6.1.4.1.5.9", stuff, 4, true);
  snmp.addReadOnlyStaticStringHandler(".1.3.6.1.4.1.5.11", staticString);
  
  // Setup read/write string
  changingString = (char*)malloc(25 * sizeof(char));
  snprintf(changingString, 25, "This is changeable");
  snmp.addReadWriteStringHandler(".1.3.6.1.4.1.5.12", &changingString, 25, true);

  // Setup SNMP Trap
  settableNumberTrap = new SNMPTrap("public", SNMP_VERSION_2C);
  timestampCallbackOID = (TimestampCallback*)snmp.addTimestampHandler(".1.3.6.1.2.1.1.3.0", &tensOfMillisCounter);
  
  settableNumberTrap->setUDP(udp);
  settableNumberTrap->setTrapOID(new OIDType(".1.3.6.1.2.1.33.2"));
  settableNumberTrap->setSpecificTrap(1);
  settableNumberTrap->setUptimeCallback(timestampCallbackOID);
  settableNumberTrap->addOIDPointer(changingNumberOID);
  settableNumberTrap->addOIDPointer(settableNumberOID);
  
  if(usingWiFi) {
    settableNumberTrap->setIP(WiFi.localIP());
  } else {
    settableNumberTrap->setIP(Ethernet.localIP());
  }

  snmp.sortHandlers();
}

void initializeEthernet() {
  Serial.println(F("Initializing ENC28J60..."));
  
  Ethernet.init(CS_PIN);
  delay(1000);
  
  // Reset Ethernet shield
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  delay(100);
  digitalWrite(CS_PIN, LOW);
  delay(100);
  digitalWrite(CS_PIN, HIGH);
  delay(100);
  
  Serial.println("Getting IP from DHCP...");
  
  if (Ethernet.begin(mac)) { // Get IP address from DHCP
    Serial.print("DHCP IP address: ");
    Serial.println(Ethernet.localIP());
    Serial.print("Gateway IP: ");
    Serial.println(Ethernet.gatewayIP());
    Serial.print("DNS Server: ");
    Serial.println(Ethernet.dnsServerIP());
    Serial.print("Subnet Mask: ");
    Serial.println(Ethernet.subnetMask());
    
    usingWiFi = false;
    ethernetServer.begin();
  } else {
    Serial.println("Failed to get DHCP address, switching to WiFi...");
    initializeWiFi();
  }
}

void initializeWiFi() {
  Serial.println("Initializing WiFi...");
  WiFi.begin(ssid, password);
  
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 5000) {
    delay(500);
    Serial.print(".");
  }
  
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    usingWiFi = true;
    setupWiFiRoutes();
    wifiServer.begin();
  } else {
    Serial.println("\nFailed to connect to WiFi");
  }
}

void checkEthernetConnection() {
  if (!usingWiFi && millis() - lastEthernetCheck > ethernetCheckInterval) {
    lastEthernetCheck = millis();
    
    uint8_t stat = Ethernet.maintain();
    if (stat == 2 || stat == 3 || stat == 4) {
      // If we got a new IP or failed to renew
      Serial.println("DHCP maintenance required, renewing...");
      if (Ethernet.begin(mac)) {
        Serial.print("New DHCP IP address: ");
        Serial.println(Ethernet.localIP());
      } else {
        Serial.println("Failed to renew DHCP, switching to WiFi...");
        initializeWiFi();
      }
    }
  }
}

void loop() {
  // Handle SNMP
  snmp.loop();
  
  // Check Ethernet stability
  checkEthernetConnection();
  
  // Handle web server
  if(usingWiFi) {
    wifiServer.handleClient();
  } else {
    EthernetClient client = ethernetServer.available();
    if(client) {
      handleEthernetClient(client);
      delay(1);
      client.stop();
    }
  }
  
  // Handle SNMP traps
  if(settableNumberOID->setOccurred) {
    handleSNMPTrap();
  }
  
  // Update sensors
  updateSNMPSensors();
  
  // Handle other functions
  handleTouch();
  updateLED();
  
  // Update SNMP counters
  changingNumber++;
  tensOfMillisCounter = millis()/10;
  
  // Small delay to prevent watchdog reset
  delay(1);
}

void updateSNMPSensors() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 5000) {  // Update every 5 seconds
    float temp = dht.readTemperature();
    float humidity = dht.readHumidity();
    
    if (!isnan(temp)) {
      currentTemp = (int)temp;
      snmp.removeHandler(tempOID);
      tempOID = snmp.addIntegerHandler(".1.3.6.1.4.1.5.2", &currentTemp);
    }
    
    if (!isnan(humidity)) {
      currentHumidity = (int)humidity;
      snmp.removeHandler(humidityOID);
      humidityOID = snmp.addIntegerHandler(".1.3.6.1.4.1.5.3", &currentHumidity);
    }
    
    currentRelayState = relayState ? 1 : 0;
    snmp.removeHandler(relayOID);
    relayOID = snmp.addIntegerHandler(".1.3.6.1.4.1.5.4", &currentRelayState, true);
    
    snmp.sortHandlers();
    lastUpdate = millis();
  }
}

void handleSNMPTrap() {
  Serial.printf("Number has been set to value: %i\n", settableNumber);
  if(settableNumber % 2 == 0) {
    settableNumberTrap->setVersion(SNMP_VERSION_2C);
    settableNumberTrap->setInform(true);
  } else {
    settableNumberTrap->setVersion(SNMP_VERSION_1);
    settableNumberTrap->setInform(false);
  }
  
  settableNumberOID->resetSetOccurred();
  
  IPAddress destinationIP = IPAddress(192, 168, 1, 243);
  if(snmp.sendTrapTo(settableNumberTrap, destinationIP, true, 2, 5000) != INVALID_SNMP_REQUEST_ID) {
    Serial.println("Sent SNMP Trap");
  } else {
    Serial.println("Couldn't send SNMP Trap");
  }
}

void handleEthernetClient(EthernetClient& client) {
  String currentLine = "";
  String requestLine = "";
  bool requestLineComplete = false;
  
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      
      if (c == '\n') {
        if (currentLine.length() == 0) {
          // Send the HTML file from SPIFFS
          File file = SPIFFS.open("/index.html", "r");
          if(!file) {
            client.println("HTTP/1.1 500 Internal Server Error");
            client.println();
            break;
          }
          
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          
          while(file.available()) {
            client.write(file.read());
          }
          file.close();
          break;
        } else {
          if (!requestLineComplete) {
            requestLine = currentLine;
            requestLineComplete = true;
          }
          currentLine = "";
        }
      } else if (c != '\r') {
        currentLine += c;
      }
      
      // Handle API endpoints
      if (requestLineComplete && !requestLine.isEmpty()) {
        if (requestLine.indexOf("GET /toggle-relay") >= 0) {
          handleRelayToggle();
          client.println("HTTP/1.1 200 OK");
          client.println();
          break;
        } else if (requestLine.indexOf("GET /set-led-mode") >= 0) {
          // Extract mode parameter
          int modeStart = requestLine.indexOf("mode=");
          if (modeStart >= 0) {
            currentLedMode = requestLine.substring(modeStart + 5, modeStart + 6).toInt();
          }
          client.println("HTTP/1.1 200 OK");
          client.println();
          break;
        } else if (requestLine.indexOf("GET /get-data") >= 0) {
          sendEthernetSensorData(client);
          break;
        }
      }
    }
  }
}

void sendEthernetSensorData(EthernetClient& client) {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  
  String json = "{";
  json += "\"temperature\":" + String(temperature) + ",";
  json += "\"humidity\":" + String(humidity) + ",";
  json += "\"relay\":" + String(relayState);
  json += "}";
  
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println();
  client.println(json);
}

// WiFi Handlers
void setupWiFiRoutes() {
  wifiServer.on("/", HTTP_GET, handleWiFiRoot);
  wifiServer.on("/toggle-relay", HTTP_GET, handleWiFiRelayToggle);
  wifiServer.on("/set-led-mode", HTTP_GET, handleWiFiLedMode);
  wifiServer.on("/get-data", HTTP_GET, handleWiFiGetData);
}

void handleWiFiRoot() {
  File file = SPIFFS.open("/index.html", "r");
  if(!file) {
    wifiServer.send(500, "text/plain", "Internal Server Error");
    return;
  }
  
  wifiServer.streamFile(file, "text/html");
  file.close();
}

void handleWiFiRelayToggle() {
  handleRelayToggle();
  wifiServer.send(200, "text/plain", "OK");
}

void handleWiFiLedMode() {
  if (wifiServer.hasArg("mode")) {
    currentLedMode = wifiServer.arg("mode").toInt();
  }
  wifiServer.send(200, "text/plain", "OK");
}

void handleWiFiGetData() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  
  String json = "{";
  json += "\"temperature\":" + String(temperature) + ",";
  json += "\"humidity\":" + String(humidity) + ",";
  json += "\"relay\":" + String(relayState);
  json += "}";
  
  wifiServer.send(200, "application/json", json);
}

// Common functions for both modes
void handleRelayToggle() {
  relayState = !relayState;
  digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
}

void handleTouch() {
  if (digitalRead(TOUCH_PIN) == HIGH) {
    if (millis() - lastTouchTime > debounceDelay) {
      relayState = !relayState;
      digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
      lastTouchTime = millis();
    }
  }
}

void updateLED() {
  switch (currentLedMode) {
    case 0:
      // Rainbow effect
      static uint8_t hue = 0;
      leds[0] = CHSV(hue++, 255, 255);
      break;
      
    case 1:
      // Breathing blue
      static uint8_t brightness = 0;
      static bool increasing = true;
      leds[0] = CRGB::Blue;
      leds[0].fadeLightBy(brightness);
      if (increasing) {
        brightness += 5;
        if (brightness >= 250) increasing = false;
      } else {
        brightness -= 5;
        if (brightness <= 5) increasing = true;
      }
      break;
      
    case 2:
      // Color changing
      static unsigned long lastChange = 0;
      if (millis() - lastChange > 1000) {
        static uint8_t colorIndex = 0;
        CRGB colors[] = {CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::Yellow, CRGB::Purple};
        leds[0] = colors[colorIndex];
        colorIndex = (colorIndex + 1) % 5;
        lastChange = millis();
      }
      break;
  }
  FastLED.show();
}
