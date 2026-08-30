#pragma once
#include <cstdint>
#include <string>
#include "sdk.h"

namespace Scanner
{
	struct AnchorResult
	{
		std::uint64_t dataModel = 0;
		std::uint64_t visualEngine = 0;
		std::uint64_t fakeDataModel = 0;
		std::uint64_t fakeDataModelRva = 0;
		std::uint64_t visualEngineRva = 0;
		std::uint64_t realDataModelOffset = 0;
		std::string method;
		bool success = false;
	};

	// Tries hardcoded offsets first, then scans the Roblox module for valid anchors.
	AnchorResult ResolveAnchors();

	bool ValidateVisualEngine(std::uint64_t visualEngine);
	bool ViewMatrixLooksValid(const RBX::Mat4& matrix);
}
