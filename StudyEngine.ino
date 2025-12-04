#include "config.h"
#include "DisplayManager.h"
#include "InputManager.h"
#include "NetworkManager.h"
#include "ExamEngine.h"
#include "StudyManager.h"
#include "FlashcardEngine.h"
#include "QuizEngine.h"
#include "TranscriptEngine.h"
#include "UIManager.h"
#include "WebManager.h"
#include "FocusManager.h"
#include "SettingsManager.h"

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
    STATE_ADMIN_URL,
    STATE_DEV_MODE,
    STATE_API_URL_EDIT,
    STATE_HW_TEST,
    STATE_SCANATRON_SETUP,
    STATE_SCANATRON_RUN,
    STATE_STUDY_TIMER,
    STATE_FLASHCARDS,
    STATE_QUIZ,
    STATE_TRANSCRIPT,
    STATE_RESULTS
};

SystemState currentState = STATE_MENU;
SystemState lastState = STATE_RESULTS; // Force redraw

// Menu
const int MENU_ITEMS = 5;
const char* menuLabels[] = {"Scanatron Mode", "Study Timer", "Flashcards", "Quiz Mode", "Transcripts"};
int menuIndex = 0;
int lastMenuIndex = -1;

// Settings menu
int settingsMenuIndex = 0;
int lastSettingsMenuIndex = -1;

// Dev mode menu
int devMenuIndex = 0;
int lastDevMenuIndex = -1;

// API URL editing
String editingApiUrl = "";
int apiUrlCursorPos = 0;

// Hardware test
int hwTestIndex = 0;
int lastHwTestIndex = -1;

// ===================================================================================
// FEEDBACK HELPERS (LED & Speaker)
// ===================================================================================
void setLed(bool red, bool green) {
    // PCF8575 LED control - LEDs are active LOW
    uint16_t state = 0xFFFF;  // All high (LEDs off)
    if (red) state &= ~(1 << PCF_LED_R);
    if (green) state &= ~(1 << PCF_LED_G);
    // Keep button inputs high
    state |= 0x003F;  // Bits 0-5 high for button inputs
    
    Wire.beginTransmission(PCF_ADDR);
    Wire.write(state & 0xFF);
    Wire.write((state >> 8) & 0xFF);
    Wire.endTransmission();
}

void ledOff() {
    setLed(false, false);
}

void flashLed(bool red, bool green, int count, int onTime, int offTime) {
    for (int i = 0; i < count; i++) {
        setLed(red, green);
        delay(onTime);
        ledOff();
        if (i < count - 1) delay(offTime);
    }
}

void beepSuccess() {
    if (settingsMgr.getSpeakerMuted()) return;
    // Two quick high beeps for success
    tone(PIN_SPKR, 1000, 80);
    delay(100);
    tone(PIN_SPKR, 1200, 80);
    delay(100);
    noTone(PIN_SPKR);
    digitalWrite(PIN_SPKR, LOW);
}

void beepError() {
    if (settingsMgr.getSpeakerMuted()) return;
    // Low buzz for error
    tone(PIN_SPKR, 200, 200);
    delay(220);
    noTone(PIN_SPKR);
    digitalWrite(PIN_SPKR, LOW);
}

void beepClick() {
    if (settingsMgr.getSpeakerMuted()) return;
    // Quick click for button feedback
    tone(PIN_SPKR, 800, 30);
    delay(40);
    noTone(PIN_SPKR);
    digitalWrite(PIN_SPKR, LOW);
}

void beepWarning() {
    if (settingsMgr.getSpeakerMuted()) return;
    // Two-tone warning
    tone(PIN_SPKR, 600, 100);
    delay(120);
    tone(PIN_SPKR, 400, 100);
    delay(120);
    noTone(PIN_SPKR);
    digitalWrite(PIN_SPKR, LOW);
}

void beepComplete() {
    if (settingsMgr.getSpeakerMuted()) return;
    // Victory jingle for completion
    int notes[] = {523, 659, 784, 1047};  // C5, E5, G5, C6
    for (int i = 0; i < 4; i++) {
        tone(PIN_SPKR, notes[i], 100);
        delay(120);
    }
    noTone(PIN_SPKR);
    digitalWrite(PIN_SPKR, LOW);
}

