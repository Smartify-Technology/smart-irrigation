#ifndef MQTT_HPP
#define MQTT_HPP

#include <Arduino.h>
#include <WiFiClient.h>
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

    WiFiClient wifiClient;
    PubSubClient client;
    unsigned long lastReconnectAttempt = 0;

    std::function<void(String, String)> messageCallback;

    bool reconnect();

public:
    MQTT();
    MQTT(String broker, uint16_t port);

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
