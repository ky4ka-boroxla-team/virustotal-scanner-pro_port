#pragma once

namespace VersionInfo {
    constexpr const char* CompanyName      = "ky4ka-boroxla";
    constexpr const char* ProductName      = "VirusTotal Scanner Pro";
    constexpr const char* FileDescription  = "VirusTotal Scanner Pro";
    constexpr const char* FileVersion      = "1.0.0";
    constexpr const char* ProductVersion   = "1.0.0";
    constexpr const char* LegalCopyright   = "Copyright (C) 2026 ky4ka-boroxla";
    constexpr const char* InternalName     = "vtscanner";
    constexpr const char* OriginalFilename = "vtscanner";
    inline void PrintVersion() {
        printf("%s\n", ProductName);
        printf("Version: %s\n", FileVersion);
        printf("%s\n", LegalCopyright);
        printf("Company: %s\n", CompanyName);
    }
}
