// WaterLevelManager.cpp
#include "WaterLevelManager.h"
#include "hardware/RelayManager.h"
#include "hardware/LCDManager.h"

WaterLevelManager& WaterLevelManager::getInstance() {
    static WaterLevelManager instance;
    return instance;
}

void WaterLevelManager::begin() {
    pinMode(sensorPin, INPUT_PULLUP);
    currentStatus = Status::OK;
    systemBlocked = false;
    dryRunDetected = false;
    dryRunCooldownEnd = 0;
    alarmState = false;
    lastAlarmTime = 0;
    
    // Initial read
    updateStatus();
    
    #ifdef DEBUG_WATER_LEVEL
    Serial.println(F("💧 Water Level Manager Initialized"));
    printStatus();
    #endif
}

void WaterLevelManager::update() {
    // Update status dari sensor
    updateStatus();
    
    // Check safety interlock
    checkSafetyInterlock();
    
    // Handle alarm blinking
    auto& relays = RelayManager::getInstance();
    if (systemBlocked) {
        // Blink alarm indicator
        unsigned long now = millis();
        if (now - lastAlarmTime >= ALARM_INTERVAL) {
            alarmState = !alarmState;
            relays.setRelay(RelayType::WATER_LEVEL_ALARM, alarmState);
            lastAlarmTime = now;
        }
    } else {
        // Turn off alarm
        if (alarmState) {
            alarmState = false;
            relays.setRelay(RelayType::WATER_LEVEL_ALARM, false);
        }
    }
}

bool WaterLevelManager::readSensor() {
    // ⚠️ FLOAT SWITCH BINARY DETECTION
    // 
    // Konfigurasi tergantung wiring:
    // 
    // Opsi 1: Tanpa Optocoupler (Direct)
    //   - Pin = INPUT_PULLUP
    //   - Float switch CLOSED (ada air) → pin HIGH
    //   - Float switch OPEN (kosong) → pin LOW
    //   
    // Opsi 2: Dengan Optocoupler (Aktif LOW)
    //   - Pin = INPUT_PULLUP
    //   - Float switch CLOSED (ada air) → Opto ON → pin LOW
    //   - Float switch OPEN (kosong) → Opto OFF → pin HIGH
    
    // Default: Tanpa optocoupler
    return digitalRead(sensorPin) == HIGH;
    
    // Jika pakai optocoupler:
    //return digitalRead(sensorPin) == LOW;  // LOW = ada air
}

void WaterLevelManager::updateStatus() {
    Status oldStatus = currentStatus;
    bool waterAvailable = readSensor();
    
    if (waterAvailable) {
        currentStatus = Status::OK;
    } else {
        currentStatus = Status::EMPTY;
    }
    
    // Detect status change
    if (oldStatus != currentStatus) {
        #ifdef DEBUG_WATER_LEVEL
        Serial.print(F("💧 Water Level Status Changed: "));
        Serial.println(getStatusText());
        #endif
    }
}

