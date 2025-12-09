#ifndef RTC_HPP
#define RTC_HPP

#include <Wire.h>
#include <RTClib.h>  // Use RTClib which is more standard

#if defined(ESP8266) || defined(ESP32)
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <WiFi.h>
#endif

class RTC {
public:
    RTC();

    bool begin(const int &sda, const int &scl);

    // Existing methods
    bool isRunning();
    void setDateTime(uint16_t year, uint8_t month, uint8_t day,
                     uint8_t hour, uint8_t minute, uint8_t second);
    DateTime now();

    bool isTime(uint8_t hour, uint8_t minute);
    bool isBetween(uint8_t startHour, uint8_t startMinute,
                   uint8_t endHour, uint8_t endMinute);

#if defined(ESP8266) || defined(ESP32)
    bool syncWithNTP(const char* ntpServer = "pool.ntp.org",
                     long timeZone = 0,
                     long dst = 0,
                     unsigned long timeoutMs = 5000);
#endif

private:
    RTC_DS3231 rtc;

#if defined(ESP8266) || defined(ESP32)
    WiFiUDP ntpUDP;
    NTPClient* timeClient = nullptr;
#endif
};

#endif