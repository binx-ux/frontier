#include "driver_link.h"
#include "frontier_ioctl.h"
#include <cstring>
#include <string>

namespace {

    HANDLE gDevice = INVALID_HANDLE_VALUE;

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

    bool LoadDriverService(const std::wstring& sysPath)
    {
        SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
        if (!scm)
            return false;

        SC_HANDLE existing = OpenServiceW(scm, FRONTIER_DRIVER_SERVICE_NAME, SERVICE_START | DELETE);
        if (existing) {
            SERVICE_STATUS st{};
            ControlService(existing, SERVICE_CONTROL_STOP, &st);
            DeleteService(existing);
            CloseServiceHandle(existing);
        }

        SC_HANDLE svc = CreateServiceW(
            scm,
            FRONTIER_DRIVER_SERVICE_NAME,
            FRONTIER_DRIVER_SERVICE_NAME,
            SERVICE_START | DELETE | SERVICE_STOP,
            SERVICE_KERNEL_DRIVER,
            SERVICE_DEMAND_START,
            SERVICE_ERROR_NORMAL,
            sysPath.c_str(),
            nullptr, nullptr, nullptr, nullptr, nullptr);

        if (!svc) {
            svc = OpenServiceW(scm, FRONTIER_DRIVER_SERVICE_NAME, SERVICE_START);
        }

        bool ok = false;
        if (svc) {
            ok = StartServiceW(svc, 0, nullptr) != 0 || GetLastError() == ERROR_SERVICE_ALREADY_RUNNING;
            CloseServiceHandle(svc);
        }
        CloseServiceHandle(scm);
        return ok;
    }

}

namespace FrontierDriver {

    bool IsConnected()
    {
        return gDevice != INVALID_HANDLE_VALUE && gDevice != nullptr;
    }

    bool Connect()
    {
        if (IsConnected())
            return true;
        gDevice = CreateFileW(
            FRONTIER_DEVICE_PATH,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
        return IsConnected();
    }

    bool LoadFromDefaultPath()
    {
        if (Connect())
            return true;

        const std::wstring sysPath =
            ExeDir() + L"\\driver\\" + FRONTIER_DRIVER_FILE_NAME;
        if (GetFileAttributesW(sysPath.c_str()) == INVALID_FILE_ATTRIBUTES)
            return false;

        if (!LoadDriverService(sysPath))
            return false;

        return Connect();
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
        return DeviceIoControl(gDevice, IOCTL_FRONTIER_WRITE_MEMORY,
            &packet, sizeof(packet), &packet, sizeof(packet), &returned, nullptr) != 0;
    }

    void Disconnect()
    {
        if (gDevice != INVALID_HANDLE_VALUE) {
            CloseHandle(gDevice);
            gDevice = INVALID_HANDLE_VALUE;
        }
    }

}
