#include "NetworkManager.h"

void SENetworkManager::connect() {
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        connected = true;
        Serial.println("\nWiFi Connected!");
    } else {
        Serial.println("\nWiFi Connection Failed.");
    }
}

bool SENetworkManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

std::vector<ExamMetadata> SENetworkManager::fetchExamList() {
    std::vector<ExamMetadata> exams;
    if (!isConnected()) {
        Serial.println("[NET] Not connected to WiFi");
        return exams;
    }

    HTTPClient http;
    String url = String(API_BASE_URL) + "/exams";
    Serial.println("[NET] Fetching exam list...");
    
    WiFiClient client;
    http.begin(client, url);
    http.setTimeout(10000);
    
    int httpCode = http.GET();
    Serial.printf("[NET] Response: %d\n", httpCode);

    if (httpCode == 200) {
        String payload = http.getString();
        Serial.print("[NET] Response: ");
        Serial.println(payload);
        
        DynamicJsonDocument doc(4096);
        DeserializationError error = deserializeJson(doc, payload);
        
        if (error) {
            Serial.print("[NET] JSON Parse Error: ");
            Serial.println(error.c_str());
        } else {
            JsonArray arr = doc.as<JsonArray>();
            for (JsonObject obj : arr) {
                ExamMetadata meta;
                meta.id = obj["id"].as<String>();
                meta.title = obj["title"].as<String>();
                exams.push_back(meta);
                Serial.print("[NET] Found exam: ");
                Serial.println(meta.title);
            }
        }
    } else if (httpCode < 0) {
        Serial.print("[NET] Connection failed: ");
        Serial.println(http.errorToString(httpCode));
    }
    http.end();
    return exams;
}

String SENetworkManager::fetchExamJson(String examId) {
    if (!isConnected()) return "";

    Serial.println("[NET] Downloading exam...");
    
    WiFiClient client;
    HTTPClient http;
    http.begin(client, String(API_BASE_URL) + "/exams/" + examId);
    http.setTimeout(10000);
    
    int httpCode = http.GET();
    
    String payload = "";
    if (httpCode == 200) {
        payload = http.getString();
        Serial.println("[NET] Exam downloaded");
    } else {
        Serial.printf("[NET] Download failed: %d\n", httpCode);
    }
    http.end();
    return payload;
}

bool SENetworkManager::uploadResult(String jsonPayload) {
    if (!isConnected()) return false;

    WiFiClient client;
    HTTPClient http;
    http.begin(client, String(API_BASE_URL) + "/results");
    http.addHeader("Content-Type", "application/json");
    
    int httpCode = http.POST(jsonPayload);
    http.end();
    
    return (httpCode == 200 || httpCode == 201);
}

std::vector<Deck> SENetworkManager::fetchDeckList() {
    std::vector<Deck> decks;
    if (!isConnected()) return decks;

    WiFiClient client;
    HTTPClient http;
    http.begin(client, String(API_BASE_URL) + "/decks");
    int httpCode = http.GET();

    if (httpCode == 200) {
        String payload = http.getString();
        DynamicJsonDocument doc(4096);
        deserializeJson(doc, payload);
        JsonArray arr = doc.as<JsonArray>();
        for (JsonObject obj : arr) {
            Deck d;
            d.id = obj["id"].as<String>();
            d.title = obj["title"].as<String>();
            decks.push_back(d);
        }
    }
    http.end();
    return decks;
}

Deck SENetworkManager::fetchDeck(String deckId) {
    Deck deck;
    if (!isConnected()) return deck;

    WiFiClient client;
    HTTPClient http;
    http.begin(client, String(API_BASE_URL) + "/decks/" + deckId);
    int httpCode = http.GET();

    if (httpCode == 200) {
        String payload = http.getString();
        DynamicJsonDocument doc(16384); // Increase buffer for larger decks
        deserializeJson(doc, payload);
        
        deck.id = doc["id"].as<String>();
        deck.title = doc["title"].as<String>();
        
        JsonArray cards = doc["cards"];
        for (JsonObject c : cards) {
            Flashcard f;
            f.front = c["front"].as<String>();
            f.back = c["back"].as<String>();
            f.rating = 0;
            deck.cards.push_back(f);
        }
    }
    http.end();
    return deck;
}
