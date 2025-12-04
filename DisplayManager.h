#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "config.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/**
 * DisplayManager - Handles OLED status display
 * TFT is now managed by UIManager with LVGL
 */
class DisplayManager {
public:
    // Use Wire1 for the OLED on the secondary I2C bus
    Adafruit_SSD1306 oled = Adafruit_SSD1306(128, 64, &Wire1, OLED_RESET);

    void begin();
    void showStatus(const char* msg);
    void updateStatusBar(bool wifiConnected, const char* mode = "");
    
    // Timer display for exam (big timer on OLED)
    void showExamTimer(unsigned long remainingSeconds, int currentQ, int totalQ);
    void showStudyTimer(unsigned long elapsedSeconds, bool isPaused);
    
    // Clear OLED
    void clearOled();
};

#endif
