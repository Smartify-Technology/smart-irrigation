#include <Arduino.h>
#include "JsonBluetooth.hpp"
#include "Wifi.hpp"
#include "Storage.hpp"

String const DEVICE_NAME="Smartify-Irrigation-Scheduler";

// -------------------- Global Objects --------------------
JsonBluetooth* jsonBT = nullptr;
Wifi* wifi = nullptr;
Storage* storage = nullptr;

// -------------------- Shared State --------------------
String ssid = "";
String password = "";
bool shouldConnectToWiFi = false;
bool wifiConnected = false;
int relayPin = 4;

// -------------------- Reset Button --------------------
#define RESET_BUTTON_PIN 0  // GPIO0, change if needed

// -------------------- Task Handles --------------------
TaskHandle_t TaskBLEHandle;
TaskHandle_t TaskWiFiHandle;
TaskHandle_t TaskRelayHandle;
TaskHandle_t TaskResetHandle;

// -------------------- Function Prototypes --------------------
void TaskBLE(void* pvParameters);
void TaskWiFi(void* pvParameters);
void TaskRelay(void* pvParameters);
void TaskReset(void* pvParameters);
void startBLE();
void connectWiFi();
void factoryReset();

void setup() {
  Serial.begin(115200);

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

  Serial.println("\n🚀 System starting...");

  storage = new Storage();
  wifi = new Wifi();

  storage->setDeviceName(DEVICE_NAME);

  // Try loading stored credentials
  if (storage->load()) {
    ssid = storage->getSSID();
    password = storage->getPassword();
    Serial.printf("ssid : %s, password: %s\n", ssid.c_str(), password.c_str());
    if (storage->getReset()){
      shouldConnectToWiFi = false;
      storage->setReset(false);
    } else {
      shouldConnectToWiFi = true;
    }
    Serial.println("✅ Loaded stored Wi-Fi credentials");
  } else {
    Serial.println("⚠️ No credentials found, starting BLE setup...");
    shouldConnectToWiFi = false;
  }

  // -------------------- Create FreeRTOS Tasks --------------------
  if (!shouldConnectToWiFi) {
    xTaskCreatePinnedToCore(TaskBLE, "BLE Task", 8192, NULL, 1, &TaskBLEHandle, 0);
  }

  xTaskCreatePinnedToCore(TaskWiFi, "WiFi Task", 8192, NULL, 2, &TaskWiFiHandle, 1);
  xTaskCreatePinnedToCore(TaskRelay, "Relay Task", 2048, NULL, 1, &TaskRelayHandle, 1);
  xTaskCreatePinnedToCore(TaskReset, "Reset Task", 2048, NULL, 3, &TaskResetHandle, 1);

  Serial.println("✅ FreeRTOS tasks created");
}

void loop() {
  vTaskDelay(portMAX_DELAY); // FreeRTOS handles everything
}

// -------------------- BLE TASK --------------------
void TaskBLE(void* pvParameters) {
  startBLE();

  for (;;) {
    // BLE runs only once to receive credentials
    if (shouldConnectToWiFi) {
      Serial.println("📴 Stopping BLE since credentials are received.");
      if (jsonBT != nullptr) {
        delete jsonBT;
        jsonBT = nullptr;
      }
      vTaskSuspend(NULL); // Suspend BLE task
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// -------------------- WIFI TASK --------------------
void TaskWiFi(void* pvParameters) {
  for (;;) {
    if (shouldConnectToWiFi) {
      if (!wifiConnected) {
        connectWiFi();
      } else {
        // Periodically check connection
        wifiConnected = wifi->isConnected();
        if (!wifiConnected) {
          Serial.println("⚠️ Wi-Fi lost, retrying...");
        }
      }
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS); // Retry every 5s if disconnected
  }
}

// -------------------- RELAY TASK --------------------
void TaskRelay(void* pvParameters) {
  for (;;) {
    digitalWrite(relayPin, wifiConnected ? HIGH : LOW);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

// -------------------- RESET BUTTON TASK --------------------
void TaskReset(void* pvParameters) {
  unsigned long pressStart = 0;

  for (;;) {
    if (digitalRead(RESET_BUTTON_PIN) == LOW) {  // button pressed
      if (pressStart == 0) pressStart = millis();
      if (millis() - pressStart > 5000) {      // held > 5s
          factoryReset();
      }
    } else {
        pressStart = 0; // reset counter if button released
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// -------------------- BLE INITIALIZATION --------------------
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
        vTaskResume(TaskWiFiHandle); // Notify Wi-Fi Task
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

    storage->load();
    Serial.printf("reset: %d", (int)storage->getReset());

  } else {
    Serial.println("❌ BLE start failed!");
  }
}

// -------------------- WIFI CONNECT --------------------
void connectWiFi() {
    wifi->setDeviceName(DEVICE_NAME.c_str());
    wifi->setCredentials(ssid.c_str(), password.c_str());

    Serial.println("🔌 Attempting to connect to Wi-Fi...");

    if (wifi->connect()) {
        wifiConnected = true;
        Serial.println("✅ Wi-Fi connected!");
        Serial.println("  🌐 IP: " + String(wifi->getIP().c_str()));
        Serial.printf("  📶 Signal: %d dBm\n", wifi->getSignalStrength());

        // Save credentials again to ensure they persist across power loss
        storage->setSSID(ssid);
        storage->setPassword(password);
        storage->save();

        // BLE can be stopped safely
        if (jsonBT != nullptr) {
            delete jsonBT;
            jsonBT = nullptr;
        }
    } else {
        wifiConnected = false;
        Serial.println("❌ Wi-Fi connection failed. Will retry...");
    }
}


// -------------------- FACTORY RESET --------------------
void factoryReset() {
  Serial.println("⚠️ Performing factory reset...");
  if (storage) storage->setReset(true);
  delay(500);
  ESP.restart(); // Software reset
}
