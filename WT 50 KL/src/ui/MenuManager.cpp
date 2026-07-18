// src/ui/MenuManager.cpp
#include "MenuManager.h"
#include "hardware/LCDManager.h"
#include "hardware/ButtonManager.h"
#include "hardware/WaterLevelManager.h"
#include "config/ConfigManager.h"
#include "hardware/SensorManager.h"
#include "hardware/RTCManager.h"
#include "hardware/RelayManager.h"
#include "modes/IntermittentManager.h"

MenuManager& MenuManager::getInstance() {
    static MenuManager instance;
    return instance;
}

void MenuManager::begin() {
    menuActive = false;
    currentPage = MenuPage::MAIN;
    lastAutoScroll = millis();
    statusPageIndex = 0;
    lastBlinkToggle = 0;
    blinkState = false;
}

void MenuManager::update() {
    // Auto-scroll when menu is not active
    if (!menuActive) {
        unsigned long now = millis();
        if (now - lastAutoScroll >= AUTO_SCROLL_INTERVAL) {
            lastAutoScroll = now;
            displayStatus();
        }
    }
}

void MenuManager::handleButton(int buttonId, ButtonEvent event) {
    if (event == ButtonEvent::LONG_PRESS && buttonId == 3) {
        toggleMenu();
        return;
    }
    
    if (!menuActive) return;
    
    switch(event) {
        case ButtonEvent::SHORT_PRESS:
            switch(buttonId) {
                case 1: previousPage(); break;
                case 2: nextPage(); break;
                case 3: selectPage(); break;
            }
            break;
        default:
            break;
    }
}

void MenuManager::toggleMenu() {
    menuActive = !menuActive;
    if (menuActive) {
        currentPage = MenuPage::MAIN;
        renderCurrentPage();
    } else {
        LCDManager::getInstance().clear();
        statusPageIndex = 0;
    }
}

void MenuManager::setMenuActive(bool active) {
    menuActive = active;
    if (!menuActive) {
        LCDManager::getInstance().clear();
        statusPageIndex = 0;
    }
}

void MenuManager::nextPage() {
    int next = (int)currentPage + 1;
    if (next >= (int)MenuPage::NONE) {
        currentPage = MenuPage::MAIN;
    } else {
        currentPage = (MenuPage)next;
    }
    renderCurrentPage();
}

void MenuManager::previousPage() {
    int prev = (int)currentPage - 1;
    if (prev < 0) {
        currentPage = (MenuPage)((int)MenuPage::NONE - 1);
    } else {
        currentPage = (MenuPage)prev;
    }
    renderCurrentPage();
}

void MenuManager::selectPage() {
    // Enter sub-menu or toggle setting
}

// ============================================================
// RENDER CURRENT PAGE
// ============================================================
void MenuManager::renderCurrentPage() {
    switch(currentPage) {
        case MenuPage::MAIN:
            renderMain();
            break;
        case MenuPage::KPI:
            renderKPI();
            break;
        case MenuPage::SPEED:
            renderSpeed();
            break;
        case MenuPage::CONFIG:
            renderConfig();
            break;
        case MenuPage::INTERMITTENT:
            renderIntermittent();
            break;
        case MenuPage::SPRAYER:
            renderSprayer();
            break;
        case MenuPage::SENSOR:
            renderSensor();
            break;
        case MenuPage::WATER:
            renderWaterLevel();
            break;
        default:
            break;
    }
}

