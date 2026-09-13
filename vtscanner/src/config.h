#pragma once
#include <string>

struct AppConfig {
    std::string apiKey;
    bool topmost = false;
    bool thanksShown = false;
    std::string apiType = "free";   // "free" | "paid"
    int  requestsUsed = 0;
    std::string requestsResetDate;  // "YYYY-MM-DD"
    std::string lang = "ru";        // "ru" | "en"
    std::string theme = "dark";     // "dark" | "light"
    bool soundOnComplete = true;
    bool closeToTray = false;       // minimize to system tray instead of exiting on close
    bool autostart = false;         // launch automatically with Windows
};

// Returns the folder used to store settings (%LOCALAPPDATA%\VirusTotalScanner on Windows).
std::wstring GetConfigDir();
std::wstring GetConfigFilePath();

AppConfig LoadConfig();
bool SaveConfig(const AppConfig& cfg);

// Adds/removes the current executable from HKCU\...\Run so it starts with Windows.
// Returns true on success.
bool SetAutostart(bool enable);
