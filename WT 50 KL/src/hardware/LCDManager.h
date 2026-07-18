#ifndef LCD_MANAGER_H
#define LCD_MANAGER_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

class LCDManager {
public:
    static LCDManager& getInstance();
    
    void begin();
    void clear();
    void setCursor(int col, int row);
    void print(const char* text);
    void print(const __FlashStringHelper* text);
    void print(int value);
    void print(unsigned long value);
    void print(float value, int decimals = 1);
    void printProgressBar(int progress, int length = 16);
    void printCentered(int row, const char* text);
    void printMenuIndicator(int currentMenu, int totalMenu);
    
private:
    LCDManager() : lcd(0x27, 16, 2) {}
    LCDManager(const LCDManager&) = delete;
    LCDManager& operator=(const LCDManager&) = delete;
    
    LiquidCrystal_I2C lcd;
};

#endif