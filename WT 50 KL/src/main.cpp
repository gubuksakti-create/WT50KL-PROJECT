#include <Arduino.h>
#include "config/ConfigManager.h"
#include "hardware/ButtonManager.h"
#include "hardware/RelayManager.h"
#include "hardware/SensorManager.h"
#include "hardware/RTCManager.h"
#include "hardware/LCDManager.h"
#include "hardware/WaterLevelManager.h" 
#include "modes/IntermittentManager.h"
#include "modes/SprayMode.h"
#include "ui/MenuManager.h"
#include "constants.h"


// ============================================================
// GLOBAL INSTANCES - Singleton Pattern
// ============================================================
ConfigManager& config = ConfigManager::getInstance();
ButtonManager& buttons = ButtonManager::getInstance();
RelayManager& relays = RelayManager::getInstance();
SensorManager& sensor = SensorManager::getInstance();
RTCManager& rtc = RTCManager::getInstance();
LCDManager& lcd = LCDManager::getInstance();
WaterLevelManager& waterLevel = WaterLevelManager::getInstance(); 
IntermittentManager& intermittent = IntermittentManager::getInstance();
SprayMode& sprayMode = SprayMode::getInstance();
MenuManager& menu = MenuManager::getInstance();

// ============================================================
// TIMING VARIABLES
// ============================================================
unsigned long lastLoopTime = 0;
unsigned long lastEngineHourUpdate = 0;
unsigned long lastPumpCheck = 0;
unsigned long lastStatusDisplay = 0;
unsigned long lastDebugPrint = 0;
unsigned long lastDistanceUpdate = 0;

// ============================================================
// STATE VARIABLES
// ============================================================
bool engineWasRunning = false;
bool pumpWasRunning = false;
unsigned long pumpRunningTimeSession = 0;
unsigned long pumpStartTimestamp = 0;

// ============================================================
// NON-BLOCKING TEMP MESSAGE (pengganti delay() di LCD)
// ============================================================
bool tempMessageActive = false;
unsigned long tempMessageStart = 0;
unsigned long tempMessageDuration = 0;

// Tandai bahwa LCD lagi nampilin pesan sementara (non-blocking, TANPA delay())
// Panggil ini setelah lcd.print(...) selesai, ganti posisi delay(X) yang lama
void startTempMessage(unsigned long durationMs) {
    tempMessageActive = true;
    tempMessageStart = millis();
    tempMessageDuration = durationMs;
}

// ============================================================
// FUNCTION PROTOTYPES
// ============================================================
void handleButtonEvents();
void updateWaterPump();
void updateEngineHours();
void updateSprayDistance();
void checkEngineState();
void displayStatus();
void emergencyShutdown();
void checkWaterLevelSafety();

// ============================================================
// SETUP FUNCTION
// ============================================================
// Konversi ButtonEvent ke teks buat debug print (pakai F() biar hemat RAM)
const __FlashStringHelper* buttonEventToStr(ButtonEvent e) {
    switch (e) {
        case ButtonEvent::NONE: return F("NONE");
        case ButtonEvent::SHORT_PRESS: return F("SHORT");
        case ButtonEvent::LONG_PRESS: return F("LONG");
        case ButtonEvent::COMBINATION_B1_B2: return F("COMBO12");
        case ButtonEvent::COMBINATION_B2_B3: return F("COMBO23");
        case ButtonEvent::COMBINATION_ALL: return F("COMBOALL");
        default: return F("?");
    }
}

