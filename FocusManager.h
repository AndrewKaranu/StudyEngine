/**
 * Focus Manager - Global Focus Mode for Study Engine
 * Monitors phone docking (IR sensor) and user presence (PIR sensor)
 * Shows popup warnings when conditions are not met
 */

#ifndef FOCUS_MANAGER_H
#define FOCUS_MANAGER_H

#include "config.h"
#include "DisplayManager.h"
#include "InputManager.h"
#include "UIManager.h"

class FocusManager {
public:
    // Initialize focus manager
    void begin();
    
    // Check focus conditions and show popup if violated
    // Returns true if focus is OK, false if showing warning
    // Call this in loop() - handles its own timing
    bool checkFocus();
    
    // Dismiss the current warning popup
    void dismissWarning();
    
    // Enable/disable focus mode
    void setFocusMode(bool enabled);
    bool isFocusModeEnabled() const { return focusModeEnabled; }
    
    // Check if currently showing warning
    bool isShowingWarning() const { return showingWarning; }
    
    // Get current sensor states
    bool isPhoneDocked() const { return phoneDocked; }
    bool isUserPresent() const { return userPresent; }
    
    // Temporarily pause focus checks (e.g., during Scanatron exam)
    void pauseFocusChecks() { focusPaused = true; }
    void resumeFocusChecks() { focusPaused = false; }
    bool isFocusPaused() const { return focusPaused; }
    
    // Save/load settings (could use Preferences in future)
    void setSettings(int irThreshold, unsigned long presenceTimeout);

private:
    bool focusModeEnabled = false;
    bool focusPaused = false;
    bool showingWarning = false;
    
    // Sensor states
    bool phoneDocked = true;
    bool userPresent = true;
    
    // Timing
    unsigned long lastMotionTime = 0;
    unsigned long lastCheckTime = 0;
    unsigned long warningShownTime = 0;
    
    // Settings
    int irThreshold = 2000;
    unsigned long presenceTimeout = 60000; // 1 minute
    const unsigned long CHECK_INTERVAL = 500; // Check every 500ms
    const unsigned long WARNING_COOLDOWN = 5000; // Don't spam warnings
    
    // Internal methods
    void updateSensorStates();
    void showFocusWarning(bool phoneIssue, bool presenceIssue);
};

// Global instance
extern FocusManager focusMgr;

#endif
