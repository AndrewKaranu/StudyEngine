/**
 * TranscriptEngine Implementation
 * Voice Transcript Integration with Quiz/Flashcard Generation
 */

#include "TranscriptEngine.h"
#include "SettingsManager.h"
#include <Wire.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// External feedback functions
extern void beepClick();
extern void beepSuccess();
extern void beepError();
extern void beepComplete();
extern void setLed(bool red, bool green);
extern void ledOff();
extern void flashLed(bool red, bool green, int count, int onTime, int offTime);

// Global instance
TranscriptEngine transcriptEngine;

void TranscriptEngine::reset() {
    state = TRANS_INIT;
    selectedTranscriptIndex = 0;
    lastSelectedIndex = -1;
    optionIndex = 0;
    lastOptionIndex = -1;
    needsFullRedraw = true;
    availableTranscripts.clear();
    generatedQuizId = "";
    generatedDeckId = "";
    Serial.println("[TRANSCRIPT] Engine reset");
}

void TranscriptEngine::handleRun(DisplayManager& display, InputManager& input, SENetworkManager& network, int& systemState) {
    switch (state) {
        case TRANS_INIT:
            {
                uiMgr.showLoading("Fetching Transcripts...");
                display.showStatus("Loading...");
                
                // Fetch transcript list (sample data for now)
                availableTranscripts = fetchTranscriptList();
                
                if (availableTranscripts.empty()) {
                    uiMgr.showError("No Transcripts Found!");
                    delay(2000);
                    systemState = 0; // Back to menu
                } else {
                    state = TRANS_SELECT;
                    lastSelectedIndex = -1;
                    needsFullRedraw = true;
                    Serial.printf("[TRANSCRIPT] Found %d transcripts\n", availableTranscripts.size());
                }
            }
            break;

        case TRANS_SELECT:
            {
                // Map pot to transcript selection
                int newIndex = input.getScrollIndex(availableTranscripts.size());
                
                if (newIndex != selectedTranscriptIndex || needsFullRedraw) {
                    selectedTranscriptIndex = newIndex;
                    
                    // Build list for display
                    std::vector<const char*> titles;
                    std::vector<const char*> dates;
                    for (const auto& t : availableTranscripts) {
                        titles.push_back(t.title.c_str());
                        dates.push_back(t.date.c_str());
                    }
                    
                    uiMgr.showTranscriptList(titles.data(), dates.data(), 
                                             availableTranscripts.size(), 
                                             selectedTranscriptIndex);
                    display.showStatus("Select Transcript");
                    
                    lastSelectedIndex = selectedTranscriptIndex;
                    needsFullRedraw = false;
                }
                
                // A to select
                if (input.isBtnAPressed()) {
                    beepClick();
                    state = TRANS_OPTIONS;
                    optionIndex = 0;
                    lastOptionIndex = -1;
                    needsFullRedraw = true;
                    delay(200);
                }
                
                // B to go back
                if (input.isBtnBPressed()) {
                    beepClick();
                    systemState = 0; // Back to menu
                    delay(200);
                }
            }
            break;

        case TRANS_VIEW:
            {
                // View full transcript content
                if (needsFullRedraw) {
                    Transcript& t = availableTranscripts[selectedTranscriptIndex];
                    uiMgr.showTranscriptContent(t.title.c_str(), t.content.c_str());
                    display.showStatus("Transcript");
                    needsFullRedraw = false;
                }
                
                // B to go back to options
                if (input.isBtnBPressed()) {
                    beepClick();
                    state = TRANS_OPTIONS;
                    needsFullRedraw = true;
                    delay(200);
                }
            }
            break;

        case TRANS_OPTIONS:
            {
                // Options: Generate Quiz, Generate Flashcards, View Transcript, Back
                int optionCount = 4;
                int newOption = input.getScrollIndex(optionCount);
                
                if (newOption != optionIndex || needsFullRedraw) {
                    optionIndex = newOption;
                    
                    Transcript& t = availableTranscripts[selectedTranscriptIndex];
                    uiMgr.showTranscriptOptions(t.title.c_str(), optionIndex);
                    display.showStatus("Options");
                    
                    lastOptionIndex = optionIndex;
                    needsFullRedraw = false;
                }
                
                // A to select option
                if (input.isBtnAPressed()) {
                    beepClick();
                    
                    if (optionIndex == 0) {
                        // Generate Quiz
                        state = TRANS_GENERATING;
                        needsFullRedraw = true;
                        
                        // Show generating screen
                        uiMgr.showLoading("Generating Quiz...");
                        display.showStatus("AI Generating...");
                        setLed(false, true); // Green while generating
                        
                        Transcript& t = availableTranscripts[selectedTranscriptIndex];
                        bool success = generateQuizFromTranscript(network, t);
                        
                        ledOff();
                        
                        if (success) {
                            state = TRANS_SUCCESS;
                            beepComplete();
                            flashLed(false, true, 3, 100, 80);
                        } else {
                            state = TRANS_ERROR;
                            beepError();
                            flashLed(true, false, 2, 150, 100);
                        }
                        needsFullRedraw = true;
                        
                    } else if (optionIndex == 1) {
                        // Generate Flashcards
                        state = TRANS_GENERATING;
                        needsFullRedraw = true;
                        
                        uiMgr.showLoading("Generating Flashcards...");
                        display.showStatus("AI Generating...");
                        setLed(false, true);
                        
                        Transcript& t = availableTranscripts[selectedTranscriptIndex];
                        bool success = generateFlashcardsFromTranscript(network, t);
                        
                        ledOff();
                        
                        if (success) {
                            state = TRANS_SUCCESS;
                            beepComplete();
                            flashLed(false, true, 3, 100, 80);
                        } else {
                            state = TRANS_ERROR;
                            beepError();
                            flashLed(true, false, 2, 150, 100);
                        }
                        needsFullRedraw = true;
                        
                    } else if (optionIndex == 2) {
                        // View Transcript
                        state = TRANS_VIEW;
                        needsFullRedraw = true;
                        
                    } else if (optionIndex == 3) {
                        // Back to list
                        state = TRANS_SELECT;
                        needsFullRedraw = true;
                    }
                    delay(200);
                }
                
                // B to go back
                if (input.isBtnBPressed()) {
                    beepClick();
                    state = TRANS_SELECT;
                    needsFullRedraw = true;
                    delay(200);
                }
            }
            break;

        case TRANS_GENERATING:
            // This state is transitory - handled in TRANS_OPTIONS
            break;

        case TRANS_SUCCESS:
            {
                if (needsFullRedraw) {
                    String message;
                    if (!generatedQuizId.isEmpty()) {
                        message = "Quiz generated!\n\nGo to Quiz Mode\nto study it.";
                    } else if (!generatedDeckId.isEmpty()) {
                        message = "Flashcards generated!\n\nGo to Flashcards\nto study them.";
                    }
                    uiMgr.showSuccess("Generation Complete!", message.c_str());
                    display.showStatus("Success!");
                    needsFullRedraw = false;
                }
                
                // Any button to continue - go back to options for this transcript
                if (input.isBtnAPressed() || input.isBtnBPressed()) {
                    beepClick();
                    state = TRANS_OPTIONS;
                    needsFullRedraw = true;
                    delay(200);
                }
            }
            break;

        case TRANS_ERROR:
            {
                if (needsFullRedraw) {
                    uiMgr.showError("Generation Failed!\n\nPlease try again.");
                    display.showStatus("Error");
                    needsFullRedraw = false;
                }
                
                // Any button to continue
                if (input.isBtnAPressed() || input.isBtnBPressed()) {
                    beepClick();
                    state = TRANS_OPTIONS;
                    needsFullRedraw = true;
                    delay(200);
                }
            }
            break;
    }
}

