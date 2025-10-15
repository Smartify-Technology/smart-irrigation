#include "JsonBluetooth.hpp"

JsonBluetooth::JsonBluetooth(String name) 
    : deviceName(name), 
      connected(false), 
      advertising(false),
      pServer(nullptr),
      pService(nullptr),
      pTxCharacteristic(nullptr),
      pRxCharacteristic(nullptr) {
}

bool JsonBluetooth::start() {
    Serial.println("Starting JSON Bluetooth...");
    
    if (!initializeBLE()) {
        Serial.println("Failed to initialize BLE");
        return false;
    }
    
    setupService();
    
    // Start advertising
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // Help with iPhone connections
    pAdvertising->setMinPreferred(0x12);
    
    BLEDevice::startAdvertising();
    advertising = true;
    
    Serial.println("JSON Bluetooth started successfully");
    Serial.println("Device name: " + deviceName);
    Serial.println("Waiting for connections...");
    
    return true;
}

bool JsonBluetooth::initializeBLE() {
    try {
        // Initialize BLE device
        BLEDevice::init(deviceName.c_str());
        
        // Create BLE Server
        pServer = BLEDevice::createServer();
        pServer->setCallbacks(new ServerCallbacks(this));
        
        return true;
    } catch (...) {
        return false;
    }
}

void JsonBluetooth::setupService() {
    // Create BLE Service
    pService = pServer->createService(SERVICE_UUID);
    
    // Create TX Characteristic - Add WRITE property so apps can discover it
    pTxCharacteristic = pService->createCharacteristic(
        CHAR_TX_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_INDICATE
    );
    pTxCharacteristic->addDescriptor(new BLE2902());
    
    // Create RX Characteristic
    pRxCharacteristic = pService->createCharacteristic(
        CHAR_RX_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pRxCharacteristic->setCallbacks(new CharacteristicCallbacks(this));
    
    // Start the service
    pService->start();
}

bool JsonBluetooth::send(String json) {
    if (!connected || !pTxCharacteristic) {
        return false;
    }
    
    pTxCharacteristic->setValue(json.c_str());
    pTxCharacteristic->indicate(); // Use indicate instead of notify
    
    Serial.println("Sent: " + json);
    return true;
}

bool JsonBluetooth::isConnected() {
    return connected;
}

void JsonBluetooth::onMessage(std::function<void(String json)> callback) {
    messageCallback = callback;
}

void JsonBluetooth::onConnect(std::function<void()> callback) {
    connectCallback = callback;
}

void JsonBluetooth::onDisconnect(std::function<void()> callback) {
    disconnectCallback = callback;
}

void JsonBluetooth::disconnect() {
    if (pServer && connected) {
        pServer->disconnect(0); // Disconnect all clients
    }
}

void JsonBluetooth::update() {
    // Handle any periodic updates if needed
    // For example, restart advertising if not connected and not advertising
    if (!connected && !advertising) {
        BLEDevice::startAdvertising();
        advertising = true;
    }
}

void JsonBluetooth::stop() {
    BLEDevice::deinit(true);
}

JsonBluetooth::~JsonBluetooth() {
    stop();
}