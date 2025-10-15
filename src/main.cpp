#include <Arduino.h>
#include "JsonBluetooth.hpp"
#include "Wifi.hpp"
#include "Storage.hpp"

JsonBluetooth* jsonBT = nullptr;
Wifi* wifi = nullptr;
Storage* storage = nullptr;

String ssid = "";
String password = "";
bool shouldConnectToWiFi = false;
bool wifiConnected = false;
int relayPin = 4;

void startBLE();
void connectWiFi();

void setup() {
  Serial.begin(115200);

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  delay(500);

  Serial.println("\n🚀 System starting...");

  storage = new Storage();

  // Try loading stored credentials
  if (storage->load()) {
    ssid = storage->getSSID();
    password = storage->getPassword();

    Serial.println("✅ Loaded stored credentials:");
    Serial.println("  • SSID: " + ssid);
    Serial.println("  • Password: " + password);

    shouldConnectToWiFi = true;
  } else {
    Serial.println("⚠️ No stored credentials found. Starting BLE...");
    startBLE();
  }

  // Initialize WiFi object
  wifi = new Wifi();
}

void loop() {
  // Handle WiFi connection if credentials are ready
  if (shouldConnectToWiFi && !wifiConnected) {
    connectWiFi();
  }

  delay(500);
}

void startBLE() {
  jsonBT = new JsonBluetooth("ESP32-JSON");

  jsonBT->onMessage([](String message) {
    Serial.println("📩 Received via BLE: " + message);
    message.trim();

    // --- Extract SSID & Password from JSON manually ---
    int ssidIndex = message.indexOf("ssid\":\"");
    int passwordIndex = message.indexOf("password\":\"");

    if (ssidIndex != -1 && passwordIndex != -1) {
      ssidIndex += 7;
      int ssidEnd = message.indexOf("\"", ssidIndex);
      passwordIndex += 11;
      int passEnd = message.indexOf("\"", passwordIndex);

      if (ssidEnd != -1 && passEnd != -1) {
        ssid = message.substring(ssidIndex, ssidEnd);
        password = message.substring(passwordIndex, passEnd);

        Serial.println("✅ Extracted credentials:");
        Serial.println("  • SSID: " + ssid);
        Serial.println("  • Password: " + password);

        // Persist credentials
        storage->setSSID(ssid);
        storage->setPassword(password);
        storage->save();

        Serial.println("💾 Credentials saved to NVS");

        // Signal to connect to WiFi
        shouldConnectToWiFi = true;
      }
    } else {
      Serial.println("⚠️ Invalid JSON format, expected keys: ssid, password");
    }
  });

  jsonBT->onConnect([]() {
    Serial.println("📱 BLE client connected!");
  });

  jsonBT->onDisconnect([]() {
    Serial.println("❌ BLE client disconnected.");
  });

  if (jsonBT->start()) {
    Serial.println("✅ BLE started successfully!");
    Serial.println("🔗 Device name: ESP32-JSON");
    Serial.println("📶 Waiting for connections...");
  } else {
    Serial.println("❌ BLE start failed!");
  }
}

void connectWiFi() {
  Serial.println("🔌 Attempting to connect to WiFi...");

  // Stop BLE if running
  if (jsonBT != nullptr) {
    delete jsonBT;
    jsonBT = nullptr;
    Serial.println("🧹 BLE stopped to free resources.");
  }

  wifi->setDeviceName("ESP32-JSON");
  wifi->setCredentials(ssid.c_str(), password.c_str());

  if (wifi->connect()) {
    Serial.println("✅ Connected to WiFi!");
    Serial.println("  🌐 IP: " + String(wifi->getIP().c_str()));
    Serial.printf("  📶 Signal: %d dBm\n", wifi->getSignalStrength());
    wifiConnected = true;
    digitalWrite(relayPin, HIGH);
  } else {
    Serial.println("❌ WiFi connection failed. Restarting BLE...");
    shouldConnectToWiFi = false;
    startBLE(); // Allow user to resend credentials
  }
}
