/**
 * UI Manager Implementation
 * Modern LVGL-based UI for Study Engine
 * Updated for LVGL 9.x API
 */

#include "UIManager.h"
#include "UITheme.h"

// Static member initialization for LVGL 9.x
lv_display_t* UIManager::display = nullptr;
uint8_t* UIManager::draw_buf = nullptr;

// Singleton instance
UIManager uiMgr;

// Static flush callback for LVGL 9.x
void UIManager::disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    UIManager* mgr = (UIManager*)lv_display_get_user_data(disp);
    if (!mgr) {
        lv_display_flush_ready(disp);
        return;
    }
    
    mgr->tft.startWrite();
    mgr->tft.setAddrWindow(area->x1, area->y1, w, h);
    mgr->tft.pushColors((uint16_t*)px_map, w * h, true);
    mgr->tft.endWrite();
    
    lv_display_flush_ready(disp);
}

void UIManager::begin() {
    Serial.println("[UI] Initializing LVGL 9.x...");
    
    // Initialize TFT
    tft.init(); // Use init() instead of begin() for some drivers
    tft.setRotation(1);  // Landscape
    tft.fillScreen(TFT_BLACK);
    Serial.println("[UI] TFT initialized");
    
    // Initialize LVGL
    lv_init();
    Serial.println("[UI] lv_init() done");
    
    // Create display buffer (LVGL 9.x style) - reduced size for memory
    uint32_t buf_size = SCREEN_WIDTH * 20 * sizeof(lv_color_t);
    draw_buf = (uint8_t*)heap_caps_malloc(buf_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!draw_buf) {
        Serial.println("[UI] DMA alloc failed, using regular malloc");
        draw_buf = (uint8_t*)malloc(buf_size);
    }
    
    if (!draw_buf) {
        Serial.println("[UI] ERROR: Buffer allocation failed!");
        return;
    }
    
    // Create display (LVGL 9.x API)
    display = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!display) {
        Serial.println("[UI] ERROR: Display creation failed!");
        return;
    }
    lv_display_set_flush_cb(display, disp_flush);
    lv_display_set_buffers(display, draw_buf, NULL, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_user_data(display, this);
    
    // Initialize theme
    UITheme::init();
    
    Serial.println("[UI] LVGL initialized");
}

void UIManager::update() {
    lv_timer_handler();
}

// Helper to load screen and force immediate refresh
void UIManager::loadScreen(lv_obj_t* scr) {
    // Delete old screen if exists
    if (currentScreen != NULL && currentScreen != scr) {
        lv_obj_t* oldScreen = currentScreen;
        currentScreen = scr;
        lv_screen_load(scr);
        lv_obj_delete(oldScreen);
    } else {
        currentScreen = scr;
        lv_screen_load(scr);
    }
    
    // Force immediate refresh
    lv_obj_invalidate(scr);
    lv_refr_now(display);
}

lv_obj_t* UIManager::createScreen() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_add_style(scr, &UITheme::style_screen, 0);
    return scr;
}

lv_obj_t* UIManager::createHeader(lv_obj_t* parent, const char* title, bool showBack) {
    lv_obj_t* header = lv_obj_create(parent);
    lv_obj_set_size(header, SCREEN_WIDTH, 50);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_add_style(header, &UITheme::style_header, 0);
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    
    // Title label
    lv_obj_t* label = lv_label_create(header);
    lv_label_set_text(label, title);
    lv_obj_add_style(label, &UITheme::style_text_title, 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, showBack ? 40 : 16, 0);
    
    if (showBack) {
        // Back indicator
        lv_obj_t* backLabel = lv_label_create(header);
        lv_label_set_text(backLabel, LV_SYMBOL_LEFT);
        lv_obj_add_style(backLabel, &UITheme::style_text_title, 0);
        lv_obj_align(backLabel, LV_ALIGN_LEFT_MID, 12, 0);
    }
    
    return header;
}

lv_obj_t* UIManager::createCard(lv_obj_t* parent, int x, int y, int w, int h) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_pos(card, x, y);
    lv_obj_add_style(card, &UITheme::style_card, 0);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

lv_obj_t* UIManager::createButton(lv_obj_t* parent, const char* text, bool primary) {
    lv_obj_t* btn = lv_button_create(parent);
    lv_obj_add_style(btn, primary ? &UITheme::style_btn_primary : &UITheme::style_btn_secondary, 0);
    
    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    
    return btn;
}

lv_obj_t* UIManager::createAnswerButton(lv_obj_t* parent, int index, const char* text) {
    lv_obj_t* btn = lv_button_create(parent);
    lv_obj_add_style(btn, &UITheme::style_btn_answer[index], 0);
    lv_obj_set_size(btn, SCREEN_WIDTH - 40, 48);
    
    // Button letter
    const char* letters[] = {"A", "B", "C", "D"};
    lv_obj_t* letterLabel = lv_label_create(btn);
    lv_label_set_text(letterLabel, letters[index]);
    lv_obj_set_style_text_font(letterLabel, &lv_font_montserrat_20, 0);
    lv_obj_align(letterLabel, LV_ALIGN_LEFT_MID, 10, 0);
    
    // Answer text
    lv_obj_t* textLabel = lv_label_create(btn);
    lv_label_set_text(textLabel, text);
    lv_obj_set_style_text_font(textLabel, &lv_font_montserrat_16, 0);
    lv_label_set_long_mode(textLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(textLabel, SCREEN_WIDTH - 120);
    lv_obj_align(textLabel, LV_ALIGN_LEFT_MID, 45, 0);
    
    return btn;
}

void UIManager::setAnswerButtonState(lv_obj_t* btn, int index, bool isPending, bool isConfirmed) {
    if (!btn) return;
    
    // Reset to base style first
    lv_obj_remove_style_all(btn);
    
    if (isConfirmed) {
        // Confirmed - show with check and glow
        lv_obj_add_style(btn, &UITheme::style_btn_answer[index], 0);
        lv_obj_set_style_shadow_width(btn, 20, 0);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_60, 0);
        // Add border
        lv_obj_set_style_border_width(btn, 3, 0);
        lv_obj_set_style_border_color(btn, lv_color_white(), 0);
    } else if (isPending) {
        // Pending - pulsing/highlighted
        lv_obj_add_style(btn, &UITheme::style_btn_answer[index], 0);
        lv_obj_set_style_opa(btn, LV_OPA_80, 0);
        lv_obj_set_style_border_width(btn, 2, 0);
        lv_obj_set_style_border_color(btn, UI_COLOR_WARNING, 0);
    } else {
        // Unselected - dimmed
        lv_obj_add_style(btn, &UITheme::style_btn_answer[index], 0);
        lv_obj_set_style_opa(btn, LV_OPA_60, 0);
    }
}

// ===================================================================================
// SCREEN IMPLEMENTATIONS
// ===================================================================================

void UIManager::showMainMenu(int selectedIndex, int itemCount, const char** items) {
    lv_obj_t* scr = createScreen();
    
    createHeader(scr, "Study Engine", false);
    
    // Scrollable list container
    lv_obj_t* list = lv_obj_create(scr);
    lv_obj_set_size(list, SCREEN_WIDTH, SCREEN_HEIGHT - 50);
    lv_obj_set_pos(list, 0, 50);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(list, 15, 0);
    lv_obj_set_style_pad_row(list, 15, 0);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    
    // Menu items
    for (int i = 0; i < itemCount; i++) {
        lv_obj_t* item = lv_obj_create(list);
        lv_obj_set_size(item, SCREEN_WIDTH - 50, 70); // Slightly taller
        
        if (i == selectedIndex) {
            lv_obj_add_style(item, &UITheme::style_list_item_selected, 0);
            lv_obj_scroll_to_view(item, LV_ANIM_OFF);
        } else {
            lv_obj_add_style(item, &UITheme::style_list_item, 0);
        }
        lv_obj_remove_flag(item, LV_OBJ_FLAG_SCROLLABLE);
        
        // Icon based on menu item
        lv_obj_t* icon = lv_label_create(item);
        if (i == 0) {
            lv_label_set_text(icon, LV_SYMBOL_EDIT);  // Scanatron
        } else if (i == 1) {
            lv_label_set_text(icon, LV_SYMBOL_CHARGE);  // Study Timer
        } else if (i == 2) {
            lv_label_set_text(icon, LV_SYMBOL_FILE);  // Flashcards
        } else {
            lv_label_set_text(icon, LV_SYMBOL_BULLET);  // Quiz
        }
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(icon, i == selectedIndex ? UI_COLOR_PRIMARY : UI_COLOR_TEXT_SECONDARY, 0);
        lv_obj_align(icon, LV_ALIGN_LEFT_MID, 10, 0);
        
        // Label
        lv_obj_t* label = lv_label_create(item);
        lv_label_set_text(label, items[i]);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 50, 0);
        
        // Arrow
        lv_obj_t* arrow = lv_label_create(item);
        lv_label_set_text(arrow, LV_SYMBOL_RIGHT);
        lv_obj_set_style_text_color(arrow, UI_COLOR_TEXT_MUTED, 0);
        lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, -10, 0);
    }
    
    // Footer hint
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "Use dial to navigate . Press A to select");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_set_style_bg_color(hint, lv_color_hex(0x000000), 0); // Add bg to make readable over list
    lv_obj_set_style_bg_opa(hint, LV_OPA_60, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    loadScreen(scr);
}

void UIManager::showLoading(const char* message) {
    lv_obj_t* scr = createScreen();
    
    // Centered spinner - LVGL 9.x uses lv_spinner_create with anim time and arc angle
    lv_obj_t* spinner = lv_spinner_create(scr);
    lv_obj_set_size(spinner, 80, 80);
    lv_obj_center(spinner);
    
    // Style the spinner arcs - in LVGL 9.x, use LV_PART_INDICATOR for spinning arc
    lv_obj_set_style_arc_width(spinner, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_width(spinner, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(spinner, UI_COLOR_BG_CARD, LV_PART_MAIN);
    lv_obj_set_style_arc_color(spinner, UI_COLOR_PRIMARY, LV_PART_INDICATOR);
    
    // Message
    lv_obj_t* label = lv_label_create(scr);
    lv_label_set_text(label, message);
    lv_obj_add_style(label, &UITheme::style_text_body, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 70);
    
    loadScreen(scr);
}

void UIManager::showExamList(const char** examNames, int count, int selectedIndex, const char* title) {
    lv_obj_t* scr = createScreen();
    
    createHeader(scr, title, true);
    
    // List container
    lv_obj_t* list = lv_obj_create(scr);
    lv_obj_set_size(list, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 70);
    lv_obj_set_pos(list, 10, 55);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 5, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list, 10, 0);
    
    for (int i = 0; i < count; i++) {
        lv_obj_t* item = lv_obj_create(list);
        lv_obj_set_size(item, SCREEN_WIDTH - 50, 65);
        
        if (i == selectedIndex) {
            lv_obj_add_style(item, &UITheme::style_list_item_selected, 0);
        } else {
            lv_obj_add_style(item, &UITheme::style_list_item, 0);
        }
        lv_obj_remove_flag(item, LV_OBJ_FLAG_SCROLLABLE);
        
        // Exam icon
        lv_obj_t* icon = lv_label_create(item);
        lv_label_set_text(icon, LV_SYMBOL_FILE);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_22, 0);
        lv_obj_set_style_text_color(icon, i == selectedIndex ? UI_COLOR_PRIMARY : UI_COLOR_SECONDARY, 0);
        lv_obj_align(icon, LV_ALIGN_LEFT_MID, 5, 0);
        
        // Exam name
        lv_obj_t* name = lv_label_create(item);
        lv_label_set_text(name, examNames[i]);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_18, 0);
        lv_label_set_long_mode(name, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(name, SCREEN_WIDTH - 150);
        lv_obj_align(name, LV_ALIGN_LEFT_MID, 45, 0);
        
        // Arrow
        lv_obj_t* arrow = lv_label_create(item);
        lv_label_set_text(arrow, LV_SYMBOL_RIGHT);
        lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, -5, 0);
    }
    
    // Footer
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "A: Select   B: Back");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -8);
    
    loadScreen(scr);
}

