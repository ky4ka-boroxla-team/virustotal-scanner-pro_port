#pragma once
#include <string>

struct AppConfig {
    std::string apiKey;
    bool topmost = false;
    bool thanksShown = false;
    std::string apiType = "free";
    int  requestsUsed = 0;
    std::string requestsResetDate;
    std::string lang = "ru";
    std::string theme = "dark";
    bool soundOnComplete = true;
    bool closeToTray = false;   // hide to tray instead of quitting on window close
    bool autostart = false;     // launch automatically on login
};

std::string GetConfigDir();
std::string GetConfigFilePath();
AppConfig LoadConfig();
bool SaveConfig(const AppConfig& cfg);

// Adds/removes a freedesktop.org autostart entry in ~/.config/autostart so the app
// launches on login. Returns true on success.
bool SetAutostart(bool enable);