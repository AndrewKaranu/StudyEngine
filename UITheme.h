/**
 * UI Theme - Modern Study Engine Colors and Styles
 * Inspired by modern learning apps
 */

#ifndef UI_THEME_H
#define UI_THEME_H

#include <lvgl.h>

// ===================================================================================
// COLOR PALETTE - Modern Learning App Style
// ===================================================================================

// Primary Colors
#define UI_COLOR_BG_DARK        lv_color_hex(0x1A1D26)   // Dark background
#define UI_COLOR_BG_CARD        lv_color_hex(0x252836)   // Card background
#define UI_COLOR_BG_ELEVATED    lv_color_hex(0x2D3142)   // Elevated elements

// Accent Colors
#define UI_COLOR_PRIMARY        lv_color_hex(0x6C63FF)   // Purple primary
#define UI_COLOR_SECONDARY      lv_color_hex(0x00D9FF)   // Cyan secondary
#define UI_COLOR_ACCENT         lv_color_hex(0xFF6B6B)   // Coral accent

// Status Colors
#define UI_COLOR_SUCCESS        lv_color_hex(0x4ADE80)   // Green success
#define UI_COLOR_WARNING        lv_color_hex(0xFBBF24)   // Yellow warning
#define UI_COLOR_ERROR          lv_color_hex(0xEF4444)   // Red error
#define UI_COLOR_INFO           lv_color_hex(0x3B82F6)   // Blue info

// Answer Button Colors (like the app image)
#define UI_COLOR_ANSWER_A       lv_color_hex(0x4ADE80)   // Green
#define UI_COLOR_ANSWER_B       lv_color_hex(0xEF4444)   // Red
#define UI_COLOR_ANSWER_C       lv_color_hex(0xFBBF24)   // Yellow/Orange
#define UI_COLOR_ANSWER_D       lv_color_hex(0x3B82F6)   // Blue

// Text Colors
#define UI_COLOR_TEXT_PRIMARY   lv_color_hex(0xFFFFFF)   // White
#define UI_COLOR_TEXT_SECONDARY lv_color_hex(0x9CA3AF)   // Gray
#define UI_COLOR_TEXT_MUTED     lv_color_hex(0x6B7280)   // Darker gray

// Special
#define UI_COLOR_CONFIRMED      lv_color_hex(0x3B82F6)   // Blue for confirmed
#define UI_COLOR_PENDING        lv_color_hex(0xFBBF24)   // Yellow for pending
#define UI_COLOR_EMPTY          lv_color_hex(0x4B5563)   // Gray for empty

// ===================================================================================
// STYLE DEFINITIONS
// ===================================================================================

class UITheme {
public:
    // Styles
    static lv_style_t style_screen;
    static lv_style_t style_card;
    static lv_style_t style_card_selected;
    static lv_style_t style_header;
    static lv_style_t style_btn_primary;
    static lv_style_t style_btn_secondary;
    static lv_style_t style_btn_answer[4];
    static lv_style_t style_text_title;
    static lv_style_t style_text_body;
    static lv_style_t style_text_small;
    static lv_style_t style_progress_bg;
    static lv_style_t style_progress_indicator;
    static lv_style_t style_list_item;
    static lv_style_t style_list_item_selected;
    
    static void init();
    
private:
    static bool initialized;
};

#endif
