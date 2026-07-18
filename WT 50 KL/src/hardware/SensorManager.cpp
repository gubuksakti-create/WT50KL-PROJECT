#include "SensorManager.h"

SensorManager* SensorManager::instance = nullptr;

SensorManager& SensorManager::getInstance() {
    static SensorManager manager;
    return manager;
}

void SensorManager::begin() {
    instance = this;
    pinMode(PIN_SPEED_SENSOR, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_SPEED_SENSOR), pulseISR, FALLING);
    lastSpeedCalc = millis();
    
    // Setup engine detection
    pinMode(enginePin, INPUT_PULLUP);
    lastEngineState = isEngineRunning();
    currentEngineState = lastEngineState;
    engineStartTime = 0;
    engineRunningTime = 0;
    pumpRunningTime = 0;
    lastPumpState = false;
    lastEngineCheck = millis();
}

void SensorManager::update() {
    calculateSpeed();
    checkEngineState();
}

// === Engine Detection Methods ===

void SensorManager::checkEngineState() {
    currentEngineState = isEngineRunning();
    
    // Detect transition
    if (currentEngineState && !lastEngineState) {
        // Engine JUST STARTED
        engineJustStarted = true;
        engineJustStopped = false;
        engineStartTime = millis();
        
        // Reset pump time tracking
        pumpRunningTime = 0;
        pumpStartTime = millis();
        lastPumpState = false;
        
        #ifdef DEBUG_ENGINE
        Serial.println(F(">>> ENGINE STARTED <<<"));
        #endif
    }
    else if (!currentEngineState && lastEngineState) {
        // Engine JUST STOPPED - SAVE DATA!
        engineJustStopped = true;
        engineJustStarted = false;
        
        // Hitung total engine running time
        if (engineStartTime > 0) {
            unsigned long runningMs = millis() - engineStartTime;
            engineRunningTime += (runningMs / 1000);
        }
        
        // SAVE PUMP RUNNING TIME TO EEPROM
        savePumpRunningTime();
        
        // Reset untuk next cycle
        engineStartTime = 0;
        
        #ifdef DEBUG_ENGINE
        Serial.print(F(">>> ENGINE STOPPED - Pump Time: "));
        Serial.print(pumpRunningTime);
        Serial.println(F(" seconds <<<"));
        #endif
    }
    else {
        engineJustStarted = false;
        engineJustStopped = false;
    }
    
    // Update pump time jika engine running
    if (currentEngineState) {
        // Pump time diupdate melalui updatePumpRunningTime()
    }
    
    lastEngineState = currentEngineState;
}

bool SensorManager::isEngineRunning() {
    // Method 1: Digital input
    // return digitalRead(enginePin) == HIGH;
    
    // Method 2: Analog input
    int value = analogRead(enginePin);
    return (value > ENGINE_RUNNING_THRESHOLD);
}

bool SensorManager::isEngineJustStarted() {
    bool result = engineJustStarted;
    engineJustStarted = false;
    return result;
}

bool SensorManager::isEngineJustStopped() {
    bool result = engineJustStopped;
    engineJustStopped = false;
    return result;
}

void SensorManager::resetEngineRunningTime() {
    engineRunningTime = 0;
}

// === Pump Running Time Management ===

void SensorManager::updatePumpRunningTime(bool pumpState) {
    if (!currentEngineState) return;
    
    if (pumpState && !lastPumpState) {
        // Pump JUST STARTED
        pumpStartTime = millis();
        lastPumpState = true;
    }
    else if (!pumpState && lastPumpState) {
        // Pump JUST STOPPED - tambahkan durasi
        if (pumpStartTime > 0) {
            unsigned long duration = (millis() - pumpStartTime) / 1000;
            pumpRunningTime += duration;
        }
        lastPumpState = false;
        pumpStartTime = 0;
    }
    else if (pumpState && lastPumpState) {
        // Pump masih running, update durasi setiap 1 detik
        static unsigned long lastUpdate = 0;
        if (millis() - lastUpdate >= 1000) {
            pumpRunningTime++;
            lastUpdate = millis();
        }
    }
}

void SensorManager::savePumpRunningTime() {
    // Save saat engine mati
    if (pumpRunningTime > 0) {
        // Update KPI via ConfigManager
        ConfigManager::getInstance().forceWriteEngineHours(pumpRunningTime);
        
        #ifdef DEBUG_ENGINE
        Serial.print(F("💾 SAVED: "));
        Serial.print(pumpRunningTime);
        Serial.println(F(" seconds to EEPROM"));
        #endif
    }
}

// === Rest of existing methods ===

void SensorManager::pulseISR() {
    if (instance) {
        instance->pulseCount++;
        instance->lastPulseTime = millis();
    }
}

void SensorManager::calculateSpeed() {
    unsigned long now = millis();
    if (now - lastSpeedCalc >= 1000) {
        float distancePerPulse = wheelCircumference / pulsePerRev;
        float distanceTraveled = pulseCount * distancePerPulse;
        totalDistance += distanceTraveled;
        currentSpeed = distanceTraveled / ((now - lastSpeedCalc) / 1000.0f) * 3.6f;
        pulseCount = 0;
        lastSpeedCalc = now;
    }
}

void SensorManager::resetDistance() {
    totalDistance = 0;
}

void SensorManager::setWheelCircumference(float circumference) {
    wheelCircumference = circumference;
}

void SensorManager::setPulsePerRev(int pulses) {
    pulsePerRev = pulses;
}