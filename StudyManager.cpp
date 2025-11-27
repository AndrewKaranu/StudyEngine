/**
 * Study Manager - Study Timer with LVGL UI
 * Uses PIR for presence detection, IR for phone detection
 */

#include "StudyManager.h"

void StudyManager::reset() {
    startTime = 0;
    pausedTime = 0;
    totalPausedTime = 0;
    isPaused = false;
    isRunning = false;
    needsRedraw = true;
    lastMotionTime = 0;
}

void StudyManager::startSession() {
    isRunning = true;
    isPaused = false;
    startTime = millis();
    totalPausedTime = 0;
    lastMotionTime = millis();
    needsRedraw = true;
}

void StudyManager::stopSession() {
    isRunning = false;
    needsRedraw = true;
}

void StudyManager::update(DisplayManager& display, InputManager& input) {
    static unsigned long lastUpdateTime = 0;
    static unsigned long lastElapsed = 0;
    static bool lastPhoneState = false;
    static bool lastUserPresent = true;
    
    if (!isRunning) {
        // Show start screen
        if (needsRedraw) {
            uiMgr.showStudyStart();
            display.showStatus("Study Timer");
            needsRedraw = false;
        }
        
        if (input.isBtnAPressed()) {
            startSession();
            delay(200);
        }
        return;
    }

    // PIR Check (Presence)
    if (digitalRead(PIN_PIR) == HIGH) {
        lastMotionTime = millis();
    }
    
    bool userPresent = (millis() - lastMotionTime) < PRESENCE_TIMEOUT;
    
    // IR Check (Phone Distraction)
    int irVal = analogRead(PIN_IR);
    bool phoneDetected = (irVal > IR_THRESHOLD);

    // Calculate elapsed time
    unsigned long elapsed;
    if (isPaused) {
        elapsed = (pausedTime - startTime - totalPausedTime) / 1000;
    } else {
        elapsed = (millis() - startTime - totalPausedTime) / 1000;
    }
    
    // Timer Logic - auto pause when user away
    if (!userPresent && !isPaused) {
        isPaused = true;
        pausedTime = millis();
        needsRedraw = true;
    }

    // Phone detected - alert!
    if (phoneDetected && !lastPhoneState) {
        // Sound alert on state change only
        ledcWriteTone(PIN_SPKR, 1000);
        delay(50);
        ledcWriteTone(PIN_SPKR, 0);
    }

    // Update OLED with timer (this is lightweight)
    display.showStudyTimer(elapsed, isPaused);
    
    // Only update TFT screen if something changed (every second or state change)
    bool stateChanged = (phoneDetected != lastPhoneState) || 
                        (userPresent != lastUserPresent) ||
                        needsRedraw;
    
    if (stateChanged || (elapsed != lastElapsed && (millis() - lastUpdateTime) > 1000)) {
        uiMgr.showStudyTimer(elapsed, isPaused, phoneDetected, !userPresent);
        lastUpdateTime = millis();
        lastElapsed = elapsed;
        needsRedraw = false;
    }
    
    lastPhoneState = phoneDetected;
    lastUserPresent = userPresent;

    // Button A: Toggle Pause/Resume
    if (input.isBtnAPressed()) {
        if (isPaused) {
            // Resume
            totalPausedTime += (millis() - pausedTime);
            isPaused = false;
        } else {
            // Pause
            pausedTime = millis();
            isPaused = true;
        }
        needsRedraw = true;
        delay(200);
    }
    
    // Button B: Stop session
    if (input.isBtnBPressed()) {
        stopSession();
        delay(200);
    }
}
