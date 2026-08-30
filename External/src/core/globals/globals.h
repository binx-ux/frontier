#pragma once
#include "../../../src/sdk/sdk.h"
#include "../../../src/sdk/offsets.h"
#include "../../../src/memory/memory.h"

namespace Globals {
    inline RBX::RbxInstance dataModel(0);
    inline RBX::RenderEngine renderEngine(0);
    inline RBX::RbxInstance workspace(0);
    inline RBX::RbxInstance players(0);
    inline bool running = true;
    inline RBX::RbxInstance camera(0);
    inline RBX::RbxInstance localPlayer(0);

    inline RBX::RbxInstance ResolveWorkspaceCamera()
    {
        if (!workspace.Addr)
            return RBX::RbxInstance(0);

        const uintptr_t cam = memory->read<uintptr_t>(
            workspace.Addr + Offsets::Workspace::CurrentCamera);
        if (cam)
            return RBX::RbxInstance(cam);

        return workspace.FindChildByClass("Camera");
    }

    inline void RefreshLocalPlayer()
    {
        if (!players.Addr)
            return;
        const uintptr_t lp = memory->read<uintptr_t>(
            players.Addr + Offsets::Player::LocalPlayer);
        if (lp)
            localPlayer = RBX::RbxInstance(lp);
    }

    inline void RefreshServices()
    {
        if (!dataModel.Addr)
            return;
        workspace = dataModel.FindChildByClass("Workspace");
        players = dataModel.FindChildByClass("Players");
        camera = ResolveWorkspaceCamera();
        RefreshLocalPlayer();
    }
}
