#ifndef STUDY_MANAGER_H
#define STUDY_MANAGER_H

#include "config.h"
#include "DisplayManager.h"
#include "InputManager.h"
#include "UIManager.h"

class StudyManager {
private:
    unsigned long startTime = 0;
    unsigned long pausedTime = 0;
    unsigned long totalPausedTime = 0;
    bool isPaused = false;
    bool isRunning = false;
    bool needsRedraw = true;
    
    unsigned long lastMotionTime = 0;
    const unsigned long PRESENCE_TIMEOUT = 60000; // 1 min no motion = away
    
    const int IR_THRESHOLD = 2000; // Adjust based on sensor

public:
    void reset();
    void startSession();
    void stopSession();
    void update(DisplayManager& display, InputManager& input);
    bool getIsRunning() { return isRunning; }
};

#endif
