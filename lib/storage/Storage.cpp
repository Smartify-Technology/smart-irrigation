#include "Storage.hpp"

Storage::~Storage() {
    prefs.end();
}

// --- Getters ---
String Storage::getSSID() const { return ssid; }
String Storage::getPassword() const { return password; }
String Storage::getDeviceName() const { return deviceName; }
String Storage::getMode() const { return operationMode; }

// --- Setters ---
void Storage::setSSID(const String &s) { ssid = s; }
void Storage::setPassword(const String &p) { password = p; }
void Storage::setDeviceName(const String &n) { deviceName = n; }
void Storage::setMode(const String &mode) { 
  operationMode = (mode == "auto") ? "auto" : "manual"; 
  Preferences p; 
  p.begin("config", false); 
  p.putString("mode", operationMode); 
  p.end();
}

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

    // Load mode from separate namespace
    Preferences p; 
    p.begin("config", true);
    operationMode = p.getString("mode", "manual");
    p.end();

    return !(ssid.isEmpty() || password.isEmpty());
}

// --- Clear ---
bool Storage::clear() {
    if (!prefs.begin("wifi", false)) return false;
    prefs.clear();
    prefs.end();

    Preferences p; 
    p.begin("config", false);
    p.clear();
    p.end();

    ssid = password = deviceName = "";
    operationMode = "manual";
    return true;
}