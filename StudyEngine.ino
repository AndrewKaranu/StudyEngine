#include "config.h"
#include "DisplayManager.h"
#include "InputManager.h"
#include "NetworkManager.h"
#include "ExamEngine.h"
#include "StudyManager.h"
#include "FlashcardEngine.h"
#include "QuizEngine.h"
#include "UIManager.h"
#include "WebManager.h"
#include "FocusManager.h"

// ===================================================================================
// GLOBALS
// ===================================================================================
DisplayManager displayMgr;
InputManager inputMgr;
SENetworkManager networkMgr;
ExamEngine examEngine;
StudyManager studyMgr;
FlashcardEngine flashcardEngine;
QuizEngine quizEngine;
WebManager webMgr(&networkMgr);

enum SystemState {
    STATE_MENU,
    STATE_SETTINGS,
    STATE_SCANATRON_SETUP,
    STATE_SCANATRON_RUN,
    STATE_STUDY_TIMER,
    STATE_FLASHCARDS,
    STATE_QUIZ,
    STATE_RESULTS
};

SystemState currentState = STATE_MENU;
SystemState lastState = STATE_RESULTS; // Force redraw

// Menu
const int MENU_ITEMS = 4;
const char* menuLabels[] = {"Scanatron Mode", "Study Timer", "Flashcards", "Quiz Mode"};
int menuIndex = 0;
int lastMenuIndex = -1;

// Settings menu
int settingsMenuIndex = 0;
int lastSettingsMenuIndex = -1;


void setup() {
    Serial.begin(115200);
    Serial.println("\n--- STUDY ENGINE ---");

    // Disable LoRa (Commented out as pins are repurposed)
    // pinMode(LORA_CS, OUTPUT); digitalWrite(LORA_CS, HIGH);
    // pinMode(LORA_RST, OUTPUT); digitalWrite(LORA_RST, LOW);

    // Init Managers
    inputMgr.begin();
    displayMgr.begin();
    focusMgr.begin();
    
    // Init LVGL UI Manager
    uiMgr.begin();
    
    // Show loading screen
    uiMgr.showLoading("Connecting to WiFi...");
    for (int i = 0; i < 10; i++) {
        uiMgr.update();
        delay(10);
    }
    
    displayMgr.showStatus("Connecting WiFi...");
    
    // Connect WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
        uiMgr.update();
        delay(250);
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[WIFI] Connected");
        Serial.print("[WIFI] IP: ");
        Serial.println(WiFi.localIP());
        displayMgr.showStatus("WiFi Connected");
        
        // Start Web Server
        webMgr.begin();
        Serial.print("[WEB] Admin UI at http://");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("[WIFI] Connection Failed");
        displayMgr.showStatus("WiFi Failed");
    }
    
    // Show main menu
    uiMgr.update();
    uiMgr.showMainMenu(menuIndex, MENU_ITEMS, menuLabels);
    displayMgr.updateStatusBar(networkMgr.isConnected(), "MENU");
    uiMgr.update();
    
    Serial.println("[MAIN] Ready");
    Serial.println("[MAIN] Long-press D in main menu to access Settings");
}

