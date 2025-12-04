#include "QuizEngine.h"
#include <Wire.h>

// External feedback functions from main sketch
extern void beepClick();
extern void beepSuccess();
extern void beepError();
extern void beepWarning();
extern void beepComplete();
extern void setLed(bool red, bool green);
extern void ledOff();
extern void flashLed(bool red, bool green, int count, int onTime, int offTime);

void QuizEngine::reset() {
    state = QUIZ_INIT;
    selectedQuizIndex = 0;
    lastSelectedQuizIndex = -1;
    currentQuestionIndex = 0;
    needsFullRedraw = true;
    availableQuizzes.clear();
    currentQuiz.questions.clear();
    userAnswers.clear();
    currentTextInput = "";
    selectedOption = -1;
}

void QuizEngine::handleRun(DisplayManager& display, InputManager& input, SENetworkManager& network, int& systemState) {
    switch (state) {
        case QUIZ_INIT:
            uiMgr.showLoading("Fetching Quizzes...");
            display.showStatus("Fetching Quizzes...");
            
            availableQuizzes = network.fetchQuizList();
            
            if (availableQuizzes.empty()) {
                uiMgr.showError("No Quizzes Found!");
                delay(2000);
                systemState = 0; // Back to menu
            } else {
                state = QUIZ_SELECT;
                lastSelectedQuizIndex = -1;
                needsFullRedraw = true;
            }
            break;

        case QUIZ_SELECT:
            {
                int newIndex = input.getScrollIndex(availableQuizzes.size());
                
                if (newIndex != selectedQuizIndex || needsFullRedraw) {
                    selectedQuizIndex = newIndex;
                    
                    std::vector<const char*> quizNames;
                    for (size_t i = 0; i < availableQuizzes.size(); i++) {
                        quizNames.push_back(availableQuizzes[i].title.c_str());
                    }
                    
                    uiMgr.showExamList(quizNames.data(), availableQuizzes.size(), selectedQuizIndex, "Select Quiz");
                    display.showStatus("Select Quiz");
                    
                    lastSelectedQuizIndex = selectedQuizIndex;
                    needsFullRedraw = false;
                }
                
                if (input.isBtnAPressed()) {
                    state = QUIZ_DOWNLOAD;
                    needsFullRedraw = true;
                    delay(200);
                }
                
                if (input.isBtnBPressed()) {
                    systemState = 0; // Back to menu
                    delay(200);
                }
            }
            break;

        case QUIZ_DOWNLOAD:
            {
                uiMgr.showLoading("Downloading Quiz...");
                display.showStatus("Downloading...");
                
                Quiz fullQuiz = network.fetchQuiz(availableQuizzes[selectedQuizIndex].id);
                
                if (fullQuiz.questions.empty()) {
                    uiMgr.showError("Empty Quiz!");
                    delay(2000);
                    state = QUIZ_SELECT;
                    needsFullRedraw = true;
                } else {
                    currentQuiz = fullQuiz;
                    state = QUIZ_RUN;
                    currentQuestionIndex = 0;
                    userAnswers.clear();
                    for(size_t i=0; i<currentQuiz.questions.size(); i++) userAnswers.push_back("");
                    currentTextInput = "";
                    selectedOption = -1;
                    needsFullRedraw = true;
                }
            }
            break;

        case QUIZ_RUN:
            {
                // Read all buttons FIRST via direct I2C
                uint16_t pcfRaw = 0xFFFF;
                Wire.requestFrom(0x20, 2);
                if (Wire.available() == 2) {
                    pcfRaw = Wire.read();
                    pcfRaw |= (Wire.read() << 8);
                }
                bool btnAPressed = !((pcfRaw >> 0) & 1);
                bool btnBPressed = !((pcfRaw >> 1) & 1);
                bool btnCPressed = !((pcfRaw >> 2) & 1);
                bool btnDPressed = !((pcfRaw >> 3) & 1);
                
                // Read keyboard
                char key = 0;
                Wire.requestFrom(0x5F, 1);
                if (Wire.available()) {
                    char c = Wire.read();
                    if (c != 0) key = c;
                }
                
                QuizQuestion& q = currentQuiz.questions[currentQuestionIndex];
                
                // Debounce timing
                static unsigned long lastBtnTime = 0;
                const unsigned long DEBOUNCE = 200;

                if (q.type == "mcq") {
                    // Handle MCQ
                    if (needsFullRedraw) {
                        const char* options[4];
                        for (size_t i = 0; i < q.options.size() && i < 4; i++) {
                            options[i] = q.options[i].c_str();
                        }
                        
                        uiMgr.showQuestion(
                            currentQuestionIndex + 1,
                            currentQuiz.questions.size(),
                            q.text.c_str(),
                            options,
                            q.options.size(),
                            selectedOption,
                            -1 
                        );
                        
                        display.showStatus("Quiz: MCQ");
                        needsFullRedraw = false;
                    }
                    
                    // Input with debounce
                    if (millis() - lastBtnTime >= DEBOUNCE) {
                        int newSelection = -1;
                        if (btnAPressed) newSelection = 0;
                        else if (btnBPressed) newSelection = 1;
                        else if (btnCPressed) newSelection = 2;
                        else if (btnDPressed) newSelection = 3;
                        
                        if (newSelection != -1 && newSelection < (int)q.options.size()) {
                            lastBtnTime = millis();
                            if (newSelection == selectedOption) {
                                // Confirm selection if pressed again
                                beepClick(); // Confirmation click
                                userAnswers[currentQuestionIndex] = String(selectedOption);
                                currentQuestionIndex++;
                                selectedOption = -1;
                                currentTextInput = "";
                                if (currentQuestionIndex >= (int)currentQuiz.questions.size()) {
                                    state = QUIZ_RESULTS;
                                }
                                needsFullRedraw = true;
                            } else {
                                beepClick(); // Selection click
                                selectedOption = newSelection;
                                needsFullRedraw = true;
                            }
                        }
                    }
                    
                    if (key == 13) { // Enter
                        if (selectedOption != -1) {
                            beepClick(); // Confirm click
                            userAnswers[currentQuestionIndex] = String(selectedOption);
                            currentQuestionIndex++;
                            selectedOption = -1;
                            currentTextInput = "";
                            if (currentQuestionIndex >= (int)currentQuiz.questions.size()) {
                                state = QUIZ_RESULTS;
                            }
                            needsFullRedraw = true;
                        }
                    }
                    
                } else {
                    // Handle Short Answer
                    if (millis() - lastCursorBlink > 500) {
                        cursorVisible = !cursorVisible;
                        lastCursorBlink = millis();
                        needsFullRedraw = true;
                    }
                    
                    if (needsFullRedraw) {
                        uiMgr.showQuizQuestionText(
                            currentQuestionIndex + 1,
                            currentQuiz.questions.size(),
                            q.text.c_str(),
                            currentTextInput.c_str(),
                            cursorVisible
                        );
                        display.showStatus("Quiz: Type Answer");
                        needsFullRedraw = false;
                    }
                    
                    if (key == 13) { // Enter
                        if (currentTextInput.length() > 0) {
                            beepClick(); // Confirm answer
                            userAnswers[currentQuestionIndex] = currentTextInput;
                            currentQuestionIndex++;
                            currentTextInput = "";
                            selectedOption = -1;
                            if (currentQuestionIndex >= (int)currentQuiz.questions.size()) {
                                state = QUIZ_RESULTS;
                            }
                            needsFullRedraw = true;
                        }
                    } else if (key == 8) { // Backspace
                        if (currentTextInput.length() > 0) {
                            currentTextInput.remove(currentTextInput.length() - 1);
                            needsFullRedraw = true;
                        }
                    } else if (key >= 32 && key <= 126) {
                        currentTextInput += key;
                        needsFullRedraw = true;
                    }
                }
                
                // ESC key opens pause menu
                if (key == 27) {
                    state = QUIZ_PAUSED;
                    pauseMenuIndex = 0;
                    lastPauseMenuIndex = -1;
                    needsFullRedraw = true;
                    return;
                }
            }
            break;

        case QUIZ_PAUSED:
            {
                // Read all buttons FIRST via direct I2C
                uint16_t pcfRaw = 0xFFFF;
                Wire.requestFrom(0x20, 2);
                if (Wire.available() == 2) {
                    pcfRaw = Wire.read();
                    pcfRaw |= (Wire.read() << 8);
                }
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
                
                // Keep LVGL responsive
                lv_timer_handler();
                
                // Update OLED less frequently
                static unsigned long lastOledUpdate = 0;
                if (millis() - lastOledUpdate > 500) {
                    display.showStatus("PAUSED");
                    lastOledUpdate = millis();
                }
                
                if (needsFullRedraw || pauseMenuIndex != lastPauseMenuIndex) {
                    uiMgr.showFlashcardPauseMenu(pauseMenuIndex);
                    lastPauseMenuIndex = pauseMenuIndex;
                    needsFullRedraw = false;
                }
                
                // Debounce timing
                static unsigned long lastNavTime = 0;
                static unsigned long lastSelectTime = 0;
                const unsigned long NAV_DEBOUNCE = 250;
                const unsigned long SELECT_DEBOUNCE = 300;
                
                // Navigation with C/D buttons or arrows
                if (millis() - lastNavTime >= NAV_DEBOUNCE) {
                    if (kbChar == 181 || btnCPressed) { // Up
                        if (pauseMenuIndex > 0) {
                            pauseMenuIndex--;
                            needsFullRedraw = true;
                            lastNavTime = millis();
                        }
                    }
                    if (kbChar == 182 || btnDPressed) { // Down
                        if (pauseMenuIndex < 1) {
                            pauseMenuIndex++;
                            needsFullRedraw = true;
                            lastNavTime = millis();
                        }
                    }
                }
                
                // Potentiometer navigation
                int potIndex = input.getScrollIndex(2);
                if (potIndex != pauseMenuIndex) {
                    pauseMenuIndex = potIndex;
                    needsFullRedraw = true;
                }
                
                // Select with A or Enter
                if (millis() - lastSelectTime >= SELECT_DEBOUNCE) {
                    if (btnAPressed || kbChar == 13) {
                        lastSelectTime = millis();
                        if (pauseMenuIndex == 0) {
                            // Resume
                            state = QUIZ_RUN;
                            needsFullRedraw = true;
                        } else {
                            // Exit
                            state = QUIZ_SELECT;
                            needsFullRedraw = true;
                        }
                        return;
                    }
                    
                    // B or ESC to resume
                    if (btnBPressed || kbChar == 27) {
                        lastSelectTime = millis();
                        state = QUIZ_RUN;
                        needsFullRedraw = true;
                        return;
                    }
                }
            }
            break;

        case QUIZ_RESULTS:
            {
                // Read all buttons FIRST via direct I2C
                uint16_t pcfRaw = 0xFFFF;
                Wire.requestFrom(0x20, 2);
                if (Wire.available() == 2) {
                    pcfRaw = Wire.read();
                    pcfRaw |= (Wire.read() << 8);
                }
                bool btnAPressed = !((pcfRaw >> 0) & 1);
                bool btnBPressed = !((pcfRaw >> 1) & 1);
                
                static bool resultsFeedbackDone = false;
                if (needsFullRedraw) {
                    int score = 0;
                    for (size_t i = 0; i < currentQuiz.questions.size(); i++) {
                        String correct = currentQuiz.questions[i].correctAnswer;
                        String user = userAnswers[i];
                        
                        if (currentQuiz.questions[i].type == "mcq") {
                            if (user == correct) score++;
                        } else {
                            // Case insensitive comparison for text
                            if (user.equalsIgnoreCase(correct)) score++;
                        }
                    }
                    
                    float pct = (float)score / currentQuiz.questions.size() * 100.0f;
                    
                    // Play result feedback once
                    if (!resultsFeedbackDone) {
                        if (pct >= 70.0f) {
                            beepComplete();
                            flashLed(false, true, 3, 150, 100); // Green flash for passing
                        } else {
                            beepError();
                            flashLed(true, false, 2, 200, 150); // Red flash for failing
                        }
                        resultsFeedbackDone = true;
                    }
                    
                    uiMgr.showResult(score, currentQuiz.questions.size(), pct);
                    display.showStatus("Quiz Complete");
                    needsFullRedraw = false;
                }
                
                // Debounce
                static unsigned long lastBtnTime = 0;
                const unsigned long DEBOUNCE = 250;
                
                if (millis() - lastBtnTime >= DEBOUNCE) {
                    if (btnAPressed) {
                        // Review Mode
                        resultsFeedbackDone = false; // Reset for next quiz
                        state = QUIZ_REVIEW;
                        reviewQuestionIndex = 0;
                        needsFullRedraw = true;
                        lastBtnTime = millis();
                    } else if (btnBPressed) {
                        // Exit
                        resultsFeedbackDone = false; // Reset for next quiz
                        state = QUIZ_SELECT;
                        needsFullRedraw = true;
                        lastBtnTime = millis();
                    }
                }
            }
            break;

        case QUIZ_REVIEW:
            {
                // Read all buttons FIRST via direct I2C
                uint16_t pcfRaw = 0xFFFF;
                Wire.requestFrom(0x20, 2);
                if (Wire.available() == 2) {
                    pcfRaw = Wire.read();
                    pcfRaw |= (Wire.read() << 8);
                }
                bool btnBPressed = !((pcfRaw >> 1) & 1);
                bool btnCPressed = !((pcfRaw >> 2) & 1);
                bool btnDPressed = !((pcfRaw >> 3) & 1);
                
                if (needsFullRedraw) {
                    QuizQuestion& q = currentQuiz.questions[reviewQuestionIndex];
                    String userAns = userAnswers[reviewQuestionIndex];
                    String correctAns = q.correctAnswer;
                    bool isCorrect = false;
                    
                    String displayUserAns = userAns;
                    String displayCorrectAns = correctAns;
                    
                    if (q.type == "mcq") {
                        // Convert indices to text
                        int userIdx = userAns.toInt();
                        int correctIdx = correctAns.toInt();
                        
                        if (userIdx >= 0 && userIdx < (int)q.options.size()) {
                            displayUserAns = q.options[userIdx];
                        }
                        if (correctIdx >= 0 && correctIdx < (int)q.options.size()) {
                            displayCorrectAns = q.options[correctIdx];
                        }
                        
                        if (userAns == correctAns) isCorrect = true;
                    } else {
                        if (userAns.equalsIgnoreCase(correctAns)) isCorrect = true;
                    }
                    
                    uiMgr.showQuizReview(
                        reviewQuestionIndex + 1,
                        currentQuiz.questions.size(),
                        q.text.c_str(),
                        displayUserAns.c_str(),
                        displayCorrectAns.c_str(),
                        isCorrect
                    );
                    
                    display.showStatus("Review Mode");
                    needsFullRedraw = false;
                }
                
                // Debounce
                static unsigned long lastBtnTime = 0;
                const unsigned long DEBOUNCE = 200;
                
                if (millis() - lastBtnTime >= DEBOUNCE) {
                    // Navigation
                    if (btnCPressed) { // Prev
                        if (reviewQuestionIndex > 0) {
                            reviewQuestionIndex--;
                            needsFullRedraw = true;
                            lastBtnTime = millis();
                        }
                    }
                    if (btnDPressed) { // Next
                        if (reviewQuestionIndex < (int)currentQuiz.questions.size() - 1) {
                            reviewQuestionIndex++;
                            needsFullRedraw = true;
                            lastBtnTime = millis();
                        }
                    }
                    
                    // Exit Review
                    if (btnBPressed) {
                        state = QUIZ_RESULTS;
                        needsFullRedraw = true;
                        lastBtnTime = millis();
                    }
                }
            }
            break;
    }
}
