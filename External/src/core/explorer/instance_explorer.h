#pragma once
#include "../../sdk/sdk.h"
#include "../../core/globals/globals.h"
#include "../../core/variables/variables.h"
#include "../../memory/memory.h"
#include "explorer_syntax.h"
#include "../../render/frontier_ui.h"
#include "../../../ext/imgui/imgui.h"
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <functional>
#include <filesystem>
#include <ShlObj.h>
#include <Shellapi.h>

namespace InstanceExplorer {

    struct Node {
        uintptr_t addr = 0;
        std::string name;
        std::string className;
        bool childrenLoaded = false;
        bool expanded = false;
        std::vector<Node> children;
    };

    inline Node root{};
    inline uintptr_t selectedAddr = 0;
    inline std::string selectedName;
    inline std::string selectedClass;
    inline std::string selectedPath;
    inline char filter[64] = "";
    inline char saveFileName[64] = "game_dump";
    inline char statusMsg[160] = "Open a service to browse";
    inline char lastSavePath[MAX_PATH] = "";
    inline int saveMaxDepth = 12;
    inline int childCap = 400; // per node — keeps huge places usable
    inline bool saveBusy = false;

    inline std::string DumpDir()
    {
        char docs[MAX_PATH]{};
        if (FAILED(SHGetFolderPathA(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, docs)))
            strcpy_s(docs, ".");
        std::filesystem::path p = std::filesystem::path(docs) / "FRONTIER" / "dumps";
        std::error_code ec;
        std::filesystem::create_directories(p, ec);
        return p.string();
    }

