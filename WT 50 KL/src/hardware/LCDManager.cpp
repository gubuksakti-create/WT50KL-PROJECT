#include "LCDManager.h"
#include <Wire.h>

LCDManager& LCDManager::getInstance() {
    static LCDManager instance;
    return instance;
}

void LCDManager::begin() {
    Wire.setClock(100000L); // paksa balik ke 100kHz, LCD PCF8574 kebanyakan cuma stabil di speed ini
    lcd.init();
    lcd.backlight();
    lcd.clear();
}

void LCDManager::clear() {
    lcd.clear();
}

void LCDManager::setCursor(int col, int row) {
    lcd.setCursor(col, row);
}

void LCDManager::print(const char* text) {
    lcd.print(text);
}

void LCDManager::print(const __FlashStringHelper* text) {
    lcd.print(text);
}

void LCDManager::print(int value) {
    lcd.print(value);
}

void LCDManager::print(unsigned long value) {
    lcd.print(value);
}

void LCDManager::print(float value, int decimals) {
    lcd.print(value, decimals);
}

void LCDManager::printProgressBar(int progress, int length) {
    int filled = (progress * length) / 100;
    char bar[17];
    for (int i = 0; i < length; i++) {
        bar[i] = (i < filled) ? '#' : '-';
    }
    bar[length] = '\0';
    lcd.print(bar);
}

void LCDManager::printCentered(int row, const char* text) {
    int len = strlen(text);
    if (len > 16) len = 16;
    lcd.setCursor((16 - len) / 2, row);
    lcd.print(text);
}

void LCDManager::printMenuIndicator(int currentMenu, int totalMenu) {
    lcd.setCursor(0, 1);
    char indicator[7];
    snprintf(indicator, sizeof(indicator), "[%d/%d]", currentMenu, totalMenu);
    lcd.print(indicator);
}