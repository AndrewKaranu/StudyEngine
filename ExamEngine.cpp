/**
 * ExamEngine - Scanatron Exam Mode
 * Now uses LVGL via UIManager for modern UI
 */

#include "ExamEngine.h"
#include "UIManager.h"
#include <lvgl.h>

// External feedback functions from main sketch
extern void beepClick();
extern void beepSuccess();
extern void beepError();
extern void beepWarning();
extern void beepComplete();
extern void setLed(bool red, bool green);
extern void ledOff();
extern void flashLed(bool red, bool green, int count, int onTime, int offTime);
extern void feedbackSuccess();
extern void feedbackError();

void ExamEngine::reset() {
    state = EXAM_INIT;
    selectedExamIndex = 0;
    lastSelectedExamIndex = -1;
    studentName = "";
    studentId = "";
    lastInputText = "";
    currentQuestionIndex = 0;
    lastQuestionIndex = -1;
    totalPausedTime = 0;
    needsFullRedraw = true;
    pauseMenuIndex = 0;
    lastPauseMenuIndex = -1;
    btnDWasPressed = false;
    pendingAnswer = -1;
    lastPendingAnswer = -1;
    lastTimerSeconds = 0;
    overviewSelectedIndex = 0;
    lastOverviewIndex = -1;
    overviewScrollOffset = 0;
    cursorVisible = true;
    lastCursorBlink = 0;
}

unsigned long ExamEngine::getRemainingSeconds() {
    if (state != EXAM_RUNNING) return 0;
    unsigned long elapsed = (millis() - startTime - totalPausedTime) / 1000;
    unsigned long totalSeconds = currentExam.durationMinutes * 60;
    return (elapsed < totalSeconds) ? (totalSeconds - elapsed) : 0;
}

