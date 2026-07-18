#include "SprayMode.h"
#include "hardware/SensorManager.h"

SprayMode& SprayMode::getInstance() {
    static SprayMode instance;
    return instance;
}

void SprayMode::begin() {
    currentMode = SprayModeType::MANUAL;
    sprayerLeft = false;
    sprayerCenter = false;
    sprayerRight = false;
    intermittentActive = false;
    isOn = false;
    progress = 0;
    accumulatedDistance = 0;
    currentCycleDistance = 0;
    lastUpdate = millis();
    lastStateChange = millis();
    manualOverride = false;
}

void SprayMode::update() {
    switch (currentMode) {
        case SprayModeType::MANUAL:
            updateManualMode();
            break;
        case SprayModeType::INTERMITTENT:
            updateIntermittentMode();
            break;
        case SprayModeType::CONTINUOUS:
            updateContinuousMode();
            break;
    }
    
    // Apply the current state to relays
    applySprayerState();
}

void SprayMode::updateManualMode() {
    // In manual mode, sprayer states are set directly by button presses
    // No automatic updates needed here
}

void SprayMode::updateIntermittentMode() {
    if (!intermittentActive) {
        sprayerLeft = false;
        sprayerCenter = false;
        sprayerRight = false;
        return;
    }
    
    SystemConfig configRef = config.getConfig();
    
    if (configRef.triggerMode == TriggerMode::TIME_BASED) {
        updateTimeBased();
    } else {
        updateDistanceBased();
    }
}

void SprayMode::updateTimeBased() {
    SystemConfig configRef = config.getConfig();
    unsigned long now = millis();
    unsigned long targetInterval = isOn ? configRef.intervalON : configRef.intervalOFF;
    
    if (now - lastUpdate >= targetInterval) {
        lastUpdate = now;
        toggleIntermittentOutput();
    }
    
    unsigned long elapsed = now - lastUpdate;
    progress = (float)elapsed / targetInterval * 100;
    if (progress > 100) progress = 100;
}

void SprayMode::updateDistanceBased() {
    SystemConfig configRef = config.getConfig();
    auto& sensor = SensorManager::getInstance();
    
    float distanceDelta = sensor.getTotalDistance() - accumulatedDistance;
    accumulatedDistance = sensor.getTotalDistance();
    currentCycleDistance += distanceDelta;
    
    float targetDistance = isOn ? configRef.distanceON : configRef.distanceOFF;
    
    if (currentCycleDistance >= targetDistance) {
        currentCycleDistance = 0;
        toggleIntermittentOutput();
    }
    
    progress = (currentCycleDistance / targetDistance) * 100;
    if (progress > 100) progress = 100;
}

void SprayMode::updateContinuousMode() {
    // In continuous mode, sprayers are always on
    // The actual state is managed by setContinuousSprayer() or setContinuousAll()
}

void SprayMode::toggleIntermittentOutput() {
    isOn = !isOn;
}

void SprayMode::applySprayerState() {
    if (currentMode == SprayModeType::INTERMITTENT && intermittentActive) {
        if (!manualOverride) {
            bool outputState = isOn && intermittentActive;
            relays.setRelay(RelayType::SPRAYER_LH, sprayerLeft && outputState);
            relays.setRelay(RelayType::SPRAYER_CENTER, sprayerCenter && outputState);
            relays.setRelay(RelayType::SPRAYER_RH, sprayerRight && outputState);
        }
    } else {
        relays.setRelay(RelayType::SPRAYER_LH, sprayerLeft);
        relays.setRelay(RelayType::SPRAYER_CENTER, sprayerCenter);
        relays.setRelay(RelayType::SPRAYER_RH, sprayerRight);
    }
}

void SprayMode::setMode(SprayModeType mode) {
    if (currentMode == mode) return;
    
    if (currentMode == SprayModeType::INTERMITTENT) {
        intermittentActive = false;
        isOn = false;
        progress = 0;
    }
    
    currentMode = mode;
    
    if (mode == SprayModeType::INTERMITTENT) {
        resetIntermittentCycle();
    }
    
    manualOverride = false;
}

