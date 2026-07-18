#ifndef CONSTANTS_H
#define CONSTANTS_H
// FRAM FM24C64
#define FRAM_I2C_ADDRESS     0x50
#define FRAM_CONFIG_START    0x0000
#define FRAM_KPI_START       0x0040
#define FRAM_CHECKSUM_ADDR   0x00FE

// Pin Definitions
// Output Pins
#define RELAY_SPRAYER_RH     9
#define RELAY_SPRAYER_CENTER 12
#define RELAY_SPRAYER_LH     11
#define RELAY_ROTARY         6
#define RELAY_AUTO_MAN_IND   7
#define RELAY_WORKING_LAMP   8
#define PIN_MOSFET_GATE      13  
#define RELAY_WATER_PUMP     10
#define PIN_WATER_LEVEL      4   // Digital input (dengan optocoupler) untuk water level

// Input Pins
#define PIN_THROTTLE_PEDAL   A6
#define PIN_TIMER_POT        A7
#define BUTTON_SPRAYER_RH    A0
#define BUTTON_SPRAYER_CENTER A1
#define BUTTON_SPRAYER_LH    A2
// Engine Detection Pin
#define PIN_ENGINE_RUNNING    A3   // Sensor engine running (12V -> 5V via divider)
// atau
//#define PIN_IGNITION_SWITCH   A3   // Atau langsung dari kunci kontak

// Engine Running Threshold (untuk analog sensor)
#define ENGINE_RUNNING_THRESHOLD  512  // 2.5V untuk 5V reference
// Dry run protection timeout (detik) - berapa lama pump boleh running tanpa air
#define PUMP_DRY_RUN_TIMEOUT  5     // 5 detik
// Cooldown time setelah dry run (detik) - pump tidak boleh jalan selama ini
#define PUMP_COOLDOWN_TIME    5    // 60 detik
// Kapasitas tanki (liter) - untuk display saja
#define WATER_TANK_CAPACITY   50000 // 50,000 Liter

// Speed Sensor Pin
#define PIN_SPEED_SENSOR     2

// Default Values
#define DEFAULT_WHEEL_CIRCUMFERENCE 1.8f  // meter
#define DEFAULT_PULSE_PER_REV 20
#define DEFAULT_DISTANCE_ON   5.0f   // meter
#define DEFAULT_DISTANCE_OFF  10.0f  // meter
#define DEFAULT_INTERVAL_ON   3000   // ms
#define DEFAULT_INTERVAL_OFF  5000   // ms
// Button timing
#define BUTTON_LONG_PRESS_TIME_MS  800   // durasi untuk long press (ms)
#define BUTTON_DEBOUNCE_MS         30    // durasi debounce filter (ms), redam noise/chattering saklar
#define BUTTON_DEBOUNCE_TIME_MS     50   // debounce time (ms)

// Menu Definitions
#define MENU_COUNT 8

// Uncomment untuk enable debug 
/*
#define DEBUG_FRAM      // untuk log FRAMManager
#define ENABLE_POWER_LOSS_DETECT
#define DEBUG_MAIN
#define DEBUG_WATER_LEVEL
#define DEBUG_CONFIG    // untuk log ConfigManager
#define DEBUG_ENGINE
#define ENABLE_WATCHDOG

*/

#endif