#include "MQTT.hpp"

MQTT::MQTT(String broker, uint16_t port, const char* rootCA)
    : broker(broker), port(port), client(wifiClient), root_ca(rootCA) {
    clientId = "ESP32-" + String(random(0xffff), HEX);
    wifiClient.setCACert(root_ca);  // Set the root CA for TLS validation
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

    Serial.printf("Connecting to MQTT broker %s:%d (TLS)...\n", broker.c_str(), port);

    String willTopic = topicPrefix.isEmpty() ? "status" : topicPrefix + "/status";
    const char* user = username.isEmpty() ? nullptr : username.c_str();
    const char* pass = password.isEmpty() ? nullptr : password.c_str();

    bool connected = client.connect(clientId.c_str(), user, pass, willTopic.c_str(), 0, true, "offline");

    if (connected) {
        Serial.println("MQTT connected!");
        client.publish(willTopic.c_str(), "online", true);
        lastReconnectAttempt = 0;
        return true;
    } else {
        Serial.printf("MQTT connection failed, rc=%d\n", client.state());
        return false;
    }
}

void MQTT::disconnect() {
    if (client.connected()) {
        String willTopic = topicPrefix.isEmpty() ? "status" : topicPrefix + "/status";
        client.publish(willTopic.c_str(), "offline", true);
        client.disconnect();
        Serial.println("MQTT disconnected.");
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