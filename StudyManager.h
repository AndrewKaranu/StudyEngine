/**
 * Study Manager - Enhanced Study Timer with Pomodoro Support
 * Features: Basic Timer, Pomodoro Timer, Sound Notifications
 */

#ifndef STUDY_MANAGER_H
#define STUDY_MANAGER_H

#include "config.h"
#include "DisplayManager.h"
#include "InputManager.h"
#include "UIManager.h"

// Timer modes
enum TimerMode {
    TIMER_MODE_BASIC,      // Simple count-up/count-down timer
    TIMER_MODE_POMODORO    // Pomodoro technique
};

// Timer states
enum TimerState {
    TIMER_STATE_SETUP,     // Configuring timer
    TIMER_STATE_RUNNING,   // Timer active
    TIMER_STATE_PAUSED,    // Timer paused
    TIMER_STATE_BREAK,     // Pomodoro break
    TIMER_STATE_FINISHED   // Timer completed
};

// Pomodoro phases
enum PomodoroPhase {
    POMO_WORK,
    POMO_SHORT_BREAK,
    POMO_LONG_BREAK
};

class StudyManager {
public:
    void reset();
    void update(DisplayManager& display, InputManager& input);
    bool getIsRunning() { return isActive; }
    
    // Settings
    void setTimerMode(TimerMode mode) { timerMode = mode; }
    TimerMode getTimerMode() { return timerMode; }
    
    // Basic timer settings (in minutes)
    void setBasicDuration(int minutes) { basicDurationMins = minutes; }
    int getBasicDuration() { return basicDurationMins; }
    void setCountUp(bool up) { countUp = up; }
    bool isCountUp() { return countUp; }
    
    // Pomodoro settings (in minutes)
    void setPomodoroWork(int mins) { pomodoroWorkMins = mins; }
    void setPomodoroShortBreak(int mins) { pomodoroShortBreakMins = mins; }
    void setPomodoroLongBreak(int mins) { pomodoroLongBreakMins = mins; }
    void setPomodoroSessions(int count) { pomodoroTotalSessions = count; }
    
    int getPomodoroWork() { return pomodoroWorkMins; }
    int getPomodoroShortBreak() { return pomodoroShortBreakMins; }
    int getPomodoroLongBreak() { return pomodoroLongBreakMins; }
    int getPomodoroSessions() { return pomodoroTotalSessions; }

private:
    // Timer state
    bool isActive = false;
    TimerMode timerMode = TIMER_MODE_BASIC;
    TimerState timerState = TIMER_STATE_SETUP;
    bool needsRedraw = true;
    
    // Basic timer
    int basicDurationMins = 25;  // Default 25 min
    bool countUp = false;        // false = countdown, true = count up
    
    // Pomodoro settings
    int pomodoroWorkMins = 25;
    int pomodoroShortBreakMins = 5;
    int pomodoroLongBreakMins = 15;
    int pomodoroTotalSessions = 4;
    
    // Pomodoro state
    PomodoroPhase pomodoroPhase = POMO_WORK;
    int pomodoroCurrentSession = 1;
    
    // Timing
    unsigned long startTime = 0;
    unsigned long pausedTime = 0;
    unsigned long totalPausedTime = 0;
    unsigned long targetDuration = 0;  // in milliseconds
    
    // Setup menu navigation
    int setupMenuIndex = 0;
    int setupMenuMax = 0;
    int lastMenuIndex = -1;
    
    // Editing state for settings
    bool editingValue = false;
    int editValue = 0;
    
    // Internal methods
    void handleSetup(InputManager& input);
    void handleRunning(InputManager& input);
    void handlePaused(InputManager& input);
    void handleBreak(InputManager& input);
    void handleFinished(InputManager& input);
    
    void startTimer();
    void pauseTimer();
    void resumeTimer();
    void stopTimer();
    void skipPhase();
    
    unsigned long getElapsedTime();
    unsigned long getRemainingTime();
    
    void playStartSound();
    void playPauseSound();
    void playResumeSound();
    void playFinishSound();
    void playBreakStartSound();
    void playBreakEndSound();
    void playMelody(const int* notes, const int* durations, int count);
};

#endif
