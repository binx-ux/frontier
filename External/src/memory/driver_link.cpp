#include "driver_link.h"
#include "frontier_ioctl.h"
#include <cstring>
#include <string>

namespace {

    HANDLE gDevice = INVALID_HANDLE_VALUE;
    DWORD gLastError = 0;

    void SetLast(DWORD err)
    {
        gLastError = err;
    }

    std::wstring ExeDir()
    {
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        std::wstring dir(path);
        const size_t slash = dir.find_last_of(L"\\/");
        if (slash != std::wstring::npos)
            dir.resize(slash);
        return dir;
    }

    std::wstring ToAbsolute(const std::wstring& path)
    {
        wchar_t full[MAX_PATH]{};
        if (GetFullPathNameW(path.c_str(), MAX_PATH, full, nullptr) == 0)
            return path;
        return full;
    }

    bool EnableLoadDriverPrivilege()
    {
        HANDLE token = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
            return false;

        TOKEN_PRIVILEGES tp{};
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        if (!LookupPrivilegeValueW(nullptr, SE_LOAD_DRIVER_NAME, &tp.Privileges[0].Luid)) {
            CloseHandle(token);
            return false;
        }

        const BOOL ok = AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), nullptr, nullptr);
        const DWORD err = GetLastError();
        CloseHandle(token);
        return ok && err == ERROR_SUCCESS;
    }

    bool StopAndDeleteService(SC_HANDLE scm)
    {
        SC_HANDLE svc = OpenServiceW(scm, FRONTIER_DRIVER_SERVICE_NAME,
            SERVICE_STOP | DELETE | SERVICE_QUERY_STATUS);
        if (!svc)
            return false;

        SERVICE_STATUS st{};
        ControlService(svc, SERVICE_CONTROL_STOP, &st);

        for (int i = 0; i < 40; ++i) {
            if (QueryServiceStatus(svc, &st) && st.dwCurrentState == SERVICE_STOPPED)
                break;
            Sleep(50);
        }

        const bool deleted = DeleteService(svc) != 0;
        CloseServiceHandle(svc);
        return deleted;
    }

    bool LoadDriverService(const std::wstring& sysPathIn)
    {
        const std::wstring sysPath = ToAbsolute(sysPathIn);
        if (GetFileAttributesW(sysPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
            SetLast(ERROR_FILE_NOT_FOUND);
            return false;
        }

        EnableLoadDriverPrivilege();

        SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
        if (!scm) {
            SetLast(GetLastError());
            return false;
        }

        StopAndDeleteService(scm);

        SC_HANDLE svc = CreateServiceW(
            scm,
            FRONTIER_DRIVER_SERVICE_NAME,
            FRONTIER_DRIVER_SERVICE_NAME,
            SERVICE_START | DELETE | SERVICE_STOP | SERVICE_QUERY_STATUS,
            SERVICE_KERNEL_DRIVER,
            SERVICE_DEMAND_START,
            SERVICE_ERROR_NORMAL,
            sysPath.c_str(),
            nullptr, nullptr, nullptr, nullptr, nullptr);

        if (!svc) {
            const DWORD createErr = GetLastError();
            svc = OpenServiceW(scm, FRONTIER_DRIVER_SERVICE_NAME,
                SERVICE_START | DELETE | SERVICE_STOP | SERVICE_QUERY_STATUS);
            if (!svc) {
                SetLast(createErr);
                CloseServiceHandle(scm);
                return false;
            }
        }

        bool ok = false;
        if (StartServiceW(svc, 0, nullptr)) {
            ok = true;
        } else {
            const DWORD err = GetLastError();
            ok = (err == ERROR_SERVICE_ALREADY_RUNNING);
            if (!ok)
                SetLast(err);
        }

        SERVICE_STATUS st{};
        for (int i = 0; i < 40; ++i) {
            if (!QueryServiceStatus(svc, &st))
                break;
            if (st.dwCurrentState == SERVICE_RUNNING) {
                ok = true;
                break;
            }
            if (st.dwCurrentState == SERVICE_STOPPED) {
                if (st.dwWin32ExitCode == ERROR_SERVICE_NEVER_STARTED)
                    break;
                if (st.dwWin32ExitCode != 0) {
                    SetLast(st.dwWin32ExitCode);
                    ok = false;
                }
                break;
            }
            Sleep(50);
        }

        if (!ok && gLastError == 0)
            SetLast(ERROR_GEN_FAILURE);

        CloseServiceHandle(svc);
        CloseServiceHandle(scm);
        return ok;
    }

    bool TryOpenDevice(const wchar_t* path)
    {
        HANDLE h = CreateFileW(
            path,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
        if (h == INVALID_HANDLE_VALUE) {
            SetLast(GetLastError());
            return false;
        }
        if (gDevice != INVALID_HANDLE_VALUE)
            CloseHandle(gDevice);
        gDevice = h;
        SetLast(0);
        return true;
    }

    const char* DescribeWin32(DWORD err)
    {
        switch (err) {
        case ERROR_SUCCESS: return "OK";
        case ERROR_FILE_NOT_FOUND: return "FrontierDrv.sys not found next to Frontier.exe";
        case ERROR_ACCESS_DENIED: return "Access denied — run FrontierLoader and kernel mode as Administrator";
        case ERROR_PRIVILEGE_NOT_HELD: return "SeLoadDriverPrivilege missing — run as Administrator";
        case ERROR_SERVICE_ALREADY_RUNNING: return "Driver service already running";
        case ERROR_PATH_NOT_FOUND: return "Driver .sys path invalid";
        case ERROR_INVALID_IMAGE_HASH:
            return "Driver blocked by signature policy — disable Secure Boot in BIOS, enable test signing, reboot (or use Usermode)";
        default: return "See Windows error code";
        }
    }

}

namespace FrontierDriver {

    DWORD LastError()
    {
        return gLastError;
    }

    const char* LastErrorText()
    {
        return DescribeWin32(gLastError);
    }

    bool IsConnected()
    {
        return gDevice != INVALID_HANDLE_VALUE && gDevice != nullptr;
    }

    bool Connect()
    {
        if (IsConnected())
            return true;

        static const wchar_t* paths[] = {
            FRONTIER_DEVICE_PATH,
            L"\\\\.\\Global\\FrontierDrv",
        };

        for (const wchar_t* path : paths) {
            if (TryOpenDevice(path)) {
                SetLast(0);
                return true;
            }
        }

        SetLast(GetLastError());
        return false;
    }

    bool LoadFromDefaultPath()
    {
        if (Connect())
            return true;

        const std::wstring exeDir = ExeDir();
        const std::wstring candidates[] = {
            exeDir + L"\\driver\\" + FRONTIER_DRIVER_FILE_NAME,
            exeDir + L"\\kernel\\driver\\" + FRONTIER_DRIVER_FILE_NAME,
            exeDir + L"\\..\\kernel\\driver\\" + FRONTIER_DRIVER_FILE_NAME,
            exeDir + L"\\..\\dist\\kernel\\driver\\" + FRONTIER_DRIVER_FILE_NAME,
        };

        for (const auto& sysPath : candidates) {
            if (GetFileAttributesW(sysPath.c_str()) == INVALID_FILE_ATTRIBUTES)
                continue;
            if (!LoadDriverService(sysPath))
                continue;
            Sleep(200);
            if (Connect())
                return true;
        }

        return false;
    }

    bool ReadMemory(unsigned long processId, unsigned long long address, void* buffer, unsigned long size)
    {
        if (!IsConnected() || !buffer || !size || size > FRONTIER_RW_MAX)
            return false;

        FRONTIER_MEMORY_PACKET packet{};
        packet.ProcessId = processId;
        packet.Address = address;
        packet.Size = size;

        DWORD returned = 0;
        if (!DeviceIoControl(gDevice, IOCTL_FRONTIER_READ_MEMORY,
                &packet, sizeof(packet), &packet, sizeof(packet), &returned, nullptr)) {
            SetLast(GetLastError());
            return false;
        }

        memcpy(buffer, packet.Data, size);
        return true;
    }

    bool WriteMemory(unsigned long processId, unsigned long long address, const void* buffer, unsigned long size)
    {
        if (!IsConnected() || !buffer || !size || size > FRONTIER_RW_MAX)
            return false;

        FRONTIER_MEMORY_PACKET packet{};
        packet.ProcessId = processId;
        packet.Address = address;
        packet.Size = size;
        memcpy(packet.Data, buffer, size);

        DWORD returned = 0;
        if (!DeviceIoControl(gDevice, IOCTL_FRONTIER_WRITE_MEMORY,
            &packet, sizeof(packet), &packet, sizeof(packet), &returned, nullptr)) {
            SetLast(GetLastError());
            return false;
        }
        return true;
    }

    void Disconnect()
    {
        if (gDevice != INVALID_HANDLE_VALUE) {
            CloseHandle(gDevice);
            gDevice = INVALID_HANDLE_VALUE;
        }
    }

}
