// src/ui/MenuManager.h
#ifndef MENU_MANAGER_H
#define MENU_MANAGER_H

#include <Arduino.h>
#include "hardware/ButtonManager.h"

/**
 * MenuManager - Manajemen Menu Display LCD 16x2
 * 
 * Fungsi:
 * 1. 8 halaman menu navigasi
 * 2. Auto-scroll 4 halaman saat menu tidak aktif
 * 3. Water Level Safety priority display
 * 4. Navigasi: B1 (Prev), B2 (Next), B3 (Enter/Exit)
 */
enum class MenuPage {
    MAIN = 0,
    KPI,
    SPEED,
    CONFIG,
    INTERMITTENT,
    SPRAYER,
    SENSOR,
    WATER,
    NONE
};

class MenuManager {
public:
    static MenuManager& getInstance();
    
    // Lifecycle
    void begin();
    void update();
    void handleButton(int buttonId, ButtonEvent event);
    
    // Menu control
    bool isMenuActive() const { return menuActive; }
    void setMenuActive(bool active);
    void toggleMenu();
    
    void nextPage();
    void previousPage();
    void selectPage();
    
    MenuPage getCurrentPage() const { return currentPage; }
    void renderCurrentPage();
    
private:
    MenuManager() {}
    MenuManager(const MenuManager&) = delete;
    MenuManager& operator=(const MenuManager&) = delete;
    
    // ============================================================
    // MEMBERS
    // ============================================================
    bool menuActive = false;
    MenuPage currentPage = MenuPage::MAIN;
    
    // Auto-scroll
    unsigned long lastAutoScroll = 0;
    const unsigned long AUTO_SCROLL_INTERVAL = 8000;  // 8 seconds
    int statusPageIndex = 0;
    
    // Blink effect
    unsigned long lastBlinkToggle = 0;
    bool blinkState = false;
    
    // ============================================================
    // RENDER FUNCTIONS
    // ============================================================
    void renderMain();
    void renderKPI();
    void renderSpeed();
    void renderConfig();
    void renderIntermittent();
    void renderSprayer();
    void renderSensor();
    void renderWaterLevel();
    
    // ============================================================
    // AUTO-SCROLL DISPLAY
    // ============================================================
    void displayStatus();
    
    // ============================================================
    // HELPER FUNCTIONS
    // ============================================================
    void drawSprayerStatus();
    void drawProgressBar(int progress);
    void drawStatusIcon(bool status);
};

#endif