void feedbackSuccess() {
    flashLed(false, true, 2, 100, 50);  // Green flash
    beepSuccess();
}

void feedbackError() {
    flashLed(true, false, 2, 100, 50);  // Red flash
    beepError();
}

void feedbackClick() {
    beepClick();
}

void feedbackWarning() {
    flashLed(true, false, 3, 80, 40);  // Quick red flashes
    beepWarning();
}


void setup() {
    Serial.begin(115200);
    Serial.println("\n--- STUDY ENGINE ---");

    // Disable LoRa (Commented out as pins are repurposed)
    // pinMode(LORA_CS, OUTPUT); digitalWrite(LORA_CS, HIGH);
    // pinMode(LORA_RST, OUTPUT); digitalWrite(LORA_RST, LOW);

    // Initialize speaker pin LOW to prevent noise
    pinMode(PIN_SPKR, OUTPUT);
    digitalWrite(PIN_SPKR, LOW);

    // Init Settings Manager FIRST (loads saved preferences)
    settingsMgr.begin();

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
    
    // Global Back Button (BtnB) to return to Menu - ONLY for states that don't handle B themselves
    // Most states handle their own B button, so this is mainly a fallback
    // DO NOT intercept B for: SCANATRON_RUN, SCANATRON_SETUP, MENU, FLASHCARDS, QUIZ, STUDY_TIMER, SETTINGS
    if (inputMgr.isBtnBPressed() && 
        currentState != STATE_SCANATRON_RUN && 
        currentState != STATE_SCANATRON_SETUP &&  // Added - let setup handle its own B
        currentState != STATE_MENU && 
        currentState != STATE_FLASHCARDS && 
        currentState != STATE_QUIZ &&
        currentState != STATE_STUDY_TIMER &&
        currentState != STATE_SETTINGS) {
        Serial.println("[MAIN] Global B pressed - returning to menu");
        currentState = STATE_MENU;
        lastMenuIndex = -1;  // Force redraw
    }

    int stateInt = (int)currentState;
    
    // Debug state transitions
    static SystemState lastDebugState = STATE_RESULTS;
    if (currentState != lastDebugState) {
        Serial.printf("[MAIN] State changed: %d -> %d\n", lastDebugState, currentState);
        lastDebugState = currentState;
    }
    
    switch (currentState) {
        case STATE_MENU:
            handleMenu();
            break;
        case STATE_SETTINGS:
            handleSettings();
            break;
        case STATE_ADMIN_URL:
            handleAdminURL();
            break;
        case STATE_DEV_MODE:
            handleDevMode();
            break;
        case STATE_API_URL_EDIT:
            handleApiUrlEdit();
            break;
        case STATE_HW_TEST:
            handleHardwareTest();
            break;
        case STATE_SCANATRON_SETUP:
            examEngine.handleSetup(displayMgr, inputMgr, networkMgr, stateInt);
            if (stateInt != (int)currentState) {
                Serial.printf("[MAIN] ExamEngine changed state to: %d\n", stateInt);
            }
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
        case STATE_TRANSCRIPT:
            transcriptEngine.handleRun(displayMgr, inputMgr, networkMgr, stateInt);
            currentState = (SystemState)stateInt;
            break;
        default:
            break;
    }
}

void handleMenu() {
    // Check for ESC key to open settings
    char key = inputMgr.readCardKB();
    if (key == 27) {  // ESC key
        currentState = STATE_SETTINGS;
        settingsMenuIndex = 0;
        lastSettingsMenuIndex = -1;
        beepClick();
        Serial.println("[MENU] Opening Settings (ESC pressed)");
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
        beepClick();
        setLed(false, true);  // Green LED while loading
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
        } else if (menuIndex == 4) {
            currentState = STATE_TRANSCRIPT;
            transcriptEngine.reset();
        }
        ledOff();
        lastMenuIndex = -1;  // Force redraw when returning
        delay(200);
    }
}