void UIManager::showTextInput(const char* title, const char* currentText, bool showCursor) {
    lv_obj_t* scr = createScreen();
    
    createHeader(scr, title, true);
    
    // Input card
    lv_obj_t* card = createCard(scr, 20, 80, SCREEN_WIDTH - 40, 100);
    
    // Text display with cursor
    lv_obj_t* textArea = lv_label_create(card);
    String displayText = String(currentText);
    if (showCursor) displayText += "_";
    lv_label_set_text(textArea, displayText.c_str());
    lv_obj_set_style_text_font(textArea, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(textArea, UI_COLOR_TEXT_PRIMARY, 0);
    lv_obj_center(textArea);
    
    // Instruction card
    lv_obj_t* infoCard = createCard(scr, 20, 200, SCREEN_WIDTH - 40, 80);
    lv_obj_t* infoLabel = lv_label_create(infoCard);
    lv_label_set_text(infoLabel, "Type using the keyboard\nPress ENTER or A to confirm");
    lv_obj_add_style(infoLabel, &UITheme::style_text_small, 0);
    lv_obj_center(infoLabel);
    lv_obj_set_style_text_align(infoLabel, LV_TEXT_ALIGN_CENTER, 0);
    
    // Footer
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "A/Enter: Confirm   B/ESC: Back");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -8);
    
    loadScreen(scr);
}

void UIManager::showQuestion(int qNum, int totalQ, const char* questionText, 
                              const char** options, int optionCount,
                              int pendingAnswer, int confirmedAnswer) {
    lv_obj_t* scr = createScreen();
    
    // Progress header
    lv_obj_t* header = lv_obj_create(scr);
    lv_obj_set_size(header, SCREEN_WIDTH, 45);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_add_style(header, &UITheme::style_header, 0);
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    
    // Question number
    char qNumStr[20];
    sprintf(qNumStr, "Question %d/%d", qNum, totalQ);
    progressLabel = lv_label_create(header);
    lv_label_set_text(progressLabel, qNumStr);
    lv_obj_set_style_text_font(progressLabel, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(progressLabel, UI_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(progressLabel, LV_ALIGN_LEFT_MID, 15, 0);
    
    // Progress bar
    lv_obj_t* progressBar = lv_bar_create(header);
    lv_obj_set_size(progressBar, 150, 10);
    lv_obj_align(progressBar, LV_ALIGN_RIGHT_MID, -15, 0);
    lv_bar_set_range(progressBar, 0, totalQ);
    lv_bar_set_value(progressBar, qNum, LV_ANIM_OFF);
    lv_obj_add_style(progressBar, &UITheme::style_progress_bg, LV_PART_MAIN);
    lv_obj_add_style(progressBar, &UITheme::style_progress_indicator, LV_PART_INDICATOR);
    
    // Question text card
    lv_obj_t* qCard = createCard(scr, 15, 52, SCREEN_WIDTH - 30, 70);
    questionLabel = lv_label_create(qCard);
    lv_label_set_text(questionLabel, questionText);
    lv_obj_set_style_text_font(questionLabel, &lv_font_montserrat_18, 0);
    lv_label_set_long_mode(questionLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(questionLabel, SCREEN_WIDTH - 70);
    lv_obj_align(questionLabel, LV_ALIGN_TOP_LEFT, 0, 0);
    
    // Answer buttons
    int startY = 130;
    int btnHeight = 42;
    int spacing = 6;
    
    for (int i = 0; i < optionCount && i < 4; i++) {
        answerBtns[i] = lv_button_create(scr);
        lv_obj_set_size(answerBtns[i], SCREEN_WIDTH - 30, btnHeight);
        lv_obj_set_pos(answerBtns[i], 15, startY + i * (btnHeight + spacing));
        
        // Determine state
        bool isPending = (pendingAnswer == i);
        bool isConfirmed = (confirmedAnswer == i);
        
        lv_obj_add_style(answerBtns[i], &UITheme::style_btn_answer[i], 0);
        
        if (isConfirmed) {
            lv_obj_set_style_shadow_width(answerBtns[i], 15, 0);
            lv_obj_set_style_border_width(answerBtns[i], 3, 0);
            lv_obj_set_style_border_color(answerBtns[i], lv_color_white(), 0);
        } else if (isPending) {
            lv_obj_set_style_opa(answerBtns[i], LV_OPA_90, 0);
            lv_obj_set_style_border_width(answerBtns[i], 2, 0);
            lv_obj_set_style_border_color(answerBtns[i], UI_COLOR_WARNING, 0);
        } else {
            lv_obj_set_style_opa(answerBtns[i], LV_OPA_70, 0);
        }
        
        // Letter
        const char* letters[] = {"A", "B", "C", "D"};
        lv_obj_t* letterLabel = lv_label_create(answerBtns[i]);
        lv_label_set_text(letterLabel, letters[i]);
        lv_obj_set_style_text_font(letterLabel, &lv_font_montserrat_18, 0);
        lv_obj_align(letterLabel, LV_ALIGN_LEFT_MID, 12, 0);
        
        // Option text
        lv_obj_t* optLabel = lv_label_create(answerBtns[i]);
        lv_label_set_text(optLabel, options[i]);
        lv_obj_set_style_text_font(optLabel, &lv_font_montserrat_14, 0);
        lv_label_set_long_mode(optLabel, LV_LABEL_LONG_DOT);
        lv_obj_set_width(optLabel, SCREEN_WIDTH - 100);
        lv_obj_align(optLabel, LV_ALIGN_LEFT_MID, 40, 0);
    }
    
    // Footer hint
    lv_obj_t* hint = lv_label_create(scr);
    if (pendingAnswer >= 0 && confirmedAnswer < 0) {
        char hintStr[50];
        const char* letters[] = {"A", "B", "C", "D"};
        sprintf(hintStr, "Press %s again to confirm", letters[pendingAnswer]);
        lv_label_set_text(hint, hintStr);
        lv_obj_set_style_text_color(hint, UI_COLOR_WARNING, 0);
    } else {
        lv_label_set_text(hint, "[/]: Navigate   Hold D: Menu   Enter: Submit");
        lv_obj_set_style_text_color(hint, UI_COLOR_TEXT_SECONDARY, 0);
    }
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -5);
    
    loadScreen(scr);
}

void UIManager::showPauseMenu(int selectedIndex) {
    lv_obj_t* scr = createScreen();
    
    // Semi-transparent overlay effect
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0D0F14), 0);
    
    createHeader(scr, "Paused", false);
    
    // Center card
    lv_obj_t* card = createCard(scr, 60, 80, SCREEN_WIDTH - 120, 180);
    
    const char* menuItems[] = {"View All Answers", "Exit Exam"};
    const char* icons[] = {LV_SYMBOL_LIST, LV_SYMBOL_CLOSE};
    
    for (int i = 0; i < 2; i++) {
        lv_obj_t* item = lv_obj_create(card);
        lv_obj_set_size(item, SCREEN_WIDTH - 160, 55);
        lv_obj_set_pos(item, 0, 10 + i * 65);
        
        if (i == selectedIndex) {
            lv_obj_add_style(item, &UITheme::style_list_item_selected, 0);
        } else {
            lv_obj_add_style(item, &UITheme::style_list_item, 0);
        }
        lv_obj_remove_flag(item, LV_OBJ_FLAG_SCROLLABLE);
        
        // Icon
        lv_obj_t* icon = lv_label_create(item);
        lv_label_set_text(icon, icons[i]);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(icon, i == 1 ? UI_COLOR_ERROR : UI_COLOR_PRIMARY, 0);
        lv_obj_align(icon, LV_ALIGN_LEFT_MID, 10, 0);
        
        // Label
        lv_obj_t* label = lv_label_create(item);
        lv_label_set_text(label, menuItems[i]);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 45, 0);
    }
    
    // Footer
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "C/D: Navigate   A: Select   B: Resume");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    loadScreen(scr);
}

