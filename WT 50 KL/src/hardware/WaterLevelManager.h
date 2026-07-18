// WaterLevelManager.h
#ifndef WATER_LEVEL_MANAGER_H
#define WATER_LEVEL_MANAGER_H

#include <Arduino.h>
#include "constants.h"

/**
 * WaterLevelManager - Manajemen sensor level air (float switch)
 * 
 * Fungsi:
 * 1. Deteksi lower/empty level menggunakan float switch
 * 2. Safety interlock - disable semua output saat tanki kosong
 * 3. Dry run protection untuk pump
 * 4. Alarm indicator
 * 
 * NOTE: Modul ini TIDAK mengakses MenuManager secara langsung.
 *       MenuManager akan membaca status melalui getter methods.
 */
class WaterLevelManager {
public:
    static WaterLevelManager& getInstance();
    
    // Lifecycle
    void begin();
    void update();
    
    // ============================================================
    // STATUS GETTERS - Untuk dibaca oleh modul lain (MenuManager)
    // ============================================================
    
    // Status dasar
    bool isWaterAvailable() const { return currentStatus == Status::OK; }
    bool isTankEmpty() const { return currentStatus == Status::EMPTY; }
    
    // Safety interlock status
    bool isSystemBlocked() const { return systemBlocked; }
    
    // Permission checks
    bool canPumpRun() const { return isWaterAvailable() && !systemBlocked; }
    bool canSprayerRun() const { return isWaterAvailable() && !systemBlocked; }
    
    // Cooldown information (untuk display)
    unsigned long getCooldownRemaining() const;  // Returns in seconds
    
    // Status text for display
    const char* getStatusText() const {
        switch(currentStatus) {
            case Status::OK:    return "OK";
            case Status::EMPTY: return "EMPTY";
            case Status::ERROR: return "ERROR";
        }
        return "UNKNOWN";
    }
    
    // Emergency control
    void emergencyStop();
    void resetEmergencyStop();
    
    
    // FOR TESTING ONLY - Hapus setelah debugging
    #ifdef DEBUG_WATER_LEVEL
    void forceUnblock() {
    systemBlocked = false;
    dryRunDetected = false;
    dryRunCooldownEnd = 0;
    Serial.println("🔓 FORCE UNBLOCKED (TEST MODE)");
    }
    
    void printStatus();
    #endif

private:
    WaterLevelManager() {}
    WaterLevelManager(const WaterLevelManager&) = delete;
    WaterLevelManager& operator=(const WaterLevelManager&) = delete;
    
    // ============================================================
    // INTERNAL ENUM
    // ============================================================
    enum class Status {
        OK,       // Ada air di tanki
        EMPTY,    // Tanki kosong (float switch active)
        ERROR     // Sensor error (kabel putus, etc)
    };
    
    // ============================================================
    // INTERNAL METHODS
    // ============================================================
    bool readSensor();
    void updateStatus();
    void checkSafetyInterlock();
    
    // ============================================================
    // MEMBERS
    // ============================================================
    
    // Status
    Status currentStatus = Status::OK;
    
    // Safety flags
    bool systemBlocked = false;          // Flag untuk block semua output
    bool dryRunDetected = false;
    unsigned long dryRunStartTime = 0;
    unsigned long dryRunCooldownEnd = 0;
    
    // Alarm
    unsigned long lastAlarmTime = 0;
    bool alarmState = false;
    
    // Pin
    int sensorPin = PIN_WATER_LEVEL;
    
    // ============================================================
    // CONSTANTS
    // ============================================================
    const unsigned long ALARM_INTERVAL = 1000;        // Blink interval (ms)
    const unsigned long COOLDOWN_TIME = PUMP_COOLDOWN_TIME * 1000UL;
};

#endif