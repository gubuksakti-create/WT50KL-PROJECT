#include "IntermittentManager.h"
#include "hardware/SensorManager.h"
#include "hardware/RelayManager.h"

IntermittentManager& IntermittentManager::getInstance() {
    static IntermittentManager instance;
    return instance;
}

void IntermittentManager::begin() {
    autoMode = false;
    isOn = false;
    progress = 0;
    accumulatedDistance = 0;
    currentCycleDistance = 0;
}

void IntermittentManager::update() {
    if (!autoMode) {
        if (isOn) {
            isOn = false;
            toggleOutput();
        }
        return;
    }
    
    SystemConfig config = ConfigManager::getInstance().getConfig();
    
    if (config.triggerMode == TriggerMode::TIME_BASED) {
        updateTimeBased();
    } else {
        updateDistanceBased();
    }
}

void IntermittentManager::updateTimeBased() {
    SystemConfig config = ConfigManager::getInstance().getConfig();
    unsigned long now = millis();
    unsigned long targetInterval = isOn ? config.intervalON : config.intervalOFF;
    
    if (now - lastUpdate >= targetInterval) {
        lastUpdate = now;
        isOn = !isOn;
        toggleOutput();
    }
    
    unsigned long elapsed = now - lastUpdate;
    progress = (float)elapsed / targetInterval * 100;
    if (progress > 100) progress = 100;
}

void IntermittentManager::updateDistanceBased() {
    SystemConfig config = ConfigManager::getInstance().getConfig();
    auto& sensor = SensorManager::getInstance();
    
    float distanceDelta = sensor.getTotalDistance() - accumulatedDistance;
    accumulatedDistance = sensor.getTotalDistance();
    currentCycleDistance += distanceDelta;
    
    float targetDistance = isOn ? config.distanceON : config.distanceOFF;
    
    if (currentCycleDistance >= targetDistance) {
        currentCycleDistance = 0;
        isOn = !isOn;
        toggleOutput();
    }
    
    progress = (currentCycleDistance / targetDistance) * 100;
    if (progress > 100) progress = 100;
}

void IntermittentManager::toggleOutput() {
    auto& relay = RelayManager::getInstance();
    SystemConfig config = ConfigManager::getInstance().getConfig();
    
    if (config.intermittentMode == IntermittentMode::SPRAYER) {
        if (sprayerLeft) relay.setRelay(RelayType::SPRAYER_LH, isOn);
        if (sprayerCenter) relay.setRelay(RelayType::SPRAYER_CENTER, isOn);
        if (sprayerRight) relay.setRelay(RelayType::SPRAYER_RH, isOn);
    } else {
        relay.setRelay(RelayType::WATER_PUMP, isOn);
    }
}

void IntermittentManager::setAutoMode(bool active) {
    autoMode = active;
    if (!autoMode) {
        isOn = false;
        toggleOutput();
    } else {
        resetCycle();
    }
}

void IntermittentManager::toggleAutoMode() {
    setAutoMode(!autoMode);
}

void IntermittentManager::resetCycle() {
    lastUpdate = millis();
    accumulatedDistance = SensorManager::getInstance().getTotalDistance();
    currentCycleDistance = 0;
    isOn = true;
    toggleOutput();
}

void IntermittentManager::setSprayerStates(bool left, bool center, bool right) {
    sprayerLeft = left;
    sprayerCenter = center;
    sprayerRight = right;
    
    if (autoMode && ConfigManager::getInstance().getConfig().intermittentMode == IntermittentMode::SPRAYER) {
        auto& relay = RelayManager::getInstance();
        relay.setRelay(RelayType::SPRAYER_LH, isOn && sprayerLeft);
        relay.setRelay(RelayType::SPRAYER_CENTER, isOn && sprayerCenter);
        relay.setRelay(RelayType::SPRAYER_RH, isOn && sprayerRight);
    }
}