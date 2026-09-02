#pragma once
#include <cstdio>
#include <string>

namespace Net {

inline std::string UrlEncode(const char* s)
{
    static const char* hex = "0123456789ABCDEF";
    std::string out;
    if (!s) return out;
    for (const unsigned char* p = (const unsigned char*)s; *p; ++p) {
        unsigned char c = *p;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            out.push_back((char)c);
        } else {
            out.push_back('%');
            out.push_back(hex[c >> 4]);
            out.push_back(hex[c & 15]);
        }
    }
    return out;
}

} // namespace Net
