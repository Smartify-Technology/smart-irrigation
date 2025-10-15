#include <Arduino.h>
#include "JsonBluetooth.hpp"
#include "Wifi.hpp"
#include "Storage.hpp"
#include "MQTT.hpp"
#include "certificates.h"

// -------------------- Constants ------------------------
String const DEVICE_NAME = "Smartify-Irrigation-Scheduler";
String const BROKER = "h101198a.ala.eu-central-1.emqxsl.com";
int const PORT = 8883;
String const USERNAME = "medhedi";
String const PASSWORD = "Dhri6qKELqbLRUf";
String const TOPIC_PREFIX = "smartfarm/68646cb41e96a4add03b28ae";

// -------------------- Global Objects --------------------
JsonBluetooth* jsonBT = nullptr;
Wifi* wifi = nullptr;
Storage* storage = nullptr;
MQTT* mqtt = nullptr;

// -------------------- Shared State --------------------
String ssid = "";
String password = "";
bool shouldConnectToWiFi = false;
bool wifiConnected = false;
bool relayState = false; // Controlled via MQTT

// -------------------- Pins Config ---------------------
const int relayPin = 4;
#define RESET_BUTTON_PIN 0

// -------------------- Task Handles --------------------
TaskHandle_t TaskBLEHandle;
TaskHandle_t TaskWiFiHandle;
TaskHandle_t TaskMQTTHandle;
TaskHandle_t TaskResetHandle;

// -------------------- Function Prototypes --------------------
void TaskBLE(void* pvParameters);
void TaskWiFi(void* pvParameters);
void TaskMQTT(void* pvParameters);
void TaskReset(void* pvParameters);

void startBLE();
void connectWiFi();
void factoryReset();

// ======================================================
//                         SETUP
// ======================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

  Serial.println("\n🚀 System starting...");

  storage = new Storage();
  wifi = new Wifi();
  mqtt = new MQTT(BROKER, PORT, EMQX_ROOT_CA);

  storage->setDeviceName(DEVICE_NAME);
  mqtt->setCredentials(USERNAME, PASSWORD);
  mqtt->setTopicPrefix(TOPIC_PREFIX);

  // Try loading stored credentials
  if (storage->load()) {
    ssid = storage->getSSID();
    password = storage->getPassword();
    Serial.printf("ssid : %s, password: %s\n", ssid.c_str(), password.c_str());
    shouldConnectToWiFi = true;
    Serial.println("✅ Loaded stored Wi-Fi credentials");
  } else {
    Serial.println("⚠️ No credentials found, starting BLE setup...");
    shouldConnectToWiFi = false;
  }

  // -------------------- Create FreeRTOS Tasks --------------------
  if (!shouldConnectToWiFi)
    xTaskCreatePinnedToCore(TaskBLE, "BLE Task", 8192, NULL, 1, &TaskBLEHandle, 0);

  xTaskCreatePinnedToCore(TaskWiFi, "WiFi Task", 8192, NULL, 2, &TaskWiFiHandle, 1);
  xTaskCreatePinnedToCore(TaskMQTT, "MQTT Task", 8192, NULL, 2, &TaskMQTTHandle, 1);
  xTaskCreatePinnedToCore(TaskReset, "Reset Task", 2048, NULL, 3, &TaskResetHandle, 1);

  Serial.println("✅ FreeRTOS tasks created");
}

void loop() {
  vTaskDelay(portMAX_DELAY); // FreeRTOS takes over
}