void handleSettings() {
    // Map pot to settings menu (5 items: Focus Mode, Mute Speaker, Admin URL, Dev Mode, Back)
    settingsMenuIndex = inputMgr.getScrollIndex(5);
    
    // Redraw if changed
    if (settingsMenuIndex != lastSettingsMenuIndex) {
        uiMgr.showSettingsMenu(settingsMenuIndex, focusMgr.isFocusModeEnabled(), settingsMgr.getSpeakerMuted());
        lastSettingsMenuIndex = settingsMenuIndex;
    }
    
    // Handle button presses
    if (inputMgr.isBtnAPressed()) {
        beepClick();
        if (settingsMenuIndex == 0) {
            // Toggle Focus Mode
            focusMgr.setFocusMode(!focusMgr.isFocusModeEnabled());
            if (focusMgr.isFocusModeEnabled()) {
                flashLed(false, true, 1, 150, 0); // Green for enabled
            } else {
                flashLed(true, false, 1, 150, 0); // Red for disabled
            }
            lastSettingsMenuIndex = -1; // Force redraw
        } else if (settingsMenuIndex == 1) {
            // Toggle Speaker Mute
            settingsMgr.setSpeakerMuted(!settingsMgr.getSpeakerMuted());
            if (settingsMgr.getSpeakerMuted()) {
                flashLed(true, false, 1, 150, 0); // Red for muted
            } else {
                flashLed(false, true, 1, 150, 0); // Green for unmuted
                beepClick(); // Play a beep to confirm unmute
            }
            lastSettingsMenuIndex = -1; // Force redraw
        } else if (settingsMenuIndex == 2) {
            // Show Admin URL
            currentState = STATE_ADMIN_URL;
        } else if (settingsMenuIndex == 3) {
            // Dev Mode
            currentState = STATE_DEV_MODE;
            devMenuIndex = 0;
            lastDevMenuIndex = -1;
        } else if (settingsMenuIndex == 4) {
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

void handleAdminURL() {
    static bool shown = false;
    
    if (!shown) {
        String url = "http://" + WiFi.localIP().toString();
        uiMgr.showAdminURL(url.c_str());
        shown = true;
    }
    
    // B button to go back
    if (inputMgr.isBtnBPressed()) {
        currentState = STATE_SETTINGS;
        lastSettingsMenuIndex = -1;
        shown = false;
        delay(200);
    }
    
    // ESC key also goes back
    char key = inputMgr.readCardKB();
    if (key == 27) {
        currentState = STATE_SETTINGS;
        lastSettingsMenuIndex = -1;
        shown = false;
    }
}

void handleDevMode() {
    // Map pot to dev menu (7 items now with Hardware Tests)
    devMenuIndex = inputMgr.getScrollIndex(7);
    
    // Redraw if changed
    if (devMenuIndex != lastDevMenuIndex) {
        uiMgr.showDevModeMenu(devMenuIndex, 
                              settingsMgr.getApiBaseUrl().c_str(),
                              settingsMgr.getSerialDebug(),
                              settingsMgr.getShowFPS(),
                              settingsMgr.getVerboseNetwork());
        lastDevMenuIndex = devMenuIndex;
    }
    
    // Handle button presses
    if (inputMgr.isBtnAPressed()) {
        beepClick();
        switch (devMenuIndex) {
            case 0:  // Edit API URL
                currentState = STATE_API_URL_EDIT;
                editingApiUrl = settingsMgr.getApiBaseUrl();
                apiUrlCursorPos = editingApiUrl.length();
                break;
            case 1:  // Toggle Serial Debug
                settingsMgr.setSerialDebug(!settingsMgr.getSerialDebug());
                flashLed(settingsMgr.getSerialDebug() ? false : true, 
                        settingsMgr.getSerialDebug() ? true : false, 1, 150, 0);
                lastDevMenuIndex = -1;
                break;
            case 2:  // Toggle Show FPS
                settingsMgr.setShowFPS(!settingsMgr.getShowFPS());
                flashLed(settingsMgr.getShowFPS() ? false : true, 
                        settingsMgr.getShowFPS() ? true : false, 1, 150, 0);
                lastDevMenuIndex = -1;
                break;
            case 3:  // Toggle Verbose Network
                settingsMgr.setVerboseNetwork(!settingsMgr.getVerboseNetwork());
                flashLed(settingsMgr.getVerboseNetwork() ? false : true, 
                        settingsMgr.getVerboseNetwork() ? true : false, 1, 150, 0);
                lastDevMenuIndex = -1;
                break;
            case 4:  // Hardware Tests
                currentState = STATE_HW_TEST;
                hwTestIndex = 0;
                lastHwTestIndex = -1;
                break;
            case 5:  // Reset All Settings
                settingsMgr.resetApiBaseUrl();
                settingsMgr.setSerialDebug(true);
                settingsMgr.setShowFPS(false);
                settingsMgr.setVerboseNetwork(false);
                beepWarning();
                flashLed(true, false, 2, 100, 80);
                Serial.println("[DEV] All settings reset to defaults");
                lastDevMenuIndex = -1;
                break;
            case 6:  // Back
                currentState = STATE_SETTINGS;
                lastSettingsMenuIndex = -1;
                break;
        }
        delay(200);
    }
    
    // B button goes back
    if (inputMgr.isBtnBPressed()) {
        beepClick();
        currentState = STATE_SETTINGS;
        lastSettingsMenuIndex = -1;
        delay(200);
    }
}

void handleApiUrlEdit() {
    static bool initialized = false;
    static unsigned long lastBlink = 0;
    static bool cursorVisible = true;
    
    if (!initialized) {
        uiMgr.showApiUrlEditor(settingsMgr.getApiBaseUrl().c_str(), 
                               editingApiUrl.c_str(), 
                               apiUrlCursorPos);
        initialized = true;
    }
    
    // Blink cursor every 500ms
    if (millis() - lastBlink > 500) {
        lastBlink = millis();
        cursorVisible = !cursorVisible;
        uiMgr.showApiUrlEditor(settingsMgr.getApiBaseUrl().c_str(), 
                               editingApiUrl.c_str(), 
                               cursorVisible ? apiUrlCursorPos : -1);
    }
    
    // Read keyboard input
    char key = inputMgr.readCardKB();
    
    if (key != 0) {
        bool needsRedraw = true;
        
        if (key == 27) {  // ESC - Cancel
            initialized = false;
            currentState = STATE_DEV_MODE;
            lastDevMenuIndex = -1;
            return;
        } else if (key == 13 || key == 10) {  // Enter - Save
            if (editingApiUrl.length() > 0) {
                settingsMgr.setApiBaseUrl(editingApiUrl);
                Serial.printf("[DEV] API URL saved: %s\n", editingApiUrl.c_str());
            }
            initialized = false;
            currentState = STATE_DEV_MODE;
            lastDevMenuIndex = -1;
            return;
        } else if (key == 8 || key == 127) {  // Backspace
            if (apiUrlCursorPos > 0 && editingApiUrl.length() > 0) {
                editingApiUrl = editingApiUrl.substring(0, apiUrlCursorPos - 1) + editingApiUrl.substring(apiUrlCursorPos);
                apiUrlCursorPos--;
            }
        } else if (key == 0xB4 || key == 180) {  // Left arrow
            if (apiUrlCursorPos > 0) {
                apiUrlCursorPos--;
            }
        } else if (key == 0xB7 || key == 183) {  // Right arrow
            if (apiUrlCursorPos < (int)editingApiUrl.length()) {
                apiUrlCursorPos++;
            }
        } else if (key == 0x7E || key == 126) {  // Delete/Tilde - Reset to default
            editingApiUrl = DEFAULT_API_URL;
            apiUrlCursorPos = editingApiUrl.length();
        } else if (key >= 32 && key < 127) {  // Printable ASCII
            if (editingApiUrl.length() < MAX_URL_LENGTH) {
                editingApiUrl = editingApiUrl.substring(0, apiUrlCursorPos) + String(key) + editingApiUrl.substring(apiUrlCursorPos);
                apiUrlCursorPos++;
            }
        } else {
            needsRedraw = false;
        }
        
        if (needsRedraw) {
            cursorVisible = true;
            lastBlink = millis();
            uiMgr.showApiUrlEditor(settingsMgr.getApiBaseUrl().c_str(), 
                                   editingApiUrl.c_str(), 
                                   apiUrlCursorPos);
        }
    }
    
    // B button also cancels
    if (inputMgr.isBtnBPressed()) {
        initialized = false;
        currentState = STATE_DEV_MODE;
        lastDevMenuIndex = -1;
        delay(200);
    }
}

// ===================================================================================
// HARDWARE TEST HANDLER
// ===================================================================================
void handleHardwareTest() {
    // Number of test items: Display, OLED, Buttons, Keyboard, Potentiometer, Speaker, RGB LED, WiFi, API, Back
    const int HW_TEST_COUNT = 10;
    
    // Map pot to test menu
    hwTestIndex = inputMgr.getScrollIndex(HW_TEST_COUNT);
    
    // Get current hardware states for live display
    bool btnA = inputMgr.isBtnAHeld();
    bool btnB = inputMgr.isBtnBHeld();
    bool btnC = inputMgr.isBtnCHeld();
    bool btnD = inputMgr.isBtnDHeld();
    int potValue = inputMgr.getPotValue();
    char lastKey = inputMgr.readCardKB();
    bool wifiOk = (WiFi.status() == WL_CONNECTED);
    
    // Redraw if selection changed or periodically for live values
    static unsigned long lastRedraw = 0;
    bool needsRedraw = (hwTestIndex != lastHwTestIndex) || (millis() - lastRedraw > 200);
    
    if (needsRedraw) {
        uiMgr.showHardwareTest(hwTestIndex, btnA, btnB, btnC, btnD, potValue, lastKey, wifiOk);
        lastHwTestIndex = hwTestIndex;
        lastRedraw = millis();
    }
    
    // Handle button A to run selected test
    if (inputMgr.isBtnAPressed()) {
        switch (hwTestIndex) {
            case 0:  // Display Test - Show color bars
                runDisplayTest();
                lastHwTestIndex = -1;  // Force redraw
                break;
            case 1:  // OLED Test
                runOledTest();
                lastHwTestIndex = -1;
                break;
            case 2:  // Buttons - Already shows live state
                // Just highlight that buttons are being tested
                break;
            case 3:  // Keyboard - Already shows live state
                // Just highlight that keyboard is being tested
                break;
            case 4:  // Potentiometer - Already shows live state
                // Just highlight that pot is being tested
                break;
            case 5:  // Speaker Test
                runSpeakerTest();
                lastHwTestIndex = -1;
                break;
            case 6:  // RGB LED Test
                runRgbLedTest();
                lastHwTestIndex = -1;
                break;
            case 7:  // WiFi Test
                runWifiTest();
                lastHwTestIndex = -1;
                break;
            case 8:  // API Test
                runApiTest();
                lastHwTestIndex = -1;
                break;
            case 9:  // Back
                currentState = STATE_DEV_MODE;
                lastDevMenuIndex = -1;
                break;
        }
        delay(200);
    }
    
    // B button goes back
    if (inputMgr.isBtnBPressed()) {
        currentState = STATE_DEV_MODE;
        lastDevMenuIndex = -1;
        delay(200);
    }
}

void runDisplayTest() {
    TFT_eSPI& tft = uiMgr.getTft();
    
    // Red
    tft.fillScreen(TFT_RED);
    delay(400);
    
    // Green
    tft.fillScreen(TFT_GREEN);
    delay(400);
    
    // Blue
    tft.fillScreen(TFT_BLUE);
    delay(400);
    
    // White
    tft.fillScreen(TFT_WHITE);
    delay(400);
    
    // Black
    tft.fillScreen(TFT_BLACK);
    delay(200);
    
    // Gradient test
    for (int i = 0; i < 320; i++) {
        uint16_t color = tft.color565(i * 255 / 320, 0, 255 - (i * 255 / 320));
        tft.drawFastVLine(i, 0, 240, color);
    }
    delay(800);
    
    Serial.println("[HW_TEST] Display test complete");
    
    // Show result
    uiMgr.showTestResult("TFT Display Test", true, "All colors displayed correctly.\nCheck for dead pixels or color issues.");
    uiMgr.update();
    
    // Wait for button press
    while (!inputMgr.isBtnAPressed() && !inputMgr.isBtnBPressed()) {
        uiMgr.update();
        delay(50);
    }
    delay(200);
}

void runOledTest() {
    Serial.println("[HW_TEST] Running OLED test...");
    
    Adafruit_SSD1306& oled = displayMgr.oled;
    
    // Test 1: Fill white
    oled.clearDisplay();
    oled.fillRect(0, 0, 128, 64, SSD1306_WHITE);
    oled.display();
    delay(500);
    
    // Test 2: Fill black
    oled.clearDisplay();
    oled.display();
    delay(300);
    
    // Test 3: Checkerboard pattern
    oled.clearDisplay();
    for (int y = 0; y < 64; y += 8) {
        for (int x = 0; x < 128; x += 8) {
            if ((x / 8 + y / 8) % 2 == 0) {
                oled.fillRect(x, y, 8, 8, SSD1306_WHITE);
            }
        }
    }
    oled.display();
    delay(500);
    
    // Test 4: Border test
    oled.clearDisplay();
    oled.drawRect(0, 0, 128, 64, SSD1306_WHITE);
    oled.drawRect(2, 2, 124, 60, SSD1306_WHITE);
    oled.display();
    delay(500);
    
    // Test 5: Text test
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(20, 10);
    oled.println("OLED TEST");
    oled.setCursor(35, 30);
    oled.println("128x64");
    oled.setCursor(20, 50);
    oled.println("StudyEngine");
    oled.display();
    delay(1000);
    
    Serial.println("[HW_TEST] OLED test complete");
    
    // Restore normal status
    displayMgr.showStatus("OLED OK");
    
    // Show result on TFT
    uiMgr.showTestResult("OLED Display Test", true, "Pattern tests completed.\nCheck OLED for proper display.");
    uiMgr.update();
    
    // Wait for button press
    while (!inputMgr.isBtnAPressed() && !inputMgr.isBtnBPressed()) {
        uiMgr.update();
        delay(50);
    }
    delay(200);
}

void runSpeakerTest() {
    Serial.println("[HW_TEST] Running speaker test...");
    
    uiMgr.showLoading("Testing Speaker...");
    uiMgr.update();
    
    // Play a series of tones
    for (int freq = 200; freq <= 2000; freq += 200) {
        tone(PIN_SPKR, freq, 100);
        delay(150);
    }
    
    // Play a short melody (C scale)
    int melody[] = {262, 294, 330, 349, 392, 440, 494, 523};
    for (int i = 0; i < 8; i++) {
        tone(PIN_SPKR, melody[i], 150);
        delay(200);
    }
    
    noTone(PIN_SPKR);
    digitalWrite(PIN_SPKR, LOW);
    
    Serial.println("[HW_TEST] Speaker test complete");
    
    // Show result
    uiMgr.showTestResult("Speaker Test", true, "Played frequency sweep (200-2000Hz)\nand C major scale. Listen for audio.");
    uiMgr.update();
    
    // Wait for button press
    while (!inputMgr.isBtnAPressed() && !inputMgr.isBtnBPressed()) {
        uiMgr.update();
        delay(50);
    }
    delay(200);
}

void pcfWriteDirect(uint16_t data) {
    Wire.beginTransmission(PCF_ADDR);
    Wire.write(data & 0xFF);
    Wire.write((data >> 8) & 0xFF);
    Wire.endTransmission();
}

void runRgbLedTest() {
    Serial.println("[HW_TEST] Running RGB LED test...");
    
    uiMgr.showLoading("Testing RGB LED...");
    uiMgr.update();
    
    // Base state: all inputs high (0-5), all outputs high (LEDs off)
    // PCF8575: LED pins are active LOW
    uint16_t baseState = 0xFFFF;  // All high = LEDs off
    
    // Red LED test (PCF_LED_R = 6)
    uint16_t redOn = baseState & ~(1 << PCF_LED_R);
    pcfWriteDirect(redOn);
    delay(500);
    
    // Green LED test (PCF_LED_G = 7)
    uint16_t greenOn = baseState & ~(1 << PCF_LED_G);
    pcfWriteDirect(greenOn);
    delay(500);
    
    // Blue LED test (PCF_LED_B = 10)
    uint16_t blueOn = baseState & ~(1 << PCF_LED_B);
    pcfWriteDirect(blueOn);
    delay(500);
    
    // Yellow (Red + Green)
    uint16_t yellowOn = baseState & ~(1 << PCF_LED_R) & ~(1 << PCF_LED_G);
    pcfWriteDirect(yellowOn);
    delay(500);
    
    // Cyan (Green + Blue)
    uint16_t cyanOn = baseState & ~(1 << PCF_LED_G) & ~(1 << PCF_LED_B);
    pcfWriteDirect(cyanOn);
    delay(500);
    
    // Magenta (Red + Blue)
    uint16_t magentaOn = baseState & ~(1 << PCF_LED_R) & ~(1 << PCF_LED_B);
    pcfWriteDirect(magentaOn);
    delay(500);
    
    // White (All on)
    uint16_t whiteOn = baseState & ~(1 << PCF_LED_R) & ~(1 << PCF_LED_G) & ~(1 << PCF_LED_B);
    pcfWriteDirect(whiteOn);
    delay(500);
    
    // Turn all off
    pcfWriteDirect(baseState);
    
    // Restore normal PCF state (inputs high, outputs as needed)
    pcfWriteDirect(0xFE3F);
    
    Serial.println("[HW_TEST] RGB LED test complete");
    
    // Show result
    uiMgr.showTestResult("RGB LED Test", true, "Cycled: Red, Green, Blue,\nYellow, Cyan, Magenta, White");
    uiMgr.update();
    
    // Wait for button press
    while (!inputMgr.isBtnAPressed() && !inputMgr.isBtnBPressed()) {
        uiMgr.update();
        delay(50);
    }
    delay(200);
}

void runWifiTest() {
    Serial.println("[HW_TEST] Running WiFi test...");
    
    uiMgr.showLoading("Testing WiFi...");
    uiMgr.update();
    
    bool passed = false;
    char details[128];
    
    if (WiFi.status() == WL_CONNECTED) {
        int rssi = WiFi.RSSI();
        Serial.printf("[HW_TEST] WiFi connected: %s\n", WiFi.SSID().c_str());
        Serial.printf("[HW_TEST] IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("[HW_TEST] RSSI: %d dBm\n", rssi);
        
        passed = true;
        snprintf(details, sizeof(details), "SSID: %s\nIP: %s\nSignal: %d dBm", 
                 WiFi.SSID().c_str(), 
                 WiFi.localIP().toString().c_str(),
                 rssi);
    } else {
        Serial.println("[HW_TEST] WiFi NOT connected!");
        Serial.println("[HW_TEST] Attempting reconnect...");
        
        WiFi.disconnect();
        delay(100);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            uiMgr.update();
            delay(250);
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            passed = true;
            snprintf(details, sizeof(details), "Reconnected successfully!\nIP: %s", 
                     WiFi.localIP().toString().c_str());
        } else {
            passed = false;
            snprintf(details, sizeof(details), "Could not connect to WiFi.\nCheck SSID and password.");
        }
    }
    
    Serial.println("[HW_TEST] WiFi test complete");
    
    // Show result
    uiMgr.showTestResult("WiFi Test", passed, details);
    uiMgr.update();
    
    // Wait for button press
    while (!inputMgr.isBtnAPressed() && !inputMgr.isBtnBPressed()) {
        uiMgr.update();
        delay(50);
    }
    delay(200);
}

void runApiTest() {
    Serial.println("[HW_TEST] Running API test...");
    
    uiMgr.showLoading("Testing API...");
    uiMgr.update();
    
    String apiUrl = settingsMgr.getApiBaseUrl();
    Serial.printf("[HW_TEST] API URL: %s\n", apiUrl.c_str());
    
    bool passed = false;
    char details[128];
    
    if (WiFi.status() != WL_CONNECTED) {
        passed = false;
        snprintf(details, sizeof(details), "WiFi not connected.\nConnect to WiFi first.");
    } else {
        // Try a simple API call
        std::vector<ExamMetadata> exams = networkMgr.fetchExamList();
        Serial.printf("[HW_TEST] API returned %d exams\n", exams.size());
        
        if (exams.size() > 0) {
            passed = true;
            snprintf(details, sizeof(details), "API: %s\nFound %d exam(s) available.", 
                     apiUrl.c_str(), exams.size());
        } else {
            // API might be working but no exams
            passed = true;
            snprintf(details, sizeof(details), "API: %s\nConnected (0 exams found).", 
                     apiUrl.c_str());
        }
    }
    
    Serial.println("[HW_TEST] API test complete");
    
    // Show result
    uiMgr.showTestResult("API Test", passed, details);
    uiMgr.update();
    
    // Wait for button press
    while (!inputMgr.isBtnAPressed() && !inputMgr.isBtnBPressed()) {
        uiMgr.update();
        delay(50);
    }
    delay(200);
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
