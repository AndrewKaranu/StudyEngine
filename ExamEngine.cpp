/**
 * ExamEngine - Scanatron Exam Mode
 * Now uses LVGL via UIManager for modern UI
 */

#include "ExamEngine.h"
#include "UIManager.h"
#include <lvgl.h>

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
                uiMgr.showLoading("Downloading Exam...");
                display.showStatus("Downloading...");
                
                String json = network.fetchExamJson(availableExams[selectedExamIndex].id);
                
                if (json.length() == 0) {
                    uiMgr.showError("Download Failed!");
                    delay(2000);
                    state = EXAM_SELECT;
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
                    delay(2000);
                    state = EXAM_SELECT;
                    needsFullRedraw = true;
                    return;
                }
                
                currentExam.id = doc["id"].as<String>();
                currentExam.title = doc["title"].as<String>();
                currentExam.durationMinutes = doc["duration_minutes"];
                currentExam.showResultsImmediate = doc["show_results_immediate"];
                
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
                
                // Init Answers
                studentAnswers.clear();
                answersConfirmed.clear();
                for (size_t i = 0; i < currentExam.questions.size(); i++) {
                    studentAnswers.push_back(-1);
                    answersConfirmed.push_back(0);  // Use 0/1 instead of false/true for uint8_t
                }
                
                state = EXAM_RUNNING;
                startTime = millis();
                totalPausedTime = 0;
                currentQuestionIndex = 0;
                lastQuestionIndex = -1;
                pendingAnswer = -1;
                needsFullRedraw = true;
                systemState = 2; // STATE_SCANATRON_RUN
            }
            break;
            
        default:
            break;
    }
}

