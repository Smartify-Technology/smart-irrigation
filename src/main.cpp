#include <Arduino.h>
#include "JsonBluetooth.hpp"
#include "Wifi.hpp"
#include "Storage.hpp"
#include "MQTT.hpp"
#include "certificates.h"
#include "Led/Led.hpp"
#include "EV/EV.hpp"
#include "RTC.hpp"
#include "Schedule.hpp"

// -------------------- Constants ------------------------
String const DEVICE_NAME = "Smartify-Irrigation-Scheduler";
String const BROKER = "h101198a.ala.eu-central-1.emqxsl.com";
int const PORT = 8883;
String const USERNAME = "medhedi";
String const PASSWORD = "Dhri6qKELqbLRUf";
String const TOPIC_PREFIX = "smartfarm/68646cb41e96a4add03b28ae";
int EV_NUMBER = 12;
int SDA_PIN=22, SCL_PIN=23;

// -------------------- Global Objects --------------------
JsonBluetooth* jsonBT = nullptr;
Wifi* wifi = nullptr;
Storage* storage = nullptr;
MQTT* mqtt = nullptr;
EV T_ev[12] = {EV(10, 1), EV(9, 2), EV(13, 3), EV(27, 4), EV(26, 5), EV(25, 6),
               EV(33, 7), EV(32, 8), EV(19, 9), EV(18, 10), EV(5, 11), EV(17, 12)};
Led greenLed = Led(16);
Led yellowLed = Led(4);
Led redLed = Led(15);
RTC rtc;

// -------------------- Shared State --------------------
String ssid = "";
String password = "";
bool shouldConnectToWiFi = false;
bool wifiConnected = false;
String currentMode = "manual";

// -------------------- Pins Config ---------------------
const int relayPin = 14;
#define RESET_BUTTON_PIN 0

// -------------------- Task Handles --------------------
TaskHandle_t TaskBLEHandle;
TaskHandle_t TaskWiFiHandle;
TaskHandle_t TaskMQTTHandle;
TaskHandle_t TaskResetHandle;
TaskHandle_t TaskSchedulerHandle;

// -------------------- Function Prototypes --------------------
void TaskBLE(void* pvParameters);
void TaskWiFi(void* pvParameters);
void TaskMQTT(void* pvParameters);
void TaskReset(void* pvParameters);
void TaskScheduler(void* pvParameters);
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
  redLed.blink(2000, 300);
  yellowLed.blink(1000, 500);
  greenLed.blink(3000, 600);

  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

  Serial.println("\nSystem starting...");

  storage = new Storage();
  wifi = new Wifi();
  mqtt = new MQTT(BROKER, PORT, EMQX_ROOT_CA);

  storage->setDeviceName(DEVICE_NAME);
  mqtt->setCredentials(USERNAME, PASSWORD);
  mqtt->setTopicPrefix(TOPIC_PREFIX);

  if (storage->load()) {
    ssid = storage->getSSID();
    password = storage->getPassword();
    Serial.printf("Loaded WiFi: %s\n", ssid.c_str());
    shouldConnectToWiFi = true;
  } else {
    Serial.println("No WiFi credentials → starting BLE provisioning");
  }

  currentMode = storage->getMode();
  scheduleManager.load();

  Serial.printf("Mode: %s | Schedules: %d\n", currentMode.c_str(), (int)scheduleManager.schedules.size());

  // Create tasks
  if (!shouldConnectToWiFi)
    xTaskCreatePinnedToCore(TaskBLE, "BLE", 8192, NULL, 1, &TaskBLEHandle, 0);

  xTaskCreatePinnedToCore(TaskWiFi,      "WiFi",      8192, NULL, 2, &TaskWiFiHandle,      1);
  xTaskCreatePinnedToCore(TaskMQTT,      "MQTT",      8192, NULL, 2, &TaskMQTTHandle,      1);
  xTaskCreatePinnedToCore(TaskReset,     "Reset",     2048, NULL, 3, &TaskResetHandle,     1);
  xTaskCreatePinnedToCore(TaskScheduler, "Scheduler", 6144, NULL, 1, &TaskSchedulerHandle, 0);

  Serial.println("All FreeRTOS tasks started");
}

void loop() {
  vTaskDelay(portMAX_DELAY);
}

