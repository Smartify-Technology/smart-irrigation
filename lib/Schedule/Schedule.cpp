#include "Schedule.hpp"
#include <Preferences.h>
#include <ArduinoJson.h>

ScheduleManager scheduleManager;  // Global instance

// ====================== LOAD ======================
bool ScheduleManager::load() {
  Preferences prefs;
  if (!prefs.begin("schedules", true)) {  // readonly
    Serial.println("Failed to open NVS 'schedules' for reading");
    return false;
  }

  String json = prefs.getString("data", "");
  prefs.end();

  if (json.length() == 0) {
    Serial.println("No schedule data found in storage");
    return false;
  }

  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, json);
  if (err) {
    Serial.printf("JSON deserialize failed: %s\n", err.c_str());
    return false;
  }

  schedules.clear();
  JsonArray array = doc.as<JsonArray>();
  for (JsonObject obj : array) {
    IrrigationSchedule s;
    s.evId        = obj["ev"] | 0;
    s.startHour   = obj["sh"] | 0;
    s.startMinute = obj["sm"] | 0;
    s.endHour     = obj["eh"] | 0;
    s.endMinute   = obj["em"] | 0;
    s.enabled     = obj["en"] | true;

    if (s.evId >= 1 && s.evId <= 12) {
      schedules.push_back(s);
    }
  }

  Serial.printf("Loaded %d irrigation schedules\n", schedules.size());
  return true;
}

// ====================== SAVE ======================
bool ScheduleManager::save() {
  DynamicJsonDocument doc(2048);
  JsonArray array = doc.to<JsonArray>();

  for (const auto& s : schedules) {
    JsonObject obj = array.createNestedObject();
    obj["ev"] = s.evId;
    obj["sh"] = s.startHour;
    obj["sm"] = s.startMinute;
    obj["eh"] = s.endHour;
    obj["em"] = s.endMinute;
    obj["en"] = s.enabled;
  }

  String output;
  serializeJson(doc, output);

  Preferences prefs;
  if (!prefs.begin("schedules", false)) {
    Serial.println("Failed to open NVS 'schedules' for writing");
    return false;
  }

  bool ok = prefs.putString("data", output) > 0;
  prefs.end();

  if (ok) {
    Serial.println("Schedules saved to NVS");
  } else {
    Serial.println("Failed to save schedules");
  }
  return ok;
}

// ====================== ADD / UPDATE ======================
void ScheduleManager::addOrUpdate(const IrrigationSchedule& sched) {
  for (auto& s : schedules) {
    if (s.evId == sched.evId) {
      s = sched;
      save();  // Auto-save on update
      return;
    }
  }
  schedules.push_back(sched);
  save();  // Auto-save on add
}

// ====================== CLEAR ======================
void ScheduleManager::clear() {
  schedules.clear();
  Preferences prefs;
  prefs.begin("schedules", false);
  prefs.clear();
  prefs.end();
}

// ====================== TO JSON (for debugging or sending) ======================
String ScheduleManager::toJson() const {
  DynamicJsonDocument doc(2048);
  JsonArray array = doc.to<JsonArray>();

  for (const auto& s : schedules) {
    JsonObject obj = array.createNestedObject();
    obj["ev"] = s.evId;

    char start[6];
    char end[6];
    sprintf(start, "%02d:%02d", s.startHour, s.startMinute);
    sprintf(end,   "%02d:%02d", s.endHour,   s.endMinute);

    obj["start"] = start;
    obj["end"]   = end;
    obj["enabled"] = s.enabled;
  }

  String result;
  serializeJson(doc, result);
  return result;
}

// ====================== REMOVE (optional) ======================
bool ScheduleManager::remove(uint8_t evId) {
  for (auto it = schedules.begin(); it != schedules.end(); ++it) {
    if (it->evId == evId) {
      schedules.erase(it);
      save();
      return true;
    }
  }
  return false;
}