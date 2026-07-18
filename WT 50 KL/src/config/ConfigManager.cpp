#include "ConfigManager.h"

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

// ============================================================
// INIT & LOAD
// ============================================================
void ConfigManager::begin() {
    if (!FRAMManager::getInstance().begin()) {
        #ifdef DEBUG_CONFIG
        Serial.println(F("FRAM init failed, using defaults."));
        #endif
        resetToDefaults();
    } else {
        loadConfig();
    }
    lastFRAMWrite = millis();
}

void ConfigManager::loadConfig() {
    if (!verifyChecksum()) {
        resetToDefaults();
        return;
    }
    readConfigFromFRAM();
    readKPIFromFRAM();
    cachedKPI = kpi;
    kpiDirty = false;
}

void ConfigManager::resetToDefaults() {
    config.triggerMode = TriggerMode::TIME_BASED;
    config.intermittentMode = IntermittentMode::SPRAYER;
    config.intervalON = DEFAULT_INTERVAL_ON;
    config.intervalOFF = DEFAULT_INTERVAL_OFF;
    config.distanceON = DEFAULT_DISTANCE_ON;
    config.distanceOFF = DEFAULT_DISTANCE_OFF;
    config.wheelCircumference = DEFAULT_WHEEL_CIRCUMFERENCE;
    config.pulsePerRev = DEFAULT_PULSE_PER_REV;
    config.usePotentiometer = false;

    kpi.totalDistanceSprayed = 0;
    kpi.engineHours = 0;
    cachedKPI = kpi;
    kpiDirty = false;
    engineStopPending = false;

    saveConfig();
}

void ConfigManager::saveConfig() {
    writeConfigToFRAM();
    writeKPItoFRAM();
    writeChecksum();
}

// ============================================================
// CONFIG SETTERS
// ============================================================
void ConfigManager::setConfig(const SystemConfig& newConfig) {
    config = newConfig;
    saveConfig();
}

void ConfigManager::setTriggerMode(TriggerMode mode) {
    config.triggerMode = mode;
    saveConfig();
}

void ConfigManager::setIntermittentMode(IntermittentMode mode) {
    config.intermittentMode = mode;
    saveConfig();
}

void ConfigManager::setKPI(const KPI& newKPI) {
    kpi = newKPI;
    cachedKPI = kpi;
    writeKPItoFRAM();
    writeChecksum();
}

// ============================================================
// KPI UPDATE (Cache)
// ============================================================
void ConfigManager::updateKPIDistance(float distance) {
    if (distance <= 0) return;
    cachedKPI.totalDistanceSprayed += distance;
    kpiDirty = true;
    flushKPICache();
}

void ConfigManager::updateEngineHours(unsigned long seconds) {
    if (seconds == 0) return;
    cachedKPI.engineHours += seconds;
    kpiDirty = true;
    engineStopPending = true;

    #ifdef DEBUG_CONFIG
    Serial.print(F("📊 Engine hours updated (RAM): "));
    Serial.print(seconds);
    Serial.print(F("s, Total: "));
    Serial.print(cachedKPI.engineHours);
    Serial.println(F("s"));
    #endif
}

void ConfigManager::flushKPICache() {
    unsigned long now = millis();
    if (kpiDirty && (now - lastFRAMWrite >= FRAM_WRITE_INTERVAL)) {
        kpi = cachedKPI;
        writeKPItoFRAM();
        writeChecksum();
        lastFRAMWrite = now;
        kpiDirty = false;

        #ifdef DEBUG_CONFIG
        Serial.print(F("💾 FRAM Flushed (periodic): "));
        Serial.print(kpi.totalDistanceSprayed);
        Serial.print(F("m, Hours: "));
        Serial.print(kpi.engineHours / 3600);
        Serial.println(F("h"));
        #endif
    }
}

// ============================================================
// FORCE WRITE (Engine Stop / Emergency)
// ============================================================
void ConfigManager::forceWriteEngineHours(unsigned long seconds) {
    if (seconds == 0) return;
    kpi.engineHours = cachedKPI.engineHours;
    writeKPItoFRAM();
    writeChecksum();
    engineStopPending = false;
    kpiDirty = false;
    lastFRAMWrite = millis();

    #ifdef DEBUG_CONFIG
    Serial.print(F("💾 FRAM Saved (Engine Stop): "));
    Serial.print(seconds);
    Serial.print(F("s, Total Engine Hours: "));
    Serial.print(kpi.engineHours / 3600);
    Serial.print(F("h "));
    Serial.print((kpi.engineHours % 3600) / 60);
    Serial.println(F("m"));
    #endif
}

void ConfigManager::forceWriteToFRAM() {
    if (kpiDirty) {
        kpi = cachedKPI;
        writeKPItoFRAM();
        writeChecksum();
        kpiDirty = false;
        lastFRAMWrite = millis();
        engineStopPending = false;

        #ifdef DEBUG_CONFIG
        Serial.println(F("💾 FRAM Force Write Complete"));
        #endif
    }
}

bool ConfigManager::hasPendingSave() const {
    return kpiDirty || engineStopPending;
}

// ============================================================
// FRAM I/O
// ============================================================
void ConfigManager::writeConfigToFRAM() {
    FRAMManager::getInstance().writeBlock(FRAM_CONFIG_START, (uint8_t*)&config, sizeof(SystemConfig));
}

void ConfigManager::readConfigFromFRAM() {
    FRAMManager::getInstance().readBlock(FRAM_CONFIG_START, (uint8_t*)&config, sizeof(SystemConfig));
}

void ConfigManager::writeKPItoFRAM() {
    FRAMManager::getInstance().writeBlock(FRAM_KPI_START, (uint8_t*)&kpi, sizeof(KPI));
}

void ConfigManager::readKPIFromFRAM() {
    FRAMManager::getInstance().readBlock(FRAM_KPI_START, (uint8_t*)&kpi, sizeof(KPI));
}

void ConfigManager::writeChecksum() {
    FRAMManager::getInstance().writeByte(FRAM_CHECKSUM_ADDR, 0xAA);
}

bool ConfigManager::verifyChecksum() {
    return (FRAMManager::getInstance().readByte(FRAM_CHECKSUM_ADDR) == 0xAA);
}

// ============================================================
// DEBUG
// ============================================================
#ifdef DEBUG_CONFIG
void ConfigManager::printStatus() {
    Serial.println(F("=== ConfigManager Status ==="));
    Serial.print(F("Config: "));
    Serial.print(config.triggerMode == TriggerMode::TIME_BASED ? F("TIME") : F("DIST"));
    Serial.print(F(" / "));
    Serial.println(config.intermittentMode == IntermittentMode::SPRAYER ? F("SPRAYER") : F("PUMP"));

    Serial.print(F("KPI - Distance: "));
    Serial.print(kpi.totalDistanceSprayed);
    Serial.println(F("m"));

    Serial.print(F("KPI - Engine Hours: "));
    Serial.print(kpi.engineHours / 3600);
    Serial.print(F("h "));
    Serial.print((kpi.engineHours % 3600) / 60);
    Serial.println(F("m"));

    Serial.print(F("Cache Dirty: "));
    Serial.println(kpiDirty ? F("YES") : F("NO"));
    Serial.print(F("Pending Save: "));
    Serial.println(engineStopPending ? F("YES") : F("NO"));
    Serial.print(F("Last FRAM Write: "));
    Serial.print((millis() - lastFRAMWrite) / 1000);
    Serial.println(F("s ago"));
    Serial.println(F("============================"));
}
#endif