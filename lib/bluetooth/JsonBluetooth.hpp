#ifndef JSON_BLUETOOTH_HPP
#define JSON_BLUETOOTH_HPP

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <functional>
#include <string>

class JsonBluetooth {
private:
    // Use standard BLE service UUIDs for better compatibility
    const char* SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"; // Nordic UART Service
    const char* CHAR_TX_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"; // TX - Notify
    const char* CHAR_RX_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"; // RX - Write
    // BLE Components
    BLEServer* pServer;
    BLEService* pService;
    BLECharacteristic* pTxCharacteristic;
    BLECharacteristic* pRxCharacteristic;
    
    // Device configuration
    String deviceName;
    bool connected;
    bool advertising;
    bool stopped;
    

    // Callbacks
    std::function<void(String json)> messageCallback;
    std::function<void()> connectCallback;
    std::function<void()> disconnectCallback;
    
    // BLE Callback classes
    class ServerCallbacks : public BLEServerCallbacks {
    private:
        JsonBluetooth* parent;
    public:
        ServerCallbacks(JsonBluetooth* p) : parent(p) {}
        
        void onConnect(BLEServer* pServer) {
            parent->connected = true;
            if (parent->connectCallback) {
                parent->connectCallback();
            }
            Serial.println("JSON Bluetooth client connected");
        }
        
        void onDisconnect(BLEServer* pServer) {
            parent->connected = false;
            if (parent->disconnectCallback) {
                parent->disconnectCallback();
            }
            Serial.println("JSON Bluetooth client disconnected");
            // Restart advertising to allow reconnections
            pServer->getAdvertising()->start();
            parent->advertising = true;
        }
    };
    
    class CharacteristicCallbacks : public BLECharacteristicCallbacks {
    private:
        JsonBluetooth* parent;
    public:
        CharacteristicCallbacks(JsonBluetooth* p) : parent(p) {}
        
        void onWrite(BLECharacteristic* pCharacteristic) {
            std::string value = pCharacteristic->getValue();
            if (value.length() > 0 && parent->messageCallback) {
                parent->messageCallback(String(value.c_str()));
            }
        }
    };

public:
    // Constructor
    JsonBluetooth(String name);
    // Destructor
    ~JsonBluetooth();
    
    // Core methods
    bool start();
    void stop();
    bool send(String json);
    bool isConnected();
    bool isStopped();
    
    // Callback setters
    void onMessage(std::function<void(String json)> callback);
    void onConnect(std::function<void()> callback);
    void onDisconnect(std::function<void()> callback);
    
    // Utility methods
    void disconnect();
    void update();
    
private:
    bool initializeBLE();
    void setupService();
};

#endif // JSON_BLUETOOTH_HPP