// ===================================================================================
// SAMPLE DATA
// ===================================================================================

std::vector<Transcript> TranscriptEngine::fetchTranscriptList() {
    // Sample transcripts - replace with API call to friend's service
    std::vector<Transcript> transcripts;
    
    // Sample 1: CS Lecture
    Transcript t1;
    t1.id = "trans_001";
    t1.title = "Data Structures Lecture";
    t1.date = "2024-12-01";
    t1.durationSeconds = 2700; // 45 min
    t1.content = "Today we're going to discuss binary search trees. A binary search tree is a data structure that maintains sorted data in a way that allows for efficient insertion, deletion, and lookup operations. Each node in the tree has at most two children, referred to as the left child and right child. The key property is that for any node, all keys in its left subtree are less than the node's key, and all keys in its right subtree are greater. This property enables binary search, giving us O(log n) average time complexity for operations. However, in the worst case, if the tree becomes unbalanced, operations can degrade to O(n). This is why balanced tree variants like AVL trees and Red-Black trees were developed.";
    transcripts.push_back(t1);
    
    // Sample 2: Physics
    Transcript t2;
    t2.id = "trans_002";
    t2.title = "Quantum Mechanics Intro";
    t2.date = "2024-11-28";
    t2.durationSeconds = 3600; // 60 min
    t2.content = "Quantum mechanics is the fundamental theory describing nature at the smallest scales. Unlike classical mechanics, quantum mechanics introduces the concept of wave-particle duality, where particles like electrons exhibit both wave-like and particle-like properties. The Heisenberg uncertainty principle states that we cannot simultaneously know both the exact position and momentum of a particle with arbitrary precision. The wave function, denoted by psi, contains all information about a quantum system and evolves according to the Schrodinger equation. When we make a measurement, the wave function collapses to an eigenstate of the measured observable. Superposition allows quantum systems to exist in multiple states simultaneously until observed.";
    transcripts.push_back(t2);
    
    // Sample 3: History
    Transcript t3;
    t3.id = "trans_003";
    t3.title = "World War II Overview";
    t3.date = "2024-11-25";
    t3.durationSeconds = 4200; // 70 min
    t3.content = "World War II began in 1939 when Nazi Germany invaded Poland. The war involved most of the world's nations forming two opposing military alliances: the Allies and the Axis powers. Major events include the Battle of Britain in 1940, Operation Barbarossa in 1941, the attack on Pearl Harbor which brought the United States into the war, D-Day on June 6 1944, and the atomic bombings of Hiroshima and Nagasaki in 1945. The war ended with Germany's surrender in May 1945 and Japan's surrender in September 1945. An estimated 70-85 million people perished, making it the deadliest conflict in human history. The war led to the formation of the United Nations and set the stage for the Cold War.";
    transcripts.push_back(t3);
    
    // Sample 4: Biology
    Transcript t4;
    t4.id = "trans_004";
    t4.title = "Cell Biology Basics";
    t4.date = "2024-11-20";
    t4.durationSeconds = 2400; // 40 min
    t4.content = "The cell is the basic unit of life. All living organisms are composed of one or more cells. There are two main types: prokaryotic cells, found in bacteria and archaea, which lack a nucleus, and eukaryotic cells, found in plants, animals, and fungi, which have a membrane-bound nucleus. Key organelles in eukaryotic cells include the mitochondria, which produce ATP through cellular respiration, the endoplasmic reticulum for protein synthesis and lipid metabolism, the Golgi apparatus for protein modification and transport, and ribosomes where translation occurs. The cell membrane is a phospholipid bilayer that regulates what enters and exits the cell.";
    transcripts.push_back(t4);
    
    Serial.printf("[TRANSCRIPT] Loaded %d sample transcripts\n", transcripts.size());
    return transcripts;
}

