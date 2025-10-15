#include "Wifi.hpp"
#include <WiFi.h>
#include <Arduino.h>

Wifi::Wifi() : connected(false), status(Status::DISCONNECTED) {
    WiFi.mode(WIFI_STA);
}

Wifi::~Wifi() {
    disconnect();
}

void Wifi::setCredentials(const std::string& ssid, const std::string& password) {
    this->ssid = ssid;
    this->password = password;
}

void Wifi::setDeviceName(const std::string& name) {
    deviceName = name;

#ifdef ESP32
    WiFi.setHostname(deviceName.c_str());
#elif defined(ESP8266)
    WiFi.hostname(deviceName.c_str());
#endif
}

std::string Wifi::getDeviceName() const {
    return deviceName;
}

bool Wifi::connect() {
    if (!validateCredentials()) {
        return false;
    }

    updateStatus(Status::CONNECTING);

    WiFi.begin(ssid.c_str(), password.c_str());

    if (!deviceName.empty()) {
#ifdef ESP32
        WiFi.setHostname(deviceName.c_str());
#elif defined(ESP8266)
        WiFi.hostname(deviceName.c_str());
#endif
    }

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
        delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
        connected = true;
        updateStatus(Status::CONNECTED);
        notifyConnect();
        return true;
    } else {
        connected = false;
        updateStatus(Status::CONNECTION_FAILED);
        return false;
    }
}

void Wifi::disconnect() {
    WiFi.disconnect();
    connected = false;
    updateStatus(Status::DISCONNECTED);
    notifyDisconnect();
    delay(100);
}

bool Wifi::reconnect() {
    disconnect();
    delay(1000);
    return connect();
}

std::vector<std::string> Wifi::scanNetworks() {
    updateStatus(Status::SCANNING);

    std::vector<std::string> networks;
    int numNetworks = WiFi.scanNetworks();

    if (numNetworks == 0) {
        return networks;
    }

    for (int i = 0; i < numNetworks; i++) {
        networks.push_back(std::string(WiFi.SSID(i).c_str()));
    }

    WiFi.scanDelete();
    updateStatus(connected ? Status::CONNECTED : Status::DISCONNECTED);

    return networks;
}

std::string Wifi::getStatus() const {
    switch (status) {
        case Status::DISCONNECTED: return "Disconnected";
        case Status::CONNECTING: return "Connecting";
        case Status::CONNECTED: return "Connected";
        case Status::CONNECTION_FAILED: return "Connection Failed";
        case Status::SCANNING: return "Scanning";
        default: return "Unknown";
    }
}

std::string Wifi::getIP() const {
    if (connected) {
        return std::string(WiFi.localIP().toString().c_str());
    }
    return "0.0.0.0";
}

int Wifi::getSignalStrength() const {
    if (connected) {
        return WiFi.RSSI();
    }
    return 0;
}

bool Wifi::isConnected() const {
    return connected && (WiFi.status() == WL_CONNECTED);
}

void Wifi::onConnect(std::function<void()> callback) {
    connectCallback = callback;
}

void Wifi::onDisconnect(std::function<void()> callback) {
    disconnectCallback = callback;
}

bool Wifi::validateCredentials() const {
    return !ssid.empty() && !password.empty();
}

void Wifi::updateStatus(Status newStatus) {
    status = newStatus;
}

void Wifi::notifyConnect() {
    if (connectCallback) {
        connectCallback();
    }
}

void Wifi::notifyDisconnect() {
    if (disconnectCallback) {
        disconnectCallback();
    }
}