void ExamEngine::handleRun(DisplayManager& display, InputManager& input, SENetworkManager& network, int& systemState) {
    if (state == EXAM_RUNNING) {
        // Timer Check
        unsigned long elapsed = (millis() - startTime - totalPausedTime) / 1000;
        unsigned long totalSeconds = currentExam.durationMinutes * 60;
        unsigned long remaining = (elapsed < totalSeconds) ? (totalSeconds - elapsed) : 0;
        unsigned long currentTimerSeconds = remaining;
        
        // Update OLED timer
        display.showExamTimer(remaining, currentQuestionIndex + 1, currentExam.questions.size());
        
        if (remaining <= 0) {
            state = EXAM_SUBMITTING;
            needsFullRedraw = true;
            return;
        }
        
        // Long press detection for D button (pause menu)
        bool btnDPressed = input.isBtnDPressed();
        if (btnDPressed && !btnDWasPressed) {
            btnDPressStart = millis();
            btnDWasPressed = true;
        } else if (btnDPressed && btnDWasPressed) {
            if (millis() - btnDPressStart >= LONG_PRESS_MS) {
                state = EXAM_PAUSED;
                pauseStartTime = millis();
                pauseMenuIndex = 0;
                lastPauseMenuIndex = -1;
                needsFullRedraw = true;
                btnDWasPressed = false;
                delay(200);
                return;
            }
        } else if (!btnDPressed && btnDWasPressed) {
            // Short press - handle answer D with select/confirm logic
            if (millis() - btnDPressStart < LONG_PRESS_MS) {
                if (pendingAnswer == 3) {
                    studentAnswers[currentQuestionIndex] = 3;
                    answersConfirmed[currentQuestionIndex] = 1;
                    pendingAnswer = -1;
                    needsFullRedraw = true;
                } else {
                    pendingAnswer = 3;
                    needsFullRedraw = true;
                }
            }
            btnDWasPressed = false;
        }
        
        // Navigation with keyboard: '[' = prev, ']' = next
        char c = input.readCardKB();
        bool navChanged = false;
        if (c == '[' || c == 'p' || c == 'P') {
            if (currentQuestionIndex > 0) {
                currentQuestionIndex--;
                navChanged = true;
            }
        } else if (c == ']' || c == 'n' || c == 'N') {
            if (currentQuestionIndex < (int)currentExam.questions.size() - 1) {
                currentQuestionIndex++;
                navChanged = true;
            }
        } else if (c == 13) { // Enter - submit exam
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
        
        // Answer Input - Buttons A, B, C with select/confirm logic
        auto handleAnswerButton = [&](int answerIndex, bool pressed) {
            if (!pressed) return;
            
            if (pendingAnswer == answerIndex) {
                studentAnswers[currentQuestionIndex] = answerIndex;
                answersConfirmed[currentQuestionIndex] = 1;
                pendingAnswer = -1;
                needsFullRedraw = true;
            } else {
                pendingAnswer = answerIndex;
                needsFullRedraw = true;
            }
            delay(150);
        };
        
        handleAnswerButton(0, input.isBtnAPressed());
        handleAnswerButton(1, input.isBtnBPressed());
        handleAnswerButton(2, input.isBtnCPressed());
        
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
        // Keep LVGL responsive
        lv_timer_handler();
        
        // Continue updating timer on OLED even when paused
        display.showStatus("PAUSED");
        
        if (needsFullRedraw || pauseMenuIndex != lastPauseMenuIndex) {
            uiMgr.showPauseMenu(pauseMenuIndex);
            lastPauseMenuIndex = pauseMenuIndex;
            needsFullRedraw = false;
        }
        
        // Navigation with C/D buttons
        if (input.isBtnCPressed()) {
            if (pauseMenuIndex > 0) {
                pauseMenuIndex--;
            }
            delay(150);
        }
        if (input.isBtnDPressed()) {
            if (pauseMenuIndex < 1) {
                pauseMenuIndex++;
            }
            delay(150);
        }
        
        // Confirm selection with A
        if (input.isBtnAPressed()) {
            if (pauseMenuIndex == 0) {
                // View All - go to overview
                state = EXAM_OVERVIEW;
                overviewSelectedIndex = currentQuestionIndex;
                lastOverviewIndex = -1;
                overviewScrollOffset = 0;
                needsFullRedraw = true;
                // Wait for button release to prevent immediate action in overview
                unsigned long waitStart = millis();
                while (input.isBtnAPressed() && millis() - waitStart < 2000) { // 2s timeout
                    input.update();
                    lv_timer_handler();  // Keep UI responsive while waiting
                    delay(10);
                }
                delay(100);
                Serial.println("[EXAM] Entering Overview");
                return;
            } else if (pauseMenuIndex == 1) {
                // Exit exam
                reset();
                systemState = 0;
                delay(200);
                return;
            }
        }
        
        if (input.isBtnBPressed()) {
            // Resume exam
            totalPausedTime += (millis() - pauseStartTime);
            state = EXAM_RUNNING;
            needsFullRedraw = true;
            delay(200);
        }
        
    } else if (state == EXAM_OVERVIEW) {
        // Keep LVGL responsive
        lv_timer_handler();
        
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
        if (input.isBtnAPressed()) {
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
        
        if (input.isBtnBPressed()) {
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
            int score = 0;
            for (size_t i = 0; i < currentExam.questions.size(); i++) {
                if (studentAnswers[i] == currentExam.questions[i].correctOption) score++;
            }
            float pct = (float)score / currentExam.questions.size() * 100.0f;
            
            uiMgr.showResult(score, currentExam.questions.size(), pct);
            display.showStatus("Results");
            needsFullRedraw = false;
        }
        
        if (input.isBtnAPressed() || input.isBtnBPressed()) {
            reset();
            systemState = 0;
            delay(200);
        }
        
    } else if (state == EXAM_DONE) {
        if (needsFullRedraw) {
            uiMgr.showExamComplete();
            display.showStatus("Complete!");
            needsFullRedraw = false;
        }
        
        if (input.isBtnAPressed() || input.isBtnBPressed()) {
            reset();
            systemState = 0;
            delay(200);
        }
    }
}
