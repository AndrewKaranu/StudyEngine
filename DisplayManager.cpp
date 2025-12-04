#include "DisplayManager.h"

void DisplayManager::begin() {
    // 1. PERFORM THE HARDWARE/SOFTWARE RESET VIA GPIO 16 (CRITICAL FOR V1.0)
    pinMode(OLED_RESET, OUTPUT);
    digitalWrite(OLED_RESET, LOW); // Set the reset pin low
    delay(50); // Wait a moment
    digitalWrite(OLED_RESET, HIGH); // Then bring it high to release reset

    // 2. Initialize Primary I2C for PCF and CardKB (Pins 21, 22)
    Wire.begin(I2C_SDA, I2C_SCL);

    // 3. Initialize Secondary I2C for OLED (Pins 4, 15)
    Wire1.begin(OLED_SDA, OLED_SCL);

    // Init OLED using Wire1
    if(!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("[OLED] Init Failed!");
    } else {
        oled.setRotation(2); // Flip 180 degrees
        oled.clearDisplay();
        oled.setTextColor(WHITE);
        oled.display();
        Serial.println("[OLED] Initialized");
    }
}

void DisplayManager::showStatus(const char* msg) {
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(WHITE);
    oled.setCursor(0, 0);
    oled.println(msg);
    oled.display();
}

void DisplayManager::updateStatusBar(bool wifiConnected, const char* mode) {
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(WHITE);
    
    // Top bar with WiFi status
    oled.setCursor(0, 0);
    oled.print(wifiConnected ? "WiFi: OK" : "WiFi: --");
    
    // Mode indicator on right
    if (strlen(mode) > 0) {
        int modeWidth = strlen(mode) * 6;
        oled.setCursor(128 - modeWidth, 0);
        oled.print(mode);
    }
    
    oled.drawLine(0, 10, 128, 10, WHITE);
    
    // Center area - ready for content
    oled.setCursor(0, 20);
    oled.setTextSize(2);
    oled.print("Ready");
    
    oled.display();
}

void DisplayManager::showExamTimer(unsigned long remainingSeconds, int currentQ, int totalQ) {
    oled.clearDisplay();
    
    // Header
    oled.setTextSize(1);
    oled.setTextColor(WHITE);
    oled.setCursor(0, 0);
    oled.print("EXAM MODE");
    
    // Question progress on right
    char qStr[12];
    sprintf(qStr, "Q%d/%d", currentQ, totalQ);
    int qWidth = strlen(qStr) * 6;
    oled.setCursor(128 - qWidth, 0);
    oled.print(qStr);
    
    oled.drawLine(0, 10, 128, 10, WHITE);
    
    // Big timer in center
    int mins = remainingSeconds / 60;
    int secs = remainingSeconds % 60;
    
    char timeStr[10];
    sprintf(timeStr, "%02d:%02d", mins, secs);
    
    oled.setTextSize(3);
    
    // Color warning (using inverse for low time)
    if (remainingSeconds < 60) {
        // Flash effect for last minute - invert display
        if ((millis() / 500) % 2 == 0) {
            oled.fillRect(0, 15, 128, 35, WHITE);
            oled.setTextColor(BLACK);
        }
    }
    
    // Center the time
    int textWidth = 5 * 18; // 5 chars * 18 pixels per char at size 3
    oled.setCursor((128 - textWidth) / 2, 20);
    oled.print(timeStr);
    
    oled.setTextColor(WHITE);
    
    // Progress bar at bottom
    oled.drawRect(4, 54, 120, 8, WHITE);
    int progressWidth = (currentQ * 116) / totalQ;
    oled.fillRect(6, 56, progressWidth, 4, WHITE);
    
    oled.display();
}

void DisplayManager::showStudyTimer(unsigned long elapsedSeconds, bool isPaused) {
    oled.clearDisplay();
    
    // Header
    oled.setTextSize(1);
    oled.setTextColor(WHITE);
    oled.setCursor(0, 0);
    oled.print("STUDY MODE");
    
    if (isPaused) {
        oled.setCursor(90, 0);
        oled.print("PAUSE");
    }
    
    oled.drawLine(0, 10, 128, 10, WHITE);
    
    // Calculate hours:mins:secs
    int hours = elapsedSeconds / 3600;
    int mins = (elapsedSeconds % 3600) / 60;
    int secs = elapsedSeconds % 60;
    
    char timeStr[12];
    if (hours > 0) {
        sprintf(timeStr, "%d:%02d:%02d", hours, mins, secs);
        oled.setTextSize(2);
    } else {
        sprintf(timeStr, "%02d:%02d", mins, secs);
        oled.setTextSize(3);
    }
    
    // Center the time
    int charWidth = (hours > 0) ? 12 : 18;
    int numChars = strlen(timeStr);
    int textWidth = numChars * charWidth;
    oled.setCursor((128 - textWidth) / 2, 22);
    oled.print(timeStr);
    
    // Status at bottom
    oled.setTextSize(1);
    oled.setCursor(0, 55);
    if (isPaused) {
        oled.print("Press A to resume");
    } else {
        oled.print("Press B to pause");
    }
    
    oled.display();
}

void DisplayManager::clearOled() {
    oled.clearDisplay();
    oled.display();
}
