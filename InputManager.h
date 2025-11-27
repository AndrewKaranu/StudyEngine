#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include "config.h"
#include <Wire.h>

class InputManager {
private:
    uint16_t pcfState = 0xFFFF;
    
    void pcfWrite(uint16_t data);
    uint16_t pcfRead();

public:
    void begin();
    void update();
    
    // Potentiometer
    int getPotValue();
    
    // Buttons (PCF8575)
    bool isBtnAPressed();
    bool isBtnBPressed();
    bool isBtnCPressed();
    bool isBtnDPressed();
    
    // CardKB
    char readCardKB();
};

#endif
