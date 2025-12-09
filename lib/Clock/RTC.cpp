#include "RTC.hpp"
#include <time.h>
#include <WiFi.h>

RTC::RTC() {}

bool RTC::begin(const int &sda, const int &scl) {
    Wire.begin(sda,scl);  // Initialize I2C for ESP32
    if (!rtc.begin(&Wire)) {  // Pass Wire pointer
        Serial.println("[RTC] Could not find RTC module");
        return false;
    }
    
    // Check if RTC lost power and set to compile time if needed
    if (rtc.lostPower()) {
        Serial.println("[RTC] RTC lost power, setting time to compile time");
        // This sets the RTC to the date & time this sketch was compiled
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    
    Serial.println("[RTC] RTC initialized successfully");
    return true;
}

bool RTC::isRunning() {
    // Check if RTC is running by reading the time
    // If we can read a valid time, it's running
    DateTime now = rtc.now();
    return now.year() >= 2024;  // Simple check - if year is reasonable
}

void RTC::setDateTime(uint16_t year, uint8_t month, uint8_t day,
                      uint8_t hour, uint8_t minute, uint8_t second) {
    rtc.adjust(DateTime(year, month, day, hour, minute, second));
}

DateTime RTC::now() {
    return rtc.now();
}

bool RTC::isTime(uint8_t hour, uint8_t minute) {
    DateTime t = rtc.now();
    return (t.hour() == hour && t.minute() == minute);
}

bool RTC::isBetween(uint8_t startHour, uint8_t startMinute,
                    uint8_t endHour, uint8_t endMinute) {
    DateTime t = rtc.now();
    uint16_t nowMin   = t.hour() * 60 + t.minute();
    uint16_t startMin = startHour * 60 + startMinute;
    uint16_t endMin   = endHour * 60 + endMinute;

    if (startMin <= endMin)
        return nowMin >= startMin && nowMin <= endMin;

    return nowMin >= startMin || nowMin <= endMin;
}

#if defined(ESP8266) || defined(ESP32)

bool RTC::syncWithNTP(const char* ntpServer, long timeZone, long dst, unsigned long timeoutMs) {
    if (!WiFi.isConnected()) {
        Serial.println("[RTC] WiFi not connected - cannot sync NTP");
        return false;
    }

    // Create NTP client only when needed (saves RAM)
    if (timeClient == nullptr) {
        timeClient = new NTPClient(ntpUDP, ntpServer, timeZone + dst, 60000);
    } else {
        timeClient->setPoolServerName(ntpServer);
        timeClient->setTimeOffset(timeZone + dst);
    }

    Serial.print("[RTC] Requesting time from ");
    Serial.println(ntpServer);

    timeClient->begin();
    
    // Force update with timeout
    unsigned long started = millis();
    while (!timeClient->update() && (millis() - started) < timeoutMs) {
        delay(10);
    }

    if (!timeClient->isTimeSet()) {
        Serial.println("[RTC] Failed to obtain time from NTP server");
        return false;
    }

    // Get the epoch time
    unsigned long epoch = timeClient->getEpochTime();
    
    // Convert to DateTime structure
    // Note: NTPClient returns UTC time
    time_t rawtime = (time_t)epoch;
    struct tm *timeinfo;
    timeinfo = gmtime(&rawtime);

    uint16_t year   = timeinfo->tm_year + 1900;
    uint8_t  month  = timeinfo->tm_mon + 1;
    uint8_t  day    = timeinfo->tm_mday;
    uint8_t  hour   = timeinfo->tm_hour;
    uint8_t  minute = timeinfo->tm_min;
    uint8_t  second = timeinfo->tm_sec;

    Serial.printf("[RTC] Synced - %04d-%02d-%02d %02d:%02d:%02d UTC\n",
                  year, month, day, hour, minute, second);

    // Write to DS3231
    setDateTime(year, month, day, hour, minute, second);

    return true;
}

#endif  // ESP8266 || ESP32