void setup() {
    // Initialize Serial for debugging
    Serial.begin(115200);
    
    #ifdef DEBUG_MAIN
    Serial.println(F("=== WATER TRUCK CONTROLLER ==="));
    Serial.println(F("Starting initialization..."));
    #endif

    // ============================================================
    // INITIALIZE ALL MODULES (Order matters!)
    // ============================================================
    
    // 1. Config - MUST BE FIRST (other modules may need config)
    config.begin();
    #ifdef DEBUG_MAIN
    Serial.println(F("✅ Config Manager initialized"));
    #endif
    
    // 2. Hardware - Buttons
    buttons.begin();
    #ifdef DEBUG_MAIN
    Serial.println(F("✅ Button Manager initialized"));
    #endif
    
    // 3. Hardware - Relays
    relays.begin();
    #ifdef DEBUG_MAIN
    Serial.println(F("✅ Relay Manager initialized"));
    #endif
    
    // 4. Hardware - Sensors
    sensor.begin();
    #ifdef DEBUG_MAIN
    Serial.println(F("✅ Sensor Manager initialized"));
    #endif
    
    // 5. Hardware - RTC
    rtc.begin();
    #ifdef DEBUG_MAIN
    if (rtc.isRunning()) {
        Serial.println(F("✅ RTC Manager initialized"));
    } else {
        Serial.println(F("⚠️ RTC Manager - NOT RUNNING!"));
    }
    #endif
    
    // 6. Hardware - LCD
    lcd.begin();
    #ifdef DEBUG_MAIN
    Serial.println(F("✅ LCD Manager initialized"));
    #endif

    // 7.Water level -Low level 
    waterLevel.begin();  // ← NEW
    #ifdef DEBUG_MAIN
    Serial.println(F("✅ Water Level Manager initialized"));
    #endif
    
    // 8. Modes - Intermittent
    intermittent.begin();
    #ifdef DEBUG_MAIN
    Serial.println(F("✅ Intermittent Manager initialized"));
    #endif
    
    // 9. Modes - Spray Mode
    sprayMode.begin();
    #ifdef DEBUG_MAIN
    Serial.println(F("✅ Spray Mode Manager initialized"));
    #endif
    
    // 10. UI - Menu
    menu.begin();
    #ifdef DEBUG_MAIN
    Serial.println(F("✅ Menu Manager initialized"));
    #endif
    
    // ============================================================
    // SET INITIAL STATE
    // ============================================================
    
    // Turn off auto mode indicator
    relays.setRelay(RelayType::AUTO_MAN_IND, false);
    relays.setRelay(RelayType::WATER_LEVEL_ALARM, false);
    
    // Ensure all sprayers are off at startup
    relays.setRelay(RelayType::SPRAYER_LH, false);
    relays.setRelay(RelayType::SPRAYER_CENTER, false);
    relays.setRelay(RelayType::SPRAYER_RH, false);
    relays.setRelay(RelayType::WATER_PUMP, false);
    relays.setMosfetGate(0);

    // Check water status
    if (waterLevel.isTankEmpty()) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("! NO WATER !"));
        lcd.setCursor(0, 1);
        lcd.print(F("System Locked"));
        delay(3000);
    }
    
    // ============================================================
    // INITIALIZE TIMING VARIABLES
    // ============================================================
    
    unsigned long startTime = millis();
    lastLoopTime = startTime;
    lastEngineHourUpdate = startTime;
    lastPumpCheck = startTime;
    lastStatusDisplay = startTime;
    lastDebugPrint = startTime;
    lastDistanceUpdate = startTime;
    
    // ============================================================
    // DISPLAY STARTUP MESSAGE
    // ============================================================
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Water Truck Ctrl"));
    lcd.setCursor(0, 1);
    lcd.print(F("Initializing..."));
    
    // Check engine state
    bool engineRunning = sensor.isEngineRunning();
    engineWasRunning = engineRunning;
    
    delay(2000);
    
    // Display engine status
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Engine: "));
    lcd.print(engineRunning ? F("RUNNING") : F("STOPPED"));
    
    lcd.setCursor(0, 1);
    lcd.print(F("Pump: OFF"));
    
    // Display KPI summary
    KPI kpi = config.getKPI();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Total Hours: "));
    lcd.print(kpi.engineHours / 3600);
    lcd.print(F("h"));
    lcd.setCursor(0, 1);
    lcd.print(F("Distance: "));
    lcd.print(kpi.totalDistanceSprayed / 1000, 1);
    lcd.print(F("km"));
    
    delay(2000);
    lcd.clear();
    
    #ifdef DEBUG_MAIN
    Serial.println(F("=== INITIALIZATION COMPLETE ==="));
    Serial.print(F("Engine: "));
    Serial.println(engineRunning ? F("RUNNING") : F("STOPPED"));
    Serial.print(F("Total Engine Hours: "));
    Serial.print(kpi.engineHours / 3600);
    Serial.print(F("h "));
    Serial.print((kpi.engineHours % 3600) / 60);
    Serial.println(F("m"));
    Serial.print(F("Total Spray Distance: "));
    Serial.print(kpi.totalDistanceSprayed / 1000, 1);
    Serial.println(F("km"));
    Serial.println(F("================================"));
    #endif
}