Transcript TranscriptEngine::fetchTranscript(const String& id) {
    // Find transcript by ID
    for (const auto& t : availableTranscripts) {
        if (t.id == id) {
            return t;
        }
    }
    // Return empty if not found
    Transcript empty;
    return empty;
}

// ===================================================================================
// GENERATION (Calls backend API)
// ===================================================================================

bool TranscriptEngine::generateQuizFromTranscript(SENetworkManager& network, const Transcript& transcript) {
    Serial.printf("[TRANSCRIPT] Generating quiz from: %s\n", transcript.title.c_str());
    
    if (!network.isConnected()) {
        Serial.println("[TRANSCRIPT] Not connected to WiFi");
        return false;
    }
    
    HTTPClient http;
    WiFiClient client;
    String url = settingsMgr.getApiBaseUrl() + "/generate/transcript/quiz";
    
    Serial.printf("[TRANSCRIPT] POST to: %s\n", url.c_str());
    
    if (!http.begin(client, url)) {
        Serial.println("[TRANSCRIPT] HTTP begin failed");
        return false;
    }
    
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(60000); // 60 second timeout for AI generation
    
    // Build JSON request
    DynamicJsonDocument requestDoc(8192);
    requestDoc["transcript_id"] = transcript.id;
    requestDoc["transcript_content"] = transcript.content;
    requestDoc["title"] = transcript.title + " Quiz";
    requestDoc["model"] = "haiku";
    requestDoc["num_mcq"] = 5;
    requestDoc["num_short_answer"] = 2;
    
    String requestBody;
    serializeJson(requestDoc, requestBody);
    
    Serial.printf("[TRANSCRIPT] Request size: %d bytes\n", requestBody.length());
    
    int httpCode = http.POST(requestBody);
    Serial.printf("[TRANSCRIPT] Response code: %d\n", httpCode);
    
    if (httpCode != 200) {
        Serial.printf("[TRANSCRIPT] HTTP error: %s\n", http.errorToString(httpCode).c_str());
        http.end();
        return false;
    }
    
    String response = http.getString();
    http.end();
    
    // Parse response to get job_id
    DynamicJsonDocument responseDoc(1024);
    DeserializationError error = deserializeJson(responseDoc, response);
    if (error) {
        Serial.printf("[TRANSCRIPT] JSON parse error: %s\n", error.c_str());
        return false;
    }
    
    String jobId = responseDoc["job_id"].as<String>();
    Serial.printf("[TRANSCRIPT] Job started: %s\n", jobId.c_str());
    
    // Poll for completion
    return pollGenerationJob(network, jobId, true);
}

