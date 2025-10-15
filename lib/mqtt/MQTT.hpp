#ifndef MQTT_HPP
#define MQTT_HPP

#include <Arduino.h>
#include <WiFiClientSecure.h>  // Secure client for TLS
#include <PubSubClient.h>
#include <functional>

class MQTT {
private:
    String broker;
    uint16_t port;
    String clientId;
    String username;
    String password;
    String topicPrefix;

    WiFiClientSecure wifiClient;  // Secure TLS client
    PubSubClient client;
    unsigned long lastReconnectAttempt = 0;

    std::function<void(String, String)> messageCallback;
    const char* root_ca;  // Root CA certificate

    bool reconnect();

public:
    MQTT(String broker, uint16_t port, const char* rootCA);

    void setCredentials(String username, String password);
    void setTopicPrefix(String prefix);
    void setCallback(std::function<void(String, String)> cb);

    bool connect();
    void disconnect();
    bool publish(String topic, String payload, bool retain = false);
    bool subscribe(String topic);
    void loop();
    bool isConnected();
};

#endif