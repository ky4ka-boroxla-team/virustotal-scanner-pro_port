#pragma once
#include <string>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>
#include <windows.h>
#include <shellapi.h>
#include "config.h"
#include "lang.h"


struct ScanState {
    bool scanning = false;
    bool hasFile = false;
    bool rescanEnabled = false;
    std::string fileName;
    std::string fileHashHex;
    unsigned long long fileSize = 0;

    std::string statusText;
    std::string outputLog;
    float progress = 0.0f;
    bool showCheckNowButton = false;

    int requestsUsed = 0;
};

class App {
public:
    // Custom window message the tray icon posts back to the main window (mouse events on the icon).
    static constexpr UINT WM_APP_TRAYICON = WM_APP + 1;
    static constexpr UINT ID_TRAY_SHOW = 1001;
    static constexpr UINT ID_TRAY_EXIT = 1002;

    App();
    ~App();

    void Init(HWND hwnd);

    void DrawUI();

    bool WantsTopmost() const { return m_config.topmost; }
    bool WantsExit() const { return m_wantsExit; }
    std::string ThemeName() const { return m_config.theme; }

    // --- System tray ---
    bool WantsCloseToTray() const { return m_config.closeToTray; }
    void MinimizeToTray();
    void RestoreFromTray();
    void RecreateTrayIcon();               // re-add icon after explorer.exe restarts (TaskbarCreated)
    void OnTrayIconMessage(WPARAM wParam, LPARAM lParam);
    void OnTrayCommand(UINT commandId);

private:
    void DrawMainWindow();
    void DrawSettingsPopup();
    void DrawApiKeyPopup();
    void DrawAboutPopup();
    void DrawThanksPopup();

    void OpenFileDialogAndScan();
    void StartScan(const std::wstring& path, const std::string& fileNameUtf8);
    void ScanWorker(std::wstring path, std::string fileNameUtf8);
    void CheckNow();
    void ClearOutput();
    void CopyHash();
    void OpenInBrowser();
    void PlayCompleteSound();
    void BumpRequestsUsed();

    void AddTrayIcon();
    void RemoveTrayIcon();
    void ShowTrayContextMenu();

    ScanState Snapshot();
    void MutateState(const std::function<void(ScanState&)>& fn);

    Lang L() const { return LangFromString(m_config.lang); }

    AppConfig m_config;
    ScanState m_state;
    std::mutex m_mutex;

    std::thread m_worker;
    std::atomic<bool> m_checkNowRequested{false};
    std::atomic<bool> m_shuttingDown{false};
    std::wstring m_currentFilePath;

    bool m_showSettings = false;
    bool m_showApiKeyPopup = false;
    bool m_showAbout = false;
    bool m_showThanks = false;
    bool m_thanksDontShow = false;
    char m_apiKeyBuf[256] = {};

    HWND m_hwnd = nullptr;
    bool m_wantsExit = false;

    NOTIFYICONDATAW m_nid{};
    bool m_trayIconAdded = false;
};