bool TranscriptEngine::generateFlashcardsFromTranscript(SENetworkManager& network, const Transcript& transcript) {
    Serial.printf("[TRANSCRIPT] Generating flashcards from: %s\n", transcript.title.c_str());
    
    if (!network.isConnected()) {
        Serial.println("[TRANSCRIPT] Not connected to WiFi");
        return false;
    }
    
    HTTPClient http;
    WiFiClient client;
    String url = settingsMgr.getApiBaseUrl() + "/generate/transcript/flashcards";
    
    Serial.printf("[TRANSCRIPT] POST to: %s\n", url.c_str());
    
    if (!http.begin(client, url)) {
        Serial.println("[TRANSCRIPT] HTTP begin failed");
        return false;
    }
    
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(60000); // 60 second timeout for AI generation
    
    // Build JSON request
    DynamicJsonDocument requestDoc(8192);
    requestDoc["transcript_id"] = transcript.id;
    requestDoc["transcript_content"] = transcript.content;
    requestDoc["title"] = transcript.title + " Flashcards";
    requestDoc["model"] = "haiku";
    requestDoc["num_flashcards"] = 10;
    
    String requestBody;
    serializeJson(requestDoc, requestBody);
    
    Serial.printf("[TRANSCRIPT] Request size: %d bytes\n", requestBody.length());
    
    int httpCode = http.POST(requestBody);
    Serial.printf("[TRANSCRIPT] Response code: %d\n", httpCode);
    
    if (httpCode != 200) {
        Serial.printf("[TRANSCRIPT] HTTP error: %s\n", http.errorToString(httpCode).c_str());
        http.end();
        return false;
    }
    
    String response = http.getString();
    http.end();
    
    // Parse response to get job_id
    DynamicJsonDocument responseDoc(1024);
    DeserializationError error = deserializeJson(responseDoc, response);
    if (error) {
        Serial.printf("[TRANSCRIPT] JSON parse error: %s\n", error.c_str());
        return false;
    }
    
    String jobId = responseDoc["job_id"].as<String>();
    Serial.printf("[TRANSCRIPT] Job started: %s\n", jobId.c_str());
    
    // Poll for completion
    return pollGenerationJob(network, jobId, false);
}

bool TranscriptEngine::pollGenerationJob(SENetworkManager& network, const String& jobId, bool isQuiz) {
    Serial.printf("[TRANSCRIPT] Polling job: %s\n", jobId.c_str());
    
    HTTPClient http;
    WiFiClient client;
    String statusUrl = settingsMgr.getApiBaseUrl() + "/generate/status/" + jobId;
    
    int maxAttempts = 60;  // Max 60 attempts (about 60 seconds)
    
    for (int attempt = 0; attempt < maxAttempts; attempt++) {
        if (!http.begin(client, statusUrl)) {
            Serial.println("[TRANSCRIPT] HTTP begin failed");
            delay(1000);
            continue;
        }
        
        http.setTimeout(10000);
        int httpCode = http.GET();
        
        if (httpCode != 200) {
            http.end();
            delay(1000);
            continue;
        }
        
        String response = http.getString();
        http.end();
        
        DynamicJsonDocument doc(4096);
        DeserializationError error = deserializeJson(doc, response);
        if (error) {
            delay(1000);
            continue;
        }
        
        String status = doc["status"].as<String>();
        Serial.printf("[TRANSCRIPT] Job status: %s (attempt %d)\n", status.c_str(), attempt + 1);
        
        if (status == "completed") {
            // Job completed successfully - save to database
            String saveUrl = settingsMgr.getApiBaseUrl() + "/generate/save/" + jobId;
            
            HTTPClient saveHttp;
            if (saveHttp.begin(client, saveUrl)) {
                saveHttp.setTimeout(10000);
                int saveCode = saveHttp.POST("");
                
                if (saveCode == 200) {
                    String saveResponse = saveHttp.getString();
                    DynamicJsonDocument saveDoc(512);
                    deserializeJson(saveDoc, saveResponse);
                    
                    String generatedId = saveDoc["id"].as<String>();
                    
                    if (isQuiz) {
                        generatedQuizId = generatedId;
                        generatedDeckId = "";
                    } else {
                        generatedDeckId = generatedId;
                        generatedQuizId = "";
                    }
                    
                    Serial.printf("[TRANSCRIPT] Saved with ID: %s\n", generatedId.c_str());
                    saveHttp.end();
                    return true;
                }
                saveHttp.end();
            }
            return false;
        }
        else if (status == "failed") {
            String errorMsg = doc["error"].as<String>();
            Serial.printf("[TRANSCRIPT] Generation failed: %s\n", errorMsg.c_str());
            return false;
        }
        
        // Still processing, wait and try again
        delay(1000);
    }
    
    Serial.println("[TRANSCRIPT] Timeout waiting for generation");
    return false;
}
