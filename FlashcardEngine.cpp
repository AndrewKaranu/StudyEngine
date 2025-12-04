#include "FlashcardEngine.h"

void FlashcardEngine::reset() {
    state = FC_INIT;
    selectedDeckIndex = 0;
    lastSelectedDeckIndex = -1;
    currentCardIndex = 0;
    needsFullRedraw = true;
    availableDecks.clear();
    currentDeck.cards.clear();
}

void FlashcardEngine::handleRun(DisplayManager& display, InputManager& input, SENetworkManager& network, int& systemState) {
    switch (state) {
        case FC_INIT:
            uiMgr.showLoading("Fetching Decks...");
            display.showStatus("Fetching Decks...");
            
            // We need to implement fetchDeckList in NetworkManager
            // For now, we'll assume it returns a vector of Decks with just ID and Title
            availableDecks = network.fetchDeckList();
            
            if (availableDecks.empty()) {
                uiMgr.showError("No Decks Found!");
                delay(2000);
                systemState = 0; // Back to menu
            } else {
                state = FC_SELECT_DECK;
                lastSelectedDeckIndex = -1;
                needsFullRedraw = true;
            }
            break;

        case FC_SELECT_DECK:
            {
                // Navigation
                int newIndex = input.getScrollIndex(availableDecks.size());
                
                if (newIndex != selectedDeckIndex || needsFullRedraw) {
                    selectedDeckIndex = newIndex;
                    
                    const char* deckNames[availableDecks.size()];
                    for (size_t i = 0; i < availableDecks.size(); i++) {
                        deckNames[i] = availableDecks[i].title.c_str();
                    }
                    
                    // Reuse exam list UI for now, or create specific one
                    uiMgr.showExamList(deckNames, availableDecks.size(), selectedDeckIndex, "Select Deck");
                    display.showStatus("Select Deck");
                    
                    lastSelectedDeckIndex = selectedDeckIndex;
                    needsFullRedraw = false;
                }
                
                if (input.isBtnAPressed()) {
                    state = FC_DOWNLOAD;
                    needsFullRedraw = true;
                    delay(200);
                }
                
                if (input.isBtnBPressed()) {
                    systemState = 0; // Back to menu
                    delay(200);
                }
            }
            break;

        case FC_DOWNLOAD:
            {
                uiMgr.showLoading("Downloading Deck...");
                display.showStatus("Downloading...");
                
                // Fetch full deck
                Deck fullDeck = network.fetchDeck(availableDecks[selectedDeckIndex].id);
                
                if (fullDeck.cards.empty()) {
                    uiMgr.showError("Empty Deck!");
                    delay(2000);
                    state = FC_SELECT_DECK;
                    needsFullRedraw = true;
                } else {
                    currentDeck = fullDeck;
                    state = FC_SHOW_FRONT;
                    currentCardIndex = 0;
                    sessionStartTime = millis();
                    needsFullRedraw = true;
                }
            }
            break;

        case FC_SHOW_FRONT:
            {
                if (needsFullRedraw) {
                    uiMgr.showFlashcardFront(
                        currentDeck.cards[currentCardIndex].front.c_str(),
                        currentCardIndex + 1,
                        currentDeck.cards.size()
                    );
                    
                    // Update OLED
                    char status[20];
                    sprintf(status, "Card %d/%d", currentCardIndex + 1, currentDeck.cards.size());
                    display.showStatus(status);
                    
                    needsFullRedraw = false;
                }
                
                // Press A to flip
                if (input.isBtnAPressed()) {
                    state = FC_SHOW_BACK;
                    needsFullRedraw = true;
                    delay(200);
                }
                
                // Long press detection for D button (pause menu)
                bool btnDPressed = input.isBtnDPressed();
                if (btnDPressed && !btnDWasPressed) {
                    btnDPressStart = millis();
                    btnDWasPressed = true;
                } else if (btnDPressed && btnDWasPressed) {
                    if (millis() - btnDPressStart >= LONG_PRESS_MS) {
                        state = FC_PAUSED;
                        pauseMenuIndex = 0;
                        lastPauseMenuIndex = -1;
                        needsFullRedraw = true;
                        btnDWasPressed = false;
                        delay(200);
                        return;
                    }
                } else if (!btnDPressed && btnDWasPressed) {
                    btnDWasPressed = false;
                }
            }
            break;

        case FC_SHOW_BACK:
            {
                if (needsFullRedraw) {
                    uiMgr.showFlashcardBack(
                        currentDeck.cards[currentCardIndex].front.c_str(),
                        currentDeck.cards[currentCardIndex].back.c_str()
                    );
                    display.showStatus("Rate Difficulty");
                    needsFullRedraw = false;
                }
                
                // Long press detection for D button (pause menu)
                bool btnDPressed = input.isBtnDPressed();
                if (btnDPressed && !btnDWasPressed) {
                    btnDPressStart = millis();
                    btnDWasPressed = true;
                } else if (btnDPressed && btnDWasPressed) {
                    if (millis() - btnDPressStart >= LONG_PRESS_MS) {
                        state = FC_PAUSED;
                        pauseMenuIndex = 0;
                        lastPauseMenuIndex = -1;
                        needsFullRedraw = true;
                        btnDWasPressed = false;
                        delay(200);
                        return;
                    }
                } else if (!btnDPressed && btnDWasPressed) {
                    // Short press D - Easy (4)
                    if (millis() - btnDPressStart < LONG_PRESS_MS) {
                        currentDeck.cards[currentCardIndex].rating = 4;
                        currentCardIndex++;
                        if (currentCardIndex >= (int)currentDeck.cards.size()) {
                            state = FC_FINISHED;
                        } else {
                            state = FC_SHOW_FRONT;
                        }
                        needsFullRedraw = true;
                        delay(200);
                    }
                    btnDWasPressed = false;
                }
                
                // Rating buttons: A, B, C (D is handled above)
                int rating = 0;
                if (input.isBtnAPressed()) rating = 1;
                else if (input.isBtnBPressed()) rating = 2;
                else if (input.isBtnCPressed()) rating = 3;
                
                if (rating > 0) {
                    currentDeck.cards[currentCardIndex].rating = rating;
                    
                    currentCardIndex++;
                    if (currentCardIndex >= (int)currentDeck.cards.size()) {
                        state = FC_FINISHED;
                    } else {
                        state = FC_SHOW_FRONT;
                    }
                    needsFullRedraw = true;
                    delay(200);
                }
            }
            break;
            
        case FC_PAUSED:
            {
                // Keep LVGL responsive
                lv_timer_handler();
                display.showStatus("PAUSED");
                
                if (needsFullRedraw || pauseMenuIndex != lastPauseMenuIndex) {
                    uiMgr.showFlashcardPauseMenu(pauseMenuIndex);
                    lastPauseMenuIndex = pauseMenuIndex;
                    needsFullRedraw = false;
                }
                
                // Navigation
                if (input.isBtnCPressed()) {
                    if (pauseMenuIndex > 0) pauseMenuIndex--;
                    delay(150);
                }
                if (input.isBtnDPressed()) {
                    if (pauseMenuIndex < 1) pauseMenuIndex++;
                    delay(150);
                }
                
                // Select
                if (input.isBtnAPressed()) {
                    if (pauseMenuIndex == 0) {
                        // Resume
                        state = FC_SHOW_FRONT; // Or back, ideally remember previous state
                        // For simplicity, go to front of current card
                        needsFullRedraw = true;
                        delay(200);
                    } else {
                        // Exit
                        state = FC_SELECT_DECK;
                        needsFullRedraw = true;
                        delay(200);
                    }
                }
            }
            break;

        case FC_FINISHED:
            {
                if (needsFullRedraw) {
                    // Calculate stats
                    int easy = 0, hard = 0, again = 0, good = 0;
                    for (const auto& card : currentDeck.cards) {
                        if (card.rating == 4) easy++;
                        else if (card.rating == 3) good++;
                        else if (card.rating == 2) hard++;
                        else again++;
                    }
                    
                    uiMgr.showFlashcardFinished(currentDeck.cards.size(), easy, hard, again);
                    display.showStatus("Deck Complete!");
                    needsFullRedraw = false;
                }
                
                if (input.isBtnAPressed() || input.isBtnBPressed()) {
                    state = FC_SELECT_DECK;
                    needsFullRedraw = true;
                    delay(200);
                }
            }
            break;
    }
}
