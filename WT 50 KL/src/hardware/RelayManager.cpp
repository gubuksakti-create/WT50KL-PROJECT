#include "RelayManager.h"

// Static pin mapping
const int RelayManager::relayPins[7] = {
    RELAY_SPRAYER_RH,      // SPRAYER_RH
    RELAY_SPRAYER_CENTER,  // SPRAYER_CENTER
    RELAY_SPRAYER_LH,      // SPRAYER_LH
    RELAY_ROTARY,          // ROTARY
    RELAY_AUTO_MAN_IND,    // AUTO_MAN_IND
    RELAY_WORKING_LAMP,    // WORKING_LAMP
    RELAY_WATER_PUMP       // WATER_PUMP
};

RelayManager& RelayManager::getInstance() {
    static RelayManager instance;
    return instance;
}

void RelayManager::begin() {
    // Initialize all relay pins as outputs
    for (int i = 0; i < 7; i++) {
        pinMode(relayPins[i], OUTPUT);
        digitalWrite(relayPins[i], LOW); // All off initially
    }
    
    // Initialize MOSFET gate pin (PWM capable)
    pinMode(PIN_MOSFET_GATE, OUTPUT);
    analogWrite(PIN_MOSFET_GATE, 0);
    
    // Initialize all states to false
    for (int i = 0; i < 7; i++) {
        relayStates[i] = false;
    }
    
    outputDirty = false;
}

void RelayManager::setRelay(RelayType relay, bool state) {
    int index = static_cast<int>(relay);
    if (index >= 0 && index < 7) {
        relayStates[index] = state;
        outputDirty = true;
        writeRelay(relay);
    }
}

bool RelayManager::toggleRelay(RelayType relay) {
    int index = static_cast<int>(relay);
    if (index >= 0 && index < 7) {
        relayStates[index] = !relayStates[index];
        outputDirty = true;
        writeRelay(relay);
        return relayStates[index];
    }
    return false;
}

bool RelayManager::getRelayState(RelayType relay) const {
    int index = static_cast<int>(relay);
    if (index >= 0 && index < 7) {
        return relayStates[index];
    }
    return false;
}

void RelayManager::allOff() {
    for (int i = 0; i < 7; i++) {
        relayStates[i] = false;
        digitalWrite(relayPins[i], LOW);
    }
    analogWrite(PIN_MOSFET_GATE, 0);
    outputDirty = false;
}

int RelayManager::getPinForRelay(RelayType relay) const {
    int index = static_cast<int>(relay);
    if (index >= 0 && index < 7) {
        return relayPins[index];
    }
    return -1;
}

void RelayManager::updateOutputs() {
    if (outputDirty) {
        for (int i = 0; i < 7; i++) {
            digitalWrite(relayPins[i], relayStates[i] ? HIGH : LOW);
        }
        outputDirty = false;
    }
}

void RelayManager::writeRelay(RelayType relay) {
    int index = static_cast<int>(relay);
    if (index >= 0 && index < 7) {
        digitalWrite(relayPins[index], relayStates[index] ? HIGH : LOW);
    }
}

void RelayManager::setMosfetGate(int value) {
    if (value < 0) value = 0;
    if (value > 255) value = 255;
    analogWrite(PIN_MOSFET_GATE, value);
}