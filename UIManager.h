/**
 * UI Manager - LVGL Screen Manager for Study Engine
 * Handles all LVGL UI screens and navigation
 * Updated for LVGL 9.x API
 */

#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <lvgl.h>
#include <TFT_eSPI.h>
#include "UITheme.h"
#include "config.h"

// Forward declare
class InputManager;

class UIManager {
public:
    // Initialize LVGL and display driver
    void begin();
    
    // Must be called in loop
    void update();
    
    // Screen creation methods
    void showMainMenu(int selectedIndex, int itemCount, const char** items);
    void showLoading(const char* message);
    void showExamList(const char** examNames, int count, int selectedIndex, const char* title = "Select Exam");
    void showTextInput(const char* title, const char* currentText, bool showCursor);
    void showQuestion(int qNum, int totalQ, const char* questionText, 
                      const char** options, int optionCount,
                      int pendingAnswer, int confirmedAnswer);
    void showPauseMenu(int selectedIndex);
    void showOverview(int questionCount, int* answers, uint8_t* confirmed, int selectedIndex, int scrollOffset);
    void showResult(int score, int total, float percentage);
    void showExamComplete();
    void showNoExams();
    void showError(const char* message);
    
    // Flashcard Mode
    void showFlashcardFront(const char* text, int current, int total);
    void showFlashcardBack(const char* front, const char* back);
    void showFlashcardFinished(int total, int easy, int hard, int again);
    void showFlashcardPauseMenu(int selectedIndex);
    
    // Quiz Mode
    void showQuizQuestionText(int qNum, int total, const char* question, const char* currentInput, bool showCursor);
    void showQuizReview(int qNum, int total, const char* question, const char* userAnswer, const char* correctAnswer, bool isCorrect);

    // Study Timer
    void showStudyTimer(unsigned long elapsedSeconds, bool isPaused, bool phoneDetected, bool userAway);
    void showStudyStart();
    
    // Update specific elements without full redraw
    void updateAnswerState(int optionIndex, int pendingAnswer, int confirmedAnswer);
    
    // Get the TFT instance
    TFT_eSPI& getTft() { return tft; }
    
private:
    TFT_eSPI tft = TFT_eSPI();
    
    // LVGL 9.x display - uses lv_display_t instead of drivers
    static lv_display_t* display;
    static uint8_t* draw_buf;
    
    // Current screen objects
    lv_obj_t* currentScreen = nullptr;
    lv_obj_t* answerBtns[4] = {nullptr};
    lv_obj_t* questionLabel = nullptr;
    lv_obj_t* progressLabel = nullptr;
    
    // Flush callback for LVGL 9.x
    static void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
    
    // Helper methods
    lv_obj_t* createScreen();
    void loadScreen(lv_obj_t* scr);  // Load screen and force refresh
    lv_obj_t* createHeader(lv_obj_t* parent, const char* title, bool showBack = false);
    lv_obj_t* createCard(lv_obj_t* parent, int x, int y, int w, int h);
    lv_obj_t* createButton(lv_obj_t* parent, const char* text, bool primary = true);
    lv_obj_t* createAnswerButton(lv_obj_t* parent, int index, const char* text);
    void setAnswerButtonState(lv_obj_t* btn, int index, bool isPending, bool isConfirmed);
};

// Singleton instance
extern UIManager uiMgr;

#endif
