// #include <Arduino.h>
// #include "Wifi.hpp"

// void setup() {
//     // Initialize serial communication at 9600 bits per second:
//     Serial.begin(115200);
//     delay(2000);

//     Wifi wifi;

//     Serial.println("getting available networks...");
//     auto networks = wifi.scanNetworks();
//     for (const auto& net : networks) {
//         Serial.println(net.c_str());
//     }
//     Serial.println("done.");

//     wifi.setDeviceName("MyESP32");
//     Serial.println("Device name: " + String(wifi.getDeviceName().c_str()));

//     Serial.println("status: " + String(wifi.getStatus().c_str()));

//     Serial.println("connecting to wifi...");
//     wifi.setCredentials("Dlink23", "1234567890");
//     if (wifi.connect()) {
//         Serial.println("connected!");
//         Serial.println("IP address: " + String(wifi.getIP().c_str()));
//         Serial.println("signal strength: " + String(wifi.getSignalStrength()));
//     } else {
//         Serial.println("connection failed.");
//     }

//     Serial.println("Disconnecting...");
//     wifi.disconnect();
//     Serial.println("status: " + String(wifi.getStatus().c_str()));
//     Serial.println("done.");

//     while (1)
//     {
//         delay(3000);
//         Serial.println("status: " + String(wifi.getStatus().c_str()));
//         if (!wifi.isConnected()) {
//             Serial.println("reconnecting...");
//             if (wifi.reconnect()) {
//                 Serial.println("reconnected!");
//                 Serial.println("IP address: " + String(wifi.getIP().c_str()));
//                 Serial.println("signal strength: " + String(wifi.getSignalStrength()));
//             } else {
//                 Serial.println("reconnection failed.");
//             }
//         }
//     }
    
// }

// void loop() {
// }