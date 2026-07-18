#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>
#include "constants.h"

enum class ButtonEvent {
    NONE,
    SHORT_PRESS,
    LONG_PRESS,
    COMBINATION_B1_B2, // Tambahkan ini
    COMBINATION_B2_B3, // Tambahkan ini
    COMBINATION_ALL    // B1+B2+B3 ditahan bareng -> Masuk/Keluar Menu
};
struct Button {
    int pin;
    bool state;
    unsigned long pressTime;
    unsigned long releaseTime;
    bool isPressed;
    bool longPressTriggered;
    ButtonEvent lastEvent;
    bool eventProcessed;  // ← TAMBAHKAN
    bool lastRawReading;       // untuk debounce
    unsigned long lastDebounceTime; // untuk debounce
};

class ButtonManager {
public:
    static ButtonManager& getInstance();
    
    void begin();
    void update();
    
    ButtonEvent getButton1Event() const { return button1.lastEvent; }
    ButtonEvent getButton2Event() const { return button2.lastEvent; }
    ButtonEvent getButton3Event() const { return button3.lastEvent; }
    
    bool isButton1Pressed() const { return button1.isPressed; }
    bool isButton2Pressed() const { return button2.isPressed; }
    bool isButton3Pressed() const { return button3.isPressed; }
    
    void clearEvents();
    void markEventProcessed();  // ← TAMBAHKAN DEKLARASI INI
    
private:
    ButtonManager() {}
    ButtonManager(const ButtonManager&) = delete;
    ButtonManager& operator=(const ButtonManager&) = delete;
    
    Button button1 {BUTTON_SPRAYER_RH, HIGH, 0, 0, false, false, ButtonEvent::NONE, false, false, 0};
    Button button2 {BUTTON_SPRAYER_CENTER, HIGH, 0, 0, false, false, ButtonEvent::NONE, false, false, 0};
    Button button3 {BUTTON_SPRAYER_LH, HIGH, 0, 0, false, false, ButtonEvent::NONE, false, false, 0};

    bool combo12Fired = false;  // guard biar COMBINATION_B1_B2 cuma fire sekali per sesi tahan
    bool combo23Fired = false;  // guard biar COMBINATION_B2_B3 cuma fire sekali per sesi tahan
    bool combo123Fired = false; // guard biar COMBINATION_ALL (masuk menu) cuma fire sekali per sesi tahan
    
    void processButton(Button& btn, bool suppressIndividualLongPress);
    void checkCombinations();
};

#endif