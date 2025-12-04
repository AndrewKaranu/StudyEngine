/**
 * Study Manager Implementation
 * Enhanced Study Timer with Pomodoro Support and Sound Notifications
 */

#include "StudyManager.h"
#include "SettingsManager.h"
#include <Wire.h>

// External LED functions from main sketch
extern void setLed(bool red, bool green);
extern void ledOff();
extern void flashLed(bool red, bool green, int count, int onTime, int offTime);

// Musical notes (frequencies in Hz)
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_REST 0

void StudyManager::reset() {
    isActive = true;  // Mark as active so we stay in this mode
    timerState = TIMER_STATE_SETUP;
    needsRedraw = true;
    setupMenuIndex = 0;
    lastMenuIndex = -1;
    editingValue = false;
    
    // Reset timing
    startTime = 0;
    pausedTime = 0;
    totalPausedTime = 0;
    
    // Reset pomodoro
    pomodoroPhase = POMO_WORK;
    pomodoroCurrentSession = 1;
    
    Serial.println("[STUDY] Reset - entering setup");
}

void StudyManager::update(DisplayManager& display, InputManager& input) {
    static unsigned long lastUpdate = 0;
    
    // Read all buttons FIRST via direct I2C before anything else
    uint16_t pcfRaw = 0xFFFF;
    Wire.requestFrom(0x20, 2);
    if (Wire.available() == 2) {
        pcfRaw = Wire.read();
        pcfRaw |= (Wire.read() << 8);
    }
    
    // Store button states (active low)
    cachedBtnA = !((pcfRaw >> 0) & 1);
    cachedBtnB = !((pcfRaw >> 1) & 1);
    cachedBtnC = !((pcfRaw >> 2) & 1);
    cachedBtnD = !((pcfRaw >> 3) & 1);
    
    switch (timerState) {
        case TIMER_STATE_SETUP:
            handleSetup(input);
            break;
        case TIMER_STATE_RUNNING:
            handleRunning(input);
            break;
        case TIMER_STATE_PAUSED:
            handlePaused(input);
            break;
        case TIMER_STATE_BREAK:
            handleBreak(input);
            break;
        case TIMER_STATE_FINISHED:
            handleFinished(input);
            break;
    }
    
    // Update display every 100ms during running states
    if (timerState == TIMER_STATE_RUNNING || timerState == TIMER_STATE_BREAK) {
        if (millis() - lastUpdate >= 100) {
            needsRedraw = true;
            lastUpdate = millis();
        }
    }
}