// ============================================================
// DISPLAY STATUS - DENGAN PERBAIKAN SWITCH-CASE
// ============================================================
void MenuManager::displayStatus() {
    // Hanya tampilkan status jika menu tidak aktif
    if (menuActive) return;
    
    auto& waterLevel = WaterLevelManager::getInstance();
    auto& lcd = LCDManager::getInstance();
    auto& sensor = SensorManager::getInstance();
    auto& relays = RelayManager::getInstance();
    auto& intermittent = IntermittentManager::getInstance();
    auto& config = ConfigManager::getInstance();
    
    // ============================================================
    // PRIORITAS 1: WATER LEVEL EMERGENCY - TANKI KOSONG
    // ============================================================
    if (waterLevel.isTankEmpty()) {
        // Blink effect
        unsigned long now = millis();
        if (now - lastBlinkToggle >= 500) {
            blinkState = !blinkState;
            lastBlinkToggle = now;
        }
        
        lcd.clear();
        if (blinkState) {
            lcd.setCursor(0, 0);
            lcd.print(F("⚠️ NO WATER ⚠️"));
        } else {
            lcd.setCursor(0, 0);
            lcd.print(F("              "));
        }
        lcd.setCursor(0, 1);
        lcd.print(F("SYSTEM LOCKED"));
        return;
    }
    
    // ============================================================
    // PRIORITAS 2: COOLDOWN (System Blocked)
    // ============================================================
    if (waterLevel.isSystemBlocked()) {
        unsigned long remaining = waterLevel.getCooldownRemaining();
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("COOLDOWN: "));
        lcd.print(remaining);
        lcd.print(F("s"));
        lcd.setCursor(0, 1);
        lcd.print(F("System Locked"));
        return;
    }
    
    // ============================================================
    // PRIORITAS 3: NORMAL AUTO-SCROLL
    // ============================================================
    blinkState = false;
    
    // 🔥 PERBAIKAN: Tambahkan kurung kurawal untuk setiap case
    switch (statusPageIndex) {
        case 0: { // Engine & Pump Status
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
        }
        
        case 1: { // Sprayer Status
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
        }
        
        case 2: { // KPI Summary - 🔥 Deklarasi variabel di sini butuh kurung kurawal!
            lcd.clear();
            KPI kpi = config.getKPI();  // ← Deklarasi variabel di dalam case
            lcd.setCursor(0, 0);
            lcd.print(F("Hours: "));
            lcd.print(kpi.engineHours / 3600);
            lcd.print(F("h"));
            
            lcd.setCursor(0, 1);
            lcd.print(F("Dist: "));
            lcd.print(kpi.totalDistanceSprayed / 1000, 1);
            lcd.print(F("km"));
            break;
        }
        
        case 3: { // Water Level Status
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("WATER: "));
            lcd.print(waterLevel.isWaterAvailable() ? F("OK") : F("EMPTY"));
            lcd.setCursor(0, 1);
            lcd.print(F("Status: "));
            lcd.print(waterLevel.getStatusText());
            break;
        }
    }
    
    // Next page for next iteration
    statusPageIndex = (statusPageIndex + 1) % 4;
}

// ============================================================
// RENDER MENU PAGES
// ============================================================

void MenuManager::renderMain() {
    auto& lcd = LCDManager::getInstance();
    auto& waterLevel = WaterLevelManager::getInstance();
    
    lcd.clear();
    
    if (waterLevel.isTankEmpty() || waterLevel.isSystemBlocked()) {
        lcd.setCursor(0, 0);
        lcd.print(F("⚠️ SYSTEM LOCKED"));
        lcd.setCursor(0, 1);
        lcd.print(F("Press B3 for Menu"));
    } else {
        lcd.setCursor(0, 0);
        lcd.print(F("=== WATER TRUCK ==="));
        lcd.setCursor(0, 1);
        lcd.print(F("Press B3 for Menu"));
    }
    
    lcd.printMenuIndicator(1, 8);
}

void MenuManager::renderKPI() {
    auto& lcd = LCDManager::getInstance();
    KPI kpi = ConfigManager::getInstance().getKPI();
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Dist:"));
    lcd.print(kpi.totalDistanceSprayed / 1000, 1);
    lcd.print(F("km"));
    
    lcd.setCursor(0, 1);
    lcd.print(F("Hours:"));
    unsigned long hours = kpi.engineHours / 3600;
    unsigned long mins = (kpi.engineHours % 3600) / 60;
    char timeStr[9];
    snprintf(timeStr, sizeof(timeStr), "%02lu:%02lu", hours, mins);
    lcd.print(timeStr);
    
    lcd.printMenuIndicator(2, 8);
}

void MenuManager::renderSpeed() {
    auto& lcd = LCDManager::getInstance();
    auto& sensor = SensorManager::getInstance();
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Speed:"));
    lcd.print(sensor.getSpeed(), 1);
    lcd.print(F(" km/h"));
    
    lcd.setCursor(0, 1);
    lcd.print(F("Auto:"));
    bool autoMode = IntermittentManager::getInstance().isActive();
    lcd.print(autoMode ? F("ON ") : F("OFF"));
    lcd.print(F(" Dist:"));
    lcd.print(sensor.getTotalDistance() / 1000, 1);
    lcd.print(F("km"));
    
    lcd.printMenuIndicator(3, 8);
}

