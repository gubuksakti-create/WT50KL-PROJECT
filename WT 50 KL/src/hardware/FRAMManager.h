#ifndef FRAM_MANAGER_H
#define FRAM_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include "constants.h"

class FRAMManager {
public:
    static FRAMManager& getInstance();

    bool begin();
    bool isConnected() const { return connected; }

    // Operasi baca/tulis dengan retry
    bool writeByte(uint16_t addr, uint8_t data, uint8_t retries = 3);
    uint8_t readByte(uint16_t addr, uint8_t retries = 3);
    bool writeBlock(uint16_t addr, const uint8_t* data, size_t len, uint8_t retries = 3);
    bool readBlock(uint16_t addr, uint8_t* buffer, size_t len, uint8_t retries = 3);

private:
    FRAMManager() {}
    FRAMManager(const FRAMManager&) = delete;
    FRAMManager& operator=(const FRAMManager&) = delete;

    bool connected = false;
    const uint8_t deviceAddress = FRAM_I2C_ADDRESS;

    // Implementasi internal tanpa retry
    bool writeByteInternal(uint16_t addr, uint8_t data);
    uint8_t readByteInternal(uint16_t addr);
};

#endif