void UIManager::showOverview(int questionCount, int* answers, uint8_t* confirmed, int selectedIndex, int scrollOffset) {
    lv_obj_t* scr = createScreen();
    
    createHeader(scr, "Answer Sheet", true);
    
    // Info bar
    int answeredCount = 0;
    for (int i = 0; i < questionCount; i++) {
        if (confirmed[i]) answeredCount++;
    }
    
    lv_obj_t* infoBar = lv_obj_create(scr);
    lv_obj_set_size(infoBar, SCREEN_WIDTH - 20, 35);
    lv_obj_set_pos(infoBar, 10, 52);
    lv_obj_set_style_bg_color(infoBar, UI_COLOR_BG_CARD, 0);
    lv_obj_set_style_radius(infoBar, 8, 0);
    lv_obj_set_style_border_width(infoBar, 0, 0);
    lv_obj_remove_flag(infoBar, LV_OBJ_FLAG_SCROLLABLE);
    
    char progressStr[30];
    sprintf(progressStr, "Completed: %d/%d", answeredCount, questionCount);
    lv_obj_t* progressLbl = lv_label_create(infoBar);
    lv_label_set_text(progressLbl, progressStr);
    lv_obj_add_style(progressLbl, &UITheme::style_text_small, 0);
    lv_obj_align(progressLbl, LV_ALIGN_LEFT_MID, 10, 0);
    
    // Progress bar
    lv_obj_t* pbar = lv_bar_create(infoBar);
    lv_obj_set_size(pbar, 120, 12);
    lv_obj_align(pbar, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_bar_set_range(pbar, 0, questionCount);
    lv_bar_set_value(pbar, answeredCount, LV_ANIM_OFF);
    lv_obj_add_style(pbar, &UITheme::style_progress_bg, LV_PART_MAIN);
    lv_obj_add_style(pbar, &UITheme::style_progress_indicator, LV_PART_INDICATOR);
    
    // Grid of answers
    int startY = 95;
    int rowHeight = 38;
    int maxVisible = 5;
    
    const char* optLabels = "ABCD";
    
    for (int row = 0; row < maxVisible && (scrollOffset + row) < questionCount; row++) {
        int qIdx = scrollOffset + row;
        int yPos = startY + row * rowHeight;
        bool isSelected = (qIdx == selectedIndex);
        
        // Row container
        lv_obj_t* rowObj = lv_obj_create(scr);
        lv_obj_set_size(rowObj, SCREEN_WIDTH - 20, rowHeight - 4);
        lv_obj_set_pos(rowObj, 10, yPos);
        lv_obj_set_style_pad_all(rowObj, 0, 0);  // Remove padding
        
        if (isSelected) {
            lv_obj_set_style_bg_color(rowObj, UI_COLOR_BG_ELEVATED, 0);
            lv_obj_set_style_border_width(rowObj, 2, 0);
            lv_obj_set_style_border_color(rowObj, UI_COLOR_PRIMARY, 0);
        } else {
            lv_obj_set_style_bg_color(rowObj, UI_COLOR_BG_CARD, 0);
            lv_obj_set_style_border_width(rowObj, 0, 0);
        }
        lv_obj_set_style_radius(rowObj, 8, 0);
        lv_obj_remove_flag(rowObj, LV_OBJ_FLAG_SCROLLABLE);
        
        // Question number
        char qNumStr[10];
        sprintf(qNumStr, "%2d.", qIdx + 1);
        lv_obj_t* qNumLbl = lv_label_create(rowObj);
        lv_label_set_text(qNumLbl, qNumStr);
        lv_obj_set_style_text_font(qNumLbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(qNumLbl, UI_COLOR_TEXT_SECONDARY, 0);
        lv_obj_align(qNumLbl, LV_ALIGN_LEFT_MID, 8, 0);
        
        // Answer bubbles - Optimized: Use labels directly
        for (int opt = 0; opt < 4; opt++) {
            bool isFilled = (answers[qIdx] == opt);
            bool isConf = confirmed[qIdx] && isFilled;
            bool isPend = !confirmed[qIdx] && isFilled;
            
            char letter[2] = {optLabels[opt], 0};
            lv_obj_t* bubble = lv_label_create(rowObj);
            lv_label_set_text(bubble, letter);
            lv_obj_set_size(bubble, 32, 26);
            lv_obj_align(bubble, LV_ALIGN_LEFT_MID, 50 + opt * 42, 0);
            
            // Style to look like bubble
            lv_obj_set_style_radius(bubble, 13, 0);
            lv_obj_set_style_border_width(bubble, 0, 0);
            lv_obj_set_style_text_align(bubble, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_set_style_pad_top(bubble, 6, 0); // Center text vertically
            
            if (isConf) {
                lv_obj_set_style_bg_color(bubble, UI_COLOR_CONFIRMED, 0);
                lv_obj_set_style_bg_opa(bubble, LV_OPA_COVER, 0);
            } else if (isPend) {
                lv_obj_set_style_bg_color(bubble, UI_COLOR_PENDING, 0);
                lv_obj_set_style_bg_opa(bubble, LV_OPA_COVER, 0);
            } else {
                lv_obj_set_style_bg_color(bubble, UI_COLOR_EMPTY, 0);
                lv_obj_set_style_bg_opa(bubble, LV_OPA_COVER, 0);
            }
            
            lv_obj_set_style_text_font(bubble, &lv_font_montserrat_12, 0);
            lv_obj_set_style_text_color(bubble, isFilled ? lv_color_white() : UI_COLOR_TEXT_MUTED, 0);
        }
        
        // Status
        lv_obj_t* statusLbl = lv_label_create(rowObj);
        if (confirmed[qIdx]) {
            lv_label_set_text(statusLbl, LV_SYMBOL_OK);
            lv_obj_set_style_text_color(statusLbl, UI_COLOR_SUCCESS, 0);
        } else if (answers[qIdx] >= 0) {
            lv_label_set_text(statusLbl, LV_SYMBOL_REFRESH);
            lv_obj_set_style_text_color(statusLbl, UI_COLOR_WARNING, 0);
        } else {
            lv_label_set_text(statusLbl, "-");
            lv_obj_set_style_text_color(statusLbl, UI_COLOR_TEXT_MUTED, 0);
        }
        lv_obj_align(statusLbl, LV_ALIGN_RIGHT_MID, -10, 0);
    }
    
    // Legend
    lv_obj_t* legend = lv_label_create(scr);
    lv_label_set_text(legend, "Blue: Confirmed   Yellow: Pending   Gray: Empty");
    lv_obj_set_style_text_font(legend, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(legend, UI_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(legend, LV_ALIGN_BOTTOM_MID, 0, -25);
    
    // Footer
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "Dial/[]: Navigate   A: Go to Question   B: Back");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -5);
    
    Serial.println("[UI] Overview screen built");
    loadScreen(scr);
    Serial.println("[UI] Overview screen loaded");
}

void UIManager::showResult(int score, int total, float percentage) {
    lv_obj_t* scr = createScreen();
    
    createHeader(scr, "Exam Results", false);
    
    // Score card
    lv_obj_t* card = createCard(scr, 30, 60, SCREEN_WIDTH - 60, 200);
    
    // Big score
    char scoreStr[20];
    sprintf(scoreStr, "%d/%d", score, total);
    lv_obj_t* scoreLbl = lv_label_create(card);
    lv_label_set_text(scoreLbl, scoreStr);
    lv_obj_set_style_text_font(scoreLbl, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(scoreLbl, percentage >= 70 ? UI_COLOR_SUCCESS : 
                                          percentage >= 50 ? UI_COLOR_WARNING : UI_COLOR_ERROR, 0);
    lv_obj_align(scoreLbl, LV_ALIGN_TOP_MID, 0, 10);
    
    // Percentage arc
    lv_obj_t* arc = lv_arc_create(card);
    lv_obj_set_size(arc, 100, 100);
    lv_obj_align(arc, LV_ALIGN_CENTER, 0, 10);
    lv_arc_set_rotation(arc, 135);
    lv_arc_set_bg_angles(arc, 0, 270);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, (int)percentage);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_color(arc, UI_COLOR_BG_ELEVATED, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, percentage >= 70 ? UI_COLOR_SUCCESS : 
                                     percentage >= 50 ? UI_COLOR_WARNING : UI_COLOR_ERROR, LV_PART_INDICATOR);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    
    // Percentage text in center of arc
    char pctStr[10];
    sprintf(pctStr, "%.1f%%", percentage);
    lv_obj_t* pctLbl = lv_label_create(arc);
    lv_label_set_text(pctLbl, pctStr);
    lv_obj_set_style_text_font(pctLbl, &lv_font_montserrat_20, 0);
    lv_obj_center(pctLbl);
    
    // Grade
    const char* grade;
    if (percentage >= 90) grade = "Grade: A";
    else if (percentage >= 80) grade = "Grade: B";
    else if (percentage >= 70) grade = "Grade: C";
    else if (percentage >= 60) grade = "Grade: D";
    else grade = "Grade: F";
    
    lv_obj_t* gradeLbl = lv_label_create(card);
    lv_label_set_text(gradeLbl, grade);
    lv_obj_set_style_text_font(gradeLbl, &lv_font_montserrat_22, 0);
    lv_obj_align(gradeLbl, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    // Continue button hint
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "A: Review   B: Exit");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -15);
    
    loadScreen(scr);
}

void UIManager::showExamComplete() {
    lv_obj_t* scr = createScreen();
    
    createHeader(scr, "Exam Complete", false);
    
    // Success card
    lv_obj_t* card = createCard(scr, 40, 80, SCREEN_WIDTH - 80, 160);
    
    // Checkmark icon
    lv_obj_t* icon = lv_label_create(card);
    lv_label_set_text(icon, LV_SYMBOL_OK);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(icon, UI_COLOR_SUCCESS, 0);
    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 15);
    
    // Message
    lv_obj_t* msgLbl = lv_label_create(card);
    lv_label_set_text(msgLbl, "Exam Submitted\nSuccessfully!");
    lv_obj_set_style_text_font(msgLbl, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_align(msgLbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(msgLbl, LV_ALIGN_CENTER, 0, 15);
    
    // Hint
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "Press A to continue");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -20);
    
    loadScreen(scr);
}

void UIManager::showNoExams() {
    lv_obj_t* scr = createScreen();
    
    createHeader(scr, "Scanatron", true);
    
    // Error card
    lv_obj_t* card = createCard(scr, 40, 100, SCREEN_WIDTH - 80, 140);
    
    // Warning icon
    lv_obj_t* icon = lv_label_create(card);
    lv_label_set_text(icon, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(icon, UI_COLOR_WARNING, 0);
    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 10);
    
    // Message
    lv_obj_t* msgLbl = lv_label_create(card);
    lv_label_set_text(msgLbl, "No Exams Available");
    lv_obj_set_style_text_font(msgLbl, &lv_font_montserrat_22, 0);
    lv_obj_align(msgLbl, LV_ALIGN_CENTER, 0, 10);
    
    // Hint
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "Press B to go back");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -20);
    
    loadScreen(scr);
}

void UIManager::showError(const char* message) {
    lv_obj_t* scr = createScreen();
    
    // Error card centered
    lv_obj_t* card = createCard(scr, 40, 100, SCREEN_WIDTH - 80, 120);
    
    // Error icon
    lv_obj_t* icon = lv_label_create(card);
    lv_label_set_text(icon, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(icon, UI_COLOR_ERROR, 0);
    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 10);
    
    // Message
    lv_obj_t* msgLbl = lv_label_create(card);
    lv_label_set_text(msgLbl, message);
    lv_obj_set_style_text_font(msgLbl, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_align(msgLbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(msgLbl, LV_ALIGN_CENTER, 0, 15);
    
    loadScreen(scr);
}

void UIManager::showStudyTimer(unsigned long elapsedSeconds, bool isPaused, bool phoneDetected, bool userAway) {
    lv_obj_t* scr = createScreen();
    
    createHeader(scr, "Study Session", false);
    
    // Status card
    lv_obj_t* card = createCard(scr, 30, 60, SCREEN_WIDTH - 60, 200);
    
    // Calculate time
    int hours = elapsedSeconds / 3600;
    int minutes = (elapsedSeconds % 3600) / 60;
    int seconds = elapsedSeconds % 60;
    
    // Time display
    char timeStr[20];
    if (hours > 0) {
        sprintf(timeStr, "%d:%02d:%02d", hours, minutes, seconds);
    } else {
        sprintf(timeStr, "%02d:%02d", minutes, seconds);
    }
    
    lv_obj_t* timeLbl = lv_label_create(card);
    lv_label_set_text(timeLbl, timeStr);
    lv_obj_set_style_text_font(timeLbl, &lv_font_montserrat_32, 0);
    
    if (phoneDetected) {
        lv_obj_set_style_text_color(timeLbl, UI_COLOR_ERROR, 0);
    } else if (userAway || isPaused) {
        lv_obj_set_style_text_color(timeLbl, UI_COLOR_WARNING, 0);
    } else {
        lv_obj_set_style_text_color(timeLbl, UI_COLOR_SUCCESS, 0);
    }
    lv_obj_align(timeLbl, LV_ALIGN_TOP_MID, 0, 20);
    
    // Status message
    lv_obj_t* statusLbl = lv_label_create(card);
    const char* statusMsg;
    lv_color_t statusColor;
    
    if (phoneDetected) {
        statusMsg = "PHONE DETECTED!\nPut it away!";
        statusColor = UI_COLOR_ERROR;
    } else if (userAway) {
        statusMsg = "USER AWAY\nTimer Paused";
        statusColor = UI_COLOR_WARNING;
    } else if (isPaused) {
        statusMsg = "PAUSED\nPress A to Resume";
        statusColor = UI_COLOR_WARNING;
    } else {
        statusMsg = "STUDYING\nStay focused!";
        statusColor = UI_COLOR_SUCCESS;
    }
    
    lv_label_set_text(statusLbl, statusMsg);
    lv_obj_set_style_text_font(statusLbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(statusLbl, statusColor, 0);
    lv_obj_set_style_text_align(statusLbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(statusLbl, LV_ALIGN_CENTER, 0, 20);
    
    // Footer hint
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "A: Pause/Resume   B: Stop");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -15);
    
    loadScreen(scr);
}

void UIManager::showStudyStart() {
    lv_obj_t* scr = createScreen();
    
    createHeader(scr, "Study Timer", false);
    
    // Card
    lv_obj_t* card = createCard(scr, 40, 80, SCREEN_WIDTH - 80, 160);
    
    // Icon
    lv_obj_t* icon = lv_label_create(card);
    lv_label_set_text(icon, LV_SYMBOL_CHARGE);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(icon, UI_COLOR_PRIMARY, 0);
    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 15);
    
    // Message
    lv_obj_t* msgLbl = lv_label_create(card);
    lv_label_set_text(msgLbl, "Ready to Study?");
    lv_obj_set_style_text_font(msgLbl, &lv_font_montserrat_22, 0);
    lv_obj_align(msgLbl, LV_ALIGN_CENTER, 0, 10);
    
    // Hint
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "Press A to Start   B: Back to Menu");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -20);
    
    loadScreen(scr);
}

