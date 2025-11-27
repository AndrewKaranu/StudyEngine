#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ===================================================================================
// SCREEN DIMENSIONS (3.5" 480x320 TFT LCD)
// ===================================================================================
#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 320

// ===================================================================================
// PIN DEFINITIONS
// ===================================================================================

// --- I2C DEVICES ---
#define PCF_ADDR     0x20 
#define OLED_ADDR    0x3C
#define CARDKB_ADDR  0x5F
#define I2C_SDA      21
#define I2C_SCL      22

// --- SPI DEVICES (Shared VSPI Bus) ---
#define SPI_SCK      5
#define SPI_MISO     19
#define SPI_MOSI     23

#define TFT_CS       13
#define TFT_DC       2
#define TFT_RST      4 

#define RFID_CS      15
#define RFID_RST     33

// --- ANALOG & AUDIO ---
#define PIN_IR       34 // Analog Input
#define PIN_POT      35 // Analog Input
#define PIN_PIR      27 // Digital Input
#define PIN_SPKR     25 // DAC Output

// --- LORA (Disable) ---
#define LORA_CS      18
#define LORA_RST     14

// --- PCF8575 PIN MAPPING (0-15) ---
#define PCF_BTN_A    0
#define PCF_BTN_B    1
#define PCF_BTN_C    2
#define PCF_BTN_D    3
#define PCF_LED_R    6
#define PCF_LED_G    7
#define PCF_LED_B    8

// --- WIFI CONFIG ---
#define WIFI_SSID "Andrew’s iPhone"
#define WIFI_PASS "ReeceJames"
// Your PC's IP on the iPhone hotspot network
#define API_BASE_URL "http://172.20.10.11:8000"

#endif