    inline std::string SanitizeFileStem(const char* raw)
    {
        std::string s = raw ? raw : "";
        while (!s.empty() && (s.back() == ' ' || s.back() == '.')) s.pop_back();
        if (s.empty()) s = "dump";
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            if (c == '\\' || c == '/' || c == ':' || c == '*' || c == '?' ||
                c == '"' || c == '<' || c == '>' || c == '|')
                out.push_back('_');
            else if (c >= 32 && c != 127)
                out.push_back(c);
        }
        if (out.empty()) out = "dump";
        if (out.size() > 48) out.resize(48);
        return out;
    }

    inline void ClearNode(Node& n)
    {
        n.children.clear();
        n.childrenLoaded = false;
        n.expanded = false;
    }

    inline void LoadChildren(Node& n)
    {
        n.children.clear();
        n.childrenLoaded = true;
        if (!n.addr) return;

        RBX::RbxInstance inst(n.addr);
        auto kids = inst.GetChildList();
        int count = 0;
        for (auto& c : kids) {
            if (!c.Addr) continue;
            if (count >= childCap) break;
            Node child{};
            child.addr = c.Addr;
            child.name = c.GetName();
            child.className = c.GetClass();
            if (child.name.empty()) child.name = "(unnamed)";
            if (child.className.empty()) child.className = "?";
            n.children.push_back(std::move(child));
            count++;
        }
    }

    inline Node NodeFromAddr(uintptr_t addr)
    {
        Node n{};
        n.addr = addr;
        if (!addr) return n;
        RBX::RbxInstance inst(addr);
        n.name = inst.GetName();
        n.className = inst.GetClass();
        if (n.name.empty()) n.name = "(unnamed)";
        if (n.className.empty()) n.className = "?";
        return n;
    }

    inline void RefreshRoot()
    {
        root = {};
        selectedAddr = 0;
        selectedName.clear();
        selectedClass.clear();
        selectedPath.clear();

        if (!Globals::dataModel.Addr) {
            strcpy_s(statusMsg, "No DataModel — join a game first");
            return;
        }

        root.addr = Globals::dataModel.Addr;
        root.name = "game";
        root.className = "DataModel";
        root.expanded = true;
        LoadChildren(root);
        sprintf_s(statusMsg, "Loaded %d top-level services", (int)root.children.size());
    }

    inline bool PassesFilter(const Node& n)
    {
        if (!filter[0]) return true;
        std::string f = filter;
        for (auto& c : f) c = (char)tolower((unsigned char)c);
        std::string hay = n.name + " " + n.className;
        for (auto& c : hay) c = (char)tolower((unsigned char)c);
        return hay.find(f) != std::string::npos;
    }

    inline bool SubtreeMatchesFilter(Node& n)
    {
        if (PassesFilter(n)) return true;
        if (!n.childrenLoaded)
            LoadChildren(n);
        for (auto& c : n.children)
            if (SubtreeMatchesFilter(c)) return true;
        return false;
    }

    inline void RefreshInspector();

    inline void Select(const Node& n, const std::string& path)
    {
        selectedAddr = n.addr;
        selectedName = n.name;
        selectedClass = n.className;
        selectedPath = path.empty() ? n.name : path;
        RefreshInspector();
    }

    enum class IconKind {
        Folder, Service, Workspace, Players, Player, Part, Mesh, Script,
        LocalScript, ModuleScript, Gui, Sound, Light, Camera, Value, Model, Unknown
    };

    inline bool ClassEq(const std::string& a, const char* b)
    {
        return _stricmp(a.c_str(), b) == 0;
    }

    inline bool ClassEnds(const std::string& a, const char* suffix)
    {
        size_t n = strlen(suffix);
        return a.size() >= n && _stricmp(a.c_str() + (a.size() - n), suffix) == 0;
    }

    inline IconKind Classify(const std::string& cls, bool hasKids)
    {
        if (ClassEq(cls, "Folder") || ClassEq(cls, "Configuration")) return IconKind::Folder;
        if (ClassEq(cls, "Workspace")) return IconKind::Workspace;
        if (ClassEq(cls, "Players")) return IconKind::Players;
        if (ClassEq(cls, "Player")) return IconKind::Player;
        if (ClassEq(cls, "Model") || ClassEq(cls, "Actor")) return IconKind::Model;
        if (ClassEq(cls, "Part") || ClassEq(cls, "BasePart") || ClassEq(cls, "MeshPart") ||
            ClassEq(cls, "UnionOperation") || ClassEq(cls, "WedgePart") || ClassEq(cls, "CornerWedgePart") ||
            ClassEq(cls, "TrussPart") || ClassEq(cls, "SpawnLocation"))
            return IconKind::Part;
        if (ClassEq(cls, "SpecialMesh") || ClassEq(cls, "FileMesh") || ClassEq(cls, "BlockMesh") ||
            ClassEq(cls, "CylinderMesh"))
            return IconKind::Mesh;
        if (ClassEq(cls, "Script")) return IconKind::Script;
        if (ClassEq(cls, "LocalScript")) return IconKind::LocalScript;
        if (ClassEq(cls, "ModuleScript")) return IconKind::ModuleScript;
        if (ClassEq(cls, "Sound") || ClassEq(cls, "SoundGroup") || ClassEq(cls, "SoundService"))
            return IconKind::Sound;
        if (ClassEq(cls, "Camera") || ClassEq(cls, "CurrentCamera")) return IconKind::Camera;
        if (ClassEq(cls, "PointLight") || ClassEq(cls, "SpotLight") || ClassEq(cls, "SurfaceLight") ||
            ClassEq(cls, "Lighting"))
            return IconKind::Light;
        if (ClassEnds(cls, "Value") || ClassEq(cls, "Attribute") || ClassEq(cls, "StringValue") ||
            ClassEq(cls, "NumberValue") || ClassEq(cls, "IntValue") || ClassEq(cls, "BoolValue") ||
            ClassEq(cls, "ObjectValue") || ClassEq(cls, "CFrameValue") || ClassEq(cls, "Vector3Value") ||
            ClassEq(cls, "Color3Value"))
            return IconKind::Value;
        if (ClassEnds(cls, "Gui") || ClassEq(cls, "Frame") || ClassEq(cls, "TextLabel") ||
            ClassEq(cls, "TextButton") || ClassEq(cls, "ImageLabel") || ClassEq(cls, "ImageButton") ||
            ClassEq(cls, "ScreenGui") || ClassEq(cls, "BillboardGui") || ClassEq(cls, "SurfaceGui") ||
            ClassEq(cls, "ScrollingFrame") || ClassEq(cls, "ViewportFrame") || ClassEq(cls, "GuiService") ||
            ClassEq(cls, "StarterGui") || ClassEq(cls, "PlayerGui") || ClassEq(cls, "CoreGui"))
            return IconKind::Gui;
        if (ClassEnds(cls, "Service") || ClassEq(cls, "DataModel") || ClassEq(cls, "RunService") ||
            ClassEq(cls, "ReplicatedStorage") || ClassEq(cls, "ServerStorage") ||
            ClassEq(cls, "ServerScriptService") || ClassEq(cls, "StarterPlayer") ||
            ClassEq(cls, "StarterPack") || ClassEq(cls, "Teams") || ClassEq(cls, "Chat") ||
            ClassEq(cls, "HttpService") || ClassEq(cls, "TweenService") || ClassEq(cls, "UserInputService") ||
            ClassEq(cls, "Debris") || ClassEq(cls, "CollectionService") || ClassEq(cls, "PathfindingService") ||
            ClassEq(cls, "PhysicsService") || ClassEq(cls, "TeleportService") || ClassEq(cls, "MarketplaceService") ||
            ClassEq(cls, "BadgeService") || ClassEq(cls, "InsertService") || ClassEq(cls, "ContentProvider") ||
            ClassEq(cls, "Stats") || ClassEq(cls, "NetworkClient") || ClassEq(cls, "LogService") ||
            ClassEq(cls, "TimerService") || ClassEq(cls, "TextService") || ClassEq(cls, "LocalizationService"))
            return IconKind::Service;
        if (hasKids) return IconKind::Folder;
        return IconKind::Unknown;
    }

    inline ImU32 IconColor(IconKind k)
    {
        switch (k) {
        case IconKind::Folder:       return IM_COL32(230, 190, 80, 255);
        case IconKind::Service:      return IM_COL32(120, 170, 255, 255);
        case IconKind::Workspace:    return IM_COL32(120, 210, 140, 255);
        case IconKind::Players:      return IM_COL32(100, 200, 255, 255);
        case IconKind::Player:       return IM_COL32(140, 210, 255, 255);
        case IconKind::Part:         return IM_COL32(180, 180, 190, 255);
        case IconKind::Mesh:         return IM_COL32(160, 150, 220, 255);
        case IconKind::Script:       return IM_COL32(90, 160, 255, 255);
        case IconKind::LocalScript:  return IM_COL32(80, 200, 120, 255);
        case IconKind::ModuleScript: return IM_COL32(220, 140, 70, 255);
        case IconKind::Gui:          return IM_COL32(210, 130, 220, 255);
        case IconKind::Sound:        return IM_COL32(255, 170, 80, 255);
        case IconKind::Light:        return IM_COL32(255, 230, 90, 255);
        case IconKind::Camera:       return IM_COL32(200, 200, 210, 255);
        case IconKind::Value:        return IM_COL32(140, 220, 200, 255);
        case IconKind::Model:        return IM_COL32(170, 200, 110, 255);
        default:                     return IM_COL32(160, 165, 175, 255);
        }
    }

    inline void DrawClassIcon(ImDrawList* dl, ImVec2 c, float s, IconKind k, ImU32 col)
    {
        const float h = s * 0.5f;
        switch (k) {
        case IconKind::Folder: {
            dl->AddRectFilled(ImVec2(c.x - h, c.y - h * 0.15f), ImVec2(c.x + h, c.y + h * 0.85f), col, 2.f);
            dl->AddRectFilled(ImVec2(c.x - h, c.y - h * 0.75f), ImVec2(c.x - h * 0.05f, c.y - h * 0.05f), col, 2.f);
            break;
        }
        case IconKind::Service: {
            dl->AddCircleFilled(c, h * 0.85f, col, 12);
            dl->AddCircleFilled(c, h * 0.35f, IM_COL32(20, 20, 24, 255), 8);
            break;
        }
        case IconKind::Workspace: {
            dl->AddRectFilled(ImVec2(c.x - h, c.y - h * 0.7f), ImVec2(c.x + h, c.y + h * 0.7f), col, 2.f);
            dl->AddLine(ImVec2(c.x - h, c.y), ImVec2(c.x + h, c.y), IM_COL32(20, 20, 24, 200), 1.2f);
            dl->AddLine(ImVec2(c.x, c.y - h * 0.7f), ImVec2(c.x, c.y + h * 0.7f), IM_COL32(20, 20, 24, 200), 1.2f);
            break;
        }
        case IconKind::Players:
        case IconKind::Player: {
            dl->AddCircleFilled(ImVec2(c.x, c.y - h * 0.35f), h * 0.35f, col, 10);
            dl->AddRectFilled(ImVec2(c.x - h * 0.55f, c.y + h * 0.05f), ImVec2(c.x + h * 0.55f, c.y + h * 0.85f), col, 3.f);
            break;
        }
        case IconKind::Part: {
            dl->AddRectFilled(ImVec2(c.x - h * 0.75f, c.y - h * 0.55f), ImVec2(c.x + h * 0.75f, c.y + h * 0.55f), col, 2.f);
            break;
        }
        case IconKind::Mesh: {
            ImVec2 pts[3] = { ImVec2(c.x, c.y - h), ImVec2(c.x - h, c.y + h * 0.7f), ImVec2(c.x + h, c.y + h * 0.7f) };
            dl->AddConvexPolyFilled(pts, 3, col);
            break;
        }
        case IconKind::Script:
        case IconKind::LocalScript:
        case IconKind::ModuleScript: {
            dl->AddRectFilled(ImVec2(c.x - h * 0.65f, c.y - h), ImVec2(c.x + h * 0.65f, c.y + h), col, 2.f);
            dl->AddLine(ImVec2(c.x - h * 0.35f, c.y - h * 0.4f), ImVec2(c.x + h * 0.35f, c.y - h * 0.4f), IM_COL32(20, 20, 24, 220), 1.4f);
            dl->AddLine(ImVec2(c.x - h * 0.35f, c.y), ImVec2(c.x + h * 0.2f, c.y), IM_COL32(20, 20, 24, 220), 1.4f);
            dl->AddLine(ImVec2(c.x - h * 0.35f, c.y + h * 0.4f), ImVec2(c.x + h * 0.35f, c.y + h * 0.4f), IM_COL32(20, 20, 24, 220), 1.4f);
            break;
        }
        case IconKind::Gui: {
            dl->AddRect(ImVec2(c.x - h, c.y - h * 0.7f), ImVec2(c.x + h, c.y + h * 0.7f), col, 2.f, 0, 1.6f);
            dl->AddRectFilled(ImVec2(c.x - h * 0.55f, c.y - h * 0.25f), ImVec2(c.x + h * 0.55f, c.y + h * 0.35f), col, 1.5f);
            break;
        }
        case IconKind::Sound: {
            dl->AddCircleFilled(ImVec2(c.x - h * 0.25f, c.y), h * 0.4f, col, 10);
            dl->AddLine(ImVec2(c.x + h * 0.1f, c.y - h * 0.55f), ImVec2(c.x + h * 0.1f, c.y + h * 0.55f), col, 1.8f);
            dl->AddLine(ImVec2(c.x + h * 0.45f, c.y - h * 0.8f), ImVec2(c.x + h * 0.45f, c.y + h * 0.8f), col, 1.6f);
            break;
        }
        case IconKind::Light: {
            dl->AddCircleFilled(c, h * 0.35f, col, 10);
            for (int i = 0; i < 6; i++) {
                float a = (float)i * 3.14159265f / 3.f;
                dl->AddLine(
                    ImVec2(c.x + cosf(a) * h * 0.55f, c.y + sinf(a) * h * 0.55f),
                    ImVec2(c.x + cosf(a) * h, c.y + sinf(a) * h),
                    col, 1.5f);
            }
            break;
        }
        case IconKind::Camera: {
            dl->AddRectFilled(ImVec2(c.x - h * 0.7f, c.y - h * 0.45f), ImVec2(c.x + h * 0.35f, c.y + h * 0.45f), col, 2.f);
            ImVec2 pts[3] = {
                ImVec2(c.x + h * 0.35f, c.y - h * 0.2f),
                ImVec2(c.x + h, c.y - h * 0.65f),
                ImVec2(c.x + h, c.y + h * 0.65f)
            };
            dl->AddConvexPolyFilled(pts, 3, col);
            break;
        }
        case IconKind::Value: {
            dl->AddCircle(c, h * 0.75f, col, 12, 1.8f);
            dl->AddText(ImVec2(c.x - 3.f, c.y - 7.f), col, "v");
            break;
        }
        case IconKind::Model: {
            dl->AddRectFilled(ImVec2(c.x - h * 0.85f, c.y - h * 0.2f), ImVec2(c.x - h * 0.1f, c.y + h * 0.7f), col, 1.5f);
            dl->AddRectFilled(ImVec2(c.x + h * 0.1f, c.y - h * 0.55f), ImVec2(c.x + h * 0.85f, c.y + h * 0.35f), col, 1.5f);
            break;
        }
        default: {
            dl->AddCircleFilled(c, h * 0.55f, col, 10);
            break;
        }
        }
    }

    inline void DrawTreeNode(Node& n, const std::string& path, int depth, int childIndex)
    {
        if (depth > 24) return;

        ImGui::PushID(childIndex);
        ImGui::PushID((void*)(uintptr_t)(n.addr & 0xFFFFFFFFFFFFull));

        const bool filtering = filter[0] != 0;
        if (filtering && !SubtreeMatchesFilter(n)) {
            ImGui::PopID();
            ImGui::PopID();
            return;
        }

        if (!n.childrenLoaded)
            LoadChildren(n);

        const bool hasKids = !n.children.empty();
        const IconKind kind = Classify(n.className, hasKids);
        const ImU32 iconCol = IconColor(kind);
        const bool selected = (n.addr == selectedAddr);

        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth |
            ImGuiTreeNodeFlags_FramePadding |
            ImGuiTreeNodeFlags_AllowOverlap;
        if (selected) flags |= ImGuiTreeNodeFlags_Selected;
        if (!hasKids) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

        if (hasKids && (n.expanded || filtering))
            ImGui::SetNextItemOpen(true, filtering ? ImGuiCond_Always : ImGuiCond_Once);

        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.18f, 0.20f, 0.26f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.22f, 0.24f, 0.30f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.26f, 0.28f, 0.34f, 1.f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 3));

        const bool open = ImGui::TreeNodeEx("##row", flags);

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen())
            Select(n, path);

        // Custom label: icon + name + class chip
        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 min = ImGui::GetItemRectMin();
            ImVec2 max = ImGui::GetItemRectMax();
            float rowH = max.y - min.y;
            float iconX = min.x + ImGui::GetTreeNodeToLabelSpacing() - 2.f;
            float iconY = min.y + rowH * 0.5f;
            DrawClassIcon(dl, ImVec2(iconX + 7.f, iconY), 13.f, kind, iconCol);

            ImVec2 namePos(iconX + 18.f, min.y + (rowH - ImGui::GetTextLineHeight()) * 0.5f);
            dl->AddText(namePos, selected ? IM_COL32(255, 255, 255, 255) : IM_COL32(230, 232, 238, 255), n.name.c_str());

            ImVec2 ns = ImGui::CalcTextSize(n.name.c_str());
            ImVec2 classPos(namePos.x + ns.x + 8.f, namePos.y);
            ImVec2 cs = ImGui::CalcTextSize(n.className.c_str());
            if (classPos.x + cs.x + 8.f < max.x) {
                ImVec2 chipMin(classPos.x - 4.f, classPos.y - 1.f);
                ImVec2 chipMax(classPos.x + cs.x + 4.f, classPos.y + cs.y + 1.f);
                ImU32 chipBg = IM_COL32(
                    ((iconCol >> IM_COL32_R_SHIFT) & 0xFF),
                    ((iconCol >> IM_COL32_G_SHIFT) & 0xFF),
                    ((iconCol >> IM_COL32_B_SHIFT) & 0xFF),
                    36);
                dl->AddRectFilled(chipMin, chipMax, chipBg, 4.f);
                dl->AddText(classPos, iconCol, n.className.c_str());
            }

            if (hasKids) {
                char countBuf[24];
                sprintf_s(countBuf, "%d", (int)n.children.size());
                ImVec2 cts = ImGui::CalcTextSize(countBuf);
                dl->AddText(ImVec2(max.x - cts.x - 8.f, namePos.y), IM_COL32(120, 125, 135, 200), countBuf);
            }
        }

        n.expanded = open && hasKids;

        if (open && hasKids) {
            for (int i = 0; i < (int)n.children.size(); i++) {
                std::string childPath = path.empty() ? n.children[i].name : (path + "." + n.children[i].name);
                DrawTreeNode(n.children[i], childPath, depth + 1, i);
            }
            ImGui::TreePop();
        }

        ImGui::PopID();
        ImGui::PopID();
    }

    inline void DrawSelectedIcon(float size = 22.f)
    {
        if (!selectedAddr) return;
        IconKind k = Classify(selectedClass, false);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        DrawClassIcon(dl, ImVec2(p.x + size * 0.5f, p.y + size * 0.5f), size, k, IconColor(k));
        ImGui::Dummy(ImVec2(size, size));
    }

    inline ImVec4 ClassColorV4(const std::string& cls)
    {
        return ImGui::ColorConvertU32ToFloat4(IconColor(Classify(cls, false)));
    }

    inline void DumpNode(std::ofstream& out, Node& n, int depth, int maxDepth)
    {
        if (depth > maxDepth) return;
        for (int i = 0; i < depth; i++) out << "  ";
        out << n.className << " \"" << n.name << "\" @" << std::hex << n.addr << std::dec << "\n";
        if (!n.childrenLoaded)
            LoadChildren(n);
        for (auto& c : n.children)
            DumpNode(out, c, depth + 1, maxDepth);
    }

    inline bool SaveNodeToFile(Node& n, const char* label, int maxDepth)
    {
        if (!n.addr) {
            strcpy_s(statusMsg, "Nothing to save");
            return false;
        }

        auto dir = DumpDir();
        std::string stem = SanitizeFileStem(label && label[0] ? label : saveFileName);
        std::filesystem::path filePath = std::filesystem::path(dir) / (stem + ".txt");

        // Avoid overwriting — append _2, _3, ...
        if (std::filesystem::exists(filePath)) {
            for (int i = 2; i < 1000; i++) {
                std::filesystem::path alt = std::filesystem::path(dir) / (stem + "_" + std::to_string(i) + ".txt");
                if (!std::filesystem::exists(alt)) {
                    filePath = alt;
                    break;
                }
            }
        }

        // Prefer Win32 wide CreateFile — ofstream fails on some Unicode Documents paths
        auto writeAll = [](const std::filesystem::path& path, const std::string& data) -> bool {
            HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL, nullptr);
            if (h == INVALID_HANDLE_VALUE) return false;
            DWORD written = 0;
            BOOL ok = WriteFile(h, data.data(), (DWORD)data.size(), &written, nullptr);
            CloseHandle(h);
            return ok && written == (DWORD)data.size();
        };

        std::string payload;
        payload.reserve(4096);
        {
            char hdr[256]{};
            sprintf_s(hdr,
                "FRONTIER Instance Dump\nRoot: %s \"%s\"\nAddress: 0x%llX\nMaxDepth: %d\n\n",
                n.className.c_str(), n.name.c_str(),
                (unsigned long long)n.addr, maxDepth);
            payload += hdr;
        }
        {
            std::ostringstream oss;
            // Dump into string first so we don't leave a half-written file on crash
            std::function<void(Node&, int)> dump;
            dump = [&](Node& node, int depth) {
                if (depth > maxDepth) return;
                for (int i = 0; i < depth; i++) oss << "  ";
                oss << node.className << " \"" << node.name << "\" @"
                    << std::hex << node.addr << std::dec << "\n";
                if (!node.childrenLoaded)
                    LoadChildren(node);
                for (auto& c : node.children)
                    dump(c, depth + 1);
            };
            dump(n, 0);
            payload += oss.str();
        }

        if (!writeAll(filePath, payload)) {
            // Fallback next to the exe
            char exePath[MAX_PATH]{};
            GetModuleFileNameA(nullptr, exePath, MAX_PATH);
            std::filesystem::path fallback = std::filesystem::path(exePath).parent_path() / "dumps";
            std::error_code ec;
            std::filesystem::create_directories(fallback, ec);
            filePath = fallback / filePath.filename();
            if (!writeAll(filePath, payload)) {
                sprintf_s(statusMsg, "Failed to write dump (check permissions)");
                return false;
            }
        }

        strncpy_s(lastSavePath, filePath.string().c_str(), _TRUNCATE);
        sprintf_s(statusMsg, "Saved to %s", lastSavePath);
        return true;
    }

    inline void SaveSelected()
    {
        if (!selectedAddr) {
            strcpy_s(statusMsg, "Select an instance first");
            return;
        }
        if (saveBusy) return;

        Node n = NodeFromAddr(selectedAddr);
        const char* label = saveFileName[0] ? saveFileName : "instance";
        saveBusy = true;
        SaveNodeToFile(n, label, saveMaxDepth);
        saveBusy = false;
    }

    inline void SaveFullGame()
    {
        if (!Globals::dataModel.Addr) {
            strcpy_s(statusMsg, "No DataModel — join a game first");
            return;
        }
        if (saveBusy) return;

        if (!root.addr || !root.childrenLoaded)
            RefreshRoot();

        const char* label = saveFileName[0] ? saveFileName : "game_dump";
        saveBusy = true;
        SaveNodeToFile(root, label, saveMaxDepth);
        saveBusy = false;
    }

    inline void OpenDumpFolder()
    {
        auto dir = DumpDir();
        ShellExecuteA(nullptr, "open", dir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }

    struct InspectField {
        char key[48];
        char value[256];
        ExplorerSyntax::Lang lang = ExplorerSyntax::Lang::Plain;
    };

    inline std::vector<InspectField> gInspectFields;
    inline char gLuaPath[512] = "";
    inline char gPreviewText[8192] = "";
    inline ExplorerSyntax::Lang gPreviewLang = ExplorerSyntax::Lang::Plain;
    inline int gChildCount = 0;

    inline std::string ReadRobloxString(uintptr_t strObj)
    {
        if (!strObj) return {};
        return memory->read_string(strObj);
    }

    inline std::string BuildLuaPath(uintptr_t addr)
    {
        if (!addr) return "game";
        std::vector<std::string> parts;
        uintptr_t cur = addr;
        for (int i = 0; i < 32 && cur; i++) {
            RBX::RbxInstance inst(cur);
            std::string n = inst.GetName();
            if (n.empty()) n = "Instance";
            parts.push_back(n);
            cur = memory->read<uintptr_t>(cur + Offsets::Instance::Parent);
            if (cur == Globals::dataModel.Addr) {
                parts.push_back("game");
                break;
            }
        }
        std::string out;
        for (int i = (int)parts.size() - 1; i >= 0; i--) {
            if (!out.empty()) out += ".";
            const std::string& seg = parts[i];
            bool safe = !seg.empty();
            for (char c : seg) {
                if (!isalnum((unsigned char)c) && c != '_') { safe = false; break; }
            }
            if (safe)
                out += seg;
            else
                out += "[\"" + seg + "\"]";
        }
        return out.empty() ? "game" : out;
    }

    inline void AddField(const char* key, const char* val, ExplorerSyntax::Lang lang = ExplorerSyntax::Lang::Plain)
    {
        InspectField f{};
        strncpy_s(f.key, key, _TRUNCATE);
        strncpy_s(f.value, val ? val : "", _TRUNCATE);
        f.lang = lang;
        gInspectFields.push_back(f);
    }

    inline void RefreshInspector()
    {
        gInspectFields.clear();
        gLuaPath[0] = 0;
        gPreviewText[0] = 0;
        gPreviewLang = ExplorerSyntax::Lang::Plain;
        gChildCount = 0;

        if (!selectedAddr) return;

        RBX::RbxInstance inst(selectedAddr);
        auto kids = inst.GetChildList();
        gChildCount = (int)kids.size();

        std::string luaPath = BuildLuaPath(selectedAddr);
        strncpy_s(gLuaPath, luaPath.c_str(), _TRUNCATE);
        selectedPath = luaPath;

        uintptr_t parent = memory->read<uintptr_t>(selectedAddr + Offsets::Instance::Parent);
        if (parent) {
            RBX::RbxInstance p(parent);
            AddField("Parent", p.GetName().c_str(), ExplorerSyntax::Lang::Path);
        } else {
            AddField("Parent", "nil", ExplorerSyntax::Lang::Plain);
        }

        char buf[128];
        sprintf_s(buf, "%d", gChildCount);
        AddField("Children", buf, ExplorerSyntax::Lang::Number);

        const std::string& cls = selectedClass;

        if (ClassEq(cls, "StringValue")) {
            std::string v = ReadRobloxString(memory->read<uintptr_t>(selectedAddr + Offsets::Misc::Value));
            AddField("Value", v.c_str(), ExplorerSyntax::Lang::Plain);
            strncpy_s(gPreviewText, v.c_str(), _TRUNCATE);
            gPreviewLang = ExplorerSyntax::Lang::Plain;
        }
        else if (ClassEq(cls, "IntValue")) {
            int v = memory->read<int>(selectedAddr + Offsets::Misc::Value);
            sprintf_s(buf, "%d", v);
            AddField("Value", buf, ExplorerSyntax::Lang::Number);
        }
        else if (ClassEq(cls, "BoolValue")) {
            bool v = memory->read<bool>(selectedAddr + Offsets::Misc::Value);
            AddField("Value", v ? "true" : "false", ExplorerSyntax::Lang::Bool);
        }
        else if (ClassEq(cls, "NumberValue")) {
            double v = memory->read<double>(selectedAddr + Offsets::Misc::Value);
            sprintf_s(buf, "%.6g", v);
            AddField("Value", buf, ExplorerSyntax::Lang::Number);
        }
        else if (ClassEq(cls, "ObjectValue")) {
            uintptr_t ref = memory->read<uintptr_t>(selectedAddr + Offsets::Misc::Value);
            if (ref) {
                RBX::RbxInstance o(ref);
                sprintf_s(buf, "%s (%s)", o.GetName().c_str(), o.GetClass().c_str());
                AddField("Value", buf, ExplorerSyntax::Lang::Path);
            } else {
                AddField("Value", "nil", ExplorerSyntax::Lang::Plain);
            }
        }
        else if (ClassEq(cls, "Part") || ClassEq(cls, "MeshPart") || ClassEq(cls, "BasePart") ||
                 ClassEq(cls, "UnionOperation") || ClassEq(cls, "WedgePart")) {
            RBX::RbxInstance part(selectedAddr);
            RBX::Vec3 pos = part.GetPos();
            sprintf_s(buf, "%.2f, %.2f, %.2f", pos.X, pos.Y, pos.Z);
            AddField("Position", buf, ExplorerSyntax::Lang::Number);
        }
        else if (ClassEq(cls, "Player")) {
            RBX::RbxInstance plr(selectedAddr);
            AddField("DisplayName", plr.GetDisplayName().c_str(), ExplorerSyntax::Lang::Plain);
            sprintf_s(buf, "%llu", (unsigned long long)memory->read<uint64_t>(selectedAddr + Offsets::Player::UserId));
            AddField("UserId", buf, ExplorerSyntax::Lang::Number);
        }
        else if (ClassEq(cls, "Script") || ClassEq(cls, "LocalScript") || ClassEq(cls, "ModuleScript")) {
            AddField("Type", cls.c_str(), ExplorerSyntax::Lang::Lua);
            sprintf_s(gPreviewText,
                "-- Script source is bytecode in memory.\n"
                "-- Use Save Branch to export hierarchy.\n"
                "local inst = %s\n"
                "print(inst.ClassName, inst.Name)",
                luaPath.c_str());
            gPreviewLang = ClassEq(cls, "ModuleScript") ? ExplorerSyntax::Lang::Luau : ExplorerSyntax::Lang::Lua;
        }

        // Attributes sample
        std::string team = inst.GetAttributeString("Team");
        if (!team.empty())
            AddField("Attr.Team", team.c_str(), ExplorerSyntax::Lang::Plain);
    }

    inline bool JumpToService(const char* serviceName)
    {
        if (!root.childrenLoaded)
            LoadChildren(root);
        for (auto& c : root.children) {
            if (_stricmp(c.name.c_str(), serviceName) == 0) {
                Select(c, c.name);
                RefreshInspector();
                c.expanded = true;
                if (!c.childrenLoaded)
                    LoadChildren(c);
                return true;
            }
        }
        sprintf_s(statusMsg, "Service not found: %s", serviceName);
        return false;
    }

    inline void DrawInspectorPanel(bool busy)
    {
        if (!selectedAddr) {
            ImGui::Dummy(ImVec2(0, 24));
            ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Select an instance");
            return;
        }

        if (gLuaPath[0] == 0)
            RefreshInspector();

        ImGui::Spacing();
        InstanceExplorer::DrawSelectedIcon(26.f);
        ImGui::SameLine(0, 10);
        ImGui::BeginGroup();
        ImGui::TextUnformatted(selectedName.c_str());
        ImGui::TextColored(ClassColorV4(selectedClass), "%s", selectedClass.c_str());
        ImGui::EndGroup();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Lua path");
        ExplorerSyntax::DrawBlock(ExplorerSyntax::Lang::Path, gLuaPath, 36.f);

        ImGui::Spacing();
        if (ImGui::BeginTable("##insp", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 88.f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            for (const auto& f : gInspectFields) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "%s", f.key);
                ImGui::TableNextColumn();
                ImDrawList* dl = ImGui::GetWindowDrawList();
                ImVec2 pos = ImGui::GetCursorScreenPos();
                float maxX = pos.x + ImGui::GetContentRegionAvail().x;
                ExplorerSyntax::HighlightLine(dl, pos, maxX, f.lang, f.value);
                ImGui::Dummy(ImVec2(0, ImGui::GetTextLineHeight()));
            }
            ImGui::EndTable();
        }

        if (gPreviewText[0]) {
            ImGui::Spacing();
            const char* langLbl = "Preview";
            if (gPreviewLang == ExplorerSyntax::Lang::Lua) langLbl = "Lua";
            else if (gPreviewLang == ExplorerSyntax::Lang::Luau) langLbl = "Luau";
            else if (gPreviewLang == ExplorerSyntax::Lang::Json) langLbl = "JSON";
            ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "%s", langLbl);
            ExplorerSyntax::DrawBlock(gPreviewLang, gPreviewText, 100.f);
        }

        ImGui::Spacing();
        if (ImGui::Button("Copy Lua Path", ImVec2(-1, 28)))
            ImGui::SetClipboardText(gLuaPath);
        if (ImGui::Button("Copy Address", ImVec2(-1, 28))) {
            char addrBuf[32];
            sprintf_s(addrBuf, "0x%llX", (unsigned long long)selectedAddr);
            ImGui::SetClipboardText(addrBuf);
        }
        ImGui::BeginDisabled(busy);
        if (ImGui::Button("Save Branch", ImVec2(-1, 28)))
            SaveSelected();
        ImGui::EndDisabled();

        if (lastSavePath[0]) {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(FrontierUI::V4(variables::Theme::textDim), "Last save");
            ImGui::PushTextWrapPos(0);
            ImGui::TextColored(FrontierUI::V4(variables::Theme::accent), "%s", lastSavePath);
            ImGui::PopTextWrapPos();
        }
    }
}
