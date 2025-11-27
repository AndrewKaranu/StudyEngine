#include "config.h"
#include "DisplayManager.h"
#include "InputManager.h"
#include "NetworkManager.h"
#include "ExamEngine.h"
#include "StudyManager.h"
#include "FlashcardEngine.h"
#include "UIManager.h"

// ===================================================================================
// GLOBALS
// ===================================================================================
DisplayManager displayMgr;
InputManager inputMgr;
SENetworkManager networkMgr;
ExamEngine examEngine;
StudyManager studyMgr;
FlashcardEngine flashcardEngine;

enum SystemState {
    STATE_MENU,
    STATE_SCANATRON_SETUP,
    STATE_SCANATRON_RUN,
    STATE_STUDY_TIMER,
    STATE_FLASHCARDS,
    STATE_RESULTS
};

SystemState currentState = STATE_MENU;
SystemState lastState = STATE_RESULTS; // Force redraw

// Menu
const int MENU_ITEMS = 3;
const char* menuLabels[] = {"Scanatron Mode", "Study Timer", "Flashcards"};
int menuIndex = 0;
int lastMenuIndex = -1;

void setup() {
    Serial.begin(115200);
    Serial.println("\n--- STUDY ENGINE ---");

    // Disable LoRa
    pinMode(LORA_CS, OUTPUT); digitalWrite(LORA_CS, HIGH);
    pinMode(LORA_RST, OUTPUT); digitalWrite(LORA_RST, LOW);

    // Init Managers
    inputMgr.begin();
    displayMgr.begin();
    
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
}

void loop() {
    // Update LVGL
    uiMgr.update();
    
    // Update inputs
    inputMgr.update();
    
    // Global Back Button (BtnB) to return to Menu (except during exam)
    if (inputMgr.isBtnBPressed() && currentState != STATE_SCANATRON_RUN && currentState != STATE_MENU && currentState != STATE_FLASHCARDS) {
        currentState = STATE_MENU;
        lastMenuIndex = -1;  // Force redraw
    }

    int stateInt = (int)currentState;
    
    switch (currentState) {
        case STATE_MENU:
            handleMenu();
            break;
        case STATE_SCANATRON_SETUP:
            examEngine.handleSetup(displayMgr, inputMgr, networkMgr, stateInt);
            currentState = (SystemState)stateInt;
            break;
        case STATE_SCANATRON_RUN:
            examEngine.handleRun(displayMgr, inputMgr, networkMgr, stateInt);
            currentState = (SystemState)stateInt;
            break;
        case STATE_STUDY_TIMER:
            handleStudyTimer();
            break;
        case STATE_FLASHCARDS:
            flashcardEngine.handleRun(displayMgr, inputMgr, networkMgr, stateInt);
            currentState = (SystemState)stateInt;
            break;
        default:
            break;
    }
}

void handleMenu() {
    // Map Pot to Menu Index
    int potVal = inputMgr.getPotValue();
    menuIndex = map(potVal, 0, 4096, 0, MENU_ITEMS);
    if (menuIndex >= MENU_ITEMS) menuIndex = MENU_ITEMS - 1;

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
        }
        lastMenuIndex = -1;  // Force redraw when returning
        delay(200);
    }
}


void handleStudyTimer() {
    // Use StudyManager for full functionality
    studyMgr.update(displayMgr, inputMgr);
    
    // Check if study session stopped - return to menu
    if (!studyMgr.getIsRunning()) {
        currentState = STATE_MENU;
        lastMenuIndex = -1;
    }
}