// ======================================================
//                        BLE TASK
// ======================================================
void TaskBLE(void* pvParameters) {
  startBLE();
  for (;;) {
    if (shouldConnectToWiFi) {
      Serial.println("Stopping BLE – WiFi credentials received");
      if (jsonBT) { delete jsonBT; jsonBT = nullptr; }
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
    if (shouldConnectToWiFi && !wifiConnected) {
      connectWiFi();
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

// ======================================================
//                        MQTT TASK
// ======================================================
void TaskMQTT(void* pvParameters) {
  const String fullPrefix = TOPIC_PREFIX + "/";

  mqtt->setCallback([](String topic, String payload) {
    Serial.printf("MQTT → %s: %s\n", topic.c_str(), payload.c_str());

    // MODE CHANGE
    if (topic == TOPIC_PREFIX + "/mode") {
      payload.toLowerCase();
      bool deactivateAll = false;
      if (payload == "auto" || payload == "manual") {
        if (payload != currentMode) deactivateAll = true; // deactivate all EVs on mode change
        currentMode = payload;
        storage->setMode(currentMode);
        mqtt->publish("status", "Mode: " + currentMode);
        Serial.println("Mode → " + currentMode);

        if (deactivateAll) {
          deactivateAll = false;
          for (int i = 0; i < EV_NUMBER; i++) T_ev[i].deactivate();
        }
      }
      return;
    }

    // SCHEDULE COMMAND: {"ev":"3","start":"07:30","end":"08:15"}
    if (topic == TOPIC_PREFIX + "/schedule") {
      payload.trim();
      if (payload.startsWith("{") && payload.endsWith("}")) {

        int evPos = payload.indexOf("\"ev\":");
        int startPos = payload.indexOf("\"start\":\"");
        int endPos = payload.indexOf("\"end\":\"");

        if (evPos != -1 && startPos != -1 && endPos != -1) {
          evPos += 6;
          int evEnd = payload.indexOf(",", evPos);
          if (evEnd == -1) evEnd = payload.indexOf("}", evPos);
          int ev = payload.substring(evPos, evEnd).toInt();

          startPos += 9;
          int startEnd = payload.indexOf("\"", startPos);
          String start = payload.substring(startPos, startEnd);

          endPos += 7;
          int endEnd = payload.indexOf("\"", endPos);
          String end = payload.substring(endPos, endEnd);

          Serial.printf("Now : %d:%d\n", rtc.now().hour(), rtc.now().minute());
          Serial.printf("Schedule EV:%d Start:%s End:%s\n", ev, start.c_str(), end.c_str());

          if (ev >= 1 && ev <= 12 && start.length() == 5 && end.length() == 5 &&
              start[2] == ':' && end[2] == ':') {

            IrrigationSchedule s;
            s.evId = ev;
            s.startHour = start.substring(0,2).toInt();
            s.startMinute = start.substring(3).toInt();
            s.endHour = end.substring(0,2).toInt();
            s.endMinute = end.substring(3).toInt();
            s.enabled = true;

            scheduleManager.addOrUpdate(s);
            scheduleManager.save();

            mqtt->publish("status", "Schedule saved EV" + String(ev) + " " + start + "-" + end);
            Serial.println("Schedule saved: " + s.toString());
            return;
          }
        }
      }
      mqtt->publish("status", "Invalid schedule format");
      return;
    }

    // MANUAL EV CONTROL (only in manual mode)
    if (topic.endsWith("/ev")) {
      if (currentMode == "auto") {
        mqtt->publish("status", "Ignored – Auto mode active");
        return;
      }

      payload.trim();
      int evPos = payload.indexOf("\"ev\":");
      int statePos = payload.indexOf("\"state\":");

      if (evPos != -1 && statePos != -1) {
        evPos += 6;
        int evEnd = payload.indexOf(",", evPos);
        if (evEnd == -1) evEnd = payload.indexOf("}", evPos);
        int evNum = payload.substring(evPos, evEnd).toInt();

        statePos += 9;
        int stateEnd = payload.indexOf("\"}", statePos);
        String state = payload.substring(statePos, stateEnd);
        Serial.println("EV: " + String(evNum) + " State: " + state);
        if (evNum >= 1 && evNum <= 12) {
          if (state.equalsIgnoreCase("ON")) {
            T_ev[evNum-1].activate();
            mqtt->publish("status", "Relay " + String(evNum) + " ON");
          } else if (state.equalsIgnoreCase("OFF")) {
            T_ev[evNum-1].deactivate();
            mqtt->publish("status", "Relay " + String(evNum) + " OFF");
          }
        }
      }
    }

    // Get All Schedules
    if ( topic == TOPIC_PREFIX + "/get_all_schedules" ) {
        String schedJson = scheduleManager.toJson();
        mqtt->publish("status", schedJson);
        Serial.println("Published schedules: " + schedJson);
        Serial.println("Published all schedules");
        return;
    }

    // Get time status
    if ( topic == TOPIC_PREFIX + "/get_time"){
        int hour = rtc.now().hour();
        int minute = rtc.now().minute();
        int second = rtc.now().second();
        String timeStr = String(hour) + ":" + String(minute) + ":" + String(second);        
        mqtt->publish("status", "Time: " + timeStr);
        Serial.println("Published time: " + timeStr);
        return;
    }
  });



  for (;;) {
    if (wifiConnected) {
      if (!mqtt->isConnected()) {
        if (mqtt->connect()) {
          mqtt->subscribe("mode");
          mqtt->subscribe("schedule");
          mqtt->subscribe("status");
          mqtt->subscribe("ev");
          mqtt->subscribe("get_all_schedules");
          mqtt->subscribe("get_time");
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
  unsigned long pressed = 0;
  for (;;) {
    if (digitalRead(RESET_BUTTON_PIN) == LOW) {
      if (pressed == 0) pressed = millis();
      if (millis() - pressed > 5000) factoryReset();
    } else {
      pressed = 0;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// ======================================================
//                      SCHEDULER TASK
// ======================================================
void TaskScheduler(void* pvParameters) {
  if (!rtc.begin(SDA_PIN, SCL_PIN)) {
    Serial.println("RTC init failed!");
  }

  // Optional NTP sync on first boot if WiFi is up
  if (wifiConnected) {
    rtc.syncWithNTP("pool.ntp.org", 1 * 3600, 0); // Tunisia = UTC + 1
  }

  for (;;) {
    if (currentMode == "auto") {
      for (const auto& s : scheduleManager.schedules) {
        if (!s.enabled) continue;

        bool on = rtc.isBetween(s.startHour, s.startMinute, s.endHour, s.endMinute);
        uint8_t idx = s.evId - 1;

        if (on && !T_ev[idx].isActive()) {
          T_ev[idx].activate();
          Serial.println("Auto ON: " + s.toString());
          if (mqtt && mqtt->isConnected())
            mqtt->publish("status", "Auto ON " + s.toString());
        }
        else if (!on && T_ev[idx].isActive()) {
          T_ev[idx].deactivate();
          Serial.println("Auto OFF: " + s.toString());
          if (mqtt && mqtt->isConnected())
            mqtt->publish("status", "Auto OFF EV" + String(s.evId));
        }
      }
    }
    vTaskDelay(500 / portTICK_PERIOD_MS); // every 500ms
  }
}

// ======================================================
//                       BLE INIT
// ======================================================
void startBLE() {
  jsonBT = new JsonBluetooth(DEVICE_NAME.c_str());

  jsonBT->onMessage([](String msg) {
    msg.trim();
    int sPos = msg.indexOf("\"ssid\":\"");
    int pPos = msg.indexOf("\"password\":\"");

    if (sPos != -1 && pPos != -1) {
      sPos += 8;
      int sEnd = msg.indexOf("\"", sPos);
      pPos += 12;
      int pEnd = msg.indexOf("\"", pPos);

      ssid = msg.substring(sPos, sEnd);
      password = msg.substring(pPos, pEnd);

      storage->setSSID(ssid);
      storage->setPassword(password);
      storage->save();

      shouldConnectToWiFi = true;
      vTaskResume(TaskWiFiHandle);
      Serial.println("WiFi credentials saved via BLE");
    }
  });

  jsonBT->onConnect([]() { Serial.println("BLE connected"); });
  jsonBT->onDisconnect([]() { Serial.println("BLE disconnected"); });

  jsonBT->start();
}

// ======================================================
//                       WIFI CONNECT
// ======================================================
void connectWiFi() {
  wifi->setCredentials(ssid.c_str(), password.c_str());
  if (wifi->connect()) {
    wifiConnected = true;
    Serial.print("WiFi connected – IP: ");
    Serial.println(wifi->getIP().c_str());
    storage->save();
  } else {
    wifiConnected = false;
    Serial.println("WiFi failed");
  }
}

// ======================================================
//                     FACTORY RESET
// ======================================================
void factoryReset() {
  Serial.println("FACTORY RESET!");
  storage->clear();
  scheduleManager.clear();
  ESP.restart();
}