void UIManager::updateAnswerState(int optionIndex, int pendingAnswer, int confirmedAnswer) {
    if (optionIndex < 0 || optionIndex >= 4 || !answerBtns[optionIndex]) return;
    
    bool isPending = (pendingAnswer == optionIndex);
    bool isConfirmed = (confirmedAnswer == optionIndex);
    
    setAnswerButtonState(answerBtns[optionIndex], optionIndex, isPending, isConfirmed);
}

// ===================================================================================
// FLASHCARD SCREENS
// ===================================================================================

void UIManager::showFlashcardFront(const char* text, int current, int total) {
    lv_obj_t* scr = createScreen();
    
    // Header with progress
    lv_obj_t* header = lv_obj_create(scr);
    lv_obj_set_size(header, SCREEN_WIDTH, 45);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_add_style(header, &UITheme::style_header, 0);
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    
    char progStr[20];
    sprintf(progStr, "Card %d/%d", current, total);
    lv_obj_t* progLbl = lv_label_create(header);
    lv_label_set_text(progLbl, progStr);
    lv_obj_set_style_text_font(progLbl, &lv_font_montserrat_18, 0);
    lv_obj_align(progLbl, LV_ALIGN_LEFT_MID, 15, 0);
    
    // Progress bar
    lv_obj_t* bar = lv_bar_create(header);
    lv_obj_set_size(bar, 150, 10);
    lv_obj_align(bar, LV_ALIGN_RIGHT_MID, -15, 0);
    lv_bar_set_range(bar, 0, total);
    lv_bar_set_value(bar, current, LV_ANIM_OFF);
    lv_obj_add_style(bar, &UITheme::style_progress_bg, LV_PART_MAIN);
    lv_obj_add_style(bar, &UITheme::style_progress_indicator, LV_PART_INDICATOR);
    
    // Card Content
    lv_obj_t* card = createCard(scr, 20, 60, SCREEN_WIDTH - 40, 220);
    
    lv_obj_t* label = lv_label_create(card);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, SCREEN_WIDTH - 80);
    lv_obj_center(label);
    
    // Hint
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "Press A to Flip");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    loadScreen(scr);
}

