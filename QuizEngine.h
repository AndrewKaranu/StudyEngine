#ifndef QUIZ_ENGINE_H
#define QUIZ_ENGINE_H

#include "config.h"
#include "DisplayManager.h"
#include "InputManager.h"
#include "NetworkManager.h"
#include "UIManager.h"
#include <vector>
#include <ArduinoJson.h>

enum QuizState {
    QUIZ_INIT,
    QUIZ_SELECT,
    QUIZ_DOWNLOAD,
    QUIZ_RUN,
    QUIZ_PAUSED,
    QUIZ_RESULTS,
    QUIZ_REVIEW
};

class QuizEngine {
private:
    QuizState state = QUIZ_INIT;
    std::vector<Quiz> availableQuizzes;
    Quiz currentQuiz;
    
    int selectedQuizIndex = 0;
    int lastSelectedQuizIndex = -1;
    
    int currentQuestionIndex = 0;
    int reviewQuestionIndex = 0; // For review mode
    bool needsFullRedraw = true;
    
    // User answers
    std::vector<String> userAnswers;
    String currentTextInput = "";
    bool cursorVisible = true;
    unsigned long lastCursorBlink = 0;
    
    // MCQ selection
    int selectedOption = -1;

    // Pause Menu
    int pauseMenuIndex = 0;
    int lastPauseMenuIndex = -1;
    unsigned long btnDPressStart = 0;
    bool btnDWasPressed = false;
    const unsigned long LONG_PRESS_MS = 1000;

public:
    void reset();
    void handleRun(DisplayManager& display, InputManager& input, SENetworkManager& network, int& systemState);
};

#endif
