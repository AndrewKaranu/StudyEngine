/**
 * TranscriptEngine - Voice Transcript Integration Mode
 * Fetches transcripts from external API and generates study materials
 */

#ifndef TRANSCRIPT_ENGINE_H
#define TRANSCRIPT_ENGINE_H

#include <Arduino.h>
#include <vector>
#include "config.h"
#include "DisplayManager.h"
#include "InputManager.h"
#include "NetworkManager.h"
#include "UIManager.h"

// Transcript data structure
struct Transcript {
    String id;
    String title;
    String date;
    String content;      // Full transcript text
    int durationSeconds; // Audio duration
};

// Generation options
enum GenerationType {
    GEN_QUIZ,
    GEN_FLASHCARDS
};

// Engine states
enum TranscriptState {
    TRANS_INIT,
    TRANS_SELECT,
    TRANS_VIEW,
    TRANS_OPTIONS,
    TRANS_GENERATING,
    TRANS_SUCCESS,
    TRANS_ERROR
};

class TranscriptEngine {
public:
    void reset();
    void handleRun(DisplayManager& display, InputManager& input, SENetworkManager& network, int& systemState);
    
    // Get generated content for handoff to other engines
    String getGeneratedQuizId() { return generatedQuizId; }
    String getGeneratedDeckId() { return generatedDeckId; }
    bool hasGeneratedQuiz() { return !generatedQuizId.isEmpty(); }
    bool hasGeneratedDeck() { return !generatedDeckId.isEmpty(); }
    void clearGenerated() { generatedQuizId = ""; generatedDeckId = ""; }
    
private:
    TranscriptState state = TRANS_INIT;
    
    std::vector<Transcript> availableTranscripts;
    int selectedTranscriptIndex = 0;
    int lastSelectedIndex = -1;
    
    int optionIndex = 0;        // 0 = Generate Quiz, 1 = Generate Flashcards, 2 = Back
    int lastOptionIndex = -1;
    
    bool needsFullRedraw = true;
    
    // Generated content IDs (for handoff)
    String generatedQuizId;
    String generatedDeckId;
    
    // Sample data (will be replaced with API call)
    std::vector<Transcript> fetchTranscriptList();
    Transcript fetchTranscript(const String& id);
    
    // Generation (calls backend API)
    bool generateQuizFromTranscript(SENetworkManager& network, const Transcript& transcript);
    bool generateFlashcardsFromTranscript(SENetworkManager& network, const Transcript& transcript);
    bool pollGenerationJob(SENetworkManager& network, const String& jobId, bool isQuiz);
};

// Global instance
extern TranscriptEngine transcriptEngine;

#endif