void MenuManager::renderConfig() {
    auto& lcd = LCDManager::getInstance();
    SystemConfig config = ConfigManager::getInstance().getConfig();
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Mode:"));
    if (config.triggerMode == TriggerMode::TIME_BASED) {
        lcd.print(F("TIME"));
    } else {
        lcd.print(F("DIST"));
    }
    lcd.print(F(" "));
    if (config.intermittentMode == IntermittentMode::SPRAYER) {
        lcd.print(F("SPRAY"));
    } else {
        lcd.print(F("PUMP"));
    }
    
    lcd.setCursor(0, 1);
    lcd.print(F("ON:"));
    if (config.triggerMode == TriggerMode::TIME_BASED) {
        lcd.print((int)(config.intervalON / 1000));
        lcd.print(F("s"));
    } else {
        lcd.print(config.distanceON, 1);
        lcd.print(F("m"));
    }
    lcd.print(F(" OFF:"));
    if (config.triggerMode == TriggerMode::TIME_BASED) {
        lcd.print((int)(config.intervalOFF / 1000));
        lcd.print(F("s"));
    } else {
        lcd.print(config.distanceOFF, 1);
        lcd.print(F("m"));
    }
    
    lcd.printMenuIndicator(4, 8);
}

void MenuManager::renderIntermittent() {
    auto& lcd = LCDManager::getInstance();
    auto& intermittent = IntermittentManager::getInstance();
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("INT:"));
    bool isOn = intermittent.isSprayerOn();
    lcd.print(isOn ? F("ON ") : F("OFF"));
    lcd.print(F(" "));
    lcd.printProgressBar((int)intermittent.getProgress());
    
    lcd.setCursor(0, 1);
    lcd.print(F("Ch: "));
    lcd.print(intermittent.getSprayerLeft() ? 'O' : '.');
    lcd.print(intermittent.getSprayerCenter() ? 'O' : '.');
    lcd.print(intermittent.getSprayerRight() ? 'O' : '.');
    
    lcd.printMenuIndicator(5, 8);
}

void MenuManager::renderSprayer() {
    auto& lcd = LCDManager::getInstance();
    auto& relay = RelayManager::getInstance();
    auto& waterLevel = WaterLevelManager::getInstance();
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("SPRAY: "));
    lcd.print(relay.getRelayState(RelayType::SPRAYER_LH) ? 'O' : '.');
    lcd.print(relay.getRelayState(RelayType::SPRAYER_CENTER) ? 'O' : '.');
    lcd.print(relay.getRelayState(RelayType::SPRAYER_RH) ? 'O' : '.');
    lcd.print(F(" P:"));
    lcd.print(relay.getRelayState(RelayType::WATER_PUMP) ? 'O' : '.');
    
    lcd.setCursor(0, 1);
    lcd.print(F("ROT:"));
    lcd.print(relay.getRelayState(RelayType::ROTARY) ? 'O' : '.');
    lcd.print(F(" LMP:"));
    lcd.print(relay.getRelayState(RelayType::WORKING_LAMP) ? 'O' : '.');
    
    if (!waterLevel.isWaterAvailable()) {
        lcd.setCursor(12, 1);
        lcd.print(F("!W!"));
    }
    
    lcd.printMenuIndicator(6, 8);
}

void MenuManager::renderSensor() {
    auto& lcd = LCDManager::getInstance();
    auto& sensor = SensorManager::getInstance();
    auto& rtc = RTCManager::getInstance();
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Pulse:"));
    lcd.print((int)sensor.getPulseCount());
    lcd.print(F(" RTC:"));
    lcd.print(rtc.isRunning() ? F("OK") : F("ERR"));
    
    lcd.setCursor(0, 1);
    lcd.print(F("Wheel:"));
    lcd.print(1.8, 1);
    lcd.print(F("m P/R:"));
    lcd.print(20);
    
    lcd.printMenuIndicator(7, 8);
}

void MenuManager::renderWaterLevel() {
    auto& lcd = LCDManager::getInstance();
    auto& waterLevel = WaterLevelManager::getInstance();
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("WATER: "));
    
    if (waterLevel.isTankEmpty()) {
        lcd.print(F("EMPTY ❌"));
    } else if (waterLevel.isSystemBlocked()) {
        lcd.print(F("LOCKED ⚠️"));
        lcd.setCursor(0, 1);
        lcd.print(F("Cooldown: "));
        lcd.print(waterLevel.getCooldownRemaining());
        lcd.print(F("s"));
    } else {
        lcd.print(F("OK ✅"));
        lcd.setCursor(0, 1);
        lcd.print(F("System: NORMAL"));
    }
    
    lcd.printMenuIndicator(8, 8);
}

// ============================================================
// HELPER FUNCTIONS
// ============================================================

void MenuManager::drawSprayerStatus() {
    // Implementation if needed
}

void MenuManager::drawProgressBar(int progress) {
    auto& lcd = LCDManager::getInstance();
    lcd.printProgressBar(progress);
}

void MenuManager::drawStatusIcon(bool status) {
    auto& lcd = LCDManager::getInstance();
    lcd.print(status ? 'O' : '.');
}