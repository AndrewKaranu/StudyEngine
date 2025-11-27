/**
 * UI Theme Implementation
 */

#include "UITheme.h"

// Static member definitions
bool UITheme::initialized = false;
lv_style_t UITheme::style_screen;
lv_style_t UITheme::style_card;
lv_style_t UITheme::style_card_selected;
lv_style_t UITheme::style_header;
lv_style_t UITheme::style_btn_primary;
lv_style_t UITheme::style_btn_secondary;
lv_style_t UITheme::style_btn_answer[4];
lv_style_t UITheme::style_text_title;
lv_style_t UITheme::style_text_body;
lv_style_t UITheme::style_text_small;
lv_style_t UITheme::style_progress_bg;
lv_style_t UITheme::style_progress_indicator;
lv_style_t UITheme::style_list_item;
lv_style_t UITheme::style_list_item_selected;

void UITheme::init() {
    if (initialized) return;
    
    // Screen style - dark background
    lv_style_init(&style_screen);
    lv_style_set_bg_color(&style_screen, UI_COLOR_BG_DARK);
    lv_style_set_bg_opa(&style_screen, LV_OPA_COVER);
    
    // Card style
    lv_style_init(&style_card);
    lv_style_set_bg_color(&style_card, UI_COLOR_BG_CARD);
    lv_style_set_bg_opa(&style_card, LV_OPA_COVER);
    lv_style_set_radius(&style_card, 16);
    lv_style_set_pad_all(&style_card, 16);
    lv_style_set_border_width(&style_card, 0);
    lv_style_set_shadow_width(&style_card, 20);
    lv_style_set_shadow_color(&style_card, lv_color_black());
    lv_style_set_shadow_opa(&style_card, LV_OPA_20);
    
    // Card selected style
    lv_style_init(&style_card_selected);
    lv_style_set_bg_color(&style_card_selected, UI_COLOR_BG_ELEVATED);
    lv_style_set_border_width(&style_card_selected, 2);
    lv_style_set_border_color(&style_card_selected, UI_COLOR_PRIMARY);
    lv_style_set_radius(&style_card_selected, 16);
    lv_style_set_pad_all(&style_card_selected, 16);
    
    // Header style
    lv_style_init(&style_header);
    lv_style_set_bg_color(&style_header, UI_COLOR_BG_ELEVATED);
    lv_style_set_bg_opa(&style_header, LV_OPA_COVER);
    lv_style_set_pad_all(&style_header, 12);
    lv_style_set_radius(&style_header, 0);
    
    // Primary button style
    lv_style_init(&style_btn_primary);
    lv_style_set_bg_color(&style_btn_primary, UI_COLOR_PRIMARY);
    lv_style_set_bg_opa(&style_btn_primary, LV_OPA_COVER);
    lv_style_set_radius(&style_btn_primary, 12);
    lv_style_set_pad_hor(&style_btn_primary, 24);
    lv_style_set_pad_ver(&style_btn_primary, 14);
    lv_style_set_text_color(&style_btn_primary, UI_COLOR_TEXT_PRIMARY);
    lv_style_set_border_width(&style_btn_primary, 0);
    lv_style_set_shadow_width(&style_btn_primary, 15);
    lv_style_set_shadow_color(&style_btn_primary, UI_COLOR_PRIMARY);
    lv_style_set_shadow_opa(&style_btn_primary, LV_OPA_30);
    
    // Secondary button style
    lv_style_init(&style_btn_secondary);
    lv_style_set_bg_color(&style_btn_secondary, UI_COLOR_BG_CARD);
    lv_style_set_bg_opa(&style_btn_secondary, LV_OPA_COVER);
    lv_style_set_radius(&style_btn_secondary, 12);
    lv_style_set_pad_hor(&style_btn_secondary, 24);
    lv_style_set_pad_ver(&style_btn_secondary, 14);
    lv_style_set_text_color(&style_btn_secondary, UI_COLOR_TEXT_PRIMARY);
    lv_style_set_border_width(&style_btn_secondary, 2);
    lv_style_set_border_color(&style_btn_secondary, UI_COLOR_TEXT_MUTED);
    
    // Answer button styles (A, B, C, D)
    lv_color_t answerColors[4] = {
        UI_COLOR_ANSWER_A,  // Green
        UI_COLOR_ANSWER_B,  // Red
        UI_COLOR_ANSWER_C,  // Yellow
        UI_COLOR_ANSWER_D   // Blue
    };
    
    for (int i = 0; i < 4; i++) {
        lv_style_init(&style_btn_answer[i]);
        lv_style_set_bg_color(&style_btn_answer[i], answerColors[i]);
        lv_style_set_bg_opa(&style_btn_answer[i], LV_OPA_COVER);
        lv_style_set_radius(&style_btn_answer[i], 12);
        lv_style_set_pad_all(&style_btn_answer[i], 12);
        lv_style_set_text_color(&style_btn_answer[i], lv_color_white());
        lv_style_set_text_font(&style_btn_answer[i], &lv_font_montserrat_18);
        lv_style_set_border_width(&style_btn_answer[i], 0);
        lv_style_set_shadow_width(&style_btn_answer[i], 10);
        lv_style_set_shadow_color(&style_btn_answer[i], answerColors[i]);
        lv_style_set_shadow_opa(&style_btn_answer[i], LV_OPA_40);
    }
    
    // Text styles
    lv_style_init(&style_text_title);
    lv_style_set_text_color(&style_text_title, UI_COLOR_TEXT_PRIMARY);
    lv_style_set_text_font(&style_text_title, &lv_font_montserrat_24);
    
    lv_style_init(&style_text_body);
    lv_style_set_text_color(&style_text_body, UI_COLOR_TEXT_PRIMARY);
    lv_style_set_text_font(&style_text_body, &lv_font_montserrat_18);
    
    lv_style_init(&style_text_small);
    lv_style_set_text_color(&style_text_small, UI_COLOR_TEXT_SECONDARY);
    lv_style_set_text_font(&style_text_small, &lv_font_montserrat_14);
    
    // Progress bar background
    lv_style_init(&style_progress_bg);
    lv_style_set_bg_color(&style_progress_bg, UI_COLOR_BG_CARD);
    lv_style_set_bg_opa(&style_progress_bg, LV_OPA_COVER);
    lv_style_set_radius(&style_progress_bg, 8);
    
    // Progress bar indicator
    lv_style_init(&style_progress_indicator);
    lv_style_set_bg_color(&style_progress_indicator, UI_COLOR_PRIMARY);
    lv_style_set_bg_opa(&style_progress_indicator, LV_OPA_COVER);
    lv_style_set_radius(&style_progress_indicator, 8);
    
    // List item style
    lv_style_init(&style_list_item);
    lv_style_set_bg_color(&style_list_item, UI_COLOR_BG_CARD);
    lv_style_set_bg_opa(&style_list_item, LV_OPA_COVER);
    lv_style_set_radius(&style_list_item, 12);
    lv_style_set_pad_all(&style_list_item, 16);
    lv_style_set_border_width(&style_list_item, 0);
    lv_style_set_text_color(&style_list_item, UI_COLOR_TEXT_PRIMARY);
    
    // List item selected
    lv_style_init(&style_list_item_selected);
    lv_style_set_bg_color(&style_list_item_selected, UI_COLOR_BG_ELEVATED);
    lv_style_set_bg_opa(&style_list_item_selected, LV_OPA_COVER);
    lv_style_set_radius(&style_list_item_selected, 12);
    lv_style_set_pad_all(&style_list_item_selected, 16);
    lv_style_set_border_width(&style_list_item_selected, 2);
    lv_style_set_border_color(&style_list_item_selected, UI_COLOR_PRIMARY);
    lv_style_set_text_color(&style_list_item_selected, UI_COLOR_TEXT_PRIMARY);
    
    initialized = true;
}