// ============================================================
// MAIN LOOP
// ============================================================
void loop() {
    unsigned long currentMillis = millis();

    // ============================================================
    // 0. UPDATE WATER LEVEL SAFETY (PRIORITAS TERTINGGI!)
    // ============================================================
     waterLevel.update();

    // Cek safety interlock - jika system blocked, skip semua operasi
    if (waterLevel.isSystemBlocked()) {

        #ifdef DEBUG_WATER_LEVEL
        Serial.println(F("⚠️ System Blocked - Forcing Unblock (TEST)"));
        waterLevel.resetEmergencyStop();  // ← Tambahkan method ini
        #endif
        
        // Update menu tetap jalan (untuk display)
        menu.update();
        
        // Update display status
        if (currentMillis - lastStatusDisplay >= 3000) {
            if (!menu.isMenuActive()) {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print(F("! SYSTEM LOCKED !"));
                lcd.setCursor(0, 1);
                lcd.print(F("NO WATER"));
            }
            lastStatusDisplay = currentMillis;
        }
        
        lastLoopTime = currentMillis;
        //return;  // ← SKIP semua operasi!
    }
    
    // ============================================================
    // 1. UPDATE BUTTONS
    // ============================================================
    buttons.update();

    #ifdef DEBUG_MAIN
    {
        ButtonEvent b1 = buttons.getButton1Event();
        ButtonEvent b2 = buttons.getButton2Event();
        ButtonEvent b3 = buttons.getButton3Event();
        if (b1 != ButtonEvent::NONE || b2 != ButtonEvent::NONE || b3 != ButtonEvent::NONE) {
            Serial.print(F("[BTN] B1="));
            Serial.print(buttonEventToStr(b1));
            Serial.print(F(" B2="));
            Serial.print(buttonEventToStr(b2));
            Serial.print(F(" B3="));
            Serial.print(buttonEventToStr(b3));
            Serial.print(F(" | isPressed: "));
            Serial.print(buttons.isButton1Pressed() ? "1" : "0");
            Serial.print(buttons.isButton2Pressed() ? "1" : "0");
            Serial.println(buttons.isButton3Pressed() ? "1" : "0");
        }
    }
    #endif
    
    // ============================================================
    // 2. HANDLE BUTTON EVENTS
    // ============================================================
    handleButtonEvents();
    
    // ============================================================
    // 3. UPDATE SENSORS
    // ============================================================
    sensor.update();
    
    // ============================================================
    // 4. CHECK ENGINE STATE
    // ============================================================
    checkEngineState();
    
    // ============================================================
    // 5. UPDATE SPRAY MODE
    // ============================================================
    sprayMode.update();
    
    // ============================================================
    // 6. UPDATE INTERMITTENT MODE
    // ============================================================
    intermittent.update();
    
    // ============================================================
    // 7. UPDATE WATER PUMP
    // ============================================================
    updateWaterPump();
    
    // ============================================================
    // 8. UPDATE ENGINE HOURS (Every second, RAM only)
    // ============================================================
    if (currentMillis - lastEngineHourUpdate >= 1000) {
        updateEngineHours();
        lastEngineHourUpdate = currentMillis;
    }
    
    // ============================================================
    // 9. UPDATE SPRAY DISTANCE (Every 100ms, periodic EEPROM write)
    // ============================================================
    if (currentMillis - lastDistanceUpdate >= 100) {
        updateSprayDistance();
        lastDistanceUpdate = currentMillis;
    }
    
    // ============================================================
    // 10. UPDATE MENU
    // ============================================================
    menu.update();
    
    // ============================================================
    // 11a. HANDLE NON-BLOCKING TEMP MESSAGE EXPIRY
    // ============================================================
    if (tempMessageActive && (currentMillis - tempMessageStart >= tempMessageDuration)) {
        tempMessageActive = false;
        lcd.clear();
        lastStatusDisplay = currentMillis; // reset timer biar auto-scroll nggak langsung nimpa
    }

    // ============================================================
    // 11b. DISPLAY STATUS (Every 3 seconds, if menu not active, dan TIDAK sedang nampilin temp message)
    // ============================================================
    if (!tempMessageActive && (currentMillis - lastStatusDisplay >= 8000)) {
        displayStatus();
        lastStatusDisplay = currentMillis;
    }
    
    // ============================================================
    // 12. DEBUG OUTPUT (Every 10 seconds, if enabled)
    // ============================================================
    #ifdef DEBUG_MAIN
    if (currentMillis - lastDebugPrint >= 10000) {
        Serial.print(F("🔍 Engine: "));
        Serial.print(sensor.isEngineRunning() ? F("RUN") : F("OFF"));
        Serial.print(F(" | Pump: "));
        Serial.print(relays.getRelayState(RelayType::WATER_PUMP) ? F("ON ") : F("OFF"));
        Serial.print(F(" | Speed: "));
        Serial.print(sensor.getSpeed(), 1);
        Serial.print(F(" km/h"));
        Serial.print(F(" | Int: "));
        Serial.print(intermittent.isActive() ? F("ON") : F("OFF"));
        Serial.print(F(" | Menu: "));
        Serial.print(menu.isMenuActive() ? F("ACTIVE") : F("IDLE"));
        
        // EEPROM status
        if (config.hasPendingSave()) {
            Serial.print(F(" | ⚠️ EEPROM PENDING"));
        }
        
        Serial.println();
        lastDebugPrint = currentMillis;
    }
    #endif
    
    // ============================================================
    // 13. UPDATE LOOP TIMER
    // ============================================================
    lastLoopTime = currentMillis;
}

