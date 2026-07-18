#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include "constants.h"
#include "hardware/FRAMManager.h"

// ============================================================
// ENUMS
// ============================================================
enum class TriggerMode {
    TIME_BASED,
    DISTANCE_BASED
};

enum class IntermittentMode {
    SPRAYER,
    PUMP
};

// ============================================================
// STRUCTS
// ============================================================
struct SystemConfig {
    TriggerMode triggerMode;
    IntermittentMode intermittentMode;
    unsigned long intervalON;
    unsigned long intervalOFF;
    float distanceON;
    float distanceOFF;
    float wheelCircumference;
    int pulsePerRev;
    bool usePotentiometer;
};

struct KPI {
    float totalDistanceSprayed;   // meter
    unsigned long engineHours;    // detik
};

// ============================================================
// CONFIG MANAGER
// ============================================================
class ConfigManager {
public:
    static ConfigManager& getInstance();

    // Lifecycle
    void begin();
    void saveConfig();
    void loadConfig();
    void resetToDefaults();

    // Config
    SystemConfig getConfig() const { return config; }
    void setConfig(const SystemConfig& newConfig);

    // KPI
    KPI getKPI() const { return kpi; }
    void setKPI(const KPI& newKPI);

    // Update KPI (dengan cache)
    void updateKPIDistance(float distance);
    void updateEngineHours(unsigned long seconds);          // RAM only
    void forceWriteEngineHours(unsigned long seconds);     // saat engine mati
    void forceWriteToFRAM();                               // flush semua cache

    // Status
    bool hasPendingSave() const;

    // Shortcut
    TriggerMode getTriggerMode() const { return config.triggerMode; }
    void setTriggerMode(TriggerMode mode);
    IntermittentMode getIntermittentMode() const { return config.intermittentMode; }
    void setIntermittentMode(IntermittentMode mode);

    // Debug
    #ifdef DEBUG_CONFIG
    void printStatus();
    #endif

private:
    ConfigManager() {}
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    // ============================================================
    // DATA
    // ============================================================
    SystemConfig config;
    KPI kpi;

    // Cache KPI
    KPI cachedKPI;
    bool kpiDirty = false;
    bool engineStopPending = false;
    unsigned long lastFRAMWrite = 0;

    // Interval write (1 jam)
    static constexpr unsigned long FRAM_WRITE_INTERVAL = 3600000UL;

    // ============================================================
    // FRAM OPERATIONS
    // ============================================================
    void writeConfigToFRAM();
    void readConfigFromFRAM();
    void writeKPItoFRAM();
    void readKPIFromFRAM();
    void writeChecksum();
    bool verifyChecksum();

    void flushKPICache();
};

#endif