#ifndef SPRAY_MODE_H
#define SPRAY_MODE_H

#include <Arduino.h>
#include "config/ConfigManager.h"
#include "hardware/RelayManager.h"

enum class SprayModeType {
    MANUAL,
    INTERMITTENT,
    CONTINUOUS
};

class SprayMode {
public:
    static SprayMode& getInstance();
    
    void begin();
    void update();
    
    // Mode management
    SprayModeType getCurrentMode() const { return currentMode; }
    void setMode(SprayModeType mode);
    void toggleMode();
    
    // Manual mode control
    void setManualSprayer(bool left, bool center, bool right);
    void toggleManualSprayer(int channel);
    
    // Intermittent mode control
    bool isIntermittentActive() const { return intermittentActive; }
    void setIntermittentActive(bool active);
    void toggleIntermittent();
    float getIntermittentProgress() const { return progress; }
    bool isIntermittentOn() const { return isOn; }
    
    // Continuous mode control
    void setContinuousSprayer(bool left, bool center, bool right);
    void setContinuousAll(bool state);
    
    // Get current sprayer states
    bool getSprayerLeft() const { return sprayerLeft; }
    bool getSprayerCenter() const { return sprayerCenter; }
    bool getSprayerRight() const { return sprayerRight; }
    
    // Individual setter
    void setSprayerLeft(bool state);
    void setSprayerCenter(bool state);
    void setSprayerRight(bool state);
    
    // Reset
    void resetAll();
    
private:
    SprayMode() {}
    SprayMode(const SprayMode&) = delete;
    SprayMode& operator=(const SprayMode&) = delete;
    
    SprayModeType currentMode = SprayModeType::MANUAL;
    
    bool sprayerLeft = false;
    bool sprayerCenter = false;
    bool sprayerRight = false;
    
    bool intermittentActive = false;
    bool isOn = false;
    float progress = 0;
    unsigned long lastUpdate = 0;
    float accumulatedDistance = 0;
    float currentCycleDistance = 0;
    
    bool manualOverride = false;
    unsigned long lastStateChange = 0;
    
    void updateManualMode();
    void updateIntermittentMode();
    void updateContinuousMode();
    void applySprayerState();
    void toggleIntermittentOutput();
    void resetIntermittentCycle();
    void updateTimeBased();
    void updateDistanceBased();
    
    RelayManager& relays = RelayManager::getInstance();
    ConfigManager& config = ConfigManager::getInstance();
};

#endif