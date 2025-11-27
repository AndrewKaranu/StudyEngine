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
    tft.begin();
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
        } else {
            lv_label_set_text(icon, LV_SYMBOL_FILE);  // Flashcards
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
    lv_label_set_text(hint, "Press A or B to continue");
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
