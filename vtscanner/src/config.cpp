#include "config.h"
#include "http_client.h"
#include <windows.h>
#include <shlobj.h>
#include <wincrypt.h>
#include <fstream>
#include <sstream>
#include <ctime>
#include <vector>
#include <nlohmann/json.hpp>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "crypt32.lib")

using json = nlohmann::json;

namespace {
constexpr const wchar_t* kDpapiDescription = L"VirusTotalScannerPro API key";

std::string ToBase64(const std::vector<BYTE>& data) {
    if (data.empty()) return {};
    DWORD outLen = 0;
    CryptBinaryToStringA(data.data(), (DWORD)data.size(),
                          CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &outLen);
    std::string out(outLen, '\0');
    CryptBinaryToStringA(data.data(), (DWORD)data.size(),
                          CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, out.data(), &outLen);
    if (!out.empty() && out.back() == '\0') out.pop_back();
    return out;
}

std::vector<BYTE> FromBase64(const std::string& text) {
    if (text.empty()) return {};
    DWORD outLen = 0;
    if (!CryptStringToBinaryA(text.c_str(), (DWORD)text.size(), CRYPT_STRING_BASE64,
                               nullptr, &outLen, nullptr, nullptr)) {
        return {};
    }
    std::vector<BYTE> out(outLen);
    if (!CryptStringToBinaryA(text.c_str(), (DWORD)text.size(), CRYPT_STRING_BASE64,
                               out.data(), &outLen, nullptr, nullptr)) {
        return {};
    }
    return out;
}

std::string ProtectApiKey(const std::string& apiKey) {
    if (apiKey.empty()) return {};

    DATA_BLOB in{};
    in.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(apiKey.data()));
    in.cbData = (DWORD)apiKey.size();

    DATA_BLOB out{};
    BOOL ok = CryptProtectData(&in, kDpapiDescription, nullptr, nullptr, nullptr,
                                CRYPTPROTECT_UI_FORBIDDEN, &out);
    if (!ok) return {};

    std::vector<BYTE> cipher(out.pbData, out.pbData + out.cbData);
    LocalFree(out.pbData);
    return ToBase64(cipher);
}

std::string UnprotectApiKey(const std::string& base64Cipher) {
    std::vector<BYTE> cipher = FromBase64(base64Cipher);
    if (cipher.empty()) return {};

    DATA_BLOB in{};
    in.pbData = cipher.data();
    in.cbData = (DWORD)cipher.size();

    DATA_BLOB out{};
    BOOL ok = CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr,
                                  CRYPTPROTECT_UI_FORBIDDEN, &out);
    if (!ok) return {};

    std::string plain(reinterpret_cast<char*>(out.pbData), out.cbData);
    SecureZeroMemory(out.pbData, out.cbData);
    LocalFree(out.pbData);
    return plain;
}

}

std::wstring GetConfigDir() {
    PWSTR path = nullptr;
    std::wstring dir;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path))) {
        dir = path;
        CoTaskMemFree(path);
        dir += L"\\VirusTotalScanner";
    } else {
        dir = L".";
    }
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir;
}

std::wstring GetConfigFilePath() {
    return GetConfigDir() + L"\\vt_config.json";
}

static std::string TodayString() {
    std::time_t t = std::time(nullptr);
    std::tm tmv{};
    localtime_s(&tmv, &t);
    char buf[16];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday);
    return buf;
}

AppConfig LoadConfig() {
    AppConfig cfg;
    cfg.requestsResetDate = "";

    std::wstring path = GetConfigFilePath();
    std::ifstream f(path, std::ios::binary);
    if (f.is_open()) {
        try {
            json j;
            f >> j;
            if (j.contains("api_key_protected")) {
                cfg.apiKey = UnprotectApiKey(j.value("api_key_protected", ""));
            } else {
                cfg.apiKey = j.value("api_key", cfg.apiKey);
            }
            cfg.topmost = j.value("topmost", cfg.topmost);
            cfg.thanksShown = j.value("thanks_shown", cfg.thanksShown);
            cfg.apiType = j.value("api_type", cfg.apiType);
            cfg.requestsUsed = j.value("requests_used", cfg.requestsUsed);
            cfg.requestsResetDate = j.value("requests_reset_date", cfg.requestsResetDate);
            cfg.lang = j.value("lang", cfg.lang);
            cfg.theme = j.value("theme", cfg.theme);
            cfg.soundOnComplete = j.value("sound_on_complete", cfg.soundOnComplete);
        } catch (...) {
        }
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
    j["api_key_protected"] = ProtectApiKey(cfg.apiKey);
    j["topmost"] = cfg.topmost;
    j["thanks_shown"] = cfg.thanksShown;
    j["api_type"] = cfg.apiType;
    j["requests_used"] = cfg.requestsUsed;
    j["requests_reset_date"] = cfg.requestsResetDate;
    j["lang"] = cfg.lang;
    j["theme"] = cfg.theme;
    j["sound_on_complete"] = cfg.soundOnComplete;

    std::wstring path = GetConfigFilePath();
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f.is_open()) return false;
    f << j.dump(2);
    return true;
}
