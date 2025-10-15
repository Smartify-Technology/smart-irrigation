#include "MQTT.hpp"

MQTT::MQTT() : broker(""), port(1883), client(wifiClient) {}

MQTT::MQTT(String broker, uint16_t port)
    : broker(broker), port(port), client(wifiClient) {
    clientId = "ESP32-" + String(random(0xFFFF), HEX);
}

void MQTT::setCredentials(String user, String pass) {
    username = user;
    password = pass;
}

void MQTT::setTopicPrefix(String prefix) {
    topicPrefix = prefix;
}

void MQTT::setCallback(std::function<void(String, String)> cb) {
    messageCallback = cb;
    client.setCallback([this](char* topic, byte* payload, unsigned int length) {
        String msg;
        for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
        if (messageCallback) messageCallback(String(topic), msg);
    });
}

bool MQTT::connect() {
    client.setServer(broker.c_str(), port);
    if (client.connected()) return true;

    Serial.printf("🔗 Connecting to MQTT broker %s:%d...\n", broker.c_str(), port);

    if (username.isEmpty())
        client.connect(clientId.c_str());
    else
        client.connect(clientId.c_str(), username.c_str(), password.c_str());

    if (client.connected()) {
        Serial.println("✅ MQTT connected!");
        lastReconnectAttempt = 0;
        return true;
    } else {
        Serial.println("❌ MQTT connection failed.");
        return false;
    }
}

void MQTT::disconnect() {
    if (client.connected()) {
        client.disconnect();
        Serial.println("🔌 MQTT disconnected.");
    }
}

bool MQTT::reconnect() {
    if (millis() - lastReconnectAttempt < 5000) return false;
    lastReconnectAttempt = millis();
    return connect();
}

bool MQTT::publish(String topic, String payload, bool retain) {
    if (!client.connected()) return false;
    String fullTopic = topicPrefix.isEmpty() ? topic : topicPrefix + "/" + topic;
    return client.publish(fullTopic.c_str(), payload.c_str(), retain);
}

bool MQTT::subscribe(String topic) {
    if (!client.connected()) return false;
    String fullTopic = topicPrefix.isEmpty() ? topic : topicPrefix + "/" + topic;
    return client.subscribe(fullTopic.c_str());
}

void MQTT::loop() {
    if (!client.connected()) reconnect();
    client.loop();
}

bool MQTT::isConnected() {
    return client.connected();
}
