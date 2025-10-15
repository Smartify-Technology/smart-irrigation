// // Place this file in the main project directory: src/main.cpp then compile and upload to test JsonBluetooth library.

// #include <Arduino.h>
// #include "JsonBluetooth.hpp"

// int LED_BUILTIN = 2;

// JsonBluetooth jsonBT("ESP32-LED-Control");

// void setup() {
//   Serial.begin(115200);
//   delay(1000);

//   pinMode(LED_BUILTIN, OUTPUT);
//   digitalWrite(LED_BUILTIN, LOW); // Start with LED off
  
//   Serial.println("Starting BLE LED Controller...");
//   Serial.println("Send 'ON' or 'OFF' to control the LED");

//   jsonBT.onMessage([](String message) {
//     Serial.print("Raw received: '");
//     Serial.print(message);
//     Serial.println("'");
    
//     // Trim and convert to uppercase for reliable comparison
//     message.trim();
//     message.toUpperCase();
    
//     Serial.print("Processed: '");
//     Serial.print(message);
//     Serial.println("'");

//     if (message == "ON") {
//       digitalWrite(LED_BUILTIN, HIGH);
//       jsonBT.send("{\"status\":\"LED is ON\", \"command\":\"ON\"}");
//       Serial.println("LED turned ON");
//     } else if (message == "OFF") {
//       digitalWrite(LED_BUILTIN, LOW);
//       jsonBT.send("{\"status\":\"LED is OFF\", \"command\":\"OFF\"}");
//       Serial.println("LED turned OFF");
//     } else if (message == "STATUS") {
//       String ledState = digitalRead(LED_BUILTIN) ? "ON" : "OFF";
//       jsonBT.send("{\"status\":\"Current state\", \"led\":\"" + ledState + "\", \"uptime\":" + String(millis()) + "}");
//     } else {
//       jsonBT.send("{\"error\":\"Unknown command\", \"received\":\"" + message + "\", \"try\":\"ON, OFF, or STATUS\"}");
//       Serial.println("Unknown command received: " + message);
//     }
//   });

//   jsonBT.onConnect([]() {
//     Serial.println("✅ Phone connected!");
//     digitalWrite(LED_BUILTIN, HIGH); // Visual indicator
//     delay(500);
//     digitalWrite(LED_BUILTIN, LOW);
//   });

//   jsonBT.onDisconnect([]() {
//     Serial.println("❌ Phone disconnected!");
//     digitalWrite(LED_BUILTIN, LOW); // Ensure LED is off when disconnected
//   });

//   if (jsonBT.start()) {
//     Serial.println("✅ BLE started successfully!");
//     Serial.println("📱 Device name: ESP32-LED-Control");
//     Serial.println("🔗 Waiting for connections...");
    
//     // Blink LED to show ready state
//     for(int i = 0; i < 3; i++) {
//       digitalWrite(LED_BUILTIN, HIGH);
//       delay(200);
//       digitalWrite(LED_BUILTIN, LOW);
//       delay(200);
//     }
//   } else {
//     Serial.println("❌ BLE start failed!");
//     while(1) {
//       digitalWrite(LED_BUILTIN, HIGH);
//       delay(1000);
//       digitalWrite(LED_BUILTIN, LOW);
//       delay(1000);
//     }
//   }
// }

// void loop() {
//   jsonBT.update(); // Important: Keep this for BLE maintenance
  
//   // Optional: Send periodic status if connected
//   static unsigned long lastStatus = 0;
//   if (jsonBT.isConnected() && millis() - lastStatus > 10000) { // Every 10 seconds
//     String ledState = digitalRead(LED_BUILTIN) ? "ON" : "OFF";
//     jsonBT.send("{\"type\":\"heartbeat\", \"led\":\"" + ledState + "\", \"uptime\":" + String(millis()) + "}");
//     lastStatus = millis();
//   }
  
//   delay(100);
// }