// ============================================================
// UPDATE WATER PUMP (Dengan Safety Check)
// ============================================================

void updateWaterPump() {
    // 🔒 SAFETY CHECK - Sementara bypass jika sedang testing di meja
    #ifndef DEBUG_WATER_LEVEL
    if (!waterLevel.canPumpRun()) {
        relays.setRelay(RelayType::WATER_PUMP, false);
        relays.setMosfetGate(0);
        relays.setRelay(RelayType::SPRAYER_LH, false);
        relays.setRelay(RelayType::SPRAYER_CENTER, false);
        relays.setRelay(RelayType::SPRAYER_RH, false);
        relays.setRelay(RelayType::AUTO_MAN_IND, false);
        return;
    }
    #endif
    
    // Cek apakah mode intermittent aktif dan sedan dalam siklus ON (menyemprot)
    bool intermittentActive = intermittent.isActive() && intermittent.isSprayerOn();
    
    // Cek manual sprayer (hanya aktif jika intermittent sedang TIDAK mengambil alih)
    bool sprayerActive = false;
    if (!intermittent.isActive()) {
        sprayerActive = relays.getRelayState(RelayType::SPRAYER_LH) ||
                        relays.getRelayState(RelayType::SPRAYER_CENTER) ||
                        relays.getRelayState(RelayType::SPRAYER_RH);
    } else {
        // Jika intermittent AKTIF, biarkan intermittent yang menyalakan relay fisik via internalnya
        sprayerActive = intermittentActive; 
    }
    
    bool pumpShouldBeOn = sprayerActive || intermittentActive;
    relays.setRelay(RelayType::WATER_PUMP, pumpShouldBeOn);
    
    // Kontrol MOSFET
    if (pumpShouldBeOn) {
        int pedal = analogRead(PIN_THROTTLE_PEDAL); //
        int dimmer = pedal / 4;
        int solCurrent = map(dimmer, 35, 220, 250, 75);
        if (solCurrent < 0) solCurrent = 0;
        if (solCurrent > 255) solCurrent = 255;
        relays.setMosfetGate(solCurrent);
    } else {
        relays.setMosfetGate(0);
    }
}

