#ifndef WIFI_HPP
#define WIFI_HPP

#include <vector>
#include <string>
#include <functional>

class Wifi {
public:
    // Connection status enumeration
    enum class Status {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        CONNECTION_FAILED,
        SCANNING
    };

    // Constructor & Destructor
    Wifi();
    ~Wifi();

    // Credential management
    void setCredentials(const std::string& ssid, const std::string& password);

    // Device name management
    void setDeviceName(const std::string& name);
    std::string getDeviceName() const;

    // Connection management
    bool connect();
    void disconnect();
    bool reconnect();

    // Network scanning
    std::vector<std::string> scanNetworks();

    // Status information
    std::string getStatus() const;
    std::string getIP() const;
    int getSignalStrength() const;
    bool isConnected() const;

    // Event callbacks
    void onConnect(std::function<void()> callback);
    void onDisconnect(std::function<void()> callback);

private:
    std::string ssid;
    std::string password;
    std::string deviceName;
    bool connected;
    Status status;

    // Callbacks
    std::function<void()> connectCallback;
    std::function<void()> disconnectCallback;

    // Internal helpers
    bool validateCredentials() const;
    void updateStatus(Status newStatus);
    void notifyConnect();
    void notifyDisconnect();
};

#endif // WIFI_HPP
