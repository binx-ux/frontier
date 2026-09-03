#pragma once
#include <string>

// Open-source build: no license / HWID / remote auth gate.
namespace SessionGate {

    inline bool ValidateSession(std::string& errMsg)
    {
        errMsg.clear();
        return true;
    }
}