// ============================================================
// HANDLE BUTTON EVENTS
// ============================================================

// main.cpp – bagian handleButtonEvents()

// main.cpp – bagian handleButtonEvents()

void handleButtonEvents() {
    // 0. PRIORITAS TERTINGGI: KOMBINASI SEMUA TOMBOL (B1+B2+B3) → Masuk/Keluar Menu
    {
        ButtonEvent btn1Peek = buttons.getButton1Event();
        ButtonEvent btn2Peek = buttons.getButton2Event();
        ButtonEvent btn3Peek = buttons.getButton3Event();
        if (btn1Peek == ButtonEvent::COMBINATION_ALL ||
            btn2Peek == ButtonEvent::COMBINATION_ALL ||
            btn3Peek == ButtonEvent::COMBINATION_ALL) {
            menu.toggleMenu();
            buttons.markEventProcessed();
            return;
        }
    }

    // 1. PRIORITAS: MENU AKTIF
    if (menu.isMenuActive()) {
        ButtonEvent btn1 = buttons.getButton1Event();
        ButtonEvent btn2 = buttons.getButton2Event();
        ButtonEvent btn3 = buttons.getButton3Event();
        if (btn1 != ButtonEvent::NONE) menu.handleButton(1, btn1);
        if (btn2 != ButtonEvent::NONE) menu.handleButton(2, btn2);
        if (btn3 != ButtonEvent::NONE) menu.handleButton(3, btn3);
        buttons.markEventProcessed(); 
        return;
    }

    // 2. AMBIL EVENT TERBARU
    ButtonEvent btn1 = buttons.getButton1Event();
    ButtonEvent btn2 = buttons.getButton2Event();
    ButtonEvent btn3 = buttons.getButton3Event();

    // 3. DETEKSI KOMBINASI TOMBOL (Diambil dari event khusus)
    // Kombinasi B1 + B2 (Ubah Mode Intermittent)
    if (btn1 == ButtonEvent::COMBINATION_B1_B2 || btn2 == ButtonEvent::COMBINATION_B1_B2) {
        auto currentMode = config.getConfig().intermittentMode;
        config.setIntermittentMode(currentMode == IntermittentMode::SPRAYER ? IntermittentMode::PUMP : IntermittentMode::SPRAYER);
        buttons.markEventProcessed();

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("Mode: "));
        lcd.print(config.getConfig().intermittentMode == IntermittentMode::SPRAYER ? F("SPRAYER") : F("PUMP"));
        startTempMessage(1000);
        return;
    }

    // Kombinasi B2 + B3 (Ubah Trigger Mode)
    if (btn2 == ButtonEvent::COMBINATION_B2_B3 || btn3 == ButtonEvent::COMBINATION_B2_B3) {
        auto currentMode = config.getConfig().triggerMode;
        config.setTriggerMode(currentMode == TriggerMode::TIME_BASED ? TriggerMode::DISTANCE_BASED : TriggerMode::TIME_BASED);
        buttons.markEventProcessed();

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("Trigger: "));
        lcd.print(config.getConfig().triggerMode == TriggerMode::TIME_BASED ? F("TIME") : F("DIST"));
        startTempMessage(1000);
        return;
    }

    // 4. INDIVIDUAL LONG PRESS
    if (btn1 == ButtonEvent::LONG_PRESS) {
        intermittent.toggleAutoMode();
        relays.setRelay(RelayType::AUTO_MAN_IND, intermittent.isActive());
        buttons.markEventProcessed();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("Auto Mode: "));
        lcd.print(intermittent.isActive() ? F("ON") : F("OFF"));
        startTempMessage(1000);
        return;
    }

    if (btn2 == ButtonEvent::LONG_PRESS) {
        relays.toggleRelay(RelayType::ROTARY);
        buttons.markEventProcessed();
        return;
    }

    if (btn3 == ButtonEvent::LONG_PRESS) {
        relays.toggleRelay(RelayType::WORKING_LAMP);
        buttons.markEventProcessed();
        return;
    }

    // 5. INDIVIDUAL SHORT PRESS
    if (btn1 == ButtonEvent::SHORT_PRESS) {
        bool state = !relays.toggleRelay(RelayType::SPRAYER_RH);
        sprayMode.setSprayerRight(state);
        intermittent.setSprayerStates(
            relays.getRelayState(RelayType::SPRAYER_LH),
            relays.getRelayState(RelayType::SPRAYER_CENTER),
            state
        );
        buttons.markEventProcessed();
        return;
    }

    if (btn2 == ButtonEvent::SHORT_PRESS) {
        bool state = !relays.toggleRelay(RelayType::SPRAYER_CENTER);
        sprayMode.setSprayerCenter(state);
        intermittent.setSprayerStates(
            relays.getRelayState(RelayType::SPRAYER_LH),
            state,
            relays.getRelayState(RelayType::SPRAYER_RH)
        );
        buttons.markEventProcessed();
        return;
    }

    if (btn3 == ButtonEvent::SHORT_PRESS) {
        bool state = !relays.toggleRelay(RelayType::SPRAYER_LH);
        sprayMode.setSprayerLeft(state);
        intermittent.setSprayerStates(
            state,
            relays.getRelayState(RelayType::SPRAYER_CENTER),
            relays.getRelayState(RelayType::SPRAYER_RH)
        );
        buttons.markEventProcessed();
        return;
    }
    
    // PENTING: Jangan panggil clearEvents() secara membabi buta di sini!
}
/*
// ============================================================
// UPDATE WATER PUMP
// ============================================================
void updateWaterPump() {
    // Check if any sprayer is active (manual mode)
    bool sprayerActive = relays.getRelayState(RelayType::SPRAYER_LH) ||
                         relays.getRelayState(RelayType::SPRAYER_CENTER) ||
                         relays.getRelayState(RelayType::SPRAYER_RH);
    
    // Check if intermittent is active and spraying
    bool intermittentActive = intermittent.isActive() && intermittent.isSprayerOn();
    
    // Pump should be ON if either condition is true
    bool pumpShouldBeOn = sprayerActive || intermittentActive;
    
    // Update pump relay
    relays.setRelay(RelayType::WATER_PUMP, pumpShouldBeOn);
    
    // ============================================================
    // MOSFET GATE CONTROL (Proportional from throttle)
    // ============================================================
    if (pumpShouldBeOn) {
        // Read throttle pedal position (0-1023)
        int pedal = analogRead(PIN_THROTTLE_PEDAL);
        
        // Convert to 8-bit (0-255)
        int dimmer = pedal / 4;
        
        // Map to solenoid current range
        // Higher pedal = lower current (inverted control)
        // Range: 35-220 pedal input -> 250-75 PWM output
        int solCurrent = map(dimmer, 35, 220, 250, 75);
        
        // Clamp values to safe range
        if (solCurrent < 0) solCurrent = 0;
        if (solCurrent > 255) solCurrent = 255;
        
        // Set MOSFET gate PWM
        relays.setMosfetGate(solCurrent);
    } else {
        // Turn off MOSFET when pump is off
        relays.setMosfetGate(0);
    }
}
*/

