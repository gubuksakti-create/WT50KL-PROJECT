// RTCManager.cpp
#include "RTCManager.h"

RTCManager& RTCManager::getInstance() {
    static RTCManager instance;
    return instance;
}

bool RTCManager::begin() {
    if (!rtc.begin()) {
        rtcRunning = false;
        return false;
    }
    
    if (rtc.lostPower()) {
        // Set to compile time if power lost
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    rtcRunning = true;
    return true;
}

DateTime RTCManager::now() {
    if (rtcRunning) {
        return rtc.now();
    }
    return DateTime(2024, 1, 1, 0, 0, 0);
}

String RTCManager::getFormattedTime() {
    DateTime dt = now();
    char buffer[9];
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", dt.hour(), dt.minute(), dt.second());
    return String(buffer);
}

String RTCManager::getFormattedDate() {
    DateTime dt = now();
    char buffer[11];
    snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d", dt.day(), dt.month(), dt.year());
    return String(buffer);
}