void loop() {
    // Update LVGL
    uiMgr.update();
    
    // Update Web Server
    webMgr.update();
    
    // Update inputs (including long-press detection)
    inputMgr.update();
    
    // Check Focus Mode (except during Scanatron exam)
    if (currentState != STATE_SCANATRON_RUN && currentState != STATE_SCANATRON_SETUP) {
        if (!focusMgr.checkFocus()) {
            // Focus warning is being shown - check for dismiss
            if (inputMgr.isBtnAPressed()) {
                focusMgr.dismissWarning();
                lastMenuIndex = -1; // Force redraw of current screen
                delay(200);
            }
            return; // Skip normal processing while warning shown
        }
    }
    
    // Global Back Button (BtnB) to return to Menu (except during certain states)
    if (inputMgr.isBtnBPressed() && 
        currentState != STATE_SCANATRON_RUN && 
        currentState != STATE_MENU && 
        currentState != STATE_FLASHCARDS && 
        currentState != STATE_QUIZ &&
        currentState != STATE_STUDY_TIMER &&
        currentState != STATE_SETTINGS) {
        currentState = STATE_MENU;
        lastMenuIndex = -1;  // Force redraw
    }

    int stateInt = (int)currentState;
    
    switch (currentState) {
        case STATE_MENU:
            handleMenu();
            break;
        case STATE_SETTINGS:
            handleSettings();
            break;
        case STATE_SCANATRON_SETUP:
            examEngine.handleSetup(displayMgr, inputMgr, networkMgr, stateInt);
            currentState = (SystemState)stateInt;
            break;
        case STATE_SCANATRON_RUN:
            // Pause focus checks during exam
            focusMgr.pauseFocusChecks();
            examEngine.handleRun(displayMgr, inputMgr, networkMgr, stateInt);
            currentState = (SystemState)stateInt;
            if (currentState != STATE_SCANATRON_RUN) {
                focusMgr.resumeFocusChecks();
            }
            break;
        case STATE_STUDY_TIMER:
            handleStudyTimer();
            break;
        case STATE_FLASHCARDS:
            flashcardEngine.handleRun(displayMgr, inputMgr, networkMgr, stateInt);
            currentState = (SystemState)stateInt;
            break;
        case STATE_QUIZ:
            quizEngine.handleRun(displayMgr, inputMgr, networkMgr, stateInt);
            currentState = (SystemState)stateInt;
            break;
        default:
            break;
    }
}

void handleMenu() {
    // Check for long-press D to open settings
    if (inputMgr.isBtnDLongPressed()) {
        currentState = STATE_SETTINGS;
        settingsMenuIndex = 0;
        lastSettingsMenuIndex = -1;
        Serial.println("[MENU] Opening Settings");
        delay(200);
        return;
    }
    
    // Map Pot to Menu Index
    menuIndex = inputMgr.getScrollIndex(MENU_ITEMS);

    if (currentState != lastState || menuIndex != lastMenuIndex) {
        // Use LVGL UI Manager for modern menu
        uiMgr.showMainMenu(menuIndex, MENU_ITEMS, menuLabels);
        displayMgr.updateStatusBar(networkMgr.isConnected(), "MENU");
        
        lastState = currentState;
        lastMenuIndex = menuIndex;
    }

    if (inputMgr.isBtnAPressed()) {
        if (menuIndex == 0) {
            currentState = STATE_SCANATRON_SETUP;
            examEngine.reset();
        } else if (menuIndex == 1) {
            currentState = STATE_STUDY_TIMER;
            studyMgr.reset();
        } else if (menuIndex == 2) {
            currentState = STATE_FLASHCARDS;
            flashcardEngine.reset();
        } else if (menuIndex == 3) {
            currentState = STATE_QUIZ;
            quizEngine.reset();
        }
        lastMenuIndex = -1;  // Force redraw when returning
        delay(200);
    }
}

void handleSettings() {
    // Map pot to settings menu
    settingsMenuIndex = inputMgr.getScrollIndex(2);
    
    // Redraw if changed
    if (settingsMenuIndex != lastSettingsMenuIndex) {
        uiMgr.showSettingsMenu(settingsMenuIndex, focusMgr.isFocusModeEnabled());
        lastSettingsMenuIndex = settingsMenuIndex;
    }
    
    // Handle button presses
    if (inputMgr.isBtnAPressed()) {
        if (settingsMenuIndex == 0) {
            // Toggle Focus Mode
            focusMgr.setFocusMode(!focusMgr.isFocusModeEnabled());
            lastSettingsMenuIndex = -1; // Force redraw
        } else if (settingsMenuIndex == 1) {
            // Back to menu
            currentState = STATE_MENU;
            lastMenuIndex = -1;
        }
        delay(200);
    }
    
    // B button also goes back
    if (inputMgr.isBtnBPressed()) {
        currentState = STATE_MENU;
        lastMenuIndex = -1;
        delay(200);
    }
}

void handleStudyTimer() {
    // Use StudyManager for full functionality
    studyMgr.update(displayMgr, inputMgr);
    
    // Check if exited timer - return to menu
    if (!studyMgr.getIsRunning()) {
        currentState = STATE_MENU;
        lastMenuIndex = -1;
    }
}
