#pragma once
#include <atomic>
#include <mutex>
#include <string>

// Open-source builds: no auto-update ping. Check GitHub Releases manually.
namespace Updater {

    inline constexpr int kLocalVersion = 36;
    inline constexpr const char* kLocalDisplay = "FRONTIER dev";
    inline constexpr const char* kDownloadUrl = "https://github.com/binx-ux/frontier/releases";

    inline std::atomic<bool> checked{ false };
    inline std::atomic<bool> outOfDate{ false };
    inline std::atomic<int> remoteVersion{ 0 };
    inline char remoteDisplay[32] = "";
    inline char status[96] = "";
    inline std::mutex gMutex;

    inline void CheckAsync()
    {
        checked = true;
    }

    inline void OpenDownload()
    {
        ShellExecuteA(nullptr, "open", kDownloadUrl, nullptr, nullptr, SW_SHOWNORMAL);
    }
}