// ============================================================
// UPDATE ENGINE HOURS
// ============================================================
void updateEngineHours() {
    // Only update if engine is running
    if (!sensor.isEngineRunning()) {
        return;
    }
    
    // Check if any pump/solenoid is active
    bool pumpActive = relays.getRelayState(RelayType::WATER_PUMP) ||
                      relays.getRelayState(RelayType::SPRAYER_LH) ||
                      relays.getRelayState(RelayType::SPRAYER_CENTER) ||
                      relays.getRelayState(RelayType::SPRAYER_RH);
    
    // Only count time when pump is actually running
    if (pumpActive) {
        // Update engine hours in RAM only (no EEPROM write!)
        config.updateEngineHours(1);
        
        // Update pump running time session
        pumpRunningTimeSession++;
        
        #ifdef DEBUG_MAIN
        if (pumpRunningTimeSession % 60 == 0) {
            Serial.print(F("⏱️ Pump Running: "));
            Serial.print(pumpRunningTimeSession / 60);
            Serial.println(F(" minutes"));
        }
        #endif
    }
}

// ============================================================
// UPDATE SPRAY DISTANCE
// ============================================================
void updateSprayDistance() {
    // Only calculate distance when spraying
    if (intermittent.isActive() && intermittent.isSprayerOn()) {
        float speed = sensor.getSpeed(); // km/h
        
        // Calculate distance increment in meters
        // distance = speed (km/h) / 3600 * delta_time (s) * 1000 (m/km)
        float deltaTime = (millis() - lastDistanceUpdate) / 1000.0f;
        float distanceIncrement = (speed / 3600.0f) * deltaTime * 1000.0f;
        
        if (distanceIncrement > 0) {
            // Update distance in RAM (periodic EEPROM write)
            config.updateKPIDistance(distanceIncrement);
        }
    }
}

