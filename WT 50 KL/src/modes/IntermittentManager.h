// IntermittentManager.h
#ifndef INTERMITTENT_MANAGER_H
#define INTERMITTENT_MANAGER_H

#include <Arduino.h>
#include "config/ConfigManager.h"

class IntermittentManager {
public:
    static IntermittentManager& getInstance();
    
    void begin();
    void update();
    
    bool isActive() const { return autoMode; }
    void setAutoMode(bool active);
    void toggleAutoMode();
    
    void resetCycle();
    float getProgress() const { return progress; }
    bool isSprayerOn() const { return isOn; }
    
    void setSprayerStates(bool left, bool center, bool right);
    bool getSprayerLeft() const { return sprayerLeft; }
    bool getSprayerCenter() const { return sprayerCenter; }
    bool getSprayerRight() const { return sprayerRight; }
    
private:
    IntermittentManager() {}
    IntermittentManager(const IntermittentManager&) = delete;
    IntermittentManager& operator=(const IntermittentManager&) = delete;
    
    bool autoMode = false;
    bool isOn = false;
    float progress = 0;
    
    bool sprayerLeft = false;
    bool sprayerCenter = false;
    bool sprayerRight = false;
    
    unsigned long lastUpdate = 0;
    float accumulatedDistance = 0;
    float currentCycleDistance = 0;
    
    void updateTimeBased();
    void updateDistanceBased();
    void toggleOutput();
};

#endif