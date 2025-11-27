#ifndef FLASHCARD_ENGINE_H
#define FLASHCARD_ENGINE_H

#include "config.h"
#include "DisplayManager.h"
#include "InputManager.h"
#include "NetworkManager.h"
#include "UIManager.h"
#include <vector>
#include <ArduinoJson.h>

enum FlashcardState {
    FC_INIT,
    FC_SELECT_DECK,
    FC_DOWNLOAD,
    FC_SHOW_FRONT,
    FC_SHOW_BACK,
    FC_FINISHED,
    FC_PAUSED
};

class FlashcardEngine {
private:
    FlashcardState state = FC_INIT;
    std::vector<Deck> availableDecks;
    Deck currentDeck;
    
    int selectedDeckIndex = 0;
    int lastSelectedDeckIndex = -1;
    
    int currentCardIndex = 0;
    bool needsFullRedraw = true;
    
    unsigned long sessionStartTime = 0;
    
    // Pause Menu
    unsigned long btnDPressStart = 0;
    bool btnDWasPressed = false;
    const unsigned long LONG_PRESS_MS = 1000;
    
    int pauseMenuIndex = 0;
    int lastPauseMenuIndex = -1;
    
    // For OLED stats
    unsigned long lastOledUpdate = 0;

public:
    void reset();
    void handleRun(DisplayManager& display, InputManager& input, SENetworkManager& network, int& systemState);
};

#endif