void WaterLevelManager::checkSafetyInterlock() {
    auto& relays = RelayManager::getInstance();
    
    // ============================================================
    // 1. CEK KONDISI TANKI KOSONG
    // ============================================================
    if (isTankEmpty()) {
        // 🚨 TANKI KOSONG - EMERGENCY STOP!
        if (!systemBlocked) {
            systemBlocked = true;
            
            #ifdef DEBUG_WATER_LEVEL
            Serial.println(F("🚨 EMERGENCY STOP - TANK EMPTY!"));
            #endif
            
            // ============================================================
            // 2. DISABLE SEMUA OUTPUT
            // ============================================================
            
            // Disable semua sprayer
            relays.setRelay(RelayType::SPRAYER_LH, false);
            relays.setRelay(RelayType::SPRAYER_CENTER, false);
            relays.setRelay(RelayType::SPRAYER_RH, false);
            
            // Disable water pump
            relays.setRelay(RelayType::WATER_PUMP, false);
            
            // Disable MOSFET (proportional valve)
            relays.setMosfetGate(0);
            
            // Disable auto mode indicator
            relays.setRelay(RelayType::AUTO_MAN_IND, false);
            
            // Set dry run detection
            if (!dryRunDetected) {
                dryRunDetected = true;
                dryRunStartTime = millis();
            }
        }
        
        // Pastikan semua output tetap disabled
        relays.setRelay(RelayType::SPRAYER_LH, false);
        relays.setRelay(RelayType::SPRAYER_CENTER, false);
        relays.setRelay(RelayType::SPRAYER_RH, false);
        relays.setRelay(RelayType::WATER_PUMP, false);
        relays.setMosfetGate(0);
        relays.setRelay(RelayType::AUTO_MAN_IND, false);
        
        return;
    }
    
    // ============================================================
    // 3. TANKI TERISI AIR - CEK COOLDOWN
    // ============================================================
    if (systemBlocked) {
        // Cooldown setelah dry run
        if (millis() >= dryRunCooldownEnd) {
            // Cooldown selesai, reset system
            systemBlocked = false;
            dryRunDetected = false;
            dryRunCooldownEnd = 0;
            
            #ifdef DEBUG_WATER_LEVEL
            Serial.println(F("✅ System Unlocked - Water Available"));
            #endif
        } else {
            // Masih dalam cooldown - tetap disable semua output
            relays.setRelay(RelayType::SPRAYER_LH, false);
            relays.setRelay(RelayType::SPRAYER_CENTER, false);
            relays.setRelay(RelayType::SPRAYER_RH, false);
            relays.setRelay(RelayType::WATER_PUMP, false);
            relays.setMosfetGate(0);
            relays.setRelay(RelayType::AUTO_MAN_IND, false);
        }
    }
    
    // ============================================================
    // 4. DRY RUN DETECTION (Jika pump running tapi tanki kosong)
    // ============================================================
    if (dryRunDetected && !systemBlocked) {
        // Ini seharusnya tidak terjadi, tapi safety check
        systemBlocked = true;
        dryRunCooldownEnd = millis() + COOLDOWN_TIME;
        
        #ifdef DEBUG_WATER_LEVEL
        Serial.println(F("⚠️ Dry Run Detected - System Locked"));
        #endif
        
        // Disable semua output
        relays.setRelay(RelayType::SPRAYER_LH, false);
        relays.setRelay(RelayType::SPRAYER_CENTER, false);
        relays.setRelay(RelayType::SPRAYER_RH, false);
        relays.setRelay(RelayType::WATER_PUMP, false);
        relays.setMosfetGate(0);
        relays.setRelay(RelayType::AUTO_MAN_IND, false);
    }
}

void WaterLevelManager::emergencyStop() {
    systemBlocked = true;
    dryRunCooldownEnd = millis() + COOLDOWN_TIME;
    
    auto& relays = RelayManager::getInstance();
    relays.allOff();
    
    #ifdef DEBUG_WATER_LEVEL
    Serial.println(F("🔴 EMERGENCY STOP ACTIVATED!"));
    #endif
}

void WaterLevelManager::resetEmergencyStop() {
    if (!isTankEmpty()) {
        systemBlocked = false;
        dryRunDetected = false;
        dryRunCooldownEnd = 0;
        
        #ifdef DEBUG_WATER_LEVEL
        Serial.println(F("✅ Emergency Stop Reset"));
        #endif
    }
}

unsigned long WaterLevelManager::getCooldownRemaining() const {
    if (systemBlocked && dryRunCooldownEnd > 0) {
        unsigned long now = millis();
        if (now < dryRunCooldownEnd) {
            return (dryRunCooldownEnd - now) / 1000;  // Return in seconds
        }
    }
    return 0;
}

#ifdef DEBUG_WATER_LEVEL
void WaterLevelManager::printStatus() {
    Serial.println(F("=== Water Level Status ==="));
    Serial.print(F("Status: "));
    Serial.println(getStatusText());
    Serial.print(F("Water Available: "));
    Serial.println(isWaterAvailable() ? F("YES") : F("NO"));
    Serial.print(F("System Blocked: "));
    Serial.println(systemBlocked ? F("YES (LOCKED)") : F("NO"));
    Serial.print(F("Dry Run: "));
    Serial.println(dryRunDetected ? F("DETECTED") : F("CLEAR"));
    if (dryRunCooldownEnd > 0) {
        unsigned long remaining = getCooldownRemaining();
        Serial.print(F("Cooldown Remaining: "));
        Serial.print(remaining);
        Serial.println(F("s"));
    }
    Serial.println(F("==========================="));
}
#endif