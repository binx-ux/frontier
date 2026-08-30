#pragma once
#include "../../../ext/imgui/imgui.h"
#include <cstring>
#include <cctype>

namespace ExplorerSyntax {

    enum class Lang {
        Plain,
        Lua,
        Luau,
        Json,
        Number,
        Bool,
        Address,
        Path
    };

    inline ImU32 ColKeyword() { return IM_COL32(235, 120, 120, 255); }
    inline ImU32 ColString() { return IM_COL32(140, 210, 140, 255); }
    inline ImU32 ColNumber() { return IM_COL32(130, 180, 255, 255); }
    inline ImU32 ColComment() { return IM_COL32(120, 125, 140, 255); }
    inline ImU32 ColPlain() { return IM_COL32(220, 222, 230, 255); }
    inline ImU32 ColBool() { return IM_COL32(255, 190, 110, 255); }

    inline bool IsKeyword(const char* w, int len)
    {
        static const char* keys[] = {
            "local", "function", "end", "if", "then", "else", "elseif", "for", "while",
            "do", "return", "true", "false", "nil", "and", "or", "not", "in",
            "game", "workspace", "script", "require", "print", "wait", "typeof",
            "export", "type", "continue", "break", "repeat", "until"
        };
        for (auto* k : keys) {
            if ((int)strlen(k) == len && _strnicmp(w, k, len) == 0)
                return true;
        }
        return false;
    }

    inline void DrawToken(ImDrawList* dl, ImVec2& pos, const char* text, int len, ImU32 col, float wrapW)
    {
        if (!text || len <= 0) return;
        char buf[256];
        if (len >= (int)sizeof(buf)) len = (int)sizeof(buf) - 1;
        memcpy(buf, text, len);
        buf[len] = 0;
        ImVec2 sz = ImGui::CalcTextSize(buf);
        if (wrapW > 0.f && pos.x + sz.x > wrapW) {
            pos.x = ImGui::GetCursorScreenPos().x;
            pos.y += ImGui::GetTextLineHeight();
        }
        dl->AddText(pos, col, buf);
        pos.x += sz.x;
    }

    inline void HighlightLine(ImDrawList* dl, ImVec2 start, float maxX, Lang lang, const char* line)
    {
        if (!line || !line[0]) return;
        ImVec2 pos = start;
        const char* p = line;

        if (lang == Lang::Bool) {
            DrawToken(dl, pos, line, (int)strlen(line), ColBool(), maxX);
            return;
        }
        if (lang == Lang::Number || lang == Lang::Address) {
            DrawToken(dl, pos, line, (int)strlen(line), ColNumber(), maxX);
            return;
        }
        if (lang == Lang::Path) {
            DrawToken(dl, pos, line, (int)strlen(line), ColString(), maxX);
            return;
        }
        if (lang == Lang::Plain) {
            DrawToken(dl, pos, line, (int)strlen(line), ColPlain(), maxX);
            return;
        }

        while (*p) {
            if (*p == ' ' || *p == '\t') {
                pos.x += ImGui::CalcTextSize(" ").x;
                ++p;
                continue;
            }
            if (lang == Lang::Lua || lang == Lang::Luau) {
                if (p[0] == '-' && p[1] == '-') {
                    DrawToken(dl, pos, p, (int)strlen(p), ColComment(), maxX);
                    break;
                }
                if (*p == '"' || *p == '\'') {
                    char q = *p++;
                    const char* s = p;
                    while (*p && *p != q) ++p;
                    DrawToken(dl, pos, s - 1, (int)(p - s + 2), ColString(), maxX);
                    if (*p == q) ++p;
                    continue;
                }
            }
            if (lang == Lang::Json) {
                if (*p == '"') {
                    const char* s = p++;
                    while (*p && *p != '"') ++p;
                    if (*p == '"') ++p;
                    DrawToken(dl, pos, s, (int)(p - s), ColString(), maxX);
                    continue;
                }
                if (isdigit((unsigned char)*p) || *p == '-') {
                    const char* s = p;
                    while (*p && (isdigit((unsigned char)*p) || *p == '.' || *p == '-' || *p == 'e' || *p == 'E'))
                        ++p;
                    DrawToken(dl, pos, s, (int)(p - s), ColNumber(), maxX);
                    continue;
                }
            }
            if (isalpha((unsigned char)*p) || *p == '_') {
                const char* s = p;
                while (isalnum((unsigned char)*p) || *p == '_') ++p;
                int len = (int)(p - s);
                ImU32 col = (lang == Lang::Lua || lang == Lang::Luau) && IsKeyword(s, len) ? ColKeyword() : ColPlain();
                DrawToken(dl, pos, s, len, col, maxX);
                continue;
            }
            if (isdigit((unsigned char)*p)) {
                const char* s = p;
                while (isdigit((unsigned char)*p) || *p == '.') ++p;
                DrawToken(dl, pos, s, (int)(p - s), ColNumber(), maxX);
                continue;
            }
            char ch[2] = { *p, 0 };
            DrawToken(dl, pos, ch, 1, ColPlain(), maxX);
            ++p;
        }
    }

    inline void DrawBlock(Lang lang, const char* text, float height = 120.f)
    {
        if (!text || !text[0]) return;
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.04f, 0.045f, 0.055f, 1.f));
        ImGui::BeginChild("##syntax", ImVec2(-1, height), ImGuiChildFlags_Borders);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        float maxX = p0.x + ImGui::GetContentRegionAvail().x;

        const char* line = text;
        ImVec2 row(p0.x, p0.y);
        while (line && *line) {
            const char* eol = strchr(line, '\n');
            int len = eol ? (int)(eol - line) : (int)strlen(line);
            char tmp[512];
            if (len >= (int)sizeof(tmp)) len = (int)sizeof(tmp) - 1;
            memcpy(tmp, line, len);
            tmp[len] = 0;
            row.x = p0.x;
            HighlightLine(dl, row, maxX, lang, tmp);
            row.y += ImGui::GetTextLineHeight();
            if (!eol) break;
            line = eol + 1;
        }
        ImGui::Dummy(ImVec2(0, height - 28.f));
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

}