// ======================================================
//                        BLE TASK
// ======================================================
void TaskBLE(void* pvParameters) {
  startBLE();

  for (;;) {
    if (shouldConnectToWiFi) {
      Serial.println("📴 Stopping BLE since credentials are received.");
      if (jsonBT != nullptr) {
        delete jsonBT;
        jsonBT = nullptr;
      }
      vTaskSuspend(NULL);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// ======================================================
//                        WIFI TASK
// ======================================================
void TaskWiFi(void* pvParameters) {
  for (;;) {
    if (shouldConnectToWiFi) {
      if (!wifiConnected) {
        connectWiFi();
      } else {
        wifiConnected = wifi->isConnected();
        if (!wifiConnected) {
          Serial.println("⚠️ Wi-Fi lost, retrying...");
        }
      }
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

// ======================================================
//                        MQTT TASK
// ======================================================
void TaskMQTT(void* pvParameters) {
  const String commandTopic = "relay";
  const String statusTopic = "status";

  mqtt->setCallback([](String topic, String payload) {
    Serial.printf("📩 MQTT Message | Topic: %s | Payload: %s\n", topic.c_str(), payload.c_str());

    if (topic.endsWith("/relay")) {
      payload.trim();
      if (payload.equalsIgnoreCase("ON")) {
        relayState = true;
        digitalWrite(relayPin, HIGH);
        Serial.println("🔆 Relay turned ON via MQTT");
      } else if (payload.equalsIgnoreCase("OFF")) {
        relayState = false;
        digitalWrite(relayPin, LOW);
        Serial.println("🌑 Relay turned OFF via MQTT");
      } else {
        mqtt->publish("status", "⚠️ Invalid MQTT payload. Use 'ON' or 'OFF'.");
        Serial.println("⚠️ Invalid MQTT payload. Use 'ON' or 'OFF'.");
      }
    }
  });

  for (;;) {
    if (wifiConnected) {
      if (!mqtt->isConnected()) {
        if (mqtt->connect()) {
          mqtt->subscribe(commandTopic);
          mqtt->publish(statusTopic, "Device online");
          Serial.println("✅ MQTT subscribed to relay topic");
        } else {
          Serial.println("❌ MQTT reconnect failed, retrying...");
        }
      } else {
        mqtt->loop();
      }
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// ======================================================
//                       RESET BUTTON
// ======================================================
void TaskReset(void* pvParameters) {
  unsigned long pressStart = 0;

  for (;;) {
    if (digitalRead(RESET_BUTTON_PIN) == LOW) {
      if (pressStart == 0) pressStart = millis();
      if (millis() - pressStart > 5000) {
        factoryReset();
      }
    } else {
      pressStart = 0;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// ======================================================
//                       BLE INIT
// ======================================================
void startBLE() {
  jsonBT = new JsonBluetooth(DEVICE_NAME.c_str());

  jsonBT->onMessage([](String message) {
    Serial.println("📩 Received via BLE: " + message);
    message.trim();

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

        storage->setSSID(ssid);
        storage->setPassword(password);
        storage->save();

        shouldConnectToWiFi = true;
        vTaskResume(TaskWiFiHandle);
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
    Serial.print("🔗 Device name: ");
    Serial.println(DEVICE_NAME);
  } else {
    Serial.println("❌ BLE start failed!");
  }
}

// ======================================================
//                       WIFI CONNECT
// ======================================================
void connectWiFi() {
  wifi->setDeviceName(DEVICE_NAME.c_str());
  wifi->setCredentials(ssid.c_str(), password.c_str());

  Serial.println("🔌 Attempting to connect to Wi-Fi...");

  if (wifi->connect()) {
    wifiConnected = true;
    Serial.println("✅ Wi-Fi connected!");
    Serial.println("  🌐 IP: " + String(wifi->getIP().c_str()));
    Serial.printf("  📶 Signal: %d dBm\n", wifi->getSignalStrength());

    storage->setSSID(ssid);
    storage->setPassword(password);
    storage->save();
  } else {
    wifiConnected = false;
    Serial.println("❌ Wi-Fi connection failed. Will retry...");
  }
}

// ======================================================
//                     FACTORY RESET
// ======================================================
void factoryReset() {
  Serial.println("⚠️ Performing factory reset...");
  if (storage) storage->clear();
  delay(500);
  ESP.restart();
}
