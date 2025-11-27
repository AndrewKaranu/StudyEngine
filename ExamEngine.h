#ifndef EXAM_ENGINE_H
#define EXAM_ENGINE_H

#include "config.h"
#include "DisplayManager.h"
#include "InputManager.h"
#include "NetworkManager.h"
#include "UIManager.h"
#include <vector>

struct Question {
    int id;
    String text;
    std::vector<String> options;
    int correctOption;
};

struct ExamData {
    String id;
    String title;
    int durationMinutes;
    bool showResultsImmediate;
    std::vector<Question> questions;
};

enum ExamState {
    EXAM_INIT,
    EXAM_NO_EXAMS,    // No exams found - waiting for B to go back
    EXAM_SELECT,
    EXAM_NAME,
    EXAM_ID,
    EXAM_DOWNLOAD,
    EXAM_RUNNING,
    EXAM_PAUSED,      // Pause menu
    EXAM_OVERVIEW,    // Scanatron sheet view
    EXAM_SUBMITTING,
    EXAM_SHOW_RESULT,
    EXAM_DONE
};

class ExamEngine {
private:
    ExamState state = EXAM_INIT;
    std::vector<ExamMetadata> availableExams;
    int selectedExamIndex = 0;
    int lastSelectedExamIndex = -1;
    
    String studentName = "";
    String studentId = "";
    String lastInputText = "";
    
    ExamData currentExam;
    std::vector<int> studentAnswers;
    std::vector<uint8_t> answersConfirmed;  // Use uint8_t instead of bool (vector<bool> has no data())
    int currentQuestionIndex = 0;
    int lastQuestionIndex = -1;
    int pendingAnswer = -1;
    int lastPendingAnswer = -1;
    
    unsigned long startTime = 0;
    unsigned long pauseStartTime = 0;
    unsigned long totalPausedTime = 0;
    unsigned long lastDrawTime = 0;
    unsigned long lastTimerSeconds = 0;
    
    // Long press detection for D button
    unsigned long btnDPressStart = 0;
    bool btnDWasPressed = false;
    static const unsigned long LONG_PRESS_MS = 1000;
    
    int pauseMenuIndex = 0;
    int lastPauseMenuIndex = -1;
    bool needsFullRedraw = true;
    
    // Overview (Scanatron sheet) variables
    int overviewSelectedIndex = 0;
    int lastOverviewIndex = -1;
    int overviewScrollOffset = 0;
    
    // Cursor blink for text input
    unsigned long lastCursorBlink = 0;
    bool cursorVisible = true;

public:
    void handleSetup(DisplayManager& display, InputManager& input, SENetworkManager& network, int& systemState);
    void handleRun(DisplayManager& display, InputManager& input, SENetworkManager& network, int& systemState);
    void reset();
    
    // Get remaining time for OLED display
    unsigned long getRemainingSeconds();
    int getCurrentQuestion() { return currentQuestionIndex + 1; }
    int getTotalQuestions() { return currentExam.questions.size(); }
    bool isRunning() { return state == EXAM_RUNNING; }
};

#endif
