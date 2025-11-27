#include "InputManager.h"

void InputManager::begin() {
    Wire.begin(I2C_SDA, I2C_SCL);
    
    // Init PCF8575
    // Inputs: 0-5 High, Outputs: 6-8 Low (Off)
    pcfState = 0xFE3F; 
    pcfWrite(pcfState);
}

void InputManager::update() {
    // Could cache inputs here if needed
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

char InputManager::readCardKB() {
    Wire.requestFrom(CARDKB_ADDR, 1);
    if(Wire.available()) {
        char c = Wire.read();
        if(c != 0) return c;
    }
    return 0;
}
