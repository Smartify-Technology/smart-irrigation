#ifndef STORAGE_HPP
#define STORAGE_HPP

#include <Arduino.h>
#include <Preferences.h>

class Storage {
private:
    Preferences prefs;
    String ssid;
    String password;
    String deviceName;

public:
    Storage() = default;
    ~Storage();

    // Getters
    String getSSID() const;
    String getPassword() const;
    String getDeviceName() const;

    // Setters
    void setSSID(const String &ssid);
    void setPassword(const String &password);
    void setDeviceName(const String &name);

    // Persistence
    bool save();
    bool load();
    bool clear();
};

#endif
