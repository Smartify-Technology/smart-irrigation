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
    bool reset;

public:
    Storage() = default;
    ~Storage();

    // Getters
    String getSSID() const;
    String getPassword() const;
    String getDeviceName() const;
    bool getReset() const;

    // Setters
    void setSSID(const String &ssid);
    void setPassword(const String &password);
    void setDeviceName(const String &name);
    void setReset(const bool &reset);

    // Persistence
    bool save();
    bool load();
    bool clear();
};

#endif