void ExamEngine::handleSetup(DisplayManager& display, InputManager& input, SENetworkManager& network, int& systemState) {
    switch (state) {
        case EXAM_INIT:
            uiMgr.showLoading("Fetching Exams...");
            display.showStatus("Fetching Exams...");
            
            availableExams = network.fetchExamList();
            
            if (availableExams.empty()) {
                state = EXAM_NO_EXAMS;
                needsFullRedraw = true;
            } else {
                state = EXAM_SELECT;
                lastSelectedExamIndex = -1;
                needsFullRedraw = true;
            }
            break;
            
        case EXAM_NO_EXAMS:
            if (needsFullRedraw) {
                uiMgr.showNoExams();
                display.showStatus("No Exams");
                needsFullRedraw = false;
            }
            
            if (input.isBtnBPressed()) {
                reset();
                systemState = 0;
                delay(200);
            }
            break;

        case EXAM_SELECT:
            {
                // Navigation with potentiometer
                int newIndex = input.getScrollIndex(availableExams.size());
                
                // Redraw if selection changed or first draw
                if (newIndex != lastSelectedExamIndex || needsFullRedraw) {
                    selectedExamIndex = newIndex;
                    
                    // Build exam names array for UI
                    const char* examNames[availableExams.size()];
                    for (size_t i = 0; i < availableExams.size(); i++) {
                        examNames[i] = availableExams[i].title.c_str();
                    }
                    
                    uiMgr.showExamList(examNames, availableExams.size(), selectedExamIndex);
                    display.showStatus("Select Exam");
                    
                    lastSelectedExamIndex = selectedExamIndex;
                    needsFullRedraw = false;
                }
                
                // Button A: Confirm selection
                if (input.isBtnAPressed()) {
                    state = EXAM_NAME;
                    studentName = "";
                    lastInputText = "";
                    needsFullRedraw = true;
                    delay(200);
                }
                
                // Button B: Go back to menu
                if (input.isBtnBPressed()) {
                    reset();
                    systemState = 0;
                    delay(200);
                }
            }
            break;

        case EXAM_NAME:
            {
                // Cursor blink
                if (millis() - lastCursorBlink > 500) {
                    cursorVisible = !cursorVisible;
                    lastCursorBlink = millis();
                    needsFullRedraw = true;
                }
                
                // Keyboard input
                char c = input.readCardKB();
                if (c == 13 || input.isBtnAPressed()) { // Enter or Button A to confirm
                    if (studentName.length() > 0) {
                        state = EXAM_ID;
                        studentId = "";
                        lastInputText = "";
                        needsFullRedraw = true;
                        delay(200);
                    }
                } else if (c == 8) { // Backspace
                    if (studentName.length() > 0) {
                        studentName.remove(studentName.length() - 1);
                        needsFullRedraw = true;
                    }
                } else if (c == 27 || input.isBtnBPressed()) { // Escape or Button B to go back
                    state = EXAM_SELECT;
                    lastSelectedExamIndex = -1;
                    needsFullRedraw = true;
                    delay(200);
                } else if (c >= 32 && c <= 126) { // Printable characters
                    studentName += c;
                    needsFullRedraw = true;
                }
                
                // Redraw only if text changed
                if (studentName != lastInputText || needsFullRedraw) {
                    uiMgr.showTextInput("Enter Your Name", studentName.c_str(), cursorVisible);
                    display.showStatus("Enter Name");
                    lastInputText = studentName;
                    needsFullRedraw = false;
                }
            }
            break;

        case EXAM_ID:
            {
                // Cursor blink
                if (millis() - lastCursorBlink > 500) {
                    cursorVisible = !cursorVisible;
                    lastCursorBlink = millis();
                    needsFullRedraw = true;
                }
                
                char c = input.readCardKB();
                if (c == 13 || input.isBtnAPressed()) { // Enter or Button A to confirm
                    if (studentId.length() > 0) {
                        state = EXAM_DOWNLOAD;
                        needsFullRedraw = true;
                    }
                } else if (c == 8) { // Backspace
                    if (studentId.length() > 0) {
                        studentId.remove(studentId.length() - 1);
                        needsFullRedraw = true;
                    }
                } else if (c == 27 || input.isBtnBPressed()) { // Escape or Button B to go back
                    state = EXAM_NAME;
                    lastInputText = "";
                    needsFullRedraw = true;
                    delay(200);
                } else if (c >= 32 && c <= 126) {
                    studentId += c;
                    needsFullRedraw = true;
                }
                
                if (studentId != lastInputText || needsFullRedraw) {
                    uiMgr.showTextInput("Enter Student ID", studentId.c_str(), cursorVisible);
                    display.showStatus("Enter ID");
                    lastInputText = studentId;
                    needsFullRedraw = false;
                }
            }
            break;

        case EXAM_DOWNLOAD:
            {
                Serial.println("[EXAM] Starting download...");
                uiMgr.showLoading("Downloading Exam...");
                uiMgr.update();  // Force LVGL refresh
                display.showStatus("Downloading...");
                
                Serial.printf("[EXAM] Fetching exam ID: %s\n", availableExams[selectedExamIndex].id.c_str());
                String json = network.fetchExamJson(availableExams[selectedExamIndex].id);
                
                Serial.printf("[EXAM] Received %d bytes\n", json.length());
                
                if (json.length() == 0) {
                    Serial.println("[EXAM] Download failed - empty response");
                    uiMgr.showError("Download Failed!");
                    uiMgr.update();
                    delay(2000);
                    state = EXAM_SELECT;
                    lastSelectedExamIndex = -1;  // Force redraw of exam list
                    needsFullRedraw = true;
                    return;
                }
                
                // Parse JSON
                DynamicJsonDocument doc(16384);
                DeserializationError error = deserializeJson(doc, json);
                
                if (error) {
                    Serial.print("[EXAM] JSON Error: ");
                    Serial.println(error.c_str());
                    uiMgr.showError("Parse Error!");
                    uiMgr.update();
                    delay(2000);
                    state = EXAM_SELECT;
                    lastSelectedExamIndex = -1;
                    needsFullRedraw = true;
                    return;
                }
                
                currentExam.id = doc["id"].as<String>();
                currentExam.title = doc["title"].as<String>();
                currentExam.durationMinutes = doc["duration_minutes"] | 30;  // Default 30 min if missing
                currentExam.showResultsImmediate = doc["show_results_immediate"] | true;
                
                Serial.printf("[EXAM] Parsed: %s, Duration: %d min\n", 
                    currentExam.title.c_str(), currentExam.durationMinutes);
                
                currentExam.questions.clear();
                JsonArray qArr = doc["questions"];
                for (JsonObject qObj : qArr) {
                    Question q;
                    q.id = qObj["id"];
                    q.text = qObj["text"].as<String>();
                    q.correctOption = qObj["correct_option"];
                    q.options.clear();
                    JsonArray optArr = qObj["options"];
                    for (JsonVariant opt : optArr) {
                        q.options.push_back(opt.as<String>());
                    }
                    currentExam.questions.push_back(q);
                }
                
                Serial.printf("[EXAM] Loaded %d questions\n", currentExam.questions.size());
                
                if (currentExam.questions.size() == 0) {
                    Serial.println("[EXAM] No questions in exam!");
                    uiMgr.showError("No Questions!");
                    uiMgr.update();
                    delay(2000);
                    state = EXAM_SELECT;
                    lastSelectedExamIndex = -1;
                    needsFullRedraw = true;
                    return;
                }
                
                // Init Answers
                studentAnswers.clear();
                answersConfirmed.clear();
                for (size_t i = 0; i < currentExam.questions.size(); i++) {
                    studentAnswers.push_back(-1);
                    answersConfirmed.push_back(0);  // Use 0/1 instead of false/true for uint8_t
                }
                
                Serial.println("[EXAM] Starting exam!");
                state = EXAM_RUNNING;
                startTime = millis();
                totalPausedTime = 0;
                currentQuestionIndex = 0;
                lastQuestionIndex = -1;
                pendingAnswer = -1;
                needsFullRedraw = true;
                systemState = 7; // STATE_SCANATRON_RUN (MENU=0, SETTINGS=1, ADMIN_URL=2, DEV_MODE=3, API_URL_EDIT=4, HW_TEST=5, SCANATRON_SETUP=6, SCANATRON_RUN=7)
                
                // Force immediate draw of first question
                Serial.println("[EXAM] Drawing first question...");
                Question& q = currentExam.questions[0];
                const char* options[4];
                for (size_t i = 0; i < q.options.size() && i < 4; i++) {
                    options[i] = q.options[i].c_str();
                }
                uiMgr.showQuestion(1, currentExam.questions.size(), q.text.c_str(), options, q.options.size(), -1, -1);
                uiMgr.update();
                display.showExamTimer(currentExam.durationMinutes * 60, 1, currentExam.questions.size());
                Serial.println("[EXAM] First question displayed!");
            }
            break;
            
        default:
            break;
    }
}