// ============================================================
// CHECK ENGINE STATE
// ============================================================
void checkEngineState() {
    bool engineRunning = sensor.isEngineRunning();
    
    // Check for engine start
    if (engineRunning && !engineWasRunning) {
        // Engine JUST STARTED
        #ifdef DEBUG_MAIN
        Serial.println(F("🚀 ENGINE STARTED"));
        #endif
        
        // Reset pump running time for this session
        pumpRunningTimeSession = 0;
        pumpStartTimestamp = millis();
        
        // Display notification
        if (!menu.isMenuActive()) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("Engine Started"));
            lcd.setCursor(0, 1);
            lcd.print(F("Pump: 0m"));
            startTempMessage(1000);
        }
    }
    
    // Check for engine stop
    if (!engineRunning && engineWasRunning) {
        // ENGINE JUST STOPPED - SAVE DATA!
        #ifdef DEBUG_MAIN
        Serial.println(F("🛑 ENGINE STOPPED - Saving data..."));
        #endif
        
        // Save pump running time to EEPROM
        if (pumpRunningTimeSession > 0) {
            config.forceWriteEngineHours(pumpRunningTimeSession);
            
            // Display confirmation
            if (!menu.isMenuActive()) {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print(F("Engine Stopped"));
                lcd.setCursor(0, 1);
                lcd.print(F("Saved: "));
                lcd.print(pumpRunningTimeSession / 60);
                lcd.print(F("m"));
                startTempMessage(2000);
            }
        } else {
            // No pump time to save
            if (!menu.isMenuActive()) {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print(F("Engine Stopped"));
                lcd.setCursor(0, 1);
                lcd.print(F("No pump time"));
                startTempMessage(1000);
            }
        }
        
        // Reset session counter
        pumpRunningTimeSession = 0;
        pumpStartTimestamp = 0;
    }
    
    // Update state for next iteration
    engineWasRunning = engineRunning;
}