void UIManager::showFlashcardBack(const char* front, const char* back) {
    lv_obj_t* scr = createScreen();
    
    createHeader(scr, "Answer", false);
    
    // Front text (small, top)
    lv_obj_t* frontCard = createCard(scr, 20, 55, SCREEN_WIDTH - 40, 60);
    lv_obj_set_style_bg_color(frontCard, UI_COLOR_BG_CARD, 0);
    
    lv_obj_t* frontLbl = lv_label_create(frontCard);
    lv_label_set_text(frontLbl, front);
    lv_obj_set_style_text_font(frontLbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(frontLbl, UI_COLOR_TEXT_SECONDARY, 0);
    lv_label_set_long_mode(frontLbl, LV_LABEL_LONG_DOT);
    lv_obj_set_width(frontLbl, SCREEN_WIDTH - 60);
    lv_obj_center(frontLbl);
    
    // Back text (large, middle)
    lv_obj_t* backCard = createCard(scr, 20, 125, SCREEN_WIDTH - 40, 140);
    lv_obj_set_style_border_color(backCard, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_border_width(backCard, 2, 0);
    
    lv_obj_t* backLbl = lv_label_create(backCard);
    lv_label_set_text(backLbl, back);
    lv_obj_set_style_text_font(backLbl, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_align(backLbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(backLbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(backLbl, SCREEN_WIDTH - 80);
    lv_obj_center(backLbl);
    
    // Rating Buttons (Footer)
    lv_obj_t* footer = lv_obj_create(scr);
    lv_obj_set_size(footer, SCREEN_WIDTH, 45);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(footer, lv_color_hex(0x1A1F25), 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_remove_flag(footer, LV_OBJ_FLAG_SCROLLABLE);
    
    const char* labels[] = {"A:Again", "B:Hard", "C:Good", "D:Easy"};
    lv_color_t colors[] = {UI_COLOR_ERROR, UI_COLOR_WARNING, UI_COLOR_PRIMARY, UI_COLOR_SUCCESS};
    
    int btnW = (SCREEN_WIDTH - 20) / 4;
    for (int i = 0; i < 4; i++) {
        lv_obj_t* btn = lv_label_create(footer);
        lv_label_set_text(btn, labels[i]);
        lv_obj_set_style_text_font(btn, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(btn, colors[i], 0);
        lv_obj_align(btn, LV_ALIGN_LEFT_MID, 10 + i * btnW, 0);
    }
    
    loadScreen(scr);
}

void UIManager::showFlashcardFinished(int total, int easy, int hard, int again) {
    lv_obj_t* scr = createScreen();
    
    createHeader(scr, "Session Complete", false);
    
    lv_obj_t* card = createCard(scr, 30, 60, SCREEN_WIDTH - 60, 200);
    
    lv_obj_t* title = lv_label_create(card);
    lv_label_set_text(title, "Great Job!");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_SUCCESS, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);
    
    // Stats
    char statsStr[100];
    sprintf(statsStr, "Total Cards: %d\n\nEasy: %d\nHard: %d\nAgain: %d", total, easy, hard, again);
    
    lv_obj_t* stats = lv_label_create(card);
    lv_label_set_text(stats, statsStr);
    lv_obj_set_style_text_font(stats, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_align(stats, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(stats, LV_ALIGN_CENTER, 0, 20);
    
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "Press A to Continue");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -15);
    
    loadScreen(scr);
}

void UIManager::showFlashcardPauseMenu(int selectedIndex) {
    lv_obj_t* scr = createScreen();
    
    // Semi-transparent overlay effect
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0D0F14), 0);
    
    createHeader(scr, "Paused", false);
    
    // Center card
    lv_obj_t* card = createCard(scr, 60, 80, SCREEN_WIDTH - 120, 140);
    
    const char* menuItems[] = {"Resume", "Exit Session"};
    const char* icons[] = {LV_SYMBOL_PLAY, LV_SYMBOL_CLOSE};
    
    for (int i = 0; i < 2; i++) {
        lv_obj_t* item = lv_obj_create(card);
        lv_obj_set_size(item, SCREEN_WIDTH - 160, 55);
        lv_obj_set_pos(item, 0, 10 + i * 65);
        
        if (i == selectedIndex) {
            lv_obj_add_style(item, &UITheme::style_list_item_selected, 0);
        } else {
            lv_obj_add_style(item, &UITheme::style_list_item, 0);
        }
        lv_obj_remove_flag(item, LV_OBJ_FLAG_SCROLLABLE);
        
        // Icon
        lv_obj_t* icon = lv_label_create(item);
        lv_label_set_text(icon, icons[i]);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(icon, i == 1 ? UI_COLOR_ERROR : UI_COLOR_SUCCESS, 0);
        lv_obj_align(icon, LV_ALIGN_LEFT_MID, 10, 0);
        
        // Label
        lv_obj_t* label = lv_label_create(item);
        lv_label_set_text(label, menuItems[i]);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 45, 0);
    }
    
    // Footer
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "C/D: Navigate   A: Select");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    loadScreen(scr);
}

void UIManager::showQuizQuestionText(int qNum, int total, const char* question, const char* currentInput, bool showCursor) {
    lv_obj_t* scr = createScreen();
    
    // Header
    lv_obj_t* header = lv_obj_create(scr);
    lv_obj_set_size(header, SCREEN_WIDTH, 45);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_add_style(header, &UITheme::style_header, 0);
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    
    char progStr[20];
    sprintf(progStr, "Question %d/%d", qNum, total);
    lv_obj_t* progLbl = lv_label_create(header);
    lv_label_set_text(progLbl, progStr);
    lv_obj_set_style_text_font(progLbl, &lv_font_montserrat_18, 0);
    lv_obj_align(progLbl, LV_ALIGN_LEFT_MID, 15, 0);
    
    // Question Text
    lv_obj_t* qCard = createCard(scr, 20, 60, SCREEN_WIDTH - 40, 80);
    lv_obj_t* qLbl = lv_label_create(qCard);
    lv_label_set_text(qLbl, question);
    lv_obj_set_style_text_font(qLbl, &lv_font_montserrat_18, 0);
    lv_label_set_long_mode(qLbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(qLbl, SCREEN_WIDTH - 80);
    lv_obj_align(qLbl, LV_ALIGN_TOP_LEFT, 0, 0);
    
    // Input Area
    lv_obj_t* inputCard = createCard(scr, 20, 150, SCREEN_WIDTH - 40, 60);
    lv_obj_set_style_bg_color(inputCard, lv_color_hex(0x2A303C), 0);
    
    lv_obj_t* inputLbl = lv_label_create(inputCard);
    String dispText = String(currentInput);
    if (showCursor) dispText += "_";
    lv_label_set_text(inputLbl, dispText.c_str());
    lv_obj_set_style_text_font(inputLbl, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(inputLbl, UI_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(inputLbl, LV_ALIGN_LEFT_MID, 10, 0);
    
    // Hint
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "Type answer & press ENTER");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    loadScreen(scr);
}

void UIManager::showQuizReview(int qNum, int total, const char* question, const char* userAnswer, const char* correctAnswer, bool isCorrect) {
    lv_obj_t* scr = createScreen();
    
    createHeader(scr, "Review Quiz", false);
    
    // Question Number
    char progStr[20];
    sprintf(progStr, "Question %d/%d", qNum, total);
    lv_obj_t* progLbl = lv_label_create(scr);
    lv_label_set_text(progLbl, progStr);
    lv_obj_set_style_text_font(progLbl, &lv_font_montserrat_18, 0);
    lv_obj_align(progLbl, LV_ALIGN_TOP_LEFT, 15, 55);
    
    // Result Icon
    lv_obj_t* icon = lv_label_create(scr);
    lv_label_set_text(icon, isCorrect ? LV_SYMBOL_OK : LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(icon, isCorrect ? UI_COLOR_SUCCESS : UI_COLOR_ERROR, 0);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, 0);
    lv_obj_align(icon, LV_ALIGN_TOP_RIGHT, -15, 55);
    
    // Question Text
    lv_obj_t* qCard = createCard(scr, 15, 85, SCREEN_WIDTH - 30, 70);
    lv_obj_t* qLbl = lv_label_create(qCard);
    lv_label_set_text(qLbl, question);
    lv_obj_set_style_text_font(qLbl, &lv_font_montserrat_16, 0);
    lv_label_set_long_mode(qLbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(qLbl, SCREEN_WIDTH - 60);
    lv_obj_align(qLbl, LV_ALIGN_TOP_LEFT, 0, 0);
    
    // User Answer
    lv_obj_t* userCard = createCard(scr, 15, 165, SCREEN_WIDTH - 30, 50);
    lv_obj_set_style_bg_color(userCard, isCorrect ? lv_color_hex(0x1E2820) : lv_color_hex(0x2D1E1E), 0);
    lv_obj_set_style_border_color(userCard, isCorrect ? UI_COLOR_SUCCESS : UI_COLOR_ERROR, 0);
    lv_obj_set_style_border_width(userCard, 1, 0);
    
    lv_obj_t* userLbl = lv_label_create(userCard);
    String userStr = "You: " + String(userAnswer);
    lv_label_set_text(userLbl, userStr.c_str());
    lv_obj_set_style_text_font(userLbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(userLbl, isCorrect ? UI_COLOR_SUCCESS : UI_COLOR_ERROR, 0);
    lv_obj_align(userLbl, LV_ALIGN_LEFT_MID, 10, 0);
    
    // Correct Answer (if wrong)
    if (!isCorrect) {
        lv_obj_t* corrCard = createCard(scr, 15, 225, SCREEN_WIDTH - 30, 50);
        lv_obj_set_style_bg_color(corrCard, lv_color_hex(0x1E2820), 0);
        lv_obj_set_style_border_color(corrCard, UI_COLOR_SUCCESS, 0);
        lv_obj_set_style_border_width(corrCard, 1, 0);
        
        lv_obj_t* corrLbl = lv_label_create(corrCard);
        String corrStr = "Correct: " + String(correctAnswer);
        lv_label_set_text(corrLbl, corrStr.c_str());
        lv_obj_set_style_text_font(corrLbl, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(corrLbl, UI_COLOR_SUCCESS, 0);
        lv_obj_align(corrLbl, LV_ALIGN_LEFT_MID, 10, 0);
    }
    
    // Footer
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "C/D: Prev/Next   B: Exit Review");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    loadScreen(scr);
}

// ===================================================================================
// ENHANCED STUDY TIMER SCREENS
// ===================================================================================

void UIManager::showTimerSetup(int timerMode, int selectedIndex,
                                int basicDuration, bool countUp,
                                int pomoWork, int pomoShortBreak, int pomoLongBreak, int pomoSessions,
                                bool editing, int editValue) {
    lv_obj_t* scr = createScreen();
    
    createHeader(scr, "Study Timer", true);
    
    // Timer mode indicator
    lv_obj_t* modeCard = createCard(scr, 15, 55, SCREEN_WIDTH - 30, 45);
    lv_obj_t* modeLbl = lv_label_create(modeCard);
    lv_label_set_text(modeLbl, timerMode == UI_TIMER_BASIC ? "Basic Timer" : "Pomodoro Timer");
    lv_obj_set_style_text_font(modeLbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(modeLbl, UI_COLOR_PRIMARY, 0);
    lv_obj_center(modeLbl);
    
    // Settings list
    lv_obj_t* list = lv_obj_create(scr);
    lv_obj_set_size(list, SCREEN_WIDTH - 20, 175);
    lv_obj_set_pos(list, 10, 105);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 5, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list, 8, 0);
    
    // Build menu items based on mode
    const char* labels[6];
    char values[6][20];
    int itemCount;
    
    if (timerMode == UI_TIMER_BASIC) {
        labels[0] = "Mode";
        labels[1] = "Duration";
        labels[2] = "Direction";
        labels[3] = "Start Timer";
        itemCount = 4;
        
        strcpy(values[0], "Basic");
        sprintf(values[1], "%d min", editing && selectedIndex == 1 ? editValue : basicDuration);
        strcpy(values[2], countUp ? "Count Up" : "Countdown");
        strcpy(values[3], LV_SYMBOL_PLAY);
    } else {
        labels[0] = "Mode";
        labels[1] = "Work Time";
        labels[2] = "Short Break";
        labels[3] = "Long Break";
        labels[4] = "Sessions";
        labels[5] = "Start Timer";
        itemCount = 6;
        
        strcpy(values[0], "Pomodoro");
        sprintf(values[1], "%d min", editing && selectedIndex == 1 ? editValue : pomoWork);
        sprintf(values[2], "%d min", editing && selectedIndex == 2 ? editValue : pomoShortBreak);
        sprintf(values[3], "%d min", editing && selectedIndex == 3 ? editValue : pomoLongBreak);
        sprintf(values[4], "%d", editing && selectedIndex == 4 ? editValue : pomoSessions);
        strcpy(values[5], LV_SYMBOL_PLAY);
    }
    
    for (int i = 0; i < itemCount; i++) {
        lv_obj_t* item = lv_obj_create(list);
        lv_obj_set_size(item, SCREEN_WIDTH - 50, 38);
        
        bool isSelected = (i == selectedIndex);
        bool isEditing = editing && isSelected;
        
        if (isEditing) {
            lv_obj_set_style_bg_color(item, UI_COLOR_WARNING, 0);
            lv_obj_set_style_border_width(item, 2, 0);
            lv_obj_set_style_border_color(item, lv_color_white(), 0);
        } else if (isSelected) {
            lv_obj_add_style(item, &UITheme::style_list_item_selected, 0);
        } else {
            lv_obj_add_style(item, &UITheme::style_list_item, 0);
        }
        lv_obj_remove_flag(item, LV_OBJ_FLAG_SCROLLABLE);
        
        // Label
        lv_obj_t* label = lv_label_create(item);
        lv_label_set_text(label, labels[i]);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 10, 0);
        
        // Value
        lv_obj_t* value = lv_label_create(item);
        lv_label_set_text(value, values[i]);
        lv_obj_set_style_text_font(value, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(value, isSelected ? lv_color_white() : UI_COLOR_TEXT_SECONDARY, 0);
        lv_obj_align(value, LV_ALIGN_RIGHT_MID, -10, 0);
        
        // Scroll selected item into view
        if (isSelected) {
            lv_obj_scroll_to_view(item, LV_ANIM_OFF);
        }
    }
    
    // Footer hint
    lv_obj_t* hint = lv_label_create(scr);
    if (editing) {
        lv_label_set_text(hint, "C/D: Adjust   A: Confirm   B: Cancel");
    } else {
        lv_label_set_text(hint, "Dial: Select   A: Edit/Toggle   B: Back");
    }
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -8);
    
    loadScreen(scr);
}

void UIManager::showBasicTimer(unsigned long elapsedSecs, unsigned long remainingSecs, bool isPaused, bool isBreak) {
    lv_obj_t* scr = createScreen();
    
    createHeader(scr, "Study Timer", false);
    
    // Main timer card
    lv_obj_t* card = createCard(scr, 30, 60, SCREEN_WIDTH - 60, 200);
    
    // Calculate display time
    unsigned long displaySecs = remainingSecs > 0 ? remainingSecs : elapsedSecs;
    int hours = displaySecs / 3600;
    int minutes = (displaySecs % 3600) / 60;
    int seconds = displaySecs % 60;
    
    // Large time display
    char timeStr[20];
    if (hours > 0) {
        sprintf(timeStr, "%d:%02d:%02d", hours, minutes, seconds);
    } else {
        sprintf(timeStr, "%02d:%02d", minutes, seconds);
    }
    
    lv_obj_t* timeLbl = lv_label_create(card);
    lv_label_set_text(timeLbl, timeStr);
    lv_obj_set_style_text_font(timeLbl, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(timeLbl, isPaused ? UI_COLOR_WARNING : UI_COLOR_SUCCESS, 0);
    lv_obj_align(timeLbl, LV_ALIGN_TOP_MID, 0, 25);
    
    // Progress arc (for countdown mode)
    if (remainingSecs > 0) {
        unsigned long totalSecs = elapsedSecs + remainingSecs;
        int progress = (elapsedSecs * 100) / totalSecs;
        
        lv_obj_t* arc = lv_arc_create(card);
        lv_obj_set_size(arc, 100, 100);
        lv_obj_align(arc, LV_ALIGN_CENTER, 0, 20);
        lv_arc_set_rotation(arc, 135);
        lv_arc_set_bg_angles(arc, 0, 270);
        lv_arc_set_range(arc, 0, 100);
        lv_arc_set_value(arc, progress);
        lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
        lv_obj_set_style_arc_color(arc, UI_COLOR_BG_ELEVATED, LV_PART_MAIN);
        lv_obj_set_style_arc_color(arc, isPaused ? UI_COLOR_WARNING : UI_COLOR_PRIMARY, LV_PART_INDICATOR);
        lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    }
    
    // Status text
    lv_obj_t* statusLbl = lv_label_create(card);
    const char* statusText = isPaused ? "PAUSED" : (remainingSecs > 0 ? "FOCUS TIME" : "STUDYING");
    lv_label_set_text(statusLbl, statusText);
    lv_obj_set_style_text_font(statusLbl, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(statusLbl, isPaused ? UI_COLOR_WARNING : UI_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(statusLbl, LV_ALIGN_BOTTOM_MID, 0, -15);
    
    // Footer hint
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, isPaused ? "A: Resume   B: Stop" : "A: Pause   B: Stop");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    loadScreen(scr);
}

void UIManager::showPomodoroTimer(unsigned long remainingSecs, int phase, int currentSession, int totalSessions, bool isPaused, bool isBreak) {
    lv_obj_t* scr = createScreen();
    
    // Phase-based header color
    const char* phaseTitle;
    lv_color_t phaseColor;
    
    switch (phase) {
        case UI_POMO_WORK:
            phaseTitle = "Focus Time";
            phaseColor = UI_COLOR_PRIMARY;
            break;
        case UI_POMO_SHORT_BREAK:
            phaseTitle = "Short Break";
            phaseColor = UI_COLOR_SUCCESS;
            break;
        case UI_POMO_LONG_BREAK:
            phaseTitle = "Long Break";
            phaseColor = UI_COLOR_SECONDARY;
            break;
        default:
            phaseTitle = "Pomodoro";
            phaseColor = UI_COLOR_PRIMARY;
    }
    
    // Custom header with phase color
    lv_obj_t* header = lv_obj_create(scr);
    lv_obj_set_size(header, SCREEN_WIDTH, 50);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, phaseColor, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t* titleLbl = lv_label_create(header);
    lv_label_set_text(titleLbl, phaseTitle);
    lv_obj_set_style_text_font(titleLbl, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(titleLbl, lv_color_white(), 0);
    lv_obj_align(titleLbl, LV_ALIGN_LEFT_MID, 15, 0);
    
    // Session counter in header
    char sessStr[20];
    sprintf(sessStr, "%d/%d", currentSession, totalSessions);
    lv_obj_t* sessLbl = lv_label_create(header);
    lv_label_set_text(sessLbl, sessStr);
    lv_obj_set_style_text_font(sessLbl, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(sessLbl, lv_color_white(), 0);
    lv_obj_align(sessLbl, LV_ALIGN_RIGHT_MID, -15, 0);
    
    // Main timer card
    lv_obj_t* card = createCard(scr, 30, 60, SCREEN_WIDTH - 60, 200);
    
    // Calculate time
    int minutes = remainingSecs / 60;
    int seconds = remainingSecs % 60;
    
    char timeStr[10];
    sprintf(timeStr, "%02d:%02d", minutes, seconds);
    
    lv_obj_t* timeLbl = lv_label_create(card);
    lv_label_set_text(timeLbl, timeStr);
    lv_obj_set_style_text_font(timeLbl, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(timeLbl, isPaused ? UI_COLOR_WARNING : phaseColor, 0);
    lv_obj_align(timeLbl, LV_ALIGN_TOP_MID, 0, 30);
    
    // Tomato icons for sessions completed
    lv_obj_t* tomatoRow = lv_obj_create(card);
    lv_obj_set_size(tomatoRow, SCREEN_WIDTH - 100, 40);
    lv_obj_align(tomatoRow, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_bg_opa(tomatoRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(tomatoRow, 0, 0);
    lv_obj_set_flex_flow(tomatoRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tomatoRow, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(tomatoRow, LV_OBJ_FLAG_SCROLLABLE);
    
    for (int i = 0; i < totalSessions && i < 8; i++) {
        lv_obj_t* tomato = lv_label_create(tomatoRow);
        lv_label_set_text(tomato, LV_SYMBOL_CHARGE); // Using charge as tomato substitute
        lv_obj_set_style_text_font(tomato, &lv_font_montserrat_20, 0);
        if (i < currentSession - 1 || (i == currentSession - 1 && phase != UI_POMO_WORK)) {
            lv_obj_set_style_text_color(tomato, UI_COLOR_SUCCESS, 0); // Completed
        } else if (i == currentSession - 1) {
            lv_obj_set_style_text_color(tomato, phaseColor, 0); // Current
        } else {
            lv_obj_set_style_text_color(tomato, UI_COLOR_TEXT_MUTED, 0); // Not started
        }
    }
    
    // Status text
    lv_obj_t* statusLbl = lv_label_create(card);
    const char* statusText = isPaused ? "PAUSED" : (isBreak ? "Take a break!" : "Stay focused!");
    lv_label_set_text(statusLbl, statusText);
    lv_obj_set_style_text_font(statusLbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(statusLbl, isPaused ? UI_COLOR_WARNING : UI_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(statusLbl, LV_ALIGN_BOTTOM_MID, 0, -15);
    
    // Footer hint
    lv_obj_t* hint = lv_label_create(scr);
    if (isBreak) {
        lv_label_set_text(hint, "A: Skip Break   B: Stop");
    } else {
        lv_label_set_text(hint, isPaused ? "A: Resume   B: Stop" : "A: Pause   B: Stop");
    }
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    loadScreen(scr);
}

void UIManager::showTimerComplete(int sessionsCompleted, unsigned long totalSeconds) {
    lv_obj_t* scr = createScreen();
    
    createHeader(scr, "Session Complete!", false);
    
    lv_obj_t* card = createCard(scr, 40, 70, SCREEN_WIDTH - 80, 180);
    
    // Trophy icon
    lv_obj_t* icon = lv_label_create(card);
    lv_label_set_text(icon, LV_SYMBOL_OK);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(icon, UI_COLOR_SUCCESS, 0);
    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 15);
    
    // Stats
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    
    char statsStr[100];
    if (hours > 0) {
        sprintf(statsStr, "Sessions: %d\nTotal Time: %dh %dm", sessionsCompleted, hours, minutes);
    } else {
        sprintf(statsStr, "Sessions: %d\nTotal Time: %d minutes", sessionsCompleted, minutes);
    }
    
    lv_obj_t* statsLbl = lv_label_create(card);
    lv_label_set_text(statsLbl, statsStr);
    lv_obj_set_style_text_font(statsLbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_align(statsLbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(statsLbl, LV_ALIGN_CENTER, 0, 20);
    
    // Great job message
    lv_obj_t* msgLbl = lv_label_create(card);
    lv_label_set_text(msgLbl, "Great work!");
    lv_obj_set_style_text_font(msgLbl, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(msgLbl, UI_COLOR_PRIMARY, 0);
    lv_obj_align(msgLbl, LV_ALIGN_BOTTOM_MID, 0, -15);
    
    // Footer
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "Press any button to continue");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -15);
    
    loadScreen(scr);
}

// ===================================================================================
// FOCUS MODE WARNING
// ===================================================================================

void UIManager::showFocusWarning(const char* message, bool phoneIssue, bool presenceIssue) {
    lv_obj_t* scr = createScreen();
    
    // Dark overlay background
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1A0A0A), 0);
    
    // Warning header
    lv_obj_t* header = lv_obj_create(scr);
    lv_obj_set_size(header, SCREEN_WIDTH, 55);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, UI_COLOR_ERROR, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t* warnIcon = lv_label_create(header);
    lv_label_set_text(warnIcon, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_font(warnIcon, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(warnIcon, lv_color_white(), 0);
    lv_obj_align(warnIcon, LV_ALIGN_LEFT_MID, 15, 0);
    
    lv_obj_t* titleLbl = lv_label_create(header);
    lv_label_set_text(titleLbl, "Focus Alert!");
    lv_obj_set_style_text_font(titleLbl, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(titleLbl, lv_color_white(), 0);
    lv_obj_align(titleLbl, LV_ALIGN_LEFT_MID, 55, 0);
    
    // Warning card
    lv_obj_t* card = createCard(scr, 30, 75, SCREEN_WIDTH - 60, 180);
    lv_obj_set_style_border_color(card, UI_COLOR_ERROR, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    
    // Issue icons
    int iconY = 20;
    
    if (phoneIssue) {
        lv_obj_t* phoneRow = lv_obj_create(card);
        lv_obj_set_size(phoneRow, SCREEN_WIDTH - 100, 45);
        lv_obj_set_pos(phoneRow, 10, iconY);
        lv_obj_set_style_bg_color(phoneRow, lv_color_hex(0x2D1E1E), 0);
        lv_obj_set_style_radius(phoneRow, 8, 0);
        lv_obj_set_style_border_width(phoneRow, 0, 0);
        lv_obj_remove_flag(phoneRow, LV_OBJ_FLAG_SCROLLABLE);
        
        lv_obj_t* phoneIcon = lv_label_create(phoneRow);
        lv_label_set_text(phoneIcon, LV_SYMBOL_CALL);
        lv_obj_set_style_text_font(phoneIcon, &lv_font_montserrat_22, 0);
        lv_obj_set_style_text_color(phoneIcon, UI_COLOR_ERROR, 0);
        lv_obj_align(phoneIcon, LV_ALIGN_LEFT_MID, 10, 0);
        
        lv_obj_t* phoneLbl = lv_label_create(phoneRow);
        lv_label_set_text(phoneLbl, "Phone not docked!");
        lv_obj_set_style_text_font(phoneLbl, &lv_font_montserrat_18, 0);
        lv_obj_align(phoneLbl, LV_ALIGN_LEFT_MID, 45, 0);
        
        iconY += 55;
    }
    
    if (presenceIssue) {
        lv_obj_t* presRow = lv_obj_create(card);
        lv_obj_set_size(presRow, SCREEN_WIDTH - 100, 45);
        lv_obj_set_pos(presRow, 10, iconY);
        lv_obj_set_style_bg_color(presRow, lv_color_hex(0x2D1E1E), 0);
        lv_obj_set_style_radius(presRow, 8, 0);
        lv_obj_set_style_border_width(presRow, 0, 0);
        lv_obj_remove_flag(presRow, LV_OBJ_FLAG_SCROLLABLE);
        
        lv_obj_t* presIcon = lv_label_create(presRow);
        lv_label_set_text(presIcon, LV_SYMBOL_EYE_CLOSE);
        lv_obj_set_style_text_font(presIcon, &lv_font_montserrat_22, 0);
        lv_obj_set_style_text_color(presIcon, UI_COLOR_WARNING, 0);
        lv_obj_align(presIcon, LV_ALIGN_LEFT_MID, 10, 0);
        
        lv_obj_t* presLbl = lv_label_create(presRow);
        lv_label_set_text(presLbl, "User not detected!");
        lv_obj_set_style_text_font(presLbl, &lv_font_montserrat_18, 0);
        lv_obj_align(presLbl, LV_ALIGN_LEFT_MID, 45, 0);
    }
    
    // Instruction
    lv_obj_t* instrLbl = lv_label_create(card);
    lv_label_set_text(instrLbl, "Get back to focusing!");
    lv_obj_set_style_text_font(instrLbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(instrLbl, UI_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(instrLbl, LV_ALIGN_BOTTOM_MID, 0, -15);
    
    // Footer
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "A: Dismiss Warning");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -15);
    
    loadScreen(scr);
}

// ===================================================================================
// GLOBAL SETTINGS MENU
// ===================================================================================

void UIManager::showSettingsMenu(int selectedIndex, bool focusModeEnabled, bool speakerMuted) {
    lv_obj_t* scr = createScreen();
    
    createHeader(scr, "Settings", true);
    
    // Settings list
    lv_obj_t* list = lv_obj_create(scr);
    lv_obj_set_size(list, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 80);
    lv_obj_set_pos(list, 10, 55);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list, 10, 0);
    lv_obj_set_style_pad_all(list, 10, 0);
    
    // Menu items - 5 options
    const char* labels[] = {"Focus Mode", "Mute Speaker", "Show Admin URL", "Developer Mode", "Back to Menu"};
    const char* icons[] = {LV_SYMBOL_EYE_OPEN, LV_SYMBOL_MUTE, LV_SYMBOL_WIFI, LV_SYMBOL_SETTINGS, LV_SYMBOL_LEFT};
    int itemCount = 5;
    
    for (int i = 0; i < itemCount; i++) {
        lv_obj_t* item = lv_obj_create(list);
        lv_obj_set_size(item, SCREEN_WIDTH - 50, 44);
        
        if (i == selectedIndex) {
            lv_obj_add_style(item, &UITheme::style_list_item_selected, 0);
        } else {
            lv_obj_add_style(item, &UITheme::style_list_item, 0);
        }
        lv_obj_remove_flag(item, LV_OBJ_FLAG_SCROLLABLE);
        
        // Icon
        lv_obj_t* icon = lv_label_create(item);
        lv_label_set_text(icon, icons[i]);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(icon, i == selectedIndex ? UI_COLOR_PRIMARY : UI_COLOR_TEXT_SECONDARY, 0);
        lv_obj_align(icon, LV_ALIGN_LEFT_MID, 10, 0);
        
        // Label
        lv_obj_t* label = lv_label_create(item);
        lv_label_set_text(label, labels[i]);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 45, 0);
        
        // Toggle/Value for Focus Mode
        if (i == 0) {
            lv_obj_t* toggle = lv_label_create(item);
            lv_label_set_text(toggle, focusModeEnabled ? "ON" : "OFF");
            lv_obj_set_style_text_font(toggle, &lv_font_montserrat_16, 0);
            lv_obj_set_style_text_color(toggle, focusModeEnabled ? UI_COLOR_SUCCESS : UI_COLOR_TEXT_MUTED, 0);
            lv_obj_align(toggle, LV_ALIGN_RIGHT_MID, -15, 0);
        }
        
        // Toggle for Speaker Mute
        if (i == 1) {
            lv_obj_t* toggle = lv_label_create(item);
            lv_label_set_text(toggle, speakerMuted ? "MUTED" : "ON");
            lv_obj_set_style_text_font(toggle, &lv_font_montserrat_16, 0);
            lv_obj_set_style_text_color(toggle, speakerMuted ? UI_COLOR_WARNING : UI_COLOR_SUCCESS, 0);
            lv_obj_align(toggle, LV_ALIGN_RIGHT_MID, -15, 0);
        }
        
        // Arrow for submenus
        if (i == 2 || i == 3) {
            lv_obj_t* arrow = lv_label_create(item);
            lv_label_set_text(arrow, LV_SYMBOL_RIGHT);
            lv_obj_set_style_text_color(arrow, UI_COLOR_TEXT_MUTED, 0);
            lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, -10, 0);
        }
    }
    
    // Footer hint
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "Dial: Navigate   A: Select   B: Back");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    loadScreen(scr);
}

// ===================================================================================
// ADMIN URL DISPLAY
// ===================================================================================

void UIManager::showAdminURL(const char* url) {
    lv_obj_t* scr = createScreen();
    
    createHeader(scr, "Admin Panel", true);
    
    // Create a nice card to display the URL
    lv_obj_t* card = lv_obj_create(scr);
    lv_obj_set_size(card, SCREEN_WIDTH - 40, 200);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_style_bg_color(card, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 15, 0);
    lv_obj_set_style_border_width(card, 3, 0);
    lv_obj_set_style_border_color(card, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_pad_all(card, 15, 0);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(card, 10, 0);
    
    // WiFi icon
    lv_obj_t* icon = lv_label_create(card);
    lv_label_set_text(icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(icon, UI_COLOR_PRIMARY, 0);
    
    // Title
    lv_obj_t* titleLabel = lv_label_create(card);
    lv_label_set_text(titleLabel, "Admin Panel URL");
    lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(titleLabel, UI_COLOR_TEXT_PRIMARY, 0);
    
    // URL in prominent box
    lv_obj_t* urlBox = lv_obj_create(card);
    lv_obj_set_size(urlBox, SCREEN_WIDTH - 80, 50);
    lv_obj_set_style_bg_color(urlBox, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_bg_opa(urlBox, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(urlBox, 8, 0);
    lv_obj_set_style_border_width(urlBox, 0, 0);
    lv_obj_remove_flag(urlBox, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t* urlLabel = lv_label_create(urlBox);
    lv_label_set_text(urlLabel, url);
    lv_obj_set_style_text_font(urlLabel, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(urlLabel, lv_color_white(), 0);
    lv_obj_center(urlLabel);
    
    // Instructions
    lv_obj_t* instrLabel = lv_label_create(card);
    lv_label_set_text(instrLabel, "Open in browser\n(same WiFi network)");
    lv_obj_set_style_text_font(instrLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(instrLabel, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_align(instrLabel, LV_TEXT_ALIGN_CENTER, 0);
    
    // Footer hint
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "B: Back to Settings");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    loadScreen(scr);
}

// ===================================================================================
// DEV MODE MENU
// ===================================================================================

void UIManager::showDevModeMenu(int selectedIndex, const char* apiUrl, bool serialDebug, bool showFPS, bool verboseNet) {
    lv_obj_t* scr = createScreen();
    
    createHeader(scr, "Developer Mode", true);
    
    // Settings list
    lv_obj_t* list = lv_obj_create(scr);
    lv_obj_set_size(list, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 80);
    lv_obj_set_pos(list, 10, 55);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list, 8, 0);
    lv_obj_set_style_pad_all(list, 5, 0);
    
    // Menu items (7 items now)
    const char* labels[] = {"API Base URL", "Serial Debug", "Show FPS", "Verbose Network", "Hardware Tests", "Reset All Settings", "Back"};
    const char* icons[] = {LV_SYMBOL_UPLOAD, LV_SYMBOL_LIST, LV_SYMBOL_CHARGE, LV_SYMBOL_DOWNLOAD, LV_SYMBOL_SETTINGS, LV_SYMBOL_REFRESH, LV_SYMBOL_LEFT};
    bool toggles[] = {false, serialDebug, showFPS, verboseNet, false, false, false};
    int itemCount = 7;
    
    for (int i = 0; i < itemCount; i++) {
        lv_obj_t* item = lv_obj_create(list);
        lv_obj_set_size(item, SCREEN_WIDTH - 50, 42);
        
        if (i == selectedIndex) {
            lv_obj_add_style(item, &UITheme::style_list_item_selected, 0);
        } else {
            lv_obj_add_style(item, &UITheme::style_list_item, 0);
        }
        lv_obj_remove_flag(item, LV_OBJ_FLAG_SCROLLABLE);
        
        // Icon
        lv_obj_t* icon = lv_label_create(item);
        lv_label_set_text(icon, icons[i]);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(icon, i == selectedIndex ? UI_COLOR_PRIMARY : UI_COLOR_TEXT_SECONDARY, 0);
        lv_obj_align(icon, LV_ALIGN_LEFT_MID, 8, 0);
        
        // Label
        lv_obj_t* label = lv_label_create(item);
        lv_label_set_text(label, labels[i]);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 38, 0);
        
        // Value for API URL (first item)
        if (i == 0) {
            lv_obj_t* val = lv_label_create(item);
            // Truncate URL for display
            String displayUrl = String(apiUrl);
            if (displayUrl.length() > 25) {
                displayUrl = displayUrl.substring(0, 22) + "...";
            }
            lv_label_set_text(val, displayUrl.c_str());
            lv_obj_set_style_text_font(val, &lv_font_montserrat_12, 0);
            lv_obj_set_style_text_color(val, UI_COLOR_TEXT_MUTED, 0);
            lv_obj_align(val, LV_ALIGN_RIGHT_MID, -30, 0);
            
            lv_obj_t* arrow = lv_label_create(item);
            lv_label_set_text(arrow, LV_SYMBOL_RIGHT);
            lv_obj_set_style_text_color(arrow, UI_COLOR_TEXT_MUTED, 0);
            lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, -8, 0);
        }
        // Toggle for boolean settings (items 1-3)
        else if (i >= 1 && i <= 3) {
            lv_obj_t* toggle = lv_label_create(item);
            lv_label_set_text(toggle, toggles[i] ? "ON" : "OFF");
            lv_obj_set_style_text_font(toggle, &lv_font_montserrat_16, 0);
            lv_obj_set_style_text_color(toggle, toggles[i] ? UI_COLOR_SUCCESS : UI_COLOR_TEXT_MUTED, 0);
            lv_obj_align(toggle, LV_ALIGN_RIGHT_MID, -12, 0);
        }
        // Hardware Tests (item 4) - show arrow
        else if (i == 4) {
            lv_obj_t* arrow = lv_label_create(item);
            lv_label_set_text(arrow, LV_SYMBOL_RIGHT);
            lv_obj_set_style_text_color(arrow, UI_COLOR_TEXT_MUTED, 0);
            lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, -8, 0);
        }
        // Reset settings (item 5) - warning color
        else if (i == 5) {
            lv_obj_set_style_text_color(icon, UI_COLOR_WARNING, 0);
        }
    }
    
    // Footer hint
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "Dial: Navigate   A: Toggle/Edit   B: Back");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -8);
    
    loadScreen(scr);
}

// ===================================================================================
// API URL EDITOR
// ===================================================================================

void UIManager::showApiUrlEditor(const char* currentUrl, const char* editingUrl, int cursorPos) {
    lv_obj_t* scr = createScreen();
    
    createHeader(scr, "Edit API URL", true);
    
    // Current URL display
    lv_obj_t* currentCard = createCard(scr, 20, 55, SCREEN_WIDTH - 40, 50);
    lv_obj_set_style_bg_color(currentCard, UI_COLOR_BG_CARD, 0);
    
    lv_obj_t* currLbl = lv_label_create(currentCard);
    lv_label_set_text(currLbl, "Current:");
    lv_obj_set_style_text_font(currLbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(currLbl, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_align(currLbl, LV_ALIGN_LEFT_MID, 10, -8);
    
    lv_obj_t* currVal = lv_label_create(currentCard);
    lv_label_set_text(currVal, currentUrl);
    lv_obj_set_style_text_font(currVal, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(currVal, UI_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(currVal, LV_ALIGN_LEFT_MID, 10, 10);
    
    // Editing area
    lv_obj_t* editCard = createCard(scr, 20, 115, SCREEN_WIDTH - 40, 70);
    lv_obj_set_style_border_color(editCard, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_border_width(editCard, 2, 0);
    
    lv_obj_t* editLbl = lv_label_create(editCard);
    lv_label_set_text(editLbl, "New URL:");
    lv_obj_set_style_text_font(editLbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(editLbl, UI_COLOR_PRIMARY, 0);
    lv_obj_align(editLbl, LV_ALIGN_TOP_LEFT, 10, 5);
    
    // URL with cursor
    String displayText = String(editingUrl);
    if (cursorPos >= 0) {
        displayText = displayText.substring(0, cursorPos) + "|" + displayText.substring(cursorPos);
    }
    
    lv_obj_t* urlText = lv_label_create(editCard);
    lv_label_set_text(urlText, displayText.c_str());
    lv_obj_set_style_text_font(urlText, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(urlText, UI_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_width(urlText, SCREEN_WIDTH - 80);
    lv_label_set_long_mode(urlText, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_align(urlText, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    
    // Instructions card
    lv_obj_t* instrCard = createCard(scr, 20, 195, SCREEN_WIDTH - 40, 85);
    lv_obj_set_style_bg_color(instrCard, lv_color_hex(0x1A1F25), 0);
    
    lv_obj_t* instr = lv_label_create(instrCard);
    lv_label_set_text(instr, 
        "Type URL using keyboard\n"
        "Enter: Save   ESC/B: Cancel\n"
        "Backspace: Delete   Del: Reset to default");
    lv_obj_set_style_text_font(instr, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(instr, UI_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_align(instr, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(instr);
    
    // Footer hint
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "Left/Right: Move cursor");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    loadScreen(scr);
}

// ===================================================================================
// HARDWARE TEST SCREEN
// ===================================================================================

void UIManager::showHardwareTest(int selectedIndex, bool btnA, bool btnB, bool btnC, bool btnD, 
                                  int potValue, char lastKey, bool wifiConnected) {
    lv_obj_t* scr = createScreen();
    
    createHeader(scr, "Hardware Tests", true);
    
    // Create two columns layout
    lv_obj_t* container = lv_obj_create(scr);
    lv_obj_set_size(container, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 80);
    lv_obj_set_pos(container, 10, 55);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);
    
    // Left column - Test menu (scrollable)
    lv_obj_t* menuCol = lv_obj_create(container);
    lv_obj_set_size(menuCol, 140, SCREEN_HEIGHT - 95);
    lv_obj_set_pos(menuCol, 0, 0);
    lv_obj_set_style_bg_opa(menuCol, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(menuCol, 0, 0);
    lv_obj_set_flex_flow(menuCol, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(menuCol, 3, 0);
    lv_obj_set_style_pad_all(menuCol, 2, 0);
    
    // Menu items (10 items)
    const char* labels[] = {"TFT Display", "OLED", "Buttons", "Keyboard", "Potentiometer", "Speaker", "RGB LED", "WiFi", "API", "Back"};
    const char* icons[] = {LV_SYMBOL_IMAGE, LV_SYMBOL_EYE_OPEN, LV_SYMBOL_KEYBOARD, LV_SYMBOL_EDIT, LV_SYMBOL_REFRESH, LV_SYMBOL_AUDIO, LV_SYMBOL_TINT, LV_SYMBOL_WIFI, LV_SYMBOL_DOWNLOAD, LV_SYMBOL_LEFT};
    
    for (int i = 0; i < 10; i++) {
        lv_obj_t* item = lv_obj_create(menuCol);
        lv_obj_set_size(item, 130, 24);
        
        if (i == selectedIndex) {
            lv_obj_add_style(item, &UITheme::style_list_item_selected, 0);
        } else {
            lv_obj_add_style(item, &UITheme::style_list_item, 0);
        }
        lv_obj_remove_flag(item, LV_OBJ_FLAG_SCROLLABLE);
        
        // Icon
        lv_obj_t* icon = lv_label_create(item);
        lv_label_set_text(icon, icons[i]);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(icon, i == selectedIndex ? UI_COLOR_PRIMARY : UI_COLOR_TEXT_SECONDARY, 0);
        lv_obj_align(icon, LV_ALIGN_LEFT_MID, 3, 0);
        
        // Label
        lv_obj_t* label = lv_label_create(item);
        lv_label_set_text(label, labels[i]);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 20, 0);
    }
    
    // Right column - Live status display
    lv_obj_t* statusCol = createCard(container, 145, 0, 155, SCREEN_HEIGHT - 95);
    lv_obj_set_style_bg_color(statusCol, lv_color_hex(0x1A1F25), 0);
    lv_obj_set_flex_flow(statusCol, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(statusCol, 8, 0);
    lv_obj_set_style_pad_row(statusCol, 3, 0);
    
    // Title
    lv_obj_t* title = lv_label_create(statusCol);
    lv_label_set_text(title, "Live Status");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_PRIMARY, 0);
    
    // Buttons status
    lv_obj_t* btnRow = lv_obj_create(statusCol);
    lv_obj_set_size(btnRow, 140, 28);
    lv_obj_set_style_bg_opa(btnRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btnRow, 0, 0);
    lv_obj_set_style_pad_all(btnRow, 0, 0);
    
    const char* btnLabels[] = {"A", "B", "C", "D"};
    bool btnStates[] = {btnA, btnB, btnC, btnD};
    
    for (int i = 0; i < 4; i++) {
        lv_obj_t* btn = lv_obj_create(btnRow);
        lv_obj_set_size(btn, 30, 22);
        lv_obj_set_pos(btn, i * 34, 2);
        lv_obj_set_style_radius(btn, 4, 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_border_color(btn, btnStates[i] ? UI_COLOR_SUCCESS : UI_COLOR_TEXT_MUTED, 0);
        lv_obj_set_style_bg_color(btn, btnStates[i] ? UI_COLOR_SUCCESS : lv_color_hex(0x2A2F35), 0);
        lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        
        lv_obj_t* lbl = lv_label_create(btn);
        lv_label_set_text(lbl, btnLabels[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(lbl, btnStates[i] ? lv_color_hex(0x000000) : UI_COLOR_TEXT_SECONDARY, 0);
        lv_obj_center(lbl);
    }
    
    // Potentiometer value
    lv_obj_t* potLbl = lv_label_create(statusCol);
    char potStr[32];
    snprintf(potStr, sizeof(potStr), "Pot: %d", potValue);
    lv_label_set_text(potLbl, potStr);
    lv_obj_set_style_text_font(potLbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(potLbl, UI_COLOR_TEXT_SECONDARY, 0);
    
    // Pot progress bar
    lv_obj_t* potBar = lv_bar_create(statusCol);
    lv_obj_set_size(potBar, 130, 10);
    lv_bar_set_range(potBar, 0, 4095);
    lv_bar_set_value(potBar, potValue, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(potBar, lv_color_hex(0x2A2F35), LV_PART_MAIN);
    lv_obj_set_style_bg_color(potBar, UI_COLOR_PRIMARY, LV_PART_INDICATOR);
    
    // Keyboard last key
    lv_obj_t* kbLbl = lv_label_create(statusCol);
    char kbStr[32];
    if (lastKey >= 32 && lastKey < 127) {
        snprintf(kbStr, sizeof(kbStr), "Key: '%c' (%d)", lastKey, (int)lastKey);
    } else if (lastKey != 0) {
        snprintf(kbStr, sizeof(kbStr), "Key: 0x%02X (%d)", (uint8_t)lastKey, (int)lastKey);
    } else {
        snprintf(kbStr, sizeof(kbStr), "Key: (none)");
    }
    lv_label_set_text(kbLbl, kbStr);
    lv_obj_set_style_text_font(kbLbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(kbLbl, UI_COLOR_TEXT_SECONDARY, 0);
    
    // WiFi status
    lv_obj_t* wifiLbl = lv_label_create(statusCol);
    lv_label_set_text(wifiLbl, wifiConnected ? "WiFi: Connected" : "WiFi: Disconnected");
    lv_obj_set_style_text_font(wifiLbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(wifiLbl, wifiConnected ? UI_COLOR_SUCCESS : UI_COLOR_WARNING, 0);
    
    // Footer hint
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "Dial: Select   A: Run Test   B: Back");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -8);
    
    loadScreen(scr);
}

// ===================================================================================
// TEST RESULT SCREEN
// ===================================================================================

void UIManager::showTestResult(const char* testName, bool passed, const char* details) {
    lv_obj_t* scr = createScreen();
    
    createHeader(scr, "Test Result", true);
    
    // Result card
    lv_obj_t* card = createCard(scr, 20, 60, SCREEN_WIDTH - 40, 140);
    lv_obj_set_style_bg_color(card, passed ? lv_color_hex(0x1A3A1A) : lv_color_hex(0x3A1A1A), 0);
    lv_obj_set_style_border_color(card, passed ? UI_COLOR_SUCCESS : UI_COLOR_WARNING, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    
    // Test name
    lv_obj_t* nameLabel = lv_label_create(card);
    lv_label_set_text(nameLabel, testName);
    lv_obj_set_style_text_font(nameLabel, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(nameLabel, UI_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(nameLabel, LV_ALIGN_TOP_MID, 0, 10);
    
    // Pass/Fail icon and text
    lv_obj_t* statusLabel = lv_label_create(card);
    if (passed) {
        lv_label_set_text(statusLabel, LV_SYMBOL_OK "  PASSED");
        lv_obj_set_style_text_color(statusLabel, UI_COLOR_SUCCESS, 0);
    } else {
        lv_label_set_text(statusLabel, LV_SYMBOL_CLOSE "  FAILED");
        lv_obj_set_style_text_color(statusLabel, UI_COLOR_WARNING, 0);
    }
    lv_obj_set_style_text_font(statusLabel, &lv_font_montserrat_24, 0);
    lv_obj_align(statusLabel, LV_ALIGN_CENTER, 0, -5);
    
    // Details text
    lv_obj_t* detailsLabel = lv_label_create(card);
    lv_label_set_text(detailsLabel, details);
    lv_obj_set_style_text_font(detailsLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(detailsLabel, UI_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_align(detailsLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(detailsLabel, SCREEN_WIDTH - 80);
    lv_obj_align(detailsLabel, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    // Footer hint
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "Press any button to continue");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -15);
    
    loadScreen(scr);
}

// ===================================================================================
// TRANSCRIPT MODE SCREENS
// ===================================================================================

void UIManager::showTranscriptList(const char** titles, const char** dates, int count, int selectedIndex) {
    lv_obj_t* scr = createScreen();
    
    createHeader(scr, "Transcripts", true);
    
    // Scrollable list container
    lv_obj_t* list = lv_obj_create(scr);
    lv_obj_set_size(list, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 85);
    lv_obj_set_pos(list, 10, 55);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list, 8, 0);
    lv_obj_set_style_pad_all(list, 5, 0);
    
    for (int i = 0; i < count; i++) {
        lv_obj_t* item = lv_obj_create(list);
        lv_obj_set_size(item, SCREEN_WIDTH - 45, 52);
        
        if (i == selectedIndex) {
            lv_obj_add_style(item, &UITheme::style_list_item_selected, 0);
        } else {
            lv_obj_add_style(item, &UITheme::style_list_item, 0);
        }
        lv_obj_remove_flag(item, LV_OBJ_FLAG_SCROLLABLE);
        
        // Microphone icon
        lv_obj_t* icon = lv_label_create(item);
        lv_label_set_text(icon, LV_SYMBOL_AUDIO);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(icon, i == selectedIndex ? UI_COLOR_PRIMARY : UI_COLOR_TEXT_SECONDARY, 0);
        lv_obj_align(icon, LV_ALIGN_LEFT_MID, 8, 0);
        
        // Title
        lv_obj_t* titleLbl = lv_label_create(item);
        lv_label_set_text(titleLbl, titles[i]);
        lv_obj_set_style_text_font(titleLbl, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(titleLbl, UI_COLOR_TEXT_PRIMARY, 0);
        lv_obj_set_width(titleLbl, SCREEN_WIDTH - 120);
        lv_label_set_long_mode(titleLbl, LV_LABEL_LONG_DOT);
        lv_obj_align(titleLbl, LV_ALIGN_LEFT_MID, 40, -8);
        
        // Date
        lv_obj_t* dateLbl = lv_label_create(item);
        lv_label_set_text(dateLbl, dates[i]);
        lv_obj_set_style_text_font(dateLbl, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(dateLbl, UI_COLOR_TEXT_MUTED, 0);
        lv_obj_align(dateLbl, LV_ALIGN_LEFT_MID, 40, 10);
        
        // Arrow
        lv_obj_t* arrow = lv_label_create(item);
        lv_label_set_text(arrow, LV_SYMBOL_RIGHT);
        lv_obj_set_style_text_color(arrow, UI_COLOR_TEXT_MUTED, 0);
        lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, -10, 0);
    }
    
    // Scroll to selected item
    if (selectedIndex > 0) {
        lv_obj_scroll_to_y(list, selectedIndex * 60 - 30, LV_ANIM_OFF);
    }
    
    // Footer
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "Dial: Select   A: Open   B: Back");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -8);
    
    loadScreen(scr);
}

void UIManager::showTranscriptOptions(const char* title, int selectedIndex) {
    lv_obj_t* scr = createScreen();
    
    createHeader(scr, title, true);
    
    // Options list
    lv_obj_t* list = lv_obj_create(scr);
    lv_obj_set_size(list, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 85);
    lv_obj_set_pos(list, 10, 55);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list, 10, 0);
    lv_obj_set_style_pad_all(list, 8, 0);
    
    // Menu options
    const char* labels[] = {"Generate Quiz", "Generate Flashcards", "View Transcript", "Back to List"};
    const char* icons[] = {LV_SYMBOL_LIST, LV_SYMBOL_FILE, LV_SYMBOL_EYE_OPEN, LV_SYMBOL_LEFT};
    const char* descriptions[] = {"Create quiz questions", "Create study cards", "Read full content", "Return to list"};
    int itemCount = 4;
    
    for (int i = 0; i < itemCount; i++) {
        lv_obj_t* item = lv_obj_create(list);
        lv_obj_set_size(item, SCREEN_WIDTH - 45, 48);
        
        if (i == selectedIndex) {
            lv_obj_add_style(item, &UITheme::style_list_item_selected, 0);
        } else {
            lv_obj_add_style(item, &UITheme::style_list_item, 0);
        }
        lv_obj_remove_flag(item, LV_OBJ_FLAG_SCROLLABLE);
        
        // Icon
        lv_obj_t* icon = lv_label_create(item);
        lv_label_set_text(icon, icons[i]);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(icon, i == selectedIndex ? UI_COLOR_PRIMARY : UI_COLOR_TEXT_SECONDARY, 0);
        lv_obj_align(icon, LV_ALIGN_LEFT_MID, 10, 0);
        
        // Label
        lv_obj_t* label = lv_label_create(item);
        lv_label_set_text(label, labels[i]);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 45, -6);
        
        // Description
        lv_obj_t* desc = lv_label_create(item);
        lv_label_set_text(desc, descriptions[i]);
        lv_obj_set_style_text_font(desc, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(desc, UI_COLOR_TEXT_MUTED, 0);
        lv_obj_align(desc, LV_ALIGN_LEFT_MID, 45, 10);
        
        // Arrow for Generate options
        if (i < 2) {
            lv_obj_t* arrow = lv_label_create(item);
            lv_label_set_text(arrow, LV_SYMBOL_RIGHT);
            lv_obj_set_style_text_color(arrow, UI_COLOR_TEXT_MUTED, 0);
            lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, -10, 0);
        }
    }
    
    // Footer
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "Dial: Select   A: Confirm   B: Back");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -8);
    
    loadScreen(scr);
}

void UIManager::showTranscriptContent(const char* title, const char* content) {
    lv_obj_t* scr = createScreen();
    
    createHeader(scr, title, true);
    
    // Scrollable content area
    lv_obj_t* contentArea = lv_obj_create(scr);
    lv_obj_set_size(contentArea, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 85);
    lv_obj_set_pos(contentArea, 10, 55);
    lv_obj_add_style(contentArea, &UITheme::style_card, 0);
    lv_obj_set_style_pad_all(contentArea, 12, 0);
    
    // Content text
    lv_obj_t* textLabel = lv_label_create(contentArea);
    lv_label_set_text(textLabel, content);
    lv_obj_set_style_text_font(textLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(textLabel, UI_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_width(textLabel, SCREEN_WIDTH - 55);
    lv_label_set_long_mode(textLabel, LV_LABEL_LONG_WRAP);
    
    // Footer
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "Scroll to read   B: Back");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -8);
    
    loadScreen(scr);
}

void UIManager::showSuccess(const char* title, const char* message) {
    lv_obj_t* scr = createScreen();
    
    createHeader(scr, title, false);
    
    // Success card
    lv_obj_t* card = createCard(scr, 20, 60, SCREEN_WIDTH - 40, 140);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1A3A1A), 0);
    lv_obj_set_style_border_color(card, UI_COLOR_SUCCESS, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    
    // Success icon
    lv_obj_t* icon = lv_label_create(card);
    lv_label_set_text(icon, LV_SYMBOL_OK);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(icon, UI_COLOR_SUCCESS, 0);
    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 15);
    
    // Message
    lv_obj_t* msgLabel = lv_label_create(card);
    lv_label_set_text(msgLabel, message);
    lv_obj_set_style_text_font(msgLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(msgLabel, UI_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_align(msgLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(msgLabel, SCREEN_WIDTH - 80);
    lv_obj_align(msgLabel, LV_ALIGN_CENTER, 0, 15);
    
    // Footer
    lv_obj_t* hint = lv_label_create(scr);
    lv_label_set_text(hint, "Press any button to continue");
    lv_obj_add_style(hint, &UITheme::style_text_small, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -15);
    
    loadScreen(scr);
}
