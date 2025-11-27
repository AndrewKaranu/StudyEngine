#include "QuizEngine.h"

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
                int pot = input.getPotValue();
                int newIndex = map(pot, 0, 4095, 0, availableQuizzes.size());
                if (newIndex >= (int)availableQuizzes.size()) newIndex = availableQuizzes.size() - 1;
                if (newIndex < 0) newIndex = 0;
                
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
                QuizQuestion& q = currentQuiz.questions[currentQuestionIndex];
                char key = input.readCardKB(); // Read once

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
                    
                    // Input
                    int newSelection = -1;
                    if (input.isBtnAPressed()) newSelection = 0;
                    else if (input.isBtnBPressed()) newSelection = 1;
                    else if (input.isBtnCPressed()) newSelection = 2;
                    // D is handled separately for Long Press support
                    
                    if (newSelection != -1 && newSelection < (int)q.options.size()) {
                        if (newSelection == selectedOption) {
                            // Confirm selection if pressed again
                            userAnswers[currentQuestionIndex] = String(selectedOption);
                            currentQuestionIndex++;
                            selectedOption = -1;
                            currentTextInput = "";
                            if (currentQuestionIndex >= (int)currentQuiz.questions.size()) {
                                state = QUIZ_RESULTS;
                            }
                            needsFullRedraw = true;
                            delay(200);
                        } else {
                            selectedOption = newSelection;
                            needsFullRedraw = true;
                            delay(200);
                        }
                    }
                    
                    if (key == 13) { // Enter
                        if (selectedOption != -1) {
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
                
                // Long press detection for D button (pause menu)
                // Note: In MCQ mode, D is used for option 4. 
                // We need to be careful not to trigger option 4 if it's a long press.
                // However, the user specifically asked for "D long press to open the menu".
                // If D is pressed, we start timing. If released quickly -> Option 4. If held -> Menu.
                
                bool btnDPressed = input.isBtnDPressed();
                if (btnDPressed && !btnDWasPressed) {
                    btnDPressStart = millis();
                    btnDWasPressed = true;
                } else if (btnDPressed && btnDWasPressed) {
                    if (millis() - btnDPressStart >= LONG_PRESS_MS) {
                        state = QUIZ_PAUSED;
                        pauseMenuIndex = 0;
                        lastPauseMenuIndex = -1;
                        needsFullRedraw = true;
                        btnDWasPressed = false;
                        delay(200);
                        return;
                    }
                } else if (!btnDPressed && btnDWasPressed) {
                    // Short press D
                    if (millis() - btnDPressStart < LONG_PRESS_MS) {
                        // Only trigger Option 4 if we are in MCQ mode
                        if (q.type == "mcq") {
                             int selection = 3;
                             if (selection < (int)q.options.size()) {
                                 if (selection == selectedOption) {
                                     // Confirm
                                     userAnswers[currentQuestionIndex] = String(selectedOption);
                                     currentQuestionIndex++;
                                     selectedOption = -1;
                                     currentTextInput = "";
                                     if (currentQuestionIndex >= (int)currentQuiz.questions.size()) {
                                         state = QUIZ_RESULTS;
                                     }
                                     needsFullRedraw = true;
                                     delay(200);
                                 } else {
                                     selectedOption = selection;
                                     needsFullRedraw = true;
                                     delay(200);
                                 }
                             }
                        }
                    }
                    btnDWasPressed = false;
                }
                
                // Global Back / ESC
                if (key == 27) { // ESC
                    state = QUIZ_PAUSED; // Go to pause menu instead of direct exit
                    pauseMenuIndex = 0;
                    lastPauseMenuIndex = -1;
                    needsFullRedraw = true;
                }
            }
            break;

        case QUIZ_PAUSED:
            {
                // Keep LVGL responsive
                lv_timer_handler();
                display.showStatus("PAUSED");
                
                if (needsFullRedraw || pauseMenuIndex != lastPauseMenuIndex) {
                    uiMgr.showFlashcardPauseMenu(pauseMenuIndex); // Reuse Flashcard Pause Menu
                    lastPauseMenuIndex = pauseMenuIndex;
                    needsFullRedraw = false;
                }
                
                // Navigation
                if (input.isBtnCPressed()) {
                    if (pauseMenuIndex > 0) pauseMenuIndex--;
                    delay(150);
                }
                if (input.isBtnDPressed()) {
                    if (pauseMenuIndex < 1) pauseMenuIndex++;
                    delay(150);
                }
                
                // Select
                if (input.isBtnAPressed()) {
                    if (pauseMenuIndex == 0) {
                        // Resume
                        state = QUIZ_RUN;
                        needsFullRedraw = true;
                        delay(200);
                    } else {
                        // Exit
                        state = QUIZ_SELECT;
                        needsFullRedraw = true;
                        delay(200);
                    }
                }
            }
            break;

        case QUIZ_RESULTS:
            {
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
                    uiMgr.showResult(score, currentQuiz.questions.size(), pct);
                    display.showStatus("Quiz Complete");
                    needsFullRedraw = false;
                }
                
                if (input.isBtnAPressed()) {
                    // Review Mode
                    state = QUIZ_REVIEW;
                    reviewQuestionIndex = 0;
                    needsFullRedraw = true;
                    delay(200);
                } else if (input.isBtnBPressed()) {
                    // Exit
                    state = QUIZ_SELECT;
                    needsFullRedraw = true;
                    delay(200);
                }
            }
            break;

        case QUIZ_REVIEW:
            {
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
                
                // Navigation
                if (input.isBtnCPressed()) { // Prev
                    if (reviewQuestionIndex > 0) {
                        reviewQuestionIndex--;
                        needsFullRedraw = true;
                        delay(200);
                    }
                }
                if (input.isBtnDPressed()) { // Next
                    if (reviewQuestionIndex < (int)currentQuiz.questions.size() - 1) {
                        reviewQuestionIndex++;
                        needsFullRedraw = true;
                        delay(200);
                    }
                }
                
                // Exit Review
                if (input.isBtnBPressed()) {
                    state = QUIZ_RESULTS;
                    needsFullRedraw = true;
                    delay(200);
                }
            }
            break;
    }
}