void StudyManager::handleSetup(InputManager& input) {
    // Setup menu items depend on timer mode
    if (timerMode == TIMER_MODE_BASIC) {
        setupMenuMax = 4; // Mode, Duration, Count Direction, Start
    } else {
        setupMenuMax = 6; // Mode, Work, Short Break, Long Break, Sessions, Start
    }
    
    // Map pot to menu selection
    setupMenuIndex = input.getScrollIndex(setupMenuMax);
    
    // Redraw if selection changed
    if (setupMenuIndex != lastMenuIndex || needsRedraw) {
        uiMgr.showTimerSetup(timerMode, setupMenuIndex,
                             basicDurationMins, countUp,
                             pomodoroWorkMins, pomodoroShortBreakMins,
                             pomodoroLongBreakMins, pomodoroTotalSessions,
                             editingValue, editValue);
        lastMenuIndex = setupMenuIndex;
        needsRedraw = false;
    }
    
    // Handle input
    if (editingValue) {
        // Adjust value with C/D buttons
        if (input.isBtnCPressed()) {
            editValue--;
            if (editValue < 1) editValue = 1;
            needsRedraw = true;
            delay(150);
        }
        if (input.isBtnDPressed()) {
            editValue++;
            if (editValue > 120) editValue = 120; // Max 2 hours
            needsRedraw = true;
            delay(150);
        }
        
        // Confirm with A
        if (input.isBtnAPressed()) {
            // Save the edited value
            if (timerMode == TIMER_MODE_BASIC) {
                if (setupMenuIndex == 1) basicDurationMins = editValue;
            } else {
                switch (setupMenuIndex) {
                    case 1: pomodoroWorkMins = editValue; break;
                    case 2: pomodoroShortBreakMins = editValue; break;
                    case 3: pomodoroLongBreakMins = editValue; break;
                    case 4: pomodoroTotalSessions = editValue; break;
                }
            }
            editingValue = false;
            needsRedraw = true;
            delay(200);
        }
        
        // Cancel with B
        if (input.isBtnBPressed()) {
            editingValue = false;
            needsRedraw = true;
            delay(200);
        }
    } else {
        // Normal menu navigation
        if (input.isBtnAPressed()) {
            if (setupMenuIndex == 0) {
                // Toggle mode
                timerMode = (timerMode == TIMER_MODE_BASIC) ? TIMER_MODE_POMODORO : TIMER_MODE_BASIC;
                needsRedraw = true;
            } else if ((timerMode == TIMER_MODE_BASIC && setupMenuIndex == 2)) {
                // Toggle count direction
                countUp = !countUp;
                needsRedraw = true;
            } else if ((timerMode == TIMER_MODE_BASIC && setupMenuIndex == 3) ||
                       (timerMode == TIMER_MODE_POMODORO && setupMenuIndex == 5)) {
                // Start timer
                startTimer();
            } else {
                // Enter edit mode for numeric values
                editingValue = true;
                if (timerMode == TIMER_MODE_BASIC) {
                    editValue = basicDurationMins;
                } else {
                    switch (setupMenuIndex) {
                        case 1: editValue = pomodoroWorkMins; break;
                        case 2: editValue = pomodoroShortBreakMins; break;
                        case 3: editValue = pomodoroLongBreakMins; break;
                        case 4: editValue = pomodoroTotalSessions; break;
                    }
                }
                needsRedraw = true;
            }
            delay(200);
        }
        
        // Exit with B
        if (input.isBtnBPressed()) {
            isActive = false;
            delay(200);
        }
    }
}

void StudyManager::handleRunning(InputManager& input) {
    // Check if timer finished
    if (!countUp && timerMode == TIMER_MODE_BASIC) {
        if (getElapsedTime() >= targetDuration) {
            timerState = TIMER_STATE_FINISHED;
            playFinishSound();
            needsRedraw = true;
            return;
        }
    } else if (timerMode == TIMER_MODE_POMODORO) {
        if (getElapsedTime() >= targetDuration) {
            // Phase complete
            if (pomodoroPhase == POMO_WORK) {
                // Work done, start break
                if (pomodoroCurrentSession >= pomodoroTotalSessions) {
                    // Long break after all sessions
                    pomodoroPhase = POMO_LONG_BREAK;
                    targetDuration = pomodoroLongBreakMins * 60000UL;
                } else {
                    // Short break
                    pomodoroPhase = POMO_SHORT_BREAK;
                    targetDuration = pomodoroShortBreakMins * 60000UL;
                }
                timerState = TIMER_STATE_BREAK;
                startTime = millis();
                totalPausedTime = 0;
                playBreakStartSound();
            }
            needsRedraw = true;
            return;
        }
    }
    
    // Update display
    if (needsRedraw) {
        unsigned long elapsed = getElapsedTime();
        unsigned long remaining = (targetDuration > elapsed) ? (targetDuration - elapsed) : 0;
        
        if (timerMode == TIMER_MODE_BASIC) {
            if (countUp) {
                uiMgr.showBasicTimer(elapsed / 1000, 0, false, false);
            } else {
                uiMgr.showBasicTimer(elapsed / 1000, remaining / 1000, false, false);
            }
        } else {
            uiMgr.showPomodoroTimer(remaining / 1000, pomodoroPhase, 
                                    pomodoroCurrentSession, pomodoroTotalSessions,
                                    false, false);
        }
        needsRedraw = false;
    }
    
    // Handle input using cached button states with debounce
    static unsigned long lastBtnTime = 0;
    const unsigned long DEBOUNCE = 250;
    
    if (millis() - lastBtnTime >= DEBOUNCE) {
        if (cachedBtnA) {
            pauseTimer();
            lastBtnTime = millis();
        }
        
        if (cachedBtnB) {
            stopTimer();
            lastBtnTime = millis();
        }
    }
}

