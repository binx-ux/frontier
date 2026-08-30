#include "tp_handler.h"

#include <chrono>
#include <thread>
#include <windows.h>

#include "../globals/globals.h"
#include "../../memory/memory.h"
#include "../../sdk/offsets.h"
#include "../../sdk/scanner.h"
#include "../../sdk/sdk.h"
#include "../../sdk/window_manager.h"
#include "../debug_log.h"

namespace {
	void reset_globals()
	{
		Globals::dataModel = RBX::RbxInstance(0);
		Globals::renderEngine = RBX::RenderEngine(0);
		Globals::workspace = RBX::RbxInstance(0);
		Globals::players = RBX::RbxInstance(0);
		Globals::camera = RBX::RbxInstance(0);
		Globals::localPlayer = RBX::RbxInstance(0);
	}
}

void Core::tp_handler::thread()
{
	int failStreak = 0;
	while (Globals::running) {
		if (!memory->IsConnected()) {
			DebugLog::Write("tp_handler: memory disconnected");
			Globals::running = false;
			break;
		}

		if (!WindowManager::IsRobloxOpen() && !memory->find_process_id("RobloxPlayerBeta.exe")) {
			DebugLog::Write("tp_handler: Roblox closed");
			Globals::running = false;
			break;
		}

		auto baseAddr = memory->get_module_address();
		if (!baseAddr) {
			if (++failStreak >= 8) {
				DebugLog::Write("tp_handler: module base lost");
				reset_globals();
				failStreak = 0;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		auto fakeDataModelAddr = baseAddr + Offsets::FakeDataModel::Pointer;
		auto fakeDataModel = memory->read<uintptr_t>(fakeDataModelAddr);
		if (!fakeDataModel) {
			if (++failStreak >= 8) {
				const auto anchors = Scanner::ResolveAnchors();
				if (anchors.success) {
					Globals::dataModel = RBX::RbxInstance(anchors.dataModel);
					Globals::renderEngine = RBX::RenderEngine(anchors.visualEngine);
					Globals::RefreshServices();
					failStreak = 0;
					DebugLog::Write("tp_handler: re-resolved anchors after fake DM miss");
				} else {
					reset_globals();
					failStreak = 0;
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		auto dataModelAddr = fakeDataModel + Offsets::FakeDataModel::RealDataModel;
		auto dataModelPtr = memory->read<uintptr_t>(dataModelAddr);
		auto visualEngineAddr = baseAddr + Offsets::VisualEngine::Pointer;
		auto visualEngine = memory->read<uintptr_t>(visualEngineAddr);

		if (!dataModelPtr || !visualEngine || !Scanner::ValidateVisualEngine(visualEngine)) {
			if (++failStreak >= 8) {
				const auto anchors = Scanner::ResolveAnchors();
				if (anchors.success) {
					Globals::dataModel = RBX::RbxInstance(anchors.dataModel);
					Globals::renderEngine = RBX::RenderEngine(anchors.visualEngine);
					Globals::RefreshServices();
					failStreak = 0;
					DebugLog::Write("tp_handler: re-resolved anchors after VE/DM miss");
				} else {
					reset_globals();
					failStreak = 0;
					DebugLog::Write("tp_handler: anchor re-resolve failed");
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		failStreak = 0;
		Globals::dataModel = RBX::RbxInstance(dataModelPtr);
		Globals::renderEngine = RBX::RenderEngine(visualEngine);
		Globals::RefreshServices();

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}
