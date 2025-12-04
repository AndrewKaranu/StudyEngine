#ifndef SE_NETWORK_MANAGER_H
#define SE_NETWORK_MANAGER_H

#include "config.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

struct ExamMetadata {
    String id;
    String title;
};

struct Flashcard {
    String front;
    String back;
    int rating; // 0=None, 1=Again, 2=Hard, 3=Good, 4=Easy
};

struct Deck {
    String id;
    String title;
    std::vector<Flashcard> cards;
};

struct QuizQuestion {
    int id;
    String type; // "mcq" or "short_answer"
    String text;
    std::vector<String> options;
    String correctAnswer;
};

struct Quiz {
    String id;
    String title;
    std::vector<QuizQuestion> questions;
};

class SENetworkManager {
private:
    bool connected = false;

public:
    void connect();
    bool isConnected();
    
    // API Calls
    std::vector<ExamMetadata> fetchExamList();
    String fetchExamJson(String examId);
    bool uploadResult(String jsonPayload);
    
    // Flashcard API
    std::vector<Deck> fetchDeckList();
    Deck fetchDeck(String deckId);
    
    // Quiz API
    std::vector<Quiz> fetchQuizList();
    Quiz fetchQuiz(String quizId);

    String getApiBaseUrl() { return API_BASE_URL; }
};

#endif