void StudyManager::handlePaused(InputManager& input) {
    if (needsRedraw) {
        unsigned long elapsed = getElapsedTime();
        unsigned long remaining = (targetDuration > elapsed) ? (targetDuration - elapsed) : 0;
        
        if (timerMode == TIMER_MODE_BASIC) {
            uiMgr.showBasicTimer(elapsed / 1000, remaining / 1000, true, false);
        } else {
            uiMgr.showPomodoroTimer(remaining / 1000, pomodoroPhase,
                                    pomodoroCurrentSession, pomodoroTotalSessions,
                                    true, false);
        }
        needsRedraw = false;
    }
    
    // Handle input using cached button states with debounce
    static unsigned long lastBtnTime = 0;
    const unsigned long DEBOUNCE = 250;
    
    if (millis() - lastBtnTime >= DEBOUNCE) {
        // Resume with A
        if (cachedBtnA) {
            resumeTimer();
            lastBtnTime = millis();
        }
        
        // Stop with B
        if (cachedBtnB) {
            stopTimer();
            lastBtnTime = millis();
        }
    }
}

void StudyManager::handleBreak(InputManager& input) {
    // Check if break finished
    if (getElapsedTime() >= targetDuration) {
        if (pomodoroPhase == POMO_LONG_BREAK) {
            // All done!
            timerState = TIMER_STATE_FINISHED;
            playFinishSound();
        } else {
            // Back to work
            pomodoroCurrentSession++;
            pomodoroPhase = POMO_WORK;
            targetDuration = pomodoroWorkMins * 60000UL;
            timerState = TIMER_STATE_RUNNING;
            startTime = millis();
            totalPausedTime = 0;
            playBreakEndSound();
        }
        needsRedraw = true;
        return;
    }
    
    // Update display
    if (needsRedraw) {
        unsigned long elapsed = getElapsedTime();
        unsigned long remaining = (targetDuration > elapsed) ? (targetDuration - elapsed) : 0;
        
        uiMgr.showPomodoroTimer(remaining / 1000, pomodoroPhase,
                                pomodoroCurrentSession, pomodoroTotalSessions,
                                false, true);
        needsRedraw = false;
    }
    
    // Handle input using cached button states with debounce
    static unsigned long lastBtnTime = 0;
    const unsigned long DEBOUNCE = 250;
    
    if (millis() - lastBtnTime >= DEBOUNCE) {
        // Skip break with A
        if (cachedBtnA) {
            skipPhase();
            lastBtnTime = millis();
        }
        
        // Stop with B
        if (cachedBtnB) {
            stopTimer();
            lastBtnTime = millis();
        }
    }
}

void StudyManager::handleFinished(InputManager& input) {
    if (needsRedraw) {
        unsigned long total = getElapsedTime();
        if (timerMode == TIMER_MODE_POMODORO) {
            uiMgr.showTimerComplete(pomodoroTotalSessions, total / 1000);
        } else {
            uiMgr.showTimerComplete(1, total / 1000);
        }
        needsRedraw = false;
    }
    
    // Handle input using cached button states with debounce
    static unsigned long lastBtnTime = 0;
    const unsigned long DEBOUNCE = 250;
    
    if (millis() - lastBtnTime >= DEBOUNCE) {
        // Any button returns to setup
        if (cachedBtnA || cachedBtnB) {
            timerState = TIMER_STATE_SETUP;
            setupMenuIndex = 0;
            lastMenuIndex = -1;
            needsRedraw = true;
            lastBtnTime = millis();
        }
    }
}

void StudyManager::startTimer() {
    timerState = TIMER_STATE_RUNNING;
    startTime = millis();
    pausedTime = 0;
    totalPausedTime = 0;
    
    if (timerMode == TIMER_MODE_BASIC) {
        targetDuration = basicDurationMins * 60000UL;
    } else {
        pomodoroPhase = POMO_WORK;
        pomodoroCurrentSession = 1;
        targetDuration = pomodoroWorkMins * 60000UL;
    }
    
    playStartSound();
    needsRedraw = true;
    Serial.println("[STUDY] Timer started");
}

void StudyManager::pauseTimer() {
    pausedTime = millis();
    timerState = TIMER_STATE_PAUSED;
    playPauseSound();
    needsRedraw = true;
    Serial.println("[STUDY] Timer paused");
}