// ============================================================
// DISPLAY STATUS (Auto-scroll when menu not active)
// ============================================================
void displayStatus() {
    // Only show status if menu is not active
    if (menu.isMenuActive()) {
        return;
    }
    
    static int statusPage = 0;
    
    switch (statusPage) {
        case 0: // Engine & Pump Status
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("E:"));
            lcd.print(sensor.isEngineRunning() ? F("RUN ") : F("OFF "));
            lcd.print(F("P:"));
            lcd.print(relays.getRelayState(RelayType::WATER_PUMP) ? F("ON ") : F("OFF"));
            
            lcd.setCursor(0, 1);
            lcd.print(F("Auto:"));
            lcd.print(intermittent.isActive() ? F("ON ") : F("OFF"));
            lcd.print(F(" Spd:"));
            lcd.print(sensor.getSpeed(), 0);
            break;
            
        case 1: // Sprayer Status
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("SPRY: "));
            lcd.print(relays.getRelayState(RelayType::SPRAYER_LH) ? 'O' : '.');
            lcd.print(relays.getRelayState(RelayType::SPRAYER_CENTER) ? 'O' : '.');
            lcd.print(relays.getRelayState(RelayType::SPRAYER_RH) ? 'O' : '.');
            
            lcd.setCursor(0, 1);
            lcd.print(F("ROT:"));
            lcd.print(relays.getRelayState(RelayType::ROTARY) ? 'O' : '.');
            lcd.print(F(" LMP:"));
            lcd.print(relays.getRelayState(RelayType::WORKING_LAMP) ? 'O' : '.');
            break;
            
        case 2: // KPI Summary
            lcd.clear();
            KPI kpi = config.getKPI();
            lcd.setCursor(0, 0);
            lcd.print(F("Hours: "));
            lcd.print(kpi.engineHours / 3600);
            lcd.print(F("h"));
            
            lcd.setCursor(0, 1);
            lcd.print(F("Dist: "));
            lcd.print(kpi.totalDistanceSprayed / 1000, 1);
            lcd.print(F("km"));
            break;

        case 3: // Water Level Status
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("WATER: "));
            lcd.print(waterLevel.isWaterAvailable() ? F("OK") : F("EMPTY"));

            lcd.setCursor(0, 1);
            lcd.print(F("Status: "));
            lcd.print(waterLevel.isSystemBlocked() ? F("LOCKED") : F("NORMAL"));
            break;
    }
    
    // Next page for next iteration (4 halaman sesuai spesifikasi README)
    statusPage = (statusPage + 1) % 4;
}

// ============================================================
// EMERGENCY SHUTDOWN
// ============================================================
/*
void emergencyShutdown() {
    #ifdef DEBUG_MAIN
    Serial.println(F("⚠️ EMERGENCY SHUTDOWN ACTIVATED!"));
    #endif
    
    // Turn off all relays
    relays.allOff();
    
    // Save any pending data to EEPROM
    if (config.hasPendingSave()) {
        config.forceWriteToEEPROM();
    }
    
    // Display message
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("! EMERGENCY !"));
    lcd.setCursor(0, 1);
    lcd.print(F("All Systems Off"));
}
*/

void emergencyShutdown() {
    #ifdef DEBUG_MAIN
    Serial.println(F("⚠️ EMERGENCY SHUTDOWN ACTIVATED!"));
    #endif
    
    // Trigger water level emergency stop
    waterLevel.emergencyStop();
    
    // Turn off all relays
    relays.allOff();
    
    // Save pending data
    if (config.hasPendingSave()) {
        config.forceWriteToFRAM();
    }
    
    // Display message
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("! EMERGENCY !"));
    lcd.setCursor(0, 1);
    lcd.print(F("NO WATER - STOP"));
}

// ============================================================
// OPTIONAL: Watchdog Timer Reset
// ============================================================
#ifdef ENABLE_WATCHDOG
#include <avr/wdt.h>

void setupWatchdog() {
    // Enable watchdog with 4 second timeout
    wdt_enable(WDTO_4S);
}

void resetWatchdog() {
    wdt_reset();
}
#endif

// ============================================================
// OPTIONAL: Power Loss Detection
// ============================================================
#ifdef ENABLE_POWER_LOSS_DETECT
#include <avr/sleep.h>

void setupPowerLossDetection() {
    // Setup interrupt on pin for power loss detection
    // Requires additional hardware (capacitor + resistor)
    pinMode(PIN_POWER_LOSS, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_POWER_LOSS), powerLossISR, FALLING);
}

void powerLossISR() {
    // Emergency save on power loss
    if (config.hasPendingSave()) {
        config.forceWriteToEEPROM();
    }
    
    // Turn off all outputs
    relays.allOff();
    
    // Enter sleep mode to preserve power
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sleep_mode();
}
#endif