void ExamEngine::handleRun(DisplayManager& display, InputManager& input, SENetworkManager& network, int& systemState) {
    // Debug: Log state on first few calls
    static int runCallCount = 0;
    if (runCallCount < 5) {
        Serial.printf("[EXAM] handleRun called, state=%d, runCount=%d\n", state, runCallCount);
        runCallCount++;
    }
    
    if (state == EXAM_RUNNING) {
        // ========== READ ALL INPUTS FIRST (before any blocking I2C operations) ==========
        bool btnAPressed = input.isBtnAPressed();
        bool btnBPressed = input.isBtnBPressed();
        bool btnCPressed = input.isBtnCPressed();
        bool btnDPressed = input.isBtnDPressed();
        char kbChar = input.readCardKB();
        
        // Periodic debug heartbeat
        static unsigned long lastHeartbeat = 0;
        if (millis() - lastHeartbeat > 2000) {
            Serial.printf("[EXAM-RUN] Alive. Q:%d, pending:%d, Btns: A=%d B=%d C=%d D=%d\n", 
                currentQuestionIndex, pendingAnswer, btnAPressed, btnBPressed, btnCPressed, btnDPressed);
            lastHeartbeat = millis();
        }
        
        // Timer Check
        unsigned long elapsed = (millis() - startTime - totalPausedTime) / 1000;
        unsigned long totalSeconds = currentExam.durationMinutes * 60;
        unsigned long remaining = (elapsed < totalSeconds) ? (totalSeconds - elapsed) : 0;
        unsigned long currentTimerSeconds = remaining;
        
        // Time warning: Red LED and beep when 1 minute left
        static bool oneMinuteWarningGiven = false;
        static bool thirtySecWarningGiven = false;
        if (remaining <= 60 && remaining > 30 && !oneMinuteWarningGiven) {
            beepWarning();
            flashLed(true, false, 3, 100, 100);
            oneMinuteWarningGiven = true;
            Serial.println("[EXAM] 1 minute warning!");
        }
        if (remaining <= 30 && remaining > 0 && !thirtySecWarningGiven) {
            beepWarning();
            flashLed(true, false, 5, 80, 80);
            thirtySecWarningGiven = true;
            Serial.println("[EXAM] 30 second warning!");
        }
        // Blink red LED in last 30 seconds
        if (remaining <= 30 && remaining > 0) {
            static unsigned long lastBlink = 0;
            if (millis() - lastBlink > 500) {
                static bool ledState = false;
                setLed(ledState, false);
                ledState = !ledState;
                lastBlink = millis();
            }
        } else {
            ledOff();
        }
        
        // Update OLED timer (only once per second to reduce I2C traffic)
        static unsigned long lastOledUpdate = 0;
        if (millis() - lastOledUpdate >= 1000) {
            display.showExamTimer(remaining, currentQuestionIndex + 1, currentExam.questions.size());
            lastOledUpdate = millis();
        }
        
        if (remaining <= 0) {
            ledOff();
            beepError();
            state = EXAM_SUBMITTING;
            needsFullRedraw = true;
            return;
        }
        
        // ESC key (27) opens pause menu
        if (kbChar == 27) {
            Serial.println("[EXAM] ESC pressed - opening pause menu");
            state = EXAM_PAUSED;
            pauseStartTime = millis();
            pauseMenuIndex = 0;
            lastPauseMenuIndex = -1;
            needsFullRedraw = true;
            return;
        }
        
        // D button - just selects/confirms answer D (no long press needed)
        static bool lastBtnD = false;
        static unsigned long lastBtnDTime = 0;
        const unsigned long BTN_D_DEBOUNCE = 200;
        
        if (btnDPressed && !lastBtnD && (millis() - lastBtnDTime >= BTN_D_DEBOUNCE)) {
            lastBtnDTime = millis();
            if (pendingAnswer == 3) {
                studentAnswers[currentQuestionIndex] = 3;
                answersConfirmed[currentQuestionIndex] = 1;
                pendingAnswer = -1;
                needsFullRedraw = true;
                flashLed(false, true, 1, 100, 0);
                beepSuccess();
                Serial.println("[EXAM] Answer D CONFIRMED");
            } else {
                pendingAnswer = 3;
                needsFullRedraw = true;
                beepClick();
                Serial.println("[EXAM] Answer D SELECTED");
            }
        }
        lastBtnD = btnDPressed;
        
        // Navigation with keyboard: '[' or left arrow = prev, ']' or right arrow = next
        // CardKB arrow keys: Left=180, Right=183, Up=181, Down=182
        bool navChanged = false;
        if (kbChar == '[' || kbChar == 'p' || kbChar == 'P' || kbChar == 180) {  // Left arrow
            if (currentQuestionIndex > 0) {
                currentQuestionIndex--;
                navChanged = true;
                Serial.printf("[EXAM] Nav: prev question -> %d\n", currentQuestionIndex + 1);
            }
        } else if (kbChar == ']' || kbChar == 'n' || kbChar == 'N' || kbChar == 183) {  // Right arrow
            if (currentQuestionIndex < (int)currentExam.questions.size() - 1) {
                currentQuestionIndex++;
                navChanged = true;
                Serial.printf("[EXAM] Nav: next question -> %d\n", currentQuestionIndex + 1);
            }
        } else if (kbChar == 13) { // Enter - submit exam
            state = EXAM_SUBMITTING;
            needsFullRedraw = true;
            return;
        }
        
        if (navChanged) {
            if (answersConfirmed[currentQuestionIndex]) {
                pendingAnswer = studentAnswers[currentQuestionIndex];
            } else {
                pendingAnswer = studentAnswers[currentQuestionIndex];
            }
            needsFullRedraw = true;
        }
        
        // Answer Input - Buttons A, B, C with debouncing and edge detection
        static bool lastBtnA = false, lastBtnB = false, lastBtnC = false;
        static unsigned long lastBtnATime = 0, lastBtnBTime = 0, lastBtnCTime = 0;
        const unsigned long BTN_DEBOUNCE = 200;  // 200ms debounce for answer buttons
        
        auto handleAnswerButton = [&](int answerIndex, bool pressed, bool& lastState, unsigned long& lastTime) {
            // Edge detection: only trigger on press (not hold)
            if (pressed && !lastState && (millis() - lastTime >= BTN_DEBOUNCE)) {
                lastTime = millis();
                
                Serial.printf("[EXAM] Button %d pressed! pending=%d\n", answerIndex, pendingAnswer);
                
                if (pendingAnswer == answerIndex) {
                    studentAnswers[currentQuestionIndex] = answerIndex;
                    answersConfirmed[currentQuestionIndex] = 1;
                    pendingAnswer = -1;
                    needsFullRedraw = true;
                    // Green flash + success beep for confirmed answer
                    flashLed(false, true, 1, 100, 0);
                    beepSuccess();
                    Serial.printf("[EXAM] Answer %d CONFIRMED for Q%d\n", answerIndex, currentQuestionIndex + 1);
                } else {
                    pendingAnswer = answerIndex;
                    needsFullRedraw = true;
                    // Quick beep for selection
                    beepClick();
                    Serial.printf("[EXAM] Answer %d SELECTED for Q%d\n", answerIndex, currentQuestionIndex + 1);
                }
            }
            lastState = pressed;
        };
        
        handleAnswerButton(0, btnAPressed, lastBtnA, lastBtnATime);
        handleAnswerButton(1, btnBPressed, lastBtnB, lastBtnBTime);
        handleAnswerButton(2, btnCPressed, lastBtnC, lastBtnCTime);
        
        // Draw question if needed
        bool questionChanged = (currentQuestionIndex != lastQuestionIndex);
        bool answerChanged = (pendingAnswer != lastPendingAnswer);
        
        if (needsFullRedraw || questionChanged || answerChanged) {
            Question& q = currentExam.questions[currentQuestionIndex];
            
            // Build options array
            const char* options[4];
            for (size_t i = 0; i < q.options.size() && i < 4; i++) {
                options[i] = q.options[i].c_str();
            }
            
            int confirmedAnswer = answersConfirmed[currentQuestionIndex] ? studentAnswers[currentQuestionIndex] : -1;
            
            uiMgr.showQuestion(
                currentQuestionIndex + 1,
                currentExam.questions.size(),
                q.text.c_str(),
                options,
                q.options.size(),
                pendingAnswer,
                confirmedAnswer
            );
            
            lastQuestionIndex = currentQuestionIndex;
            lastPendingAnswer = pendingAnswer;
            lastTimerSeconds = currentTimerSeconds;
            lastDrawTime = millis();
            needsFullRedraw = false;
        }
        
    } else if (state == EXAM_PAUSED) {
        // Read all inputs FIRST, before any other I2C or display operations
        uint16_t pcfRaw = 0xFFFF;
        Wire.requestFrom(0x20, 2);
        if (Wire.available() == 2) {
            pcfRaw = Wire.read();
            pcfRaw |= (Wire.read() << 8);
        }
        
        // Extract button states (active low)
        bool btnAPressed = !((pcfRaw >> 0) & 1);
        bool btnBPressed = !((pcfRaw >> 1) & 1);
        bool btnCPressed = !((pcfRaw >> 2) & 1);
        bool btnDPressed = !((pcfRaw >> 3) & 1);
        
        // Read keyboard
        char kbChar = 0;
        Wire.requestFrom(0x5F, 1);
        if (Wire.available()) {
            char c = Wire.read();
            if (c != 0) kbChar = c;
        }
        
        // Now do LVGL
        lv_timer_handler();
        
        // Debug: Log button states periodically
        static unsigned long lastPauseDebug = 0;
        if (millis() - lastPauseDebug > 1000) {
            Serial.printf("[PAUSE] Raw=0x%04X Btns: A=%d B=%d C=%d D=%d, kb=%d, menuIdx=%d\n", 
                pcfRaw, btnAPressed, btnBPressed, btnCPressed, btnDPressed, kbChar, pauseMenuIndex);
            lastPauseDebug = millis();
        }
        
        // Update OLED less frequently
        static unsigned long lastOledUpdate = 0;
        if (millis() - lastOledUpdate > 500) {
            display.showStatus("PAUSED");
            lastOledUpdate = millis();
        }
        
        if (needsFullRedraw || pauseMenuIndex != lastPauseMenuIndex) {
            uiMgr.showPauseMenu(pauseMenuIndex);
            lastPauseMenuIndex = pauseMenuIndex;
            needsFullRedraw = false;
        }
        
        // Use simple debounce timing instead of edge detection
        static unsigned long lastNavTime = 0;
        static unsigned long lastSelectTime = 0;
        const unsigned long NAV_DEBOUNCE = 250;
        const unsigned long SELECT_DEBOUNCE = 300;
        
        // Navigation with up/down arrows, C/D buttons
        if (millis() - lastNavTime >= NAV_DEBOUNCE) {
            // Up arrow (181) or C button - move up
            if (kbChar == 181 || btnCPressed) {
                if (pauseMenuIndex > 0) {
                    pauseMenuIndex--;
                    needsFullRedraw = true;
                    lastNavTime = millis();
                    Serial.printf("[PAUSE] Nav up -> %d\n", pauseMenuIndex);
                }
            }
            // Down arrow (182) or D button - move down
            if (kbChar == 182 || btnDPressed) {
                if (pauseMenuIndex < 1) {
                    pauseMenuIndex++;
                    needsFullRedraw = true;
                    lastNavTime = millis();
                    Serial.printf("[PAUSE] Nav down -> %d\n", pauseMenuIndex);
                }
            }
        }
        
        // Also allow potentiometer navigation
        int potIndex = input.getScrollIndex(2);
        if (potIndex != pauseMenuIndex) {
            pauseMenuIndex = potIndex;
            needsFullRedraw = true;
        }
        
        // Confirm selection with A or Enter
        if (millis() - lastSelectTime >= SELECT_DEBOUNCE) {
            if (btnAPressed || kbChar == 13) {
                lastSelectTime = millis();
                Serial.printf("[PAUSE] Select pressed, menuIdx=%d\n", pauseMenuIndex);
                
                if (pauseMenuIndex == 0) {
                    // View All - go to overview
                    state = EXAM_OVERVIEW;
                    overviewSelectedIndex = currentQuestionIndex;
                    lastOverviewIndex = -1;
                    overviewScrollOffset = 0;
                    needsFullRedraw = true;
                    Serial.println("[EXAM] Entering Overview");
                    return;
                } else if (pauseMenuIndex == 1) {
                    // Exit exam
                    reset();
                    systemState = 0;
                    return;
                }
            }
            
            // B button or ESC to resume exam
            if (btnBPressed || kbChar == 27) {
                lastSelectTime = millis();
                // Resume exam
                totalPausedTime += (millis() - pauseStartTime);
                state = EXAM_RUNNING;
                needsFullRedraw = true;
                Serial.println("[EXAM] Resuming exam");
                return;
            }
        }
        
    } else if (state == EXAM_OVERVIEW) {
        // Keep LVGL responsive
        lv_timer_handler();
        
        // Debounce to prevent immediate action when entering from pause menu
        static unsigned long overviewEnterTime = 0;
        static bool overviewJustEntered = true;
        
        if (needsFullRedraw) {
            overviewEnterTime = millis();
            overviewJustEntered = true;
        }
        
        // Wait 300ms after entering before accepting button presses
        bool canAcceptInput = !overviewJustEntered || (millis() - overviewEnterTime > 300);
        if (overviewJustEntered && canAcceptInput) {
            overviewJustEntered = false;
        }
        
        // Debug heartbeat
        static unsigned long lastDebug = 0;
        if (millis() - lastDebug > 1000) {
            Serial.printf("[OVERVIEW] Alive. Pot: %d, Idx: %d\n", input.getPotValue(), overviewSelectedIndex);
            lastDebug = millis();
        }
        
        // Only update OLED occasionally to not block
        static unsigned long lastOledUpdate = 0;
        if (millis() - lastOledUpdate > 500) {
            display.showStatus("Answer Sheet");
            lastOledUpdate = millis();
        }
        
        // Check buttons - A to jump to question, B to go back to pause menu
        // Only accept input after debounce period
        if (canAcceptInput && input.isBtnAPressed()) {
            Serial.println("[OVERVIEW] Button A pressed - Going to Question");
            currentQuestionIndex = overviewSelectedIndex;
            pendingAnswer = studentAnswers[currentQuestionIndex];
            totalPausedTime += (millis() - pauseStartTime);
            state = EXAM_RUNNING;
            lastQuestionIndex = -1;
            needsFullRedraw = true;
            delay(200);
            return;
        }
        
        if (canAcceptInput && input.isBtnBPressed()) {
            Serial.println("[OVERVIEW] Button B pressed - Back to Pause");
            state = EXAM_PAUSED;
            lastPauseMenuIndex = -1;
            needsFullRedraw = true;
            delay(200);
            return;
        }
        
        if (needsFullRedraw || overviewSelectedIndex != lastOverviewIndex) {
            // Calculate scroll offset
            int maxVisible = 5;  // Match UI rows
            if (overviewSelectedIndex < overviewScrollOffset) {
                overviewScrollOffset = overviewSelectedIndex;
            } else if (overviewSelectedIndex >= overviewScrollOffset + maxVisible) {
                overviewScrollOffset = overviewSelectedIndex - maxVisible + 1;
            }
            
            uiMgr.showOverview(
                currentExam.questions.size(),
                studentAnswers.data(),
                answersConfirmed.data(),
                overviewSelectedIndex,
                overviewScrollOffset
            );
            lastOverviewIndex = overviewSelectedIndex;
            needsFullRedraw = false;
        }
        
        // Navigation with potentiometer - add hysteresis to prevent jitter
        static int lastPotValue = -1;
        int pot = input.getPotValue();
        
        // Only update if pot changed significantly
        if (lastPotValue < 0 || abs(pot - lastPotValue) > 100) {
            lastPotValue = pot;
            int newIndex = input.getScrollIndex(currentExam.questions.size());
            
            if (newIndex != overviewSelectedIndex) {
                overviewSelectedIndex = newIndex;
                needsFullRedraw = true;
            }
        }
        
        // Keyboard navigation
        char c = input.readCardKB();
        if (c == '[' || c == 'p' || c == 'P') {
            if (overviewSelectedIndex > 0) {
                overviewSelectedIndex--;
                needsFullRedraw = true;
            }
        } else if (c == ']' || c == 'n' || c == 'N') {
            if (overviewSelectedIndex < (int)currentExam.questions.size() - 1) {
                overviewSelectedIndex++;
                needsFullRedraw = true;
            }
        }
        
    } else if (state == EXAM_SUBMITTING) {
        uiMgr.showLoading("Submitting Exam...");
        display.showStatus("Submitting...");
        
        // Calculate Score
        int score = 0;
        for (size_t i = 0; i < currentExam.questions.size(); i++) {
            if (studentAnswers[i] == currentExam.questions[i].correctOption) score++;
        }
        
        // Create JSON
        DynamicJsonDocument doc(4096);
        doc["exam_id"] = currentExam.id;
        doc["student_name"] = studentName;
        doc["student_id"] = studentId;
        doc["score"] = score;
        doc["total_questions"] = currentExam.questions.size();
        JsonArray ansArr = doc.createNestedArray("answers");
        for (int a : studentAnswers) ansArr.add(a);
        
        String payload;
        serializeJson(doc, payload);
        
        bool success = network.uploadResult(payload);
        
        if (!success) {
            Serial.println("[EXAM] Upload failed, but continuing...");
        }
        
        if (currentExam.showResultsImmediate) {
            state = EXAM_SHOW_RESULT;
            needsFullRedraw = true;
        } else {
            state = EXAM_DONE;
            needsFullRedraw = true;
        }
        
    } else if (state == EXAM_SHOW_RESULT) {
        if (needsFullRedraw) {
            ledOff();  // Ensure LED is off
            int score = 0;
            for (size_t i = 0; i < currentExam.questions.size(); i++) {
                if (studentAnswers[i] == currentExam.questions[i].correctOption) score++;
            }
            float pct = (float)score / currentExam.questions.size() * 100.0f;
            
            uiMgr.showResult(score, currentExam.questions.size(), pct);
            display.showStatus("Results");
            
            // Feedback based on score
            if (pct >= 80) {
                // Excellent! Green LED + victory jingle
                flashLed(false, true, 3, 150, 100);
                beepComplete();
            } else if (pct >= 50) {
                // Passed - single green flash
                flashLed(false, true, 2, 100, 100);
                beepSuccess();
            } else {
                // Failed - red flash
                flashLed(true, false, 2, 100, 100);
                beepError();
            }
            
            needsFullRedraw = false;
        }
        
        if (input.isBtnAPressed() || input.isBtnBPressed()) {
            reset();
            systemState = 0;
            delay(200);
        }
        
    } else if (state == EXAM_DONE) {
        if (needsFullRedraw) {
            ledOff();
            uiMgr.showExamComplete();
            display.showStatus("Complete!");
            flashLed(false, true, 2, 150, 100);
            beepComplete();
            needsFullRedraw = false;
        }
        
        if (input.isBtnAPressed() || input.isBtnBPressed()) {
            reset();
            systemState = 0;
            delay(200);
        }
    }
}