void SprayMode::toggleMode() {
    switch (currentMode) {
        case SprayModeType::MANUAL:
            setMode(SprayModeType::INTERMITTENT);
            break;
        case SprayModeType::INTERMITTENT:
            setMode(SprayModeType::CONTINUOUS);
            break;
        case SprayModeType::CONTINUOUS:
            setMode(SprayModeType::MANUAL);
            break;
    }
}

void SprayMode::setManualSprayer(bool left, bool center, bool right) {
    if (currentMode == SprayModeType::MANUAL) {
        sprayerLeft = left;
        sprayerCenter = center;
        sprayerRight = right;
    } else if (currentMode == SprayModeType::CONTINUOUS) {
        sprayerLeft = left;
        sprayerCenter = center;
        sprayerRight = right;
    } else {
        sprayerLeft = left;
        sprayerCenter = center;
        sprayerRight = right;
        manualOverride = true;
    }
}

void SprayMode::toggleManualSprayer(int channel) {
    switch (channel) {
        case 0:
            sprayerLeft = !sprayerLeft;
            break;
        case 1:
            sprayerCenter = !sprayerCenter;
            break;
        case 2:
            sprayerRight = !sprayerRight;
            break;
        default:
            return;
    }
    
    if (currentMode == SprayModeType::INTERMITTENT) {
        manualOverride = true;
    }
}

void SprayMode::setIntermittentActive(bool active) {
    if (currentMode != SprayModeType::INTERMITTENT) {
        setMode(SprayModeType::INTERMITTENT);
    }
    
    intermittentActive = active;
    if (!active) {
        isOn = false;
        progress = 0;
        sprayerLeft = false;
        sprayerCenter = false;
        sprayerRight = false;
    } else {
        resetIntermittentCycle();
        manualOverride = false;
    }
}

void SprayMode::toggleIntermittent() {
    setIntermittentActive(!intermittentActive);
}

void SprayMode::resetIntermittentCycle() {
    lastUpdate = millis();
    accumulatedDistance = SensorManager::getInstance().getTotalDistance();
    currentCycleDistance = 0;
    isOn = true;
    progress = 0;
}

void SprayMode::setContinuousSprayer(bool left, bool center, bool right) {
    if (currentMode != SprayModeType::CONTINUOUS) {
        setMode(SprayModeType::CONTINUOUS);
    }
    sprayerLeft = left;
    sprayerCenter = center;
    sprayerRight = right;
}

void SprayMode::setContinuousAll(bool state) {
    setContinuousSprayer(state, state, state);
}

void SprayMode::resetAll() {
    currentMode = SprayModeType::MANUAL;
    sprayerLeft = false;
    sprayerCenter = false;
    sprayerRight = false;
    intermittentActive = false;
    isOn = false;
    progress = 0;
    manualOverride = false;
    accumulatedDistance = 0;
    currentCycleDistance = 0;
    relays.allOff();
}

void SprayMode::setSprayerLeft(bool state) {
    if (currentMode == SprayModeType::MANUAL || currentMode == SprayModeType::CONTINUOUS) {
        sprayerLeft = state;
    } else {
        sprayerLeft = state;
        manualOverride = true;
    }
}

void SprayMode::setSprayerCenter(bool state) {
    if (currentMode == SprayModeType::MANUAL || currentMode == SprayModeType::CONTINUOUS) {
        sprayerCenter = state;
    } else {
        sprayerCenter = state;
        manualOverride = true;
    }
}

void SprayMode::setSprayerRight(bool state) {
    if (currentMode == SprayModeType::MANUAL || currentMode == SprayModeType::CONTINUOUS) {
        sprayerRight = state;
    } else {
        sprayerRight = state;
        manualOverride = true;
    }
}