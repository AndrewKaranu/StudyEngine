#include "InputManager.h"

void InputManager::begin() {
    Wire.begin(I2C_SDA, I2C_SCL);
    
    // Init PCF8575
    // Inputs: 0-5 High, Outputs: 6-8 Low (Off)
    pcfState = 0xFE3F; 
    pcfWrite(pcfState);
    
    // Initialize long press tracking
    btnDPressStart = 0;
    btnDWasPressed = false;
    btnDLongPressTriggered = false;
}

void InputManager::update() {
    // Update long press tracking for Button D
    uint16_t inputs = pcfRead();
    bool btnDCurrentlyPressed = !((inputs >> PCF_BTN_D) & 1);
    
    if (btnDCurrentlyPressed) {
        if (!btnDWasPressed) {
            // Button just pressed
            btnDPressStart = millis();
            btnDLongPressTriggered = false;
        }
    } else {
        // Button released - reset
        btnDWasPressed = false;
        btnDLongPressTriggered = false;
        btnDPressStart = 0;
    }
    
    btnDWasPressed = btnDCurrentlyPressed;
}

void InputManager::pcfWrite(uint16_t data) {
    Wire.beginTransmission(PCF_ADDR);
    Wire.write(data & 0xFF);
    Wire.write((data >> 8) & 0xFF);
    Wire.endTransmission();
    pcfState = data;
}

uint16_t InputManager::pcfRead() {
    Wire.requestFrom(PCF_ADDR, 2);
    if (Wire.available() == 2) {
        uint16_t data = Wire.read();
        data |= (Wire.read() << 8);
        return data;
    }
    return 0xFFFF;
}

int InputManager::getPotValue() {
    return analogRead(PIN_POT);
}

int InputManager::getScrollIndex(int itemCount) {
    if (itemCount <= 0) return 0;

    // 1. Read Raw Value
    int raw = analogRead(PIN_POT);

    // 2. Apply Deadzones
    // Fixes "hopping" at extremes and "won't reach first item"
    if (raw < 200) raw = 0;       // generous deadzone at bottom
    if (raw > 3900) raw = 4095;   // generous deadzone at top

    // 3. Map to Index
    // We use 4096 as divisor so the range 0-4095 maps perfectly to 0-(itemCount-1)
    int index = (int)((long)raw * itemCount / 4096);

    // 4. Safety Clamp
    if (index < 0) index = 0;
    if (index >= itemCount) index = itemCount - 1;

    return index;
}

bool InputManager::isBtnAPressed() {
    uint16_t inputs = pcfRead();
    return !((inputs >> PCF_BTN_A) & 1);
}

bool InputManager::isBtnBPressed() {
    uint16_t inputs = pcfRead();
    return !((inputs >> PCF_BTN_B) & 1);
}

bool InputManager::isBtnCPressed() {
    uint16_t inputs = pcfRead();
    return !((inputs >> PCF_BTN_C) & 1);
}

bool InputManager::isBtnDPressed() {
    uint16_t inputs = pcfRead();
    return !((inputs >> PCF_BTN_D) & 1);
}

bool InputManager::isBtnDHeld() {
    return btnDWasPressed;
}

bool InputManager::isBtnDLongPressed() {
    // Return true only once per long press
    if (btnDWasPressed && !btnDLongPressTriggered && btnDPressStart > 0) {
        if (millis() - btnDPressStart >= LONG_PRESS_DURATION) {
            btnDLongPressTriggered = true;
            return true;
        }
    }
    return false;
}

char InputManager::readCardKB() {
    Wire.requestFrom(CARDKB_ADDR, 1);
    if(Wire.available()) {
        char c = Wire.read();
        if(c != 0) return c;
    }
    return 0;
}
