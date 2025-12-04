/**
 * Settings Manager - Persistent storage for user preferences
 * Uses ESP32 Preferences library for non-volatile storage
 */

#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

class SettingsManager {
public:
    void begin();
    
    // API URL management
    String getApiBaseUrl();
    void setApiBaseUrl(const String& url);
    void resetApiBaseUrl();
    
    // Dev Mode settings
    bool getSerialDebug();
    void setSerialDebug(bool enabled);
    
    bool getShowFPS();
    void setShowFPS(bool enabled);
    
    bool getVerboseNetwork();
    void setVerboseNetwork(bool enabled);
    
    // Speaker mute
    bool getSpeakerMuted();
    void setSpeakerMuted(bool muted);
    
private:
    Preferences prefs;
    String cachedApiUrl;
    bool serialDebug = true;
    bool showFPS = false;
    bool verboseNetwork = false;
    bool speakerMuted = false;
};

// Global instance
extern SettingsManager settingsMgr;

#endif
