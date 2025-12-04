/**
 * Settings Manager Implementation
 * Persistent storage using ESP32 Preferences
 */

#include "SettingsManager.h"

// Global instance
SettingsManager settingsMgr;

void SettingsManager::begin() {
    prefs.begin("studyengine", false);  // false = read/write mode
    
    // Load cached settings
    cachedApiUrl = prefs.getString("apiUrl", DEFAULT_API_URL);
    serialDebug = prefs.getBool("serialDbg", true);
    showFPS = prefs.getBool("showFPS", false);
    verboseNetwork = prefs.getBool("verboseNet", false);
    speakerMuted = prefs.getBool("speakerMute", false);
    
    Serial.println("[SETTINGS] Loaded preferences:");
    Serial.printf("  API URL: %s\n", cachedApiUrl.c_str());
    Serial.printf("  Serial Debug: %s\n", serialDebug ? "ON" : "OFF");
    Serial.printf("  Show FPS: %s\n", showFPS ? "ON" : "OFF");
    Serial.printf("  Verbose Network: %s\n", verboseNetwork ? "ON" : "OFF");
    Serial.printf("  Speaker Muted: %s\n", speakerMuted ? "YES" : "NO");
}

// API URL
String SettingsManager::getApiBaseUrl() {
    return cachedApiUrl;
}

void SettingsManager::setApiBaseUrl(const String& url) {
    cachedApiUrl = url;
    prefs.putString("apiUrl", url);
    Serial.printf("[SETTINGS] API URL saved: %s\n", url.c_str());
}

void SettingsManager::resetApiBaseUrl() {
    cachedApiUrl = DEFAULT_API_URL;
    prefs.putString("apiUrl", DEFAULT_API_URL);
    Serial.printf("[SETTINGS] API URL reset to default: %s\n", DEFAULT_API_URL);
}

// Serial Debug
bool SettingsManager::getSerialDebug() {
    return serialDebug;
}

void SettingsManager::setSerialDebug(bool enabled) {
    serialDebug = enabled;
    prefs.putBool("serialDbg", enabled);
}

// Show FPS
bool SettingsManager::getShowFPS() {
    return showFPS;
}

void SettingsManager::setShowFPS(bool enabled) {
    showFPS = enabled;
    prefs.putBool("showFPS", enabled);
}

// Verbose Network
bool SettingsManager::getVerboseNetwork() {
    return verboseNetwork;
}

void SettingsManager::setVerboseNetwork(bool enabled) {
    verboseNetwork = enabled;
    prefs.putBool("verboseNet", enabled);
}

// Speaker Mute
bool SettingsManager::getSpeakerMuted() {
    return speakerMuted;
}

void SettingsManager::setSpeakerMuted(bool muted) {
    speakerMuted = muted;
    prefs.putBool("speakerMute", muted);
}