void StudyManager::resumeTimer() {
    totalPausedTime += (millis() - pausedTime);
    timerState = (pomodoroPhase == POMO_WORK || timerMode == TIMER_MODE_BASIC) 
                  ? TIMER_STATE_RUNNING : TIMER_STATE_BREAK;
    playResumeSound();
    needsRedraw = true;
    Serial.println("[STUDY] Timer resumed");
}

void StudyManager::stopTimer() {
    timerState = TIMER_STATE_SETUP;
    setupMenuIndex = 0;
    lastMenuIndex = -1;
    needsRedraw = true;
    Serial.println("[STUDY] Timer stopped");
}

void StudyManager::skipPhase() {
    if (pomodoroPhase == POMO_LONG_BREAK) {
        timerState = TIMER_STATE_FINISHED;
        playFinishSound();
    } else {
        pomodoroCurrentSession++;
        pomodoroPhase = POMO_WORK;
        targetDuration = pomodoroWorkMins * 60000UL;
        timerState = TIMER_STATE_RUNNING;
        startTime = millis();
        totalPausedTime = 0;
        playBreakEndSound();
    }
    needsRedraw = true;
}

unsigned long StudyManager::getElapsedTime() {
    if (timerState == TIMER_STATE_PAUSED) {
        return pausedTime - startTime - totalPausedTime;
    }
    return millis() - startTime - totalPausedTime;
}

unsigned long StudyManager::getRemainingTime() {
    unsigned long elapsed = getElapsedTime();
    return (targetDuration > elapsed) ? (targetDuration - elapsed) : 0;
}

// Sound functions
void StudyManager::playMelody(const int* notes, const int* durations, int count) {
    if (settingsMgr.getSpeakerMuted()) return;  // Respect mute setting
    ledcAttach(PIN_SPKR, 1000, 8);
    for (int i = 0; i < count; i++) {
        if (notes[i] == NOTE_REST) {
            ledcWriteTone(PIN_SPKR, 0);
        } else {
            ledcWriteTone(PIN_SPKR, notes[i]);
        }
        delay(durations[i]);
    }
    ledcWriteTone(PIN_SPKR, 0);
    ledcDetach(PIN_SPKR);      // Detach PWM from pin
    pinMode(PIN_SPKR, OUTPUT); // Set as regular output
    digitalWrite(PIN_SPKR, LOW); // Drive low to prevent noise
}

void StudyManager::playStartSound() {
    // Green flash + ascending chime
    setLed(false, true); // Green on
    const int notes[] = {NOTE_C5, NOTE_E5, NOTE_G5};
    const int durations[] = {100, 100, 200};
    playMelody(notes, durations, 3);
    ledOff();
}

void StudyManager::playPauseSound() {
    // Two low tones (no LED - it's just a pause)
    const int notes[] = {NOTE_G4, NOTE_REST, NOTE_G4};
    const int durations[] = {100, 50, 100};
    playMelody(notes, durations, 3);
}

void StudyManager::playResumeSound() {
    // Quick ascending
    const int notes[] = {NOTE_E5, NOTE_G5};
    const int durations[] = {100, 150};
    playMelody(notes, durations, 2);
}

void StudyManager::playFinishSound() {
    // Victory fanfare with green flash
    flashLed(false, true, 3, 100, 80); // Green flash
    const int notes[] = {NOTE_C5, NOTE_E5, NOTE_G5, NOTE_REST, NOTE_G5, NOTE_C5};
    const int durations[] = {150, 150, 150, 100, 100, 300};
    playMelody(notes, durations, 6);
}

void StudyManager::playBreakStartSound() {
    // Relaxing descending with green LED (break is good!)
    setLed(false, true);
    const int notes[] = {NOTE_G5, NOTE_E5, NOTE_C5};
    const int durations[] = {150, 150, 250};
    playMelody(notes, durations, 3);
    ledOff();
}

void StudyManager::playBreakEndSound() {
    // Alert ascending with red flash (back to work!)
    flashLed(true, false, 2, 100, 80);
    const int notes[] = {NOTE_C5, NOTE_D5, NOTE_E5, NOTE_G5};
    const int durations[] = {100, 100, 100, 200};
    playMelody(notes, durations, 4);
}
