#include "config.h"
#include <fstream>
#include <sstream>
#include <ctime>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::string GetConfigDir() {
    const char* home = getenv("HOME");
    if (!home) home = ".";
    std::string dir = std::string(home) + "/.config/VirusTotalScanner";
    mkdir(dir.c_str(), 0700);
    return dir;
}

std::string GetConfigFilePath() {
    return GetConfigDir() + "/vt_config.json";
}

static std::string TodayString() {
    std::time_t t = std::time(nullptr);
    std::tm tmv{};
    localtime_r(&t, &tmv);
    char buf[16];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday);
    return buf;
}

AppConfig LoadConfig() {
    AppConfig cfg;
    cfg.requestsResetDate = "";
    std::string path = GetConfigFilePath();
    std::ifstream f(path);
    if (f.is_open()) {
        try {
            json j;
            f >> j;
            cfg.apiKey = j.value("api_key", cfg.apiKey);
            cfg.topmost = j.value("topmost", cfg.topmost);
            cfg.thanksShown = j.value("thanks_shown", cfg.thanksShown);
            cfg.apiType = j.value("api_type", cfg.apiType);
            cfg.requestsUsed = j.value("requests_used", cfg.requestsUsed);
            cfg.requestsResetDate = j.value("requests_reset_date", cfg.requestsResetDate);
            cfg.lang = j.value("lang", cfg.lang);
            cfg.theme = j.value("theme", cfg.theme);
            cfg.soundOnComplete = j.value("sound_on_complete", cfg.soundOnComplete);
            cfg.closeToTray = j.value("close_to_tray", cfg.closeToTray);
            cfg.autostart = j.value("autostart", cfg.autostart);
        } catch (...) {}
    }
    std::string today = TodayString();
    if (cfg.requestsResetDate != today) {
        cfg.requestsUsed = 0;
        cfg.requestsResetDate = today;
        SaveConfig(cfg);
    }
    return cfg;
}

bool SaveConfig(const AppConfig& cfg) {
    json j;
    j["api_key"] = cfg.apiKey;
    j["topmost"] = cfg.topmost;
    j["thanks_shown"] = cfg.thanksShown;
    j["api_type"] = cfg.apiType;
    j["requests_used"] = cfg.requestsUsed;
    j["requests_reset_date"] = cfg.requestsResetDate;
    j["lang"] = cfg.lang;
    j["theme"] = cfg.theme;
    j["sound_on_complete"] = cfg.soundOnComplete;
    j["close_to_tray"] = cfg.closeToTray;
    j["autostart"] = cfg.autostart;
    std::string path = GetConfigFilePath();
    std::ofstream f(path, std::ios::trunc);
    if (!f.is_open()) return false;
    f << j.dump(2);
    f.close();
    chmod(path.c_str(), S_IRUSR | S_IWUSR);
    return true;
}

static std::string GetAutostartFilePath() {
    const char* xdgConfig = getenv("XDG_CONFIG_HOME");
    std::string dir;
    if (xdgConfig && *xdgConfig) {
        dir = std::string(xdgConfig) + "/autostart";
    } else {
        const char* home = getenv("HOME");
        if (!home) home = ".";
        dir = std::string(home) + "/.config/autostart";
    }
    mkdir(dir.c_str(), 0755);
    return dir + "/vtscanner.desktop";
}

bool SetAutostart(bool enable) {
    std::string desktopPath = GetAutostartFilePath();

    if (!enable) {
        // No entry left behind - a removed file is the clean, unambiguous "off" state.
        if (access(desktopPath.c_str(), F_OK) == 0) {
            return remove(desktopPath.c_str()) == 0;
        }
        return true;
    }

    char exePath[PATH_MAX] = {};
    ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    if (len <= 0) return false;
    exePath[len] = '\0';

    std::ofstream f(desktopPath, std::ios::trunc);
    if (!f.is_open()) return false;
    f << "[Desktop Entry]\n"
      << "Type=Application\n"
      << "Name=VirusTotal Scanner Pro\n"
      << "Comment=VirusTotal Scanner Pro autostart\n"
      << "Exec=\"" << exePath << "\"\n"
      << "Icon=vtscanner\n"
      << "Terminal=false\n"
      << "X-GNOME-Autostart-enabled=true\n"
      << "Hidden=false\n";
    f.close();
    chmod(desktopPath.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    return true;
}