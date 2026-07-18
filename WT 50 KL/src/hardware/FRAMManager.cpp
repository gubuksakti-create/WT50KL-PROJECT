#include "FRAMManager.h"

FRAMManager& FRAMManager::getInstance() {
    static FRAMManager instance;
    return instance;
}

bool FRAMManager::begin() {
    Wire.begin();
    Wire.setClock(400000L); // 400 kHz untuk kinerja
    Wire.beginTransmission(deviceAddress);
    uint8_t error = Wire.endTransmission();
    connected = (error == 0);
    #ifdef DEBUG_FRAM
    if (connected) Serial.println(F("FRAM detected."));
    else Serial.println(F("FRAM not detected!"));
    #endif
    return connected;
}

bool FRAMManager::writeByteInternal(uint16_t addr, uint8_t data) {
    Wire.beginTransmission(deviceAddress);
    Wire.write((uint8_t)(addr >> 8));
    Wire.write((uint8_t)(addr & 0xFF));
    Wire.write(data);
    return (Wire.endTransmission() == 0);
}

uint8_t FRAMManager::readByteInternal(uint16_t addr) {
    Wire.beginTransmission(deviceAddress);
    Wire.write((uint8_t)(addr >> 8));
    Wire.write((uint8_t)(addr & 0xFF));
    if (Wire.endTransmission(false) != 0) return 0xFF;
    Wire.requestFrom(deviceAddress, (uint8_t)1);
    if (Wire.available()) return Wire.read();
    return 0xFF;
}

bool FRAMManager::writeByte(uint16_t addr, uint8_t data, uint8_t retries) {
    if (!connected) return false;
    for (uint8_t i = 0; i < retries; i++) {
        if (writeByteInternal(addr, data)) return true;
        delay(1);
    }
    return false;
}

uint8_t FRAMManager::readByte(uint16_t addr, uint8_t retries) {
    if (!connected) return 0xFF;
    for (uint8_t i = 0; i < retries; i++) {
        uint8_t val = readByteInternal(addr);
        if (val != 0xFF) return val; // asumsi 0xFF hanya error
        delay(1);
    }
    return 0xFF;
}

bool FRAMManager::writeBlock(uint16_t addr, const uint8_t* data, size_t len, uint8_t retries) {
    if (!connected) return false;
    for (size_t i = 0; i < len; i++) {
        if (!writeByte(addr + i, data[i], retries)) return false;
    }
    return true;
}

bool FRAMManager::readBlock(uint16_t addr, uint8_t* buffer, size_t len, uint8_t retries) {
    if (!connected) return false;
    for (uint8_t attempt = 0; attempt < retries; attempt++) {
        Wire.beginTransmission(deviceAddress);
        Wire.write((uint8_t)(addr >> 8));
        Wire.write((uint8_t)(addr & 0xFF));
        if (Wire.endTransmission(false) != 0) continue;
        Wire.requestFrom(deviceAddress, (uint8_t)len);
        size_t i = 0;
        while (Wire.available() && i < len) {
            buffer[i++] = Wire.read();
        }
        if (i == len) return true;
        delay(1);
    }
    return false;
}