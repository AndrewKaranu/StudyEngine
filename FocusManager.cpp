/**
 * Focus Manager Implementation
 * Monitors focus conditions and shows warnings
 */

#include "FocusManager.h"

// Global instance
FocusManager focusMgr;

void FocusManager::begin() {
    focusModeEnabled = false;
    focusPaused = false;
    showingWarning = false;
    lastMotionTime = millis();
    lastCheckTime = millis();
    warningShownTime = 0;
    
    Serial.println("[FOCUS] Manager initialized");
}

void FocusManager::setSettings(int threshold, unsigned long timeout) {
    irThreshold = threshold;
    presenceTimeout = timeout;
}

void FocusManager::setFocusMode(bool enabled) {
    focusModeEnabled = enabled;
    if (enabled) {
        lastMotionTime = millis(); // Reset presence timer
        showingWarning = false;
    }
    Serial.printf("[FOCUS] Focus mode %s\n", enabled ? "ENABLED" : "DISABLED");
}

void FocusManager::updateSensorStates() {
    // Check PIR for user presence
    if (digitalRead(PIN_PIR) == HIGH) {
        lastMotionTime = millis();
    }
    userPresent = (millis() - lastMotionTime) < presenceTimeout;
    
    // Check IR for phone docking
    // Higher IR value = object closer = phone docked
    int irVal = analogRead(PIN_IR);
    phoneDocked = (irVal > irThreshold);
}

bool FocusManager::checkFocus() {
    // If focus mode disabled or paused, always return OK
    if (!focusModeEnabled || focusPaused) {
        showingWarning = false;
        return true;
    }
    
    // Throttle checks
    unsigned long now = millis();
    if (now - lastCheckTime < CHECK_INTERVAL) {
        return !showingWarning;
    }
    lastCheckTime = now;
    
    // Update sensor states
    updateSensorStates();
    
    // Check conditions
    bool phoneIssue = !phoneDocked;
    bool presenceIssue = !userPresent;
    
    // If all good, clear warning
    if (!phoneIssue && !presenceIssue) {
        if (showingWarning) {
            showingWarning = false;
            // Warning will be cleared by the main app redrawing its screen
        }
        return true;
    }
    
    // Issue detected - check cooldown before showing new warning
    if (!showingWarning && (now - warningShownTime) > WARNING_COOLDOWN) {
        showFocusWarning(phoneIssue, presenceIssue);
        warningShownTime = now;
        showingWarning = true;
        
        // Play alert sound
        ledcAttach(PIN_SPKR, 1000, 8);
        ledcWriteTone(PIN_SPKR, 800);
        delay(100);
        ledcWriteTone(PIN_SPKR, 1000);
        delay(100);
        ledcWriteTone(PIN_SPKR, 0);
    }
    
    return false;
}

void FocusManager::dismissWarning() {
    showingWarning = false;
    lastMotionTime = millis(); // Reset presence timer when dismissed
}

void FocusManager::showFocusWarning(bool phoneIssue, bool presenceIssue) {
    // Build warning message
    String message;
    if (phoneIssue && presenceIssue) {
        message = "Phone not docked &\nUser not detected!";
    } else if (phoneIssue) {
        message = "Phone not docked!\nPlease dock your phone.";
    } else {
        message = "User not detected!\nPlease stay focused.";
    }
    
    // Show using UI Manager's popup method
    uiMgr.showFocusWarning(message.c_str(), phoneIssue, presenceIssue);
    
    Serial.printf("[FOCUS] Warning: Phone=%s, User=%s\n", 
                  phoneIssue ? "MISSING" : "OK",
                  presenceIssue ? "AWAY" : "PRESENT");
}
