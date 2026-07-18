// RTCManager.h
#ifndef RTC_MANAGER_H
#define RTC_MANAGER_H

#include <Arduino.h>
#include <RTClib.h>

class RTCManager {
public:
    static RTCManager& getInstance();
    
    bool begin();
    bool isRunning() const { return rtcRunning; }
    DateTime now();
    void adjust(const DateTime& dt);
    
    String getFormattedTime();
    String getFormattedDate();
    
private:
    RTCManager() {}
    RTCManager(const RTCManager&) = delete;
    RTCManager& operator=(const RTCManager&) = delete;
    
    RTC_DS3231 rtc;
    bool rtcRunning = false;
};

#endif