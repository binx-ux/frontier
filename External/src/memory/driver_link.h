#pragma once
#include <Windows.h>

namespace FrontierDriver {

    bool IsConnected();
    bool Connect();
    bool LoadFromDefaultPath();
    bool ReadMemory(unsigned long processId, unsigned long long address, void* buffer, unsigned long size);
    bool WriteMemory(unsigned long processId, unsigned long long address, const void* buffer, unsigned long size);
    void Disconnect();
    DWORD LastError();
    const char* LastErrorText();

}
