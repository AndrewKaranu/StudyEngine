#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include "config.h"
#include <Wire.h>

class InputManager {
private:
    uint16_t pcfState = 0xFFFF;
    
    // Long press tracking
    unsigned long btnDPressStart = 0;
    bool btnDWasPressed = false;
    bool btnDLongPressTriggered = false;
    
    static const unsigned long LONG_PRESS_DURATION = 800; // 800ms for long press
    
    void pcfWrite(uint16_t data);
    uint16_t pcfRead();

public:
    void begin();
    void update();
    
    // Potentiometer
    int getPotValue();
    int getScrollIndex(int itemCount);
    
    // Buttons (PCF8575)
    bool isBtnAPressed();
    bool isBtnBPressed();
    bool isBtnCPressed();
    bool isBtnDPressed();
    
    // Long press detection for Button D
    bool isBtnDLongPressed();
    
    // Check if button is currently held (not edge-triggered)
    bool isBtnAHeld();
    bool isBtnBHeld();
    bool isBtnCHeld();
    bool isBtnDHeld();
    
    // CardKB
    char readCardKB();
};

#endif
