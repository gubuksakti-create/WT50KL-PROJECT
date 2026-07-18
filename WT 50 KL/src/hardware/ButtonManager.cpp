// ButtonManager.cpp
#include "ButtonManager.h"

ButtonManager& ButtonManager::getInstance() {
    static ButtonManager instance;
    return instance;
}

void ButtonManager::begin() {
    pinMode(BUTTON_SPRAYER_RH, INPUT_PULLUP);
    pinMode(BUTTON_SPRAYER_CENTER, INPUT_PULLUP);
    pinMode(BUTTON_SPRAYER_LH, INPUT_PULLUP);
}

void ButtonManager::update() {
    // Hitung dulu berapa tombol yang lagi ditekan bareng SEBELUM diproses,
    // biar processButton() individual tau harus nunda long-press-nya atau nggak.
    bool b1Down = (digitalRead(button1.pin) == LOW);
    bool b2Down = (digitalRead(button2.pin) == LOW);
    bool b3Down = (digitalRead(button3.pin) == LOW);
    int downCount = (b1Down ? 1 : 0) + (b2Down ? 1 : 0) + (b3Down ? 1 : 0);
    bool multiPress = (downCount >= 2); // 2 atau 3 tombol bareng -> ini kandidat kombinasi

    processButton(button1, multiPress); // Memproses Button 1 (Right Sprayer)
    processButton(button2, multiPress); // Memproses Button 2 (Center Sprayer)
    processButton(button3, multiPress); // Memproses Button 3 (Left Sprayer)
    checkCombinations();    // Deteksi kombinasi B1+B2, B2+B3, dan B1+B2+B3
}

void ButtonManager::processButton(Button &btn, bool suppressIndividualLongPress) {
    // 1. Baca status fisik pin (Active LOW menggunakan INPUT_PULLUP)
    bool rawReading = (digitalRead(btn.pin) == LOW);

    // 2. DEBOUNCE: kalau raw reading berubah, reset timer debounce, JANGAN proses dulu
    if (rawReading != btn.lastRawReading) {
        btn.lastDebounceTime = millis();
        btn.lastRawReading = rawReading;
    }

    // Kalau belum stabil selama BUTTON_DEBOUNCE_MS, abaikan (masih bouncing)
    if ((millis() - btn.lastDebounceTime) < BUTTON_DEBOUNCE_MS) {
        return;
    }

    bool currentReading = rawReading;

    // 3. Deteksi Perubahan Status (setelah lolos debounce)
    if (currentReading != btn.isPressed) {
        // Jika status baru saja berubah menjadi DITEKAN
        if (currentReading == true) {
            btn.pressTime = millis();
            btn.longPressTriggered = false;
            btn.eventProcessed = false;
        } 
        // Jika status baru saja DILEPAS
        else {
            btn.releaseTime = millis();
            // Jika tombol dilepas dan BELUM pernah memicu Long Press, maka ini Short Press
            if (!btn.longPressTriggered && !btn.eventProcessed) {
                btn.lastEvent = ButtonEvent::SHORT_PRESS;
            }
        }
        // Simpan status stabil saat ini
        btn.isPressed = currentReading;
    }

    // 4. Deteksi kondisi LONG PRESS (Saat tombol masih ditahan)
    // JANGAN fire individual long press kalau ada tombol lain yang lagi ditekan bareng
    // (biar keputusan diserahkan ke checkCombinations(), nggak race condition)
    if (btn.isPressed && !btn.longPressTriggered && !suppressIndividualLongPress) {
        if ((millis() - btn.pressTime) >= BUTTON_LONG_PRESS_TIME_MS) { // Menggunakan konstanta dari constants.h
            btn.longPressTriggered = true;
            btn.lastEvent = ButtonEvent::LONG_PRESS;
            btn.eventProcessed = false;
        }
    }
}

void ButtonManager::clearEvents() {
    button1.lastEvent = ButtonEvent::NONE;
    button2.lastEvent = ButtonEvent::NONE;
    button3.lastEvent = ButtonEvent::NONE;
    // Reset processed flag juga
    button1.eventProcessed = true;
    button2.eventProcessed = true;
    button3.eventProcessed = true;
}

// Deteksi kombinasi tombol ditahan bareng >= BUTTON_LONG_PRESS_TIME_MS
void ButtonManager::checkCombinations() {
    unsigned long now = millis();

    // PRIORITAS TERTINGGI: Kombinasi SEMUA tombol (B1+B2+B3) → Masuk/Keluar Menu
    if (button1.isPressed && button2.isPressed && button3.isPressed) {
        unsigned long pressStart = button1.pressTime;
        if (button2.pressTime > pressStart) pressStart = button2.pressTime;
        if (button3.pressTime > pressStart) pressStart = button3.pressTime;

        if (!combo123Fired && (now - pressStart) >= BUTTON_LONG_PRESS_TIME_MS) {
            button1.lastEvent = ButtonEvent::COMBINATION_ALL;
            button2.lastEvent = ButtonEvent::COMBINATION_ALL;
            button3.lastEvent = ButtonEvent::COMBINATION_ALL;
            combo123Fired = true;
        }
        // Jangan cek kombinasi 2-tombol kalau ke-3 tombol lagi ditahan bareng
        combo12Fired = combo123Fired;
        combo23Fired = combo123Fired;
        return;
    }
    combo123Fired = false;

    // Kombinasi B1 + B2 → Toggle Intermittent Mode (SPRAYER <-> PUMP)
    if (button1.isPressed && button2.isPressed) {
        unsigned long pressStart = (button1.pressTime > button2.pressTime) ? button1.pressTime : button2.pressTime;
        if (!combo12Fired && (now - pressStart) >= BUTTON_LONG_PRESS_TIME_MS) {
            button1.lastEvent = ButtonEvent::COMBINATION_B1_B2;
            button2.lastEvent = ButtonEvent::COMBINATION_B1_B2;
            combo12Fired = true;
        }
    } else {
        combo12Fired = false;
    }

    // Kombinasi B2 + B3 → Toggle Trigger Mode (TIME <-> DISTANCE)
    if (button2.isPressed && button3.isPressed) {
        unsigned long pressStart = (button2.pressTime > button3.pressTime) ? button2.pressTime : button3.pressTime;
        if (!combo23Fired && (now - pressStart) >= BUTTON_LONG_PRESS_TIME_MS) {
            button2.lastEvent = ButtonEvent::COMBINATION_B2_B3;
            button3.lastEvent = ButtonEvent::COMBINATION_B2_B3;
            combo23Fired = true;
        }
    } else {
        combo23Fired = false;
    }
}

// ← TAMBAHKAN IMPLEMENTASI INI
// PENTING: Fungsi ini WAJIB reset lastEvent balik ke NONE, bukan cuma nandain
// flag eventProcessed. Tanpa ini, event SHORT_PRESS/LONG_PRESS bakal "nyangkut"
// selamanya dan terus ke-proses ulang di SETIAP loop berikutnya, bikin tombol
// yang pertama kali ditekan "mengunci" seluruh sistem tombol lainnya.
void ButtonManager::markEventProcessed() {
    button1.eventProcessed = true;
    button2.eventProcessed = true;
    button3.eventProcessed = true;
    button1.lastEvent = ButtonEvent::NONE;
    button2.lastEvent = ButtonEvent::NONE;
    button3.lastEvent = ButtonEvent::NONE;
}