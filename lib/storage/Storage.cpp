#include "Storage.hpp"

Storage::~Storage() {
    prefs.end();
}

// --- Getters ---
String Storage::getSSID() const { return ssid; }
String Storage::getPassword() const { return password; }
String Storage::getDeviceName() const { return deviceName; }

// --- Setters ---
void Storage::setSSID(const String &s) { ssid = s; }
void Storage::setPassword(const String &p) { password = p; }
void Storage::setDeviceName(const String &n) { deviceName = n; }

// --- Save ---
bool Storage::save() {
    if (!prefs.begin("wifi", false)) return false;

    prefs.putString("ssid", ssid);
    prefs.putString("password", password);
    prefs.putString("deviceName", deviceName);

    prefs.end();
    return true;
}

// --- Load ---
bool Storage::load() {
    if (!prefs.begin("wifi", true)) return false;

    ssid = prefs.getString("ssid", "");
    password = prefs.getString("password", "");
    deviceName = prefs.getString("deviceName", "");

    prefs.end();
    return !(ssid.isEmpty() || password.isEmpty() || deviceName.isEmpty());
}

// --- Clear ---
bool Storage::clear() {
    if (!prefs.begin("wifi", false)) return false;
    prefs.clear();
    prefs.end();
    ssid = password = deviceName = "";
    return true;
}
