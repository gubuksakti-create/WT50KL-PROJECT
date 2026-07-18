#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include "constants.h"
#include "config/ConfigManager.h"

class SensorManager {
public:
    static SensorManager& getInstance();
    
    void begin();
    void update();
    
    // Existing methods
    float getSpeed() const { return currentSpeed; }
    float getTotalDistance() const { return totalDistance; }
    unsigned long getPulseCount() const { return pulseCount; }
    void resetDistance();
    void setWheelCircumference(float circumference);
    void setPulsePerRev(int pulses);
    
    // Engine detection
    bool isEngineRunning();
    bool isEngineJustStarted();
    bool isEngineJustStopped();
    unsigned long getEngineRunningTime() const { return engineRunningTime; }
    void resetEngineRunningTime();
    
    // Pump operation time
    void updatePumpRunningTime(bool pumpIsOn);
    unsigned long getPumpRunningTime() const { return pumpRunningTime; }
    
    // Static ISR handler
    static void pulseISR();
    
private:
    SensorManager() {}
    SensorManager(const SensorManager&) = delete;
    SensorManager& operator=(const SensorManager&) = delete;
    
    // Existing members
    volatile unsigned long pulseCount = 0;
    volatile unsigned long lastPulseTime = 0;
    float currentSpeed = 0;
    float totalDistance = 0;
    float wheelCircumference = DEFAULT_WHEEL_CIRCUMFERENCE;
    int pulsePerRev = DEFAULT_PULSE_PER_REV;
    unsigned long lastSpeedCalc = 0;
    
    static SensorManager* instance;
    void calculateSpeed();
    
    // Engine detection members
    bool lastEngineState = false;
    bool currentEngineState = false;
    bool engineJustStarted = false;
    bool engineJustStopped = false;
    unsigned long engineStartTime = 0;
    unsigned long engineRunningTime = 0;
    unsigned long lastEngineCheck = 0;
    int enginePin = PIN_ENGINE_RUNNING;
    
    // Pump operation time
    unsigned long pumpRunningTime = 0;
    unsigned long pumpStartTime = 0;
    bool lastPumpState = false;
    
    void checkEngineState();
    void savePumpRunningTime();  // Tambahkan deklarasi ini
};

#endif