#ifndef RELAY_MANAGER_H
#define RELAY_MANAGER_H

#include <Arduino.h>
#include "constants.h"

enum class RelayType {
    SPRAYER_RH,
    SPRAYER_CENTER,
    SPRAYER_LH,
    ROTARY,
    AUTO_MAN_IND,
    WORKING_LAMP,
    WATER_PUMP,
    // NEW: Water level indicator
    WATER_LEVEL_ALARM     // Optional: Buzzer/LED untuk alarm
};

class RelayManager {
public:
    static RelayManager& getInstance();
    
    void begin();
    
    // Set relay state
    void setRelay(RelayType relay, bool state);
    bool toggleRelay(RelayType relay);
    
    // Get relay state
    bool getRelayState(RelayType relay) const;
    
    // Turn all relays off (emergency stop)
    void allOff();
    
    // Get pin for relay
    int getPinForRelay(RelayType relay) const;
    
    // Update all relay outputs
    void updateOutputs();
    
    // MOSFET gate control
    void setMosfetGate(int value);
    
private:
    RelayManager() {}
    RelayManager(const RelayManager&) = delete;
    RelayManager& operator=(const RelayManager&) = delete;
    
    // Relay states
    bool relayStates[7] = {false}; // 7 relays total
    bool outputDirty = false;
    
    // Pin mapping
    static const int relayPins[7];
    
    // Internal method to write to physical pin
    void writeRelay(RelayType relay);
};

#endif