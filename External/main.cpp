#include <cstdint>
#include <cstdio>
#include <windows.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <string>
#include <unordered_map>
#include "src/memory/memory.h"
#include "src/sdk/offsets.h"
#include "src/sdk/sdk.h"
#include "src/sdk/scanner.h"
#include "src/sdk/w2s.h"
#include "src/core/cache/cache.h"
#include "src/core/globals/globals.h"
#include "src/core/tp_handler/tp_handler.h"
#include "src/core/features/visuals/visuals.h"
#include "src/core/features/aimbot/aimbot.h"
#include "src/core/features/exploits/exploits.h"
#include "src/core/features/exploits/gun_mods.h"
#include "src/core/audio/custom_music.h"
#include "src/core/games/arsenal.h"
#include "src/core/variables/variables.h"
#include "src/core/telemetry/telemetry.h"
#include "src/core/features/combat_fx.h"
#include "src/render/render.h"
#include "src/discord/frontier_presence.h"
#include "src/core/debug_log.h"
#include "src/core/auth/session_gate.h"
#include "src/core/config/config.h"

namespace
{
	constexpr const char* app = "RobloxPlayerBeta.exe";
	constexpr const wchar_t* apptitle = L"Roblox";

	// Feature keybinds only while Roblox is focused and the menu is not capturing input.
	inline bool GameKeyDown(int vk)
	{
		if ((GetAsyncKeyState(vk) & 0x8000) == 0)
			return false;
		if (!WindowManager::IsRobloxFocused())
			return false;
		if (variables::Misc::menuHovered || variables::waitingForKey)
			return false;
		return true;
	}

	void SetLoad(float p, const char* status)
	{
		variables::Loading::progress = p;
		strncpy_s(variables::Loading::status, status, _TRUNCATE);
		if (variables::Misc::discordRpc)
			FrontierPresence::SetLoading(status);
	}

	inline bool RobloxIsRunning()
	{
		return WindowManager::IsRobloxOpen() || memory->find_process_id(app) != 0;
	}

	void ShowTimedExitPrompt(OverlayWindow& overlay, const char* message, float seconds = 4.f)
	{
		variables::Loading::active = true;
		variables::Loading::failed = true;
		variables::Loading::progress = 0.f;
		strncpy_s(variables::Loading::error, message, _TRUNCATE);

		const auto start = std::chrono::steady_clock::now();
		while (true) {
			const float elapsed = std::chrono::duration<float>(
				std::chrono::steady_clock::now() - start).count();
			if (elapsed >= seconds)
				break;

			overlay.BeginFrame();
			overlay.RenderLoading();
			overlay.EndFrame();
		}
	}

	bool isgamerunning(const wchar_t*)
	{
		return RobloxIsRunning();
	}

	void PanicDisableAll()
	{
		variables::Aimbot::enabled = false;
		variables::Aimbot::toggledOn = false;
		variables::Aimbot::alwaysOn = false;
		variables::Trigger::enabled = false;
		variables::Rage::enabled = false;
		variables::ESP::enabled = false;
		variables::ESP::oofArrows = false;
		variables::Crosshair::enabled = false;
		variables::Radar::enabled = false;
		variables::Local::speedEnabled = false;
		variables::Local::jumpEnabled = false;
		variables::Local::flyEnabled = false;
		variables::Local::flyActive = false;
		variables::Local::bhopEnabled = false;
		variables::Local::desyncEnabled = false;
		variables::Local::hitboxEnabled = false;
		variables::Local::freeze = false;
		variables::Local::spin = false;
		variables::Local::walkFling = false;
		variables::Local::noclip = false;
		variables::Local::floatEnabled = false;
		variables::Local::autoTp = false;
		variables::Local::clickTp = false;
		variables::Local::gravityEnabled = false;
		variables::Local::godMode = false;
		variables::Local::tpWalk = false;
		variables::Local::autoClicker = false;
		variables::Local::orbitPlayer = false;
		variables::Hitbox::enabled = false;
		variables::MagicBullet::enabled = false;
		variables::Desync::enabled = false;
		variables::Exploits::animation_changer = false;
		variables::Misc::afkAssist = false;
		variables::menuOpen = false;
		Aimbot::OnAimReleased();
		Aimbot::lockedPlayerAddr = 0;
		CustomMusic::StopLocal();
		GunMods::DisableAll();
	}

	void WriteLocalVelocity(RBX::RbxInstance rootPart, const RBX::Vec3& vel)
	{
		auto prim = rootPart.GetPrimitivePtr();
		if (prim == 0) return;
		memory->write<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity, vel);
	}

	void WriteLocalAngularVelocity(RBX::RbxInstance rootPart, const RBX::Vec3& av)
	{
		auto prim = rootPart.GetPrimitivePtr();
		if (prim == 0) return;
		memory->write<RBX::Vec3>(prim + Offsets::Primitive::AssemblyAngularVelocity, av);
	}

	void ForceHumanoidRunning(uintptr_t humanoidAddr)
	{
		if (!humanoidAddr) return;
		const uintptr_t statePtr = memory->read<uintptr_t>(humanoidAddr + Offsets::Humanoid::HumanoidState);
		if (!statePtr) return;
		const int stateId = memory->read<int>(statePtr + Offsets::Humanoid::HumanoidStateID);
		if (stateId == 0 || stateId == 5 || stateId == 6 || stateId == 8 || stateId == 10 || stateId == 15)
			memory->write<int>(statePtr + Offsets::Humanoid::HumanoidStateID, 1);
	}

	void ApplyCharacterPartFlags(RBX::RbxInstance character, bool noCollide, bool noTouch);

	void ApplyAntiFling(RBX::RbxInstance rootPart, RBX::RbxInstance humanoid, RBX::RbxInstance character)
	{
		if (!variables::Local::antiFling || !rootPart.Addr)
			return;

		auto prim = rootPart.GetPrimitivePtr();
		if (!prim)
			return;

		static RBX::Vec3 lastSafePos{};
		static bool haveSafe = false;
		static auto recoverUntil = std::chrono::steady_clock::now();

		WriteLocalAngularVelocity(rootPart, { 0.f, 0.f, 0.f });
		memory->write<RBX::Vec3>(prim + Offsets::Primitive::AssemblyAngularVelocity, { 0.f, 0.f, 0.f });

		const RBX::Vec3 pos = rootPart.GetPos();
		RBX::Vec3 v = memory->read<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity);
		const float horiz = sqrtf(v.X * v.X + v.Z * v.Z);

		float expected = 16.f;
		if (variables::Local::speedEnabled)
			expected = (std::max)(expected, variables::Local::walkSpeed * 1.05f);
		if (variables::Local::flyEnabled && variables::Local::flyActive)
			expected = (std::max)(expected, variables::Local::flySpeed * 1.05f);
		if (variables::Local::bhopEnabled)
			expected = (std::max)(expected, variables::Local::bhopSpeed * 1.05f);

		const float softCap = expected + 6.f;
		const float hardCap = expected + 18.f;
		bool flung = horiz > softCap || fabsf(v.Y) > 22.f;

		if (haveSafe) {
			const float dx = pos.X - lastSafePos.X;
			const float dy = pos.Y - lastSafePos.Y;
			const float dz = pos.Z - lastSafePos.Z;
			const float disp = sqrtf(dx * dx + dy * dy + dz * dz);
			if (disp > 5.f)
				flung = true;
		}

		if (flung) {
			if (horiz > hardCap || fabsf(v.Y) > 36.f)
				v = { 0.f, 0.f, 0.f };
			else {
				if (horiz > softCap && horiz > 0.01f) {
					const float scale = softCap / horiz;
					v.X *= scale;
					v.Z *= scale;
				}
				if (fabsf(v.Y) > 22.f)
					v.Y *= 0.12f;
			}

			WriteLocalVelocity(rootPart, v);
			memory->write<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity, v);
			WriteLocalAngularVelocity(rootPart, { 0.f, 0.f, 0.f });

			if (haveSafe) {
				const float dx = pos.X - lastSafePos.X;
				const float dy = pos.Y - lastSafePos.Y;
				const float dz = pos.Z - lastSafePos.Z;
				if (sqrtf(dx * dx + dy * dy + dz * dz) > 5.f) {
					RBX::Vec3 fix = lastSafePos;
					fix.Y += 1.5f;
					rootPart.SetPos(fix);
					WriteLocalVelocity(rootPart, { 0.f, 0.f, 0.f });
				}
			}

			if (humanoid.Addr) {
				ForceHumanoidRunning(humanoid.Addr);
				memory->write<uint8_t>(humanoid.Addr + Offsets::Humanoid::PlatformStand, 0);
				memory->write<uint8_t>(humanoid.Addr + Offsets::Humanoid::AutoRotate, 1);
			}

			ApplyCharacterPartFlags(character, true, true);
			recoverUntil = std::chrono::steady_clock::now() + std::chrono::milliseconds(150);
		}
		else {
			lastSafePos = pos;
			haveSafe = true;
			if (humanoid.Addr && std::chrono::steady_clock::now() >= recoverUntil)
				memory->write<uint8_t>(humanoid.Addr + Offsets::Humanoid::PlatformStand, 0);
		}
	}

	void WritePrimVelocityBurst(uintptr_t prim, const RBX::Vec3& vel, int reps)
	{
		if (!prim || reps < 1) return;
		for (int i = 0; i < reps; ++i)
			memory->write<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity, vel);
	}

	inline bool IsCollidablePartClass(const std::string& cls)
	{
		return cls == "Part" || cls == "MeshPart" || cls == "UnionOperation" ||
			cls == "WedgePart" || cls == "CornerWedgePart" || cls == "TrussPart" ||
			cls == "SpawnLocation";
	}

	void ApplyCharacterPartFlags(RBX::RbxInstance character, bool noCollide, bool noTouch)
	{
		static std::unordered_map<uintptr_t, uint8_t> flagOrig;
		static uintptr_t lastChar = 0;
		if (character.Addr != lastChar) {
			flagOrig.clear();
			lastChar = character.Addr;
		}

		if (!noCollide && !noTouch) {
			for (auto& kv : flagOrig) {
				RBX::RbxInstance part(kv.first);
				auto prim = part.GetPrimitivePtr();
				if (prim)
					memory->write<uint8_t>(prim + Offsets::Primitive::Flags, kv.second);
			}
			flagOrig.clear();
			return;
		}

		for (auto& ch : character.GetChildList()) {
			if (!IsCollidablePartClass(ch.GetClass())) continue;
			auto prim = ch.GetPrimitivePtr();
			if (!prim) continue;
			if (flagOrig.find(ch.Addr) == flagOrig.end())
				flagOrig[ch.Addr] = memory->read<uint8_t>(prim + Offsets::Primitive::Flags);
			uint8_t f = flagOrig[ch.Addr];
			if (noCollide) f = (uint8_t)(f & ~Offsets::PrimitiveFlags::CanCollide);
			if (noTouch) f = (uint8_t)(f & ~Offsets::PrimitiveFlags::CanTouch);
			memory->write<uint8_t>(prim + Offsets::Primitive::Flags, f);
		}
	}

	void localThread()
	{
		bool flyKeyLatched = false;
		auto lastTick = std::chrono::steady_clock::now();
		while (Globals::running) {
			auto now = std::chrono::steady_clock::now();
			float dt = std::chrono::duration<float>(now - lastTick).count();
			lastTick = now;
			if (dt < 0.001f) dt = 0.001f;
			if (dt > 0.05f) dt = 0.05f;

			if (variables::Loading::active) {
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				continue;
			}

			if (!Globals::players.Addr || !Globals::workspace.Addr || !Globals::localPlayer.Addr)
				Globals::RefreshServices();

			// World / lighting — capture originals so disable restores cleanly
			if (Globals::dataModel.Addr != 0) {
				struct LightSnap {
					bool valid = false;
					float brightness = 1.f, fogStart = 0.f, fogEnd = 100000.f, clock = 14.f, exposure = 0.f;
					float ambient[3]{1,1,1}, outdoor[3]{1,1,1}, fogCol[3]{1,1,1}, lightCol[3]{1,1,1};
					float shiftTop[3]{}, shiftBot[3]{};
					float envDiff = 1.f, envSpec = 1.f;
					bool shadows = true;
					bool hasAtmo = false;
					uintptr_t atmoAddr = 0;
					float atmoDensity = 0.f, atmoHaze = 0.f, atmoGlare = 0.f, atmoOffset = 0.f;
				};
				static LightSnap lightSnap;
				static bool lightWasOn = false;

				auto lighting = Globals::dataModel.FindChildByClass("Lighting");
				const bool wantLight =
					variables::World::fullbright || variables::World::customBrightness ||
					variables::World::noFog || variables::World::noShadows ||
					variables::World::customClock || variables::World::nightMode ||
					variables::World::customAmbient || variables::World::removeAtmosphere;

				auto readColor = [&](uintptr_t off, float* out) {
					out[0] = memory->read<float>(lighting.Addr + off + 0);
					out[1] = memory->read<float>(lighting.Addr + off + 4);
					out[2] = memory->read<float>(lighting.Addr + off + 8);
				};
				auto writeColor = [&](uintptr_t off, float r, float g, float b) {
					memory->write<float>(lighting.Addr + off + 0, r);
					memory->write<float>(lighting.Addr + off + 4, g);
					memory->write<float>(lighting.Addr + off + 8, b);
				};

				if (lighting.Addr != 0) {
					if (wantLight) {
						if (!lightSnap.valid) {
							lightSnap.brightness = memory->read<float>(lighting.Addr + Offsets::Lighting::Brightness);
							lightSnap.fogStart = memory->read<float>(lighting.Addr + Offsets::Lighting::FogStart);
							lightSnap.fogEnd = memory->read<float>(lighting.Addr + Offsets::Lighting::FogEnd);
							lightSnap.clock = memory->read<float>(lighting.Addr + Offsets::Lighting::ClockTime);
							lightSnap.exposure = memory->read<float>(lighting.Addr + Offsets::Lighting::ExposureCompensation);
							lightSnap.envDiff = memory->read<float>(lighting.Addr + Offsets::Lighting::EnvironmentDiffuseScale);
							lightSnap.envSpec = memory->read<float>(lighting.Addr + Offsets::Lighting::EnvironmentSpecularScale);
							lightSnap.shadows = memory->read<bool>(lighting.Addr + Offsets::Lighting::GlobalShadows);
							readColor(Offsets::Lighting::Ambient, lightSnap.ambient);
							readColor(Offsets::Lighting::OutdoorAmbient, lightSnap.outdoor);
							readColor(Offsets::Lighting::FogColor, lightSnap.fogCol);
							readColor(Offsets::Lighting::LightColor, lightSnap.lightCol);
							readColor(Offsets::Lighting::ColorShift_Top, lightSnap.shiftTop);
							readColor(Offsets::Lighting::ColorShift_Bottom, lightSnap.shiftBot);
							for (auto& ch : lighting.GetChildList()) {
								if (ch.GetClass() == "Atmosphere") {
									lightSnap.hasAtmo = true;
									lightSnap.atmoAddr = ch.Addr;
									lightSnap.atmoDensity = memory->read<float>(ch.Addr + Offsets::Atmosphere::Density);
									lightSnap.atmoHaze = memory->read<float>(ch.Addr + Offsets::Atmosphere::Haze);
									lightSnap.atmoGlare = memory->read<float>(ch.Addr + Offsets::Atmosphere::Glare);
									lightSnap.atmoOffset = memory->read<float>(ch.Addr + Offsets::Atmosphere::Offset);
									break;
								}
							}
							lightSnap.valid = true;
						}
						lightWasOn = true;

						{
							if (variables::World::fullbright) {
								writeColor(Offsets::Lighting::Ambient, 1.f, 1.f, 1.f);
								writeColor(Offsets::Lighting::OutdoorAmbient, 1.f, 1.f, 1.f);
								writeColor(Offsets::Lighting::ColorShift_Top, 0.f, 0.f, 0.f);
								writeColor(Offsets::Lighting::ColorShift_Bottom, 0.f, 0.f, 0.f);
								writeColor(Offsets::Lighting::FogColor, 1.f, 1.f, 1.f);
								writeColor(Offsets::Lighting::LightColor, 1.f, 1.f, 1.f);
								memory->write<float>(lighting.Addr + Offsets::Lighting::Brightness, 4.0f);
								memory->write<float>(lighting.Addr + Offsets::Lighting::ClockTime, 14.0f);
								memory->write<float>(lighting.Addr + Offsets::Lighting::ExposureCompensation, 0.35f);
								memory->write<float>(lighting.Addr + Offsets::Lighting::FogStart, 0.0f);
								memory->write<float>(lighting.Addr + Offsets::Lighting::FogEnd, 1.0e6f);
								memory->write<float>(lighting.Addr + Offsets::Lighting::EnvironmentDiffuseScale, 1.0f);
								memory->write<float>(lighting.Addr + Offsets::Lighting::EnvironmentSpecularScale, 0.0f);
								memory->write<bool>(lighting.Addr + Offsets::Lighting::GlobalShadows, false);
								for (auto& ch : lighting.GetChildList()) {
									if (ch.GetClass() == "Atmosphere") {
										memory->write<float>(ch.Addr + Offsets::Atmosphere::Density, 0.0f);
										memory->write<float>(ch.Addr + Offsets::Atmosphere::Haze, 0.0f);
										memory->write<float>(ch.Addr + Offsets::Atmosphere::Glare, 0.0f);
										memory->write<float>(ch.Addr + Offsets::Atmosphere::Offset, 0.0f);
									}
								}
							}
							else {
								if (variables::World::nightMode) {
									memory->write<float>(lighting.Addr + Offsets::Lighting::ClockTime, 0.0f);
									if (!variables::World::customBrightness)
										memory->write<float>(lighting.Addr + Offsets::Lighting::Brightness, 0.35f);
									writeColor(Offsets::Lighting::Ambient, 0.06f, 0.06f, 0.14f);
									writeColor(Offsets::Lighting::OutdoorAmbient, 0.04f, 0.04f, 0.10f);
									writeColor(Offsets::Lighting::ColorShift_Top, 0.02f, 0.02f, 0.08f);
									writeColor(Offsets::Lighting::ColorShift_Bottom, 0.01f, 0.01f, 0.05f);
									writeColor(Offsets::Lighting::FogColor, 0.05f, 0.05f, 0.12f);
								}
								else if (variables::World::customClock) {
									memory->write<float>(lighting.Addr + Offsets::Lighting::ClockTime, variables::World::clockTime);
								}
								if (variables::World::customBrightness) {
									memory->write<float>(lighting.Addr + Offsets::Lighting::Brightness, variables::World::brightness);
								}
								if (variables::World::noFog) {
									memory->write<float>(lighting.Addr + Offsets::Lighting::FogEnd, 1.0e6f);
									memory->write<float>(lighting.Addr + Offsets::Lighting::FogStart, 0.0f);
								}
								if (variables::World::noShadows) {
									memory->write<bool>(lighting.Addr + Offsets::Lighting::GlobalShadows, false);
								}
								if (variables::World::customAmbient) {
									float ar = variables::World::ambientColor[0];
									float ag = variables::World::ambientColor[1];
									float ab = variables::World::ambientColor[2];
									variables::World::ambientR = ar;
									variables::World::ambientG = ag;
									variables::World::ambientB = ab;
									writeColor(Offsets::Lighting::Ambient, ar, ag, ab);
									writeColor(Offsets::Lighting::OutdoorAmbient, ar, ag, ab);
									writeColor(Offsets::Lighting::ColorShift_Top, ar * 0.35f, ag * 0.35f, ab * 0.35f);
									writeColor(Offsets::Lighting::LightColor, ar, ag, ab);
								}
								if (variables::World::removeAtmosphere) {
									for (auto& ch : lighting.GetChildList()) {
										if (ch.GetClass() == "Atmosphere") {
											memory->write<float>(ch.Addr + Offsets::Atmosphere::Density, 0.0f);
											memory->write<float>(ch.Addr + Offsets::Atmosphere::Haze, 0.0f);
											memory->write<float>(ch.Addr + Offsets::Atmosphere::Glare, 0.0f);
										}
									}
								}
							}
						}
					}
					else if (lightWasOn && lightSnap.valid) {
						memory->write<float>(lighting.Addr + Offsets::Lighting::Brightness, lightSnap.brightness);
						memory->write<float>(lighting.Addr + Offsets::Lighting::FogStart, lightSnap.fogStart);
						memory->write<float>(lighting.Addr + Offsets::Lighting::FogEnd, lightSnap.fogEnd);
						memory->write<float>(lighting.Addr + Offsets::Lighting::ClockTime, lightSnap.clock);
						memory->write<float>(lighting.Addr + Offsets::Lighting::ExposureCompensation, lightSnap.exposure);
						memory->write<float>(lighting.Addr + Offsets::Lighting::EnvironmentDiffuseScale, lightSnap.envDiff);
						memory->write<float>(lighting.Addr + Offsets::Lighting::EnvironmentSpecularScale, lightSnap.envSpec);
						memory->write<bool>(lighting.Addr + Offsets::Lighting::GlobalShadows, lightSnap.shadows);
						writeColor(Offsets::Lighting::Ambient, lightSnap.ambient[0], lightSnap.ambient[1], lightSnap.ambient[2]);
						writeColor(Offsets::Lighting::OutdoorAmbient, lightSnap.outdoor[0], lightSnap.outdoor[1], lightSnap.outdoor[2]);
						writeColor(Offsets::Lighting::FogColor, lightSnap.fogCol[0], lightSnap.fogCol[1], lightSnap.fogCol[2]);
						writeColor(Offsets::Lighting::LightColor, lightSnap.lightCol[0], lightSnap.lightCol[1], lightSnap.lightCol[2]);
						writeColor(Offsets::Lighting::ColorShift_Top, lightSnap.shiftTop[0], lightSnap.shiftTop[1], lightSnap.shiftTop[2]);
						writeColor(Offsets::Lighting::ColorShift_Bottom, lightSnap.shiftBot[0], lightSnap.shiftBot[1], lightSnap.shiftBot[2]);
						if (lightSnap.hasAtmo && lightSnap.atmoAddr) {
							memory->write<float>(lightSnap.atmoAddr + Offsets::Atmosphere::Density, lightSnap.atmoDensity);
							memory->write<float>(lightSnap.atmoAddr + Offsets::Atmosphere::Haze, lightSnap.atmoHaze);
							memory->write<float>(lightSnap.atmoAddr + Offsets::Atmosphere::Glare, lightSnap.atmoGlare);
							memory->write<float>(lightSnap.atmoAddr + Offsets::Atmosphere::Offset, lightSnap.atmoOffset);
						}
						lightSnap = LightSnap{};
						lightWasOn = false;
					}
				}
			}

			if (Globals::camera.Addr != 0) {
				static float fovOrig = 0.f;
				static bool fovWasOn = false;
				const bool wantFov = variables::World::customFov || variables::World::viewmodelFov;
				if (wantFov) {
					if (!fovWasOn) {
						fovOrig = memory->read<float>(Globals::camera.Addr + Offsets::Camera::FieldOfView);
						fovWasOn = true;
					}
					float deg = variables::World::customFov ? variables::World::fovAmount : variables::World::viewmodelFovAmt;
					float cur = memory->read<float>(Globals::camera.Addr + Offsets::Camera::FieldOfView);
					float writeVal = deg;
					if (cur > 0.05f && cur < 3.5f)
						writeVal = deg * 0.01745329251f;
					memory->write<float>(Globals::camera.Addr + Offsets::Camera::FieldOfView, writeVal);
				}
				else if (fovWasOn) {
					memory->write<float>(Globals::camera.Addr + Offsets::Camera::FieldOfView, fovOrig);
					fovWasOn = false;
				}
			}

			GunMods::Apply();

			if (Globals::players.Addr) {
				const uintptr_t lpAddr = memory->read<uintptr_t>(
					Globals::players.Addr + Offsets::Player::LocalPlayer);
				if (lpAddr)
					Globals::localPlayer = RBX::RbxInstance(lpAddr);
			}

			auto character = PlayerCache::ResolveLocalCharacter();
			if (character.Addr == 0) {
				std::this_thread::sleep_for(std::chrono::milliseconds(8));
				continue;
			}

			auto humanoid = character.FindChildByClass("Humanoid");
			auto rootPart = PlayerCache::FindRootPart(character);
			if (humanoid.Addr == 0 || rootPart.Addr == 0) {
				std::this_thread::sleep_for(std::chrono::milliseconds(20));
				continue;
			}

			// Hitbox / desync flags mirrored into Local::
			variables::Local::hitboxEnabled = variables::Hitbox::enabled;
			variables::Local::hitboxSize = variables::Hitbox::size;
			variables::Local::visualizeHitbox = variables::Hitbox::visualize;

			static bool hitboxKeyLatch = false;
			if (variables::Hitbox::key > 0) {
				bool hk = GameKeyDown(variables::Hitbox::key);
				if (hk && !hitboxKeyLatch) {
					variables::Hitbox::enabled = !variables::Hitbox::enabled;
					variables::MagicBullet::enabled = variables::Hitbox::enabled;
					variables::Local::hitboxEnabled = variables::Hitbox::enabled;
					hitboxKeyLatch = true;
				}
				else if (!hk) hitboxKeyLatch = false;
			}

			static bool magicKeyLatch = false;
			if (variables::MagicBullet::key > 0 && variables::MagicBullet::key != variables::Hitbox::key) {
				bool mk = GameKeyDown(variables::MagicBullet::key);
				if (mk && !magicKeyLatch) {
					variables::MagicBullet::enabled = !variables::MagicBullet::enabled;
					variables::Hitbox::enabled = variables::MagicBullet::enabled;
					variables::Local::hitboxEnabled = variables::Hitbox::enabled;
					magicKeyLatch = true;
				}
				else if (!mk) magicKeyLatch = false;
			}

			static bool rageKeyLatch = false;
			if (variables::Rage::key > 0) {
				bool rk = GameKeyDown(variables::Rage::key);
				if (rk && !rageKeyLatch) {
					variables::Rage::enabled = !variables::Rage::enabled;
					rageKeyLatch = true;
				}
				else if (!rk) rageKeyLatch = false;
			}

			static float walkOrig = 16.f;
			static bool speedWasOn = false;
			if (variables::Local::speedEnabled) {
				if (!speedWasOn) {
					walkOrig = memory->read<float>(humanoid.Addr + Offsets::Humanoid::Walkspeed);
					if (walkOrig < 1.f) walkOrig = 16.f;
					speedWasOn = true;
				}

				if (variables::Local::speedMethod == 0 || variables::Local::speedMethod == 2) {
					float spd = variables::Local::walkSpeed;
					if (variables::Local::speedMethod == 2) spd *= 1.35f;
					RBX::ModifyWalkSpeed(humanoid, spd);
					auto prim = rootPart.GetPrimitivePtr();
					if (prim && Globals::camera.Addr) {
						auto cf = Globals::camera.GetCameraCFrame();
						RBX::Vec3 look = cf.GetLookVector();
						look.X = -look.X; look.Z = -look.Z;
						float flat = sqrtf(look.X * look.X + look.Z * look.Z);
						RBX::Vec3 move{};
						if (GameKeyDown('W')) { move.X += look.X; move.Z += look.Z; }
						if (GameKeyDown('S')) { move.X -= look.X; move.Z -= look.Z; }
						RBX::Vec3 right = cf.GetRightVector();
						if (GameKeyDown('D')) { move.X += right.X; move.Z += right.Z; }
						if (GameKeyDown('A')) { move.X -= right.X; move.Z -= right.Z; }
						float ml = sqrtf(move.X * move.X + move.Z * move.Z);
						if (ml > 0.001f && flat > 0.001f) {
							RBX::Vec3 cur = memory->read<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity);
							WriteLocalVelocity(rootPart, { (move.X / ml) * spd, cur.Y, (move.Z / ml) * spd });
						}
					}
				}
				else { // Position
					RBX::ModifyWalkSpeed(humanoid, 16.f);
					if (Globals::camera.Addr) {
						auto cf = Globals::camera.GetCameraCFrame();
						RBX::Vec3 look = cf.GetLookVector();
						look.X = -look.X; look.Z = -look.Z;
						RBX::Vec3 right = cf.GetRightVector();
						RBX::Vec3 move{};
						if (GameKeyDown('W')) { move.X += look.X; move.Z += look.Z; }
						if (GameKeyDown('S')) { move.X -= look.X; move.Z -= look.Z; }
						if (GameKeyDown('D')) { move.X += right.X; move.Z += right.Z; }
						if (GameKeyDown('A')) { move.X -= right.X; move.Z -= right.Z; }
						float ml = sqrtf(move.X * move.X + move.Z * move.Z);
						if (ml > 0.001f) {
							RBX::Vec3 pos = rootPart.GetPos();
							float spd = variables::Local::walkSpeed;
							pos.X += (move.X / ml) * spd * dt;
							pos.Z += (move.Z / ml) * spd * dt;
							rootPart.SetPos(pos);
						}
					}
				}
			}
			else if (speedWasOn) {
				RBX::ModifyWalkSpeed(humanoid, walkOrig);
				speedWasOn = false;
			}

			static float jumpOrig = 50.f;
			static bool jumpWasOn = false;
			if (variables::Local::jumpEnabled) {
				if (!jumpWasOn) {
					jumpOrig = memory->read<float>(humanoid.Addr + Offsets::Humanoid::JumpPower);
					if (jumpOrig < 1.f) jumpOrig = 50.f;
					jumpWasOn = true;
				}
				RBX::ModifyJumpPower(humanoid, variables::Local::jumpPower);
			}
			else if (jumpWasOn) {
				RBX::ModifyJumpPower(humanoid, jumpOrig);
				jumpWasOn = false;
			}

			// Inf jump — velocity + humanoid jump (works across R6/R15/custom)
			static bool infJumpWasSpace = false;
			const bool spaceDown = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
			const bool spaceEdge = spaceDown && !infJumpWasSpace;
			infJumpWasSpace = spaceDown;
			if (variables::Local::infJump && WindowManager::IsRobloxFocused() &&
				(spaceEdge || spaceDown)) {
				ForceHumanoidRunning(humanoid.Addr);
				memory->write<uint8_t>(humanoid.Addr + Offsets::Humanoid::Sit, 0);
				memory->write<uint8_t>(humanoid.Addr + Offsets::Humanoid::PlatformStand, 0);
				float jp = variables::Local::jumpPower > 1.f ? variables::Local::jumpPower : 50.f;
				RBX::ModifyJumpPower(humanoid, jp);
				memory->write<float>(humanoid.Addr + Offsets::Humanoid::JumpHeight, jp * 0.18f);
				if (spaceEdge)
					RBX::ForceJump(humanoid);
				auto prim = rootPart.GetPrimitivePtr();
				if (prim) {
					RBX::Vec3 vel = memory->read<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity);
					float boost = jp * 0.75f;
					if (boost < 42.f) boost = 42.f;
					if (spaceEdge || vel.Y < boost * 0.65f)
						vel.Y = boost;
					WriteLocalVelocity(rootPart, vel);
				}
			}

			// Float ΓÇö hold height
			static float floatBaseY = 0.f;
			static bool floatArmed = false;
			if (variables::Local::floatEnabled) {
				if (!floatArmed) {
					floatBaseY = rootPart.GetPos().Y;
					floatArmed = true;
				}
				RBX::Vec3 pos = rootPart.GetPos();
				pos.Y = floatBaseY + variables::Local::floatHeight;
				rootPart.SetPos(pos);
				WriteLocalVelocity(rootPart, { 0, 0, 0 });
			}
			else {
				floatArmed = false;
			}

			// Walk fling — sustained HRP velocity on nearby targets (you stay stable)
			{
				static uintptr_t lastVictimHum = 0;

				auto clearFlingVictim = [&]() {
					if (lastVictimHum) {
						memory->write<uint8_t>(lastVictimHum + Offsets::Humanoid::PlatformStand, 0);
						lastVictimHum = 0;
					}
				};

				if (!variables::Local::walkFling) {
					clearFlingVictim();
				} else {
					const bool keyOk = variables::Local::walkFlingKey <= 0 || GameKeyDown(variables::Local::walkFlingKey);
					auto prim = rootPart.GetPrimitivePtr();
					if (!keyOk || !prim) {
						clearFlingVictim();
					} else {
						RBX::Vec3 myPos = rootPart.GetPos();
						float touchR = variables::Local::walkFlingRange;
						if (touchR < 2.f) touchR = 2.f;
						if (touchR > 16.f) touchR = 16.f;

						float power = variables::Local::walkFlingPower;
						if (power < 40.f) power = 40.f;
						if (power > 500.f) power = 500.f;

						const float acquireR = touchR * 1.5f;
						float bestD = acquireR + 1.f;
						RBX::Vec3 bestPos = myPos;
						uintptr_t bestRoot = 0, bestHum = 0;

						for (auto& plr : PlayerCache::snapshotPlayers()) {
							if (!plr.isValid || plr.health <= 0.f || !plr.rootPartAddr) continue;
							if (variables::teamCheck && !PlayerCache::PassesTeamFilter(plr)) continue;

							RBX::Vec3 theirPos = plr.position;
							RBX::RbxInstance tgtRoot(plr.rootPartAddr);
							if (tgtRoot.Addr) {
								RBX::Vec3 live = tgtRoot.GetPos();
								if (std::isfinite(live.X) && std::isfinite(live.Y) && std::isfinite(live.Z))
									theirPos = live;
							}

							const float dx = theirPos.X - myPos.X;
							const float dy = theirPos.Y - myPos.Y;
							const float dz = theirPos.Z - myPos.Z;
							const float d = sqrtf(dx * dx + dy * dy + dz * dz);
							if (d > acquireR || d < 0.05f) continue;
							if (d < bestD) {
								bestD = d;
								bestPos = theirPos;
								bestRoot = plr.rootPartAddr;
								bestHum = plr.humanoidAddr;
							}
						}

						WriteLocalVelocity(rootPart, { 0.f, 0.f, 0.f });
						WriteLocalAngularVelocity(rootPart, { 0.f, 0.f, 0.f });

						if (bestRoot && bestD <= acquireR) {
							RBX::Vec3 away{
								bestPos.X - myPos.X,
								(bestPos.Y - myPos.Y) * 0.25f + 0.75f,
								bestPos.Z - myPos.Z
							};
							float al = sqrtf(away.X * away.X + away.Y * away.Y + away.Z * away.Z);
							if (al > 0.05f) {
								away.X /= al; away.Y /= al; away.Z /= al;
							} else {
								away = { 0.f, 1.f, 0.f };
							}

							if (Globals::camera.Addr) {
								auto cf = Globals::camera.GetCameraCFrame();
								RBX::Vec3 look = cf.GetLookVector();
								look.X = -look.X; look.Z = -look.Z; look.Y = 0.f;
								const float ll = sqrtf(look.X * look.X + look.Z * look.Z);
								if (ll > 0.05f) {
									look.X /= ll; look.Z /= ll;
									away.X = away.X * 0.35f + look.X * 0.65f;
									away.Z = away.Z * 0.35f + look.Z * 0.65f;
									away.Y = 0.65f;
									al = sqrtf(away.X * away.X + away.Y * away.Y + away.Z * away.Z);
									if (al > 0.001f) { away.X /= al; away.Y /= al; away.Z /= al; }
								}
							}

							float proximity = 1.f - (bestD / acquireR);
							if (proximity < 0.2f) proximity = 0.2f;
							const float launch = power * (0.9f + proximity * 0.45f);
							const float spin = (std::min)(launch * 0.08f, 48.f);

							RBX::RbxInstance victimRoot(bestRoot);
							const uintptr_t victimPrim = victimRoot.GetPrimitivePtr();
							if (victimPrim) {
								const RBX::Vec3 flingVel{
									away.X * launch,
									launch * 0.82f,
									away.Z * launch
								};
								const int reps = bestD <= touchR * 0.55f ? 10 : (bestD <= touchR ? 7 : 5);
								WritePrimVelocityBurst(victimPrim, flingVel, reps);
								memory->write<RBX::Vec3>(
									victimPrim + Offsets::Primitive::AssemblyAngularVelocity,
									{ 0.f, spin, 0.f });
								memory->write<RBX::Vec3>(
									victimPrim + Offsets::Primitive::AssemblyAngularVelocity,
									{ 0.f, spin * 0.5f, 0.f });

								if (bestHum) {
									if (lastVictimHum && lastVictimHum != bestHum)
										memory->write<uint8_t>(lastVictimHum + Offsets::Humanoid::PlatformStand, 0);
									memory->write<uint8_t>(bestHum + Offsets::Humanoid::PlatformStand, 1);
									memory->write<uint8_t>(bestHum + Offsets::Humanoid::Sit, 0);
									lastVictimHum = bestHum;
								}
							}
						} else {
							clearFlingVictim();
						}
					}
				}
			}

			// Click TP
			static bool clickTpLatch = false;
			if (variables::Local::clickTp && variables::Local::clickTpKey > 0) {
				bool held = GameKeyDown(variables::Local::clickTpKey);
				if (held && !clickTpLatch && Globals::camera.Addr) {
					auto cf = Globals::camera.GetCameraCFrame();
					RBX::Vec3 look = cf.GetLookVector();
					look.X = -look.X; look.Y = -look.Y; look.Z = -look.Z;
					RBX::Vec3 pos = rootPart.GetPos();
					pos.X += look.X * 25.f;
					pos.Y += look.Y * 25.f;
					pos.Z += look.Z * 25.f;
					rootPart.SetPos(pos);
					clickTpLatch = true;
				}
				else if (!held) clickTpLatch = false;
			}

			// Auto TP Loop ΓÇö sticky teleport to nearest player (0 delay = every frame)
			{
				static auto lastAutoTp = std::chrono::steady_clock::now();
				static bool autoTpKeyLatch = false;
				bool want = variables::Local::autoTp;
				if (variables::Local::autoTpKey > 0) {
					bool held = GameKeyDown(variables::Local::autoTpKey);
					if (held && !autoTpKeyLatch) {
						variables::Local::autoTp = !variables::Local::autoTp;
						want = variables::Local::autoTp;
						autoTpKeyLatch = true;
					}
					else if (!held) autoTpKeyLatch = false;
				}
				if (want) {
					auto now = std::chrono::steady_clock::now();
					float delay = (std::max)(0.f, variables::Local::autoTpDelay);
					if (delay <= 0.f || std::chrono::duration<float>(now - lastAutoTp).count() >= delay) {
						lastAutoTp = now;
						float bestDist = 1.0e9f;
						uintptr_t bestRoot = 0;
						RBX::Vec3 bestVel{};
						RBX::Vec3 bestPos{};
						bool found = false;
						for (auto& plr : PlayerCache::snapshotPlayers()) {
							if (!plr.isValid || plr.health <= 0.f) continue;
							if (variables::teamCheck && !PlayerCache::PassesTeamFilter(plr))
								continue;
							if (plr.distance < bestDist && plr.distance > 0.15f) {
								bestDist = plr.distance;
								bestRoot = plr.rootPartAddr;
								bestVel = plr.velocity;
								bestPos = plr.bones.hasHrp ? plr.bones.hrp : plr.position;
								found = true;
							}
						}
						if (found) {
							// Live HRP + short prediction so moving targets don't pull away
							if (bestRoot) {
								RBX::RbxInstance tgt(bestRoot);
								RBX::Vec3 live = tgt.GetPos();
								if (live.X != 0.f || live.Y != 0.f || live.Z != 0.f)
									bestPos = live;
								auto tprim = tgt.GetPrimitivePtr();
								if (tprim)
									bestVel = memory->read<RBX::Vec3>(tprim + Offsets::Primitive::AssemblyLinearVelocity);
							}
							const float pred = 0.08f;
							bestPos.X += bestVel.X * pred;
							bestPos.Y += bestVel.Y * pred * 0.35f;
							bestPos.Z += bestVel.Z * pred;

							RBX::Vec3 me = rootPart.GetPos();
							RBX::Vec3 dir{ bestPos.X - me.X, 0.f, bestPos.Z - me.Z };
							float len = sqrtf(dir.X * dir.X + dir.Z * dir.Z);
							RBX::Vec3 dest = bestPos;
							// Sit close in front of their facing from us (tight for gun fights)
							const float standOff = 1.6f;
							if (len > 0.05f) {
								dest.X -= (dir.X / len) * standOff;
								dest.Z -= (dir.Z / len) * standOff;
							}
							dest.Y += 1.25f;

							// Double-write sticks better against server correction
							rootPart.SetPos(dest);
							WriteLocalVelocity(rootPart, { 0, 0, 0 });
							rootPart.SetPos(dest);
							auto prim = rootPart.GetPrimitivePtr();
							if (prim)
								memory->write<RBX::Vec3>(prim + Offsets::Primitive::AssemblyAngularVelocity, { 0, 0, 0 });
						}
					}
				}
			}

			// Rage TP ΓÇö sit behind target (hard to shoot you) + optional health stick
			if (variables::Rage::enabled && variables::Rage::teleport) {
				RBX::Vec3 targetPos{};
				RBX::Vec3 targetLook{};
				uintptr_t targetRoot = 0;
				bool found = false;
				bool haveLook = false;
				if (Aimbot::lockedPlayerAddr != 0) {
					for (auto& plr : PlayerCache::snapshotPlayers()) {
						if (plr.playerAddr == Aimbot::lockedPlayerAddr && plr.isValid && plr.health > 0.f) {
							targetPos = plr.bones.hasHrp ? plr.bones.hrp : plr.position;
							targetRoot = plr.rootPartAddr;
							found = true;
							break;
						}
					}
				}
				if (!found) {
					float bestDist = 1.0e9f;
					for (auto& plr : PlayerCache::snapshotPlayers()) {
						if (!plr.isValid || plr.health <= 0.f) continue;
						if (variables::teamCheck && !PlayerCache::PassesTeamFilter(plr))
							continue;
						if (plr.distance < bestDist) {
							bestDist = plr.distance;
							targetPos = plr.bones.hasHrp ? plr.bones.hrp : plr.position;
							targetRoot = plr.rootPartAddr;
							found = true;
						}
					}
				}
				if (found && targetRoot) {
					RBX::RbxInstance tgt(targetRoot);
					RBX::Vec3 live = tgt.GetPos();
					if (live.X != 0.f || live.Y != 0.f || live.Z != 0.f)
						targetPos = live;
					auto tcf = tgt.GetCFrame();
					RBX::Vec3 look = tcf.GetLookVector();
					// Match camera convention: negate Z-column ΓåÆ facing
					look.X = -look.X; look.Y = 0.f; look.Z = -look.Z;
					float flat = sqrtf(look.X * look.X + look.Z * look.Z);
					if (flat > 0.001f) {
						targetLook = { look.X / flat, 0.f, look.Z / flat };
						haveLook = true;
					}
				}
				if (found) {
					float dist = (std::max)(0.5f, variables::Rage::tpDistance);
					RBX::Vec3 dest = targetPos;
					if (variables::Rage::unkillable && haveLook) {
						// Behind their facing so they can't track you
						dest.X -= targetLook.X * dist;
						dest.Z -= targetLook.Z * dist;
						dest.Y += 1.1f;
					}
					else if (Globals::camera.Addr) {
						auto cf = Globals::camera.GetCameraCFrame();
						RBX::Vec3 look = cf.GetLookVector();
						look.X = -look.X; look.Z = -look.Z;
						float flat = sqrtf(look.X * look.X + look.Z * look.Z);
						if (flat > 0.001f) {
							dest.X -= (look.X / flat) * dist;
							dest.Z -= (look.Z / flat) * dist;
						}
						dest.Y += 1.5f;
					}
					else {
						dest.Y += 1.5f;
					}

					rootPart.SetPos(dest);
					WriteLocalVelocity(rootPart, { 0, 0, 0 });
					rootPart.SetPos(dest);
					auto prim = rootPart.GetPrimitivePtr();
					if (prim)
						memory->write<RBX::Vec3>(prim + Offsets::Primitive::AssemblyAngularVelocity, { 0, 0, 0 });

					if (variables::Rage::unkillable && humanoid.Addr) {
						float maxHp = memory->read<float>(humanoid.Addr + Offsets::Humanoid::MaxHealth);
						if (maxHp < 1.f) maxHp = 100.f;
						memory->write<float>(humanoid.Addr + Offsets::Humanoid::Health, maxHp);
					}
				}
			}

			// Fly — menu toggle activates immediately; F key toggles on/off while enabled
			static bool flyHumanoidSet = false;
			static bool prevFlyEnabled = false;
			{
				static uintptr_t lastFlyChar = 0;
				if (character.Addr != lastFlyChar) {
					flyHumanoidSet = false;
					lastFlyChar = character.Addr;
				}

				if (variables::Local::flyEnabled && !prevFlyEnabled)
					variables::Local::flyActive = true;
				if (!variables::Local::flyEnabled)
					variables::Local::flyActive = false;
				prevFlyEnabled = variables::Local::flyEnabled;

				int fk = variables::Local::flyKey;
				if (fk > 0) {
					bool flyHeld = GameKeyDown(fk);
					if (flyHeld && !flyKeyLatched) {
						if (!variables::Local::flyEnabled) {
							variables::Local::flyEnabled = true;
							variables::Local::flyActive = true;
						} else {
							variables::Local::flyActive = !variables::Local::flyActive;
						}
						flyKeyLatched = true;
					}
					else if (!flyHeld) flyKeyLatched = false;
				}
			}

			if (!Globals::camera.Addr && Globals::workspace.Addr)
				Globals::camera = Globals::ResolveWorkspaceCamera();

			if (variables::Local::flyActive && Globals::camera.Addr != 0) {
				if (humanoid.Addr) {
					memory->write<uint8_t>(humanoid.Addr + Offsets::Humanoid::PlatformStand, 0);
					memory->write<uint8_t>(humanoid.Addr + Offsets::Humanoid::AutoRotate, 0);
					memory->write<uint8_t>(humanoid.Addr + Offsets::Humanoid::Sit, 0);
					ForceHumanoidRunning(humanoid.Addr);
					flyHumanoidSet = true;
				}
				auto cf = Globals::camera.GetCameraCFrame();
				RBX::Vec3 look = cf.GetLookVector();
				look.X = -look.X; look.Y = -look.Y; look.Z = -look.Z;
				RBX::Vec3 right = cf.GetRightVector();
				RBX::Vec3 move{ 0, 0, 0 };
				if (GameKeyDown('W')) { move.X += look.X; move.Y += look.Y; move.Z += look.Z; }
				if (GameKeyDown('S')) { move.X -= look.X; move.Y -= look.Y; move.Z -= look.Z; }
				if (GameKeyDown('D')) { move.X += right.X; move.Y += right.Y; move.Z += right.Z; }
				if (GameKeyDown('A')) { move.X -= right.X; move.Y -= right.Y; move.Z -= right.Z; }
				if (GameKeyDown(VK_SPACE)) move.Y += 1.0f;
				if (GameKeyDown(VK_LCONTROL) || GameKeyDown(VK_LSHIFT)) move.Y -= 1.0f;

				float len = sqrtf(move.X * move.X + move.Y * move.Y + move.Z * move.Z);
				float spd = variables::Local::flySpeed;
				if (len > 0.001f) {
					if (variables::Local::flyMethod == 0) {
						WriteLocalVelocity(rootPart, { (move.X / len) * spd, (move.Y / len) * spd, (move.Z / len) * spd });
					}
					else {
						RBX::Vec3 pos = rootPart.GetPos();
						pos.X += (move.X / len) * spd * dt;
						pos.Y += (move.Y / len) * spd * dt;
						pos.Z += (move.Z / len) * spd * dt;
						rootPart.SetPos(pos);
						WriteLocalVelocity(rootPart, { 0, 0, 0 });
					}
				}
				else if (variables::Local::flyMethod == 0) {
					auto prim = rootPart.GetPrimitivePtr();
					if (prim) {
						RBX::Vec3 vel = memory->read<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity);
						if (vel.Y < -8.f)
							WriteLocalVelocity(rootPart, { 0.f, 0.f, 0.f });
						else
							WriteLocalVelocity(rootPart, { 0.f, vel.Y > -1.f ? 0.f : vel.Y * 0.5f, 0.f });
					}
				}
				auto prim = rootPart.GetPrimitivePtr();
				if (prim)
					memory->write<RBX::Vec3>(prim + Offsets::Primitive::AssemblyAngularVelocity, { 0, 0, 0 });
			}
			else if (flyHumanoidSet && humanoid.Addr) {
				memory->write<uint8_t>(humanoid.Addr + Offsets::Humanoid::PlatformStand, 0);
				memory->write<uint8_t>(humanoid.Addr + Offsets::Humanoid::AutoRotate, 1);
				flyHumanoidSet = false;
			}

			const bool wantNoCol = variables::Local::noclip ||
				(variables::Rage::enabled && variables::Rage::teleport);
			ApplyCharacterPartFlags(character, wantNoCol, variables::Local::antiFling && !wantNoCol);

			if (variables::Local::bhopEnabled && !variables::Local::flyActive && GameKeyDown(VK_SPACE) && Globals::camera.Addr != 0) {
				auto cf = Globals::camera.GetCameraCFrame();
				RBX::Vec3 look = cf.GetLookVector();
				look.X = -look.X; look.Z = -look.Z;
				float flat = sqrtf(look.X * look.X + look.Z * look.Z);
				if (flat > 0.001f) {
					auto prim = rootPart.GetPrimitivePtr();
					if (prim) {
						RBX::Vec3 cur = memory->read<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity);
						float spd = variables::Local::bhopSpeed;
						WriteLocalVelocity(rootPart, { (look.X / flat) * spd, cur.Y, (look.Z / flat) * spd });
					}
				}
			}

			// Hitbox extender — expand while on; restore originals for several frames when off
			// (single-frame restore often lost the race and left giant parts stuck).
			struct HitboxSave {
				rbx::vector3_t size{};
				uint8_t flags = 0;
				bool haveFlags = false;
			};
			static std::unordered_map<uintptr_t, HitboxSave> hitboxOrig;
			static bool hitboxWasOn = false;
			static int hitboxRestoreFrames = 0;

			// Keep both toggles in sync (UI vs keybind / legacy)
			variables::Local::hitboxEnabled = variables::Hitbox::enabled;
			variables::Local::hitboxSize = variables::Hitbox::size;
			variables::Local::visualizeHitbox = variables::Hitbox::visualize;

			const bool hitboxOn = variables::Hitbox::enabled || variables::MagicBullet::enabled;

			auto restoreHitboxes = [&]() {
				for (auto& kv : hitboxOrig) {
					RBX::RbxInstance part(kv.first);
					auto prim = part.GetPrimitivePtr();
					if (!prim) continue;
					memory->write<rbx::vector3_t>(prim + Offsets::Primitive::Size, kv.second.size);
					// Second write — engine sometimes ignores the first
					memory->write<rbx::vector3_t>(prim + Offsets::Primitive::Size, kv.second.size);
					if (kv.second.haveFlags)
						memory->write<uint8_t>(prim + Offsets::Primitive::Flags, kv.second.flags);
				}
			};

			if (hitboxOn) {
				hitboxWasOn = true;
				hitboxRestoreFrames = 0;
				float sz = variables::Hitbox::size;
				if (sz < 2.f) sz = 2.f;
				if (sz > 50.f) sz = 50.f;
				rbx::vector3_t ns{ sz, sz * 0.85f, sz };
				rbx::vector3_t headSz{ sz * 0.55f, sz * 0.55f, sz * 0.55f };

				auto expandPart = [&](uintptr_t partAddr, const rbx::vector3_t& want) {
					if (!partAddr) return;
					RBX::RbxInstance part(partAddr);
					auto prim = part.GetPrimitivePtr();
					if (!prim) return;

					if (hitboxOrig.find(partAddr) == hitboxOrig.end()) {
						rbx::vector3_t cur = memory->read<rbx::vector3_t>(prim + Offsets::Primitive::Size);
						// Don't treat an already-expanded part as the "original"
						float mx = cur.x; if (cur.y > mx) mx = cur.y; if (cur.z > mx) mx = cur.z;
						HitboxSave sav{};
						if (mx > 6.5f) {
							// Fallback humanoid defaults
							sav.size = { 2.f, 2.f, 1.f };
						}
						else {
							sav.size = cur;
						}
						sav.flags = memory->read<uint8_t>(prim + Offsets::Primitive::Flags);
						sav.haveFlags = true;
						hitboxOrig[partAddr] = sav;
					}

					part.SetSize(want);
					memory->write<rbx::vector3_t>(prim + Offsets::Primitive::Size, want);
					uint8_t flags = memory->read<uint8_t>(prim + Offsets::Primitive::Flags);
					flags = (uint8_t)(flags | Offsets::PrimitiveFlags::CanQuery);
					flags = (uint8_t)(flags | Offsets::PrimitiveFlags::CanTouch);
					flags = (uint8_t)(flags & ~Offsets::PrimitiveFlags::CanCollide);
					memory->write<uint8_t>(prim + Offsets::Primitive::Flags, flags);
				};

				for (auto& plr : PlayerCache::snapshotPlayers()) {
					if (!plr.isValid || plr.rootPartAddr == 0) continue;
					if (variables::Hitbox::teamCheck && !PlayerCache::PassesTeamFilter(plr)) continue;
					if (variables::Hitbox::healthCheck && plr.health <= 0) continue;

					expandPart(plr.rootPartAddr, ns);
					if (plr.headAddr) expandPart(plr.headAddr, headSz);

					if (variables::Hitbox::type == 1 && plr.characterAddr) {
						RBX::RbxInstance ch(plr.characterAddr);
						const char* parts[] = {
							"Head", "UpperTorso", "LowerTorso", "Torso",
							"HumanoidRootPart", "LeftUpperArm", "RightUpperArm",
							"LeftUpperLeg", "RightUpperLeg"
						};
						for (auto* n : parts) {
							auto p = ch.FindChild(n);
							if (!p.Addr) continue;
							bool isHead = (n[0] == 'H' && n[1] == 'e');
							expandPart(p.Addr, isHead ? headSz : ns);
						}
					}
				}
			}
			else if (hitboxWasOn || hitboxRestoreFrames > 0) {
				if (hitboxWasOn) {
					hitboxWasOn = false;
					hitboxRestoreFrames = 120; // retry restore ~2s so sizes actually stick
				}
				restoreHitboxes();
				hitboxRestoreFrames--;
				if (hitboxRestoreFrames <= 0) {
					hitboxOrig.clear();
					hitboxRestoreFrames = 0;
				}
			}

			static bool lastDesync = false;
			if (Offsets::Desync::PhysicsSenderMaxBandwidthBps != 0 && variables::Local::desyncEnabled != lastDesync) {
				auto base = memory->get_module_address();
				if (base) {
					auto addr = base + Offsets::Desync::PhysicsSenderMaxBandwidthBps;
					memory->write<float>(addr, variables::Local::desyncEnabled ? 0.0f : 5.431432847722991e-41f);
					lastDesync = variables::Local::desyncEnabled;
				}
			}

			// Speed key toggle
			static bool speedKeyLatch = false;
			if (variables::Local::speedKey > 0) {
				bool sk = GameKeyDown(variables::Local::speedKey);
				if (sk && !speedKeyLatch) {
					variables::Local::speedEnabled = !variables::Local::speedEnabled;
					speedKeyLatch = true;
				}
				else if (!sk) speedKeyLatch = false;
			}

			// Freeze ΓÇö zero linear/angular velocity
			static bool freezeKeyLatch = false;
			if (variables::Local::freezeKey > 0) {
				bool fk = GameKeyDown(variables::Local::freezeKey);
				if (fk && !freezeKeyLatch) {
					variables::Local::freeze = !variables::Local::freeze;
					freezeKeyLatch = true;
				}
				else if (!fk) freezeKeyLatch = false;
			}
			if (variables::Local::freeze) {
				WriteLocalVelocity(rootPart, { 0, 0, 0 });
				auto prim = rootPart.GetPrimitivePtr();
				if (prim)
					memory->write<RBX::Vec3>(prim + Offsets::Primitive::AssemblyAngularVelocity, { 0, 0, 0 });
			}

			// Spin — yaw angular velocity (disabled while anti-fling protects you)
			if (variables::Local::spin && !variables::Local::walkFling && !variables::Local::antiFling) {
				auto prim = rootPart.GetPrimitivePtr();
				if (prim)
					memory->write<RBX::Vec3>(prim + Offsets::Primitive::AssemblyAngularVelocity,
						{ 0, variables::Local::spinSpeed, 0 });
			}

			// Hip height
			static float hipOrig = 0.f;
			static bool hipWasOn = false;
			if (variables::Local::hipHeightEnabled && humanoid.Addr) {
				if (!hipWasOn) {
					hipOrig = memory->read<float>(humanoid.Addr + Offsets::Humanoid::HipHeight);
					hipWasOn = true;
				}
				memory->write<float>(humanoid.Addr + Offsets::Humanoid::HipHeight, variables::Local::hipHeight);
			}
			else if (hipWasOn && humanoid.Addr) {
				memory->write<float>(humanoid.Addr + Offsets::Humanoid::HipHeight, hipOrig);
				hipWasOn = false;
			}

			// Custom gravity (workspace)
			static float gravOrig = 196.2f;
			static bool gravWasOn = false;
			if (variables::Local::gravityEnabled && Globals::dataModel.Addr) {
				auto ws = Globals::dataModel.FindChildByClass("Workspace");
				if (ws.Addr) {
					if (!gravWasOn) {
						gravOrig = memory->read<float>(ws.Addr + Offsets::World::Gravity);
						if (gravOrig < 0.01f) gravOrig = 196.2f;
						gravWasOn = true;
					}
					memory->write<float>(ws.Addr + Offsets::World::Gravity, variables::Local::gravity);
				}
			}
			else if (gravWasOn && Globals::dataModel.Addr) {
				auto ws = Globals::dataModel.FindChildByClass("Workspace");
				if (ws.Addr)
					memory->write<float>(ws.Addr + Offsets::World::Gravity, gravOrig);
				gravWasOn = false;
			}

			// God mode ΓÇö stick health near max
			if (variables::Local::godMode && humanoid.Addr) {
				float maxHp = memory->read<float>(humanoid.Addr + Offsets::Humanoid::MaxHealth);
				if (maxHp < 1.f) maxHp = 100.f;
				memory->write<float>(humanoid.Addr + Offsets::Humanoid::Health, maxHp);
			}

			// Anti-void ΓÇö teleport up if falling too far
			if (variables::Local::antiVoid) {
				auto pos = rootPart.GetPos();
				if (pos.Y < -50.f) {
					pos.Y = 80.f;
					rootPart.SetPos(pos);
				}
			}

			// TP walk — nudge HRP while holding WASD (dt-scaled, keep Y)
			if (variables::Local::tpWalk && Globals::camera.Addr) {
				auto cf = Globals::camera.GetCameraCFrame();
				RBX::Vec3 look = cf.GetLookVector();
				look.X = -look.X; look.Z = -look.Z;
				look.Y = 0;
				float flat = sqrtf(look.X * look.X + look.Z * look.Z);
				if (flat > 0.001f) { look.X /= flat; look.Z /= flat; }
				RBX::Vec3 right = cf.GetRightVector();
				RBX::Vec3 step{};
				const float rate = (std::max)(variables::Local::tpWalkStep, 0.5f);
				const float stepScale = rate * dt * 60.f;
				if (GameKeyDown('W')) { step.X += look.X * stepScale; step.Z += look.Z * stepScale; }
				if (GameKeyDown('S')) { step.X -= look.X * stepScale; step.Z -= look.Z * stepScale; }
				if (GameKeyDown('D')) { step.X += right.X * stepScale; step.Z += right.Z * stepScale; }
				if (GameKeyDown('A')) { step.X -= right.X * stepScale; step.Z -= right.Z * stepScale; }
				if (step.X != 0.f || step.Z != 0.f) {
					auto pos = rootPart.GetPos();
					pos.X += step.X;
					pos.Z += step.Z;
					rootPart.SetPos(pos);
					auto prim = rootPart.GetPrimitivePtr();
					if (prim) {
						RBX::Vec3 vel = memory->read<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity);
						WriteLocalVelocity(rootPart, { vel.X * 0.12f, vel.Y, vel.Z * 0.12f });
					}
				}
			}

			// Auto clicker
			if (variables::Local::autoClicker) {
				bool keyOk = variables::Local::autoClickerKey == 0 ||
					GameKeyDown(variables::Local::autoClickerKey);
				static auto lastClick = std::chrono::steady_clock::now();
				float interval = 1.f / (std::max)(1.f, variables::Local::autoClickerCps);
				if (keyOk && std::chrono::duration<float>(now - lastClick).count() >= interval) {
					lastClick = now;
					INPUT in[2] = {};
					in[0].type = INPUT_MOUSE; in[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
					in[1].type = INPUT_MOUSE; in[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
					SendInput(2, in, sizeof(INPUT));
				}
			}

			// Orbit locked / nearest enemy
			if (variables::Local::orbitPlayer && rootPart.Addr) {
				uintptr_t target = Aimbot::lockedPlayerAddr;
				RBX::Vec3 tpos{};
				bool have = false;
				auto snap = PlayerCache::snapshotPlayers();
				for (auto& p : snap) {
					if (!p.isValid || p.health <= 0.f) continue;
					if (target && p.playerAddr == target) {
						tpos = p.bones.hasHrp ? p.bones.hrp : p.position;
						have = true;
						break;
					}
				}
				if (!have) {
					float best = 1e9f;
					for (auto& p : snap) {
						if (!p.isValid || p.health <= 0.f) continue;
						if (variables::teamCheck && !PlayerCache::PassesTeamFilter(p)) continue;
						if (p.distance < best) {
							best = p.distance;
							tpos = p.bones.hasHrp ? p.bones.hrp : p.position;
							have = true;
						}
					}
				}
				if (have) {
					static float orbitAng = 0.f;
					orbitAng += variables::Local::orbitSpeed * dt;
					float r = variables::Local::orbitRadius;
					RBX::Vec3 pos = rootPart.GetPos();
					pos.X = tpos.X + cosf(orbitAng) * r;
					pos.Z = tpos.Z + sinf(orbitAng) * r;
					pos.Y = tpos.Y + 1.5f;
					rootPart.SetPos(pos);
					WriteLocalVelocity(rootPart, { 0, 0, 0 });
				}
			}

			// Sit spam ΓÇö toggle Humanoid.Sit
			if (variables::Local::sitSpam && humanoid.Addr) {
				static auto lastSit = std::chrono::steady_clock::now();
				if (std::chrono::duration<float, std::milli>(now - lastSit).count() >= 120.f) {
					lastSit = now;
					static bool sitFlip = false;
					sitFlip = !sitFlip;
					memory->write<uint8_t>(humanoid.Addr + Offsets::Humanoid::Sit, sitFlip ? 1 : 0);
				}
			}

			// Vehicle boost ΓÇö raise MaxSpeed on occupied VehicleSeat
			if (variables::Local::vehicleBoost && humanoid.Addr) {
				uintptr_t seat = memory->read<uintptr_t>(humanoid.Addr + Offsets::Humanoid::SeatPart);
				if (seat) {
					memory->write<float>(seat + Offsets::VehicleSeat::MaxSpeed, variables::Local::vehicleBoostAmt);
					memory->write<float>(seat + Offsets::VehicleSeat::Torque, variables::Local::vehicleBoostAmt * 2.f);
				}
			}

			// Unlock / third-person zoom — restore when both off
			{
				static float maxZoomOrig = 128.f, minZoomOrig = 0.5f;
				static bool zoomWasOn = false;
				const bool wantZoom = (variables::World::unlockZoom || variables::World::thirdPerson) && Globals::localPlayer.Addr;
				if (wantZoom) {
					if (!zoomWasOn) {
						maxZoomOrig = memory->read<float>(Globals::localPlayer.Addr + Offsets::Player::MaxZoomDistance);
						minZoomOrig = memory->read<float>(Globals::localPlayer.Addr + Offsets::Player::MinZoomDistance);
						zoomWasOn = true;
					}
					if (variables::World::thirdPerson) {
						memory->write<float>(Globals::localPlayer.Addr + Offsets::Player::MaxZoomDistance,
							(std::max)(variables::World::thirdPersonDistance, 8.f));
						memory->write<float>(Globals::localPlayer.Addr + Offsets::Player::MinZoomDistance, 0.5f);
					}
					else if (variables::World::unlockZoom) {
						memory->write<float>(Globals::localPlayer.Addr + Offsets::Player::MaxZoomDistance, variables::World::maxZoom);
					}
				}
				else if (zoomWasOn && Globals::localPlayer.Addr) {
					memory->write<float>(Globals::localPlayer.Addr + Offsets::Player::MaxZoomDistance, maxZoomOrig);
					memory->write<float>(Globals::localPlayer.Addr + Offsets::Player::MinZoomDistance, minZoomOrig);
					zoomWasOn = false;
				}
			}

			// Anti-AFK / AFK Assist ΓÇö keep Roblox from idling when you walk away
			if (variables::Misc::antiAfk || variables::Misc::afkAssist) {
				static auto lastAfk = std::chrono::steady_clock::now();
				static POINT lastPt{ 0, 0 };
				static int pulse = 0;
				POINT pt{};
				GetCursorPos(&pt);
				bool moved = (pt.x != lastPt.x || pt.y != lastPt.y);
				lastPt = pt;
				auto nowAfk = std::chrono::steady_clock::now();
				if (moved)
					lastAfk = nowAfk;
				float interval = variables::Misc::antiAfkSeconds;
				if (interval < 5.f) interval = 5.f;
				if (variables::Misc::afkAssist && interval > 14.f) interval = 14.f;
				if (std::chrono::duration<float>(nowAfk - lastAfk).count() > interval) {
					INPUT in = {};
					in.type = INPUT_MOUSE;
					in.mi.dwFlags = MOUSEEVENTF_MOVE;
					in.mi.dx = (pulse % 2 == 0) ? 1 : -1;
					in.mi.dy = (pulse % 3 == 0) ? 1 : 0;
					SendInput(1, &in, sizeof(INPUT));
					in.mi.dx = -in.mi.dx;
					in.mi.dy = -in.mi.dy;
					SendInput(1, &in, sizeof(INPUT));

					INPUT keys[2] = {};
					keys[0].type = INPUT_KEYBOARD;
					keys[0].ki.wVk = VK_SHIFT;
					keys[1].type = INPUT_KEYBOARD;
					keys[1].ki.wVk = VK_SHIFT;
					keys[1].ki.dwFlags = KEYEVENTF_KEYUP;
					SendInput(2, keys, sizeof(INPUT));

					pulse++;
					lastAfk = nowAfk;
				}
			}

			ApplyAntiFling(rootPart, humanoid, character);

			// Keep exploit loop tight while Auto TP is sticking to a target
			if (variables::Local::autoTp)
				std::this_thread::sleep_for(std::chrono::milliseconds(0));
			else if (variables::Local::antiFling)
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			else
				std::this_thread::sleep_for(std::chrono::milliseconds(4));
		}
	}

	bool IsGameSessionReady()
	{
		if (!Globals::dataModel.Addr || !Globals::players.Addr)
			return false;

		const int64_t placeId = memory->read<int64_t>(Globals::dataModel.Addr + Offsets::DataModel::PlaceId);
		if (placeId <= 0)
			return false;

		const uint8_t loaded = memory->read<uint8_t>(Globals::dataModel.Addr + Offsets::DataModel::GameLoaded);
		return loaded != 0;
	}

	bool IsInActiveGame()
	{
		if (!Globals::dataModel.Addr || !Globals::players.Addr)
			return false;

		const int64_t placeId = memory->read<int64_t>(Globals::dataModel.Addr + Offsets::DataModel::PlaceId);
		if (placeId <= 0)
			return false;

		const uint8_t loaded = memory->read<uint8_t>(Globals::dataModel.Addr + Offsets::DataModel::GameLoaded);
		if (loaded == 0)
			return false;

		const uintptr_t localPlayerAddr = memory->read<uintptr_t>(Globals::players.Addr + Offsets::Player::LocalPlayer);
		if (!localPlayerAddr)
			return false;

		Globals::localPlayer = RBX::RbxInstance(localPlayerAddr);

		auto character = PlayerCache::ResolveCharacter(Globals::localPlayer);
		if (!character.Addr || !PlayerCache::CharacterLooksAlive(character))
			return false;

		return PlayerCache::FindRootPart(character).Addr != 0;
	}

	bool FailAttach(const char* msg)
	{
		variables::Loading::failed = true;
		variables::Loading::active = false;
		strncpy_s(variables::Loading::error, msg, _TRUNCATE);
		Telemetry::ReportError("Attach failed", msg);
		return false;
	}

	bool AttachGame()
	{
		SetLoad(0.10f, "Connecting to Roblox");
		int tries = 0;
		while (!memory->find_process_id(app)) {
			if (++tries > 5)
				return FailAttach("Please join a game");
			SetLoad(0.10f + (tries % 20) * 0.01f, "Waiting for Roblox");
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

		SetLoad(0.25f, "Attaching process");
#ifdef FRONTIER_KERNEL
		if (!memory->attach_to_process(app)) {
			char errBuf[384];
			snprintf(errBuf, sizeof(errBuf),
				"Kernel driver failed (error %lu): %s. "
				"Run loader as Admin, enable test signing, reboot, then use kernel mode.",
				(unsigned long)FrontierDriver::LastError(),
				FrontierDriver::LastErrorText());
			return FailAttach(errBuf);
		}
#else
		if (!memory->attach_to_process(app) || !memory->find_module_address(app))
			return FailAttach("Failed to attach to Roblox.");
#endif
		if (!memory->find_module_address(app))
			return FailAttach("Failed to resolve Roblox module.");

		SetLoad(0.40f, "Resolving offsets");
		const auto anchors = Scanner::ResolveAnchors();
		if (!anchors.success)
			return FailAttach("Offset resolve failed. Update offsets.");

		Globals::dataModel = RBX::RbxInstance(anchors.dataModel);
		Globals::renderEngine = RBX::RenderEngine(anchors.visualEngine);
		Globals::RefreshServices();
		DebugLog::Write("Attach: anchors ok method=%s dm=%llX ve=%llX",
			anchors.method.c_str(),
			(unsigned long long)anchors.dataModel,
			(unsigned long long)anchors.visualEngine);

		if (Globals::players.Addr == 0)
			return FailAttach("Please join a game");

		SetLoad(0.55f, "Waiting for game");
		tries = 0;
		int homeTries = 0;
		while (!IsGameSessionReady()) {
			if (!memory->find_process_id(app))
				return FailAttach("Please join a game");

			const int64_t placeId = Games::ReadPlaceId();
			if (placeId <= 0) {
				if (++homeTries > 8)
					return FailAttach("Please join a game");
			} else {
				homeTries = 0;
				if (++tries > 150)
					return FailAttach("Please join a game");
			}

			if (((tries + homeTries) % 10) == 0) {
				Globals::workspace = Globals::dataModel.FindChildByClass("Workspace");
				Globals::players = Globals::dataModel.FindChildByClass("Players");
				if (Globals::workspace.Addr)
					Globals::camera = Globals::ResolveWorkspaceCamera();
				if (!Globals::dataModel.Addr || !Globals::players.Addr) {
					const auto again = Scanner::ResolveAnchors();
					if (again.success) {
						Globals::dataModel = RBX::RbxInstance(again.dataModel);
						Globals::renderEngine = RBX::RenderEngine(again.visualEngine);
						Globals::workspace = Globals::dataModel.FindChildByClass("Workspace");
						Globals::players = Globals::dataModel.FindChildByClass("Players");
						if (Globals::workspace.Addr)
							Globals::camera = Globals::ResolveWorkspaceCamera();
					}
				}
			}
			float pulse = 0.55f + 0.25f * (((tries + homeTries) % 20) / 20.0f);
			char waitMsg[96];
			if (placeId <= 0)
				sprintf_s(waitMsg, "Please join a game");
			else
				sprintf_s(waitMsg, "Waiting — join %s", Games::SupportedListShort());
			SetLoad(pulse, waitMsg);
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

		SetLoad(0.72f, "Waiting for character");
		for (int charWait = 0; charWait < 120 && !IsInActiveGame(); charWait++) {
			Globals::RefreshServices();
			PlayerCache::EnsureCacheWorker();
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		char readyMsg[96];
		sprintf_s(readyMsg, "Loaded %s", Games::Name());

		const float minHold = (std::max)(2.5f, variables::Loading::minSeconds);
		auto holdStart = std::chrono::steady_clock::now();
		float elapsed = 0.f;
		while (elapsed < minHold) {
			elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - holdStart).count();
			float t = elapsed / minHold;
			if (t > 1.f) t = 1.f;
			float p = 0.88f + 0.12f * t;
			SetLoad(p, readyMsg);
			std::this_thread::sleep_for(std::chrono::milliseconds(40));
		}

		Globals::RefreshServices();
		DebugLog::Write("Attach complete: %s players=%llX camera=%llX",
			Games::Name(),
			(unsigned long long)Globals::players.Addr,
			(unsigned long long)Globals::camera.Addr);

		{
			auto localChar = PlayerCache::ResolveCharacter(Globals::localPlayer);
			auto localHrp = PlayerCache::FindRootPart(localChar);
			auto pos = localHrp.Addr ? localHrp.GetPos() : RBX::Vec3{};
			DebugLog::Write("Self-test: lp=%s char=%llX hrp=%llX pos=(%.1f,%.1f,%.1f) workspace=%s",
				Globals::localPlayer.GetName().c_str(),
				(unsigned long long)localChar.Addr,
				(unsigned long long)localHrp.Addr,
				pos.X, pos.Y, pos.Z,
				Globals::workspace.GetName().c_str());
		}

		SetLoad(1.0f, readyMsg);
		std::this_thread::sleep_for(std::chrono::milliseconds(280));
		return true;
	}
}

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
	FreeConsole();

	SetUnhandledExceptionFilter([](EXCEPTION_POINTERS* ep) -> LONG {
		char detail[160]{};
		DWORD code = ep && ep->ExceptionRecord ? ep->ExceptionRecord->ExceptionCode : 0;
		sprintf_s(detail, "Unhandled exception 0x%08lX", code);
		Telemetry::ReportError("Crash", detail);
		Sleep(400);
		TerminateProcess(GetCurrentProcess(), 1);
		return EXCEPTION_EXECUTE_HANDLER;
	});

	OverlayWindow overlay;
	if (!overlay.Initialize())
		return 1;

	ConfigIO::Load();

	if (!RobloxIsRunning()) {
		ShowTimedExitPrompt(overlay, "Please join a game");
		overlay.Cleanup();
		return 0;
	}

	FrontierPresence::SyncEnabled(variables::Misc::discordRpc);
	FrontierPresence::StartWorker();

	std::string licenseErr;
	if (!SessionGate::ValidateSession(licenseErr)) {
		ShowTimedExitPrompt(overlay, licenseErr.c_str());
		overlay.Cleanup();
		return 0;
	}

	std::atomic<bool> attachSucceeded{ false };
	std::thread attachThread;
	bool attachStarted = false;

	std::thread tpThread;
	std::thread locThread;
	std::thread animThread;
	bool workersStarted = false;
	int frameCounter = 0;

	while (Globals::running) {
		// Throttle expensive process scans ΓÇö window check is cheap
		static auto lastAliveCheck = std::chrono::steady_clock::now() - std::chrono::seconds(2);
		auto aliveNow = std::chrono::steady_clock::now();
		if (std::chrono::duration<float>(aliveNow - lastAliveCheck).count() >= 0.75f) {
			lastAliveCheck = aliveNow;
			if (!WindowManager::FindRobloxHwnd() && !memory->find_process_id(app)) {
				if (!attachSucceeded.load()) {
					if (!variables::Loading::failed)
						FailAttach("Please join a game");
				} else {
					break;
				}
			}
		} else if (!WindowManager::IsRobloxOpen() && memory->get_process_id() == 0) {
			if (!WindowManager::FindRobloxHwnd() && !memory->find_process_id(app)) {
				if (!attachSucceeded.load()) {
					if (!variables::Loading::failed)
						FailAttach("Please join a game");
				} else {
					break;
				}
			}
		}

		if (variables::Loading::failed) {
			static auto failedSince = std::chrono::steady_clock::time_point{};
			static bool failedTimerActive = false;
			if (!failedTimerActive) {
				failedSince = std::chrono::steady_clock::now();
				failedTimerActive = true;
			}

			overlay.BeginFrame();
			overlay.RenderLoading();
			overlay.EndFrame();

			const float failedFor = std::chrono::duration<float>(
				std::chrono::steady_clock::now() - failedSince).count();
			if (failedFor >= 4.f || (GetAsyncKeyState(VK_ESCAPE) & 1))
				break;
			continue;
		}

		if (!attachStarted) {
			attachStarted = true;
			variables::Loading::active = true;
			SetLoad(0.0f, "Loading session");
			attachThread = std::thread([&]() {
				if (AttachGame()) {
					attachSucceeded.store(true);
					variables::Loading::active = false;
					variables::pendingMenuReveal.store(true);
					Visibility::EnsureWorker();
					Visibility::RequestRebuild();
					if (variables::Misc::discordRpc) {
						FrontierPresence::SyncEnabled(true);
						FrontierPresence::UpdateFromSession();
					}
				}
			});
		}

		if (attachSucceeded.load() && !variables::Loading::active && !workersStarted) {
			tpThread = std::thread(Core::tp_handler::thread);
			locThread = std::thread(localThread);
			animThread = std::thread(animation_changer);
			workersStarted = true;
		}

		int menuVk = variables::Misc::menuVk;
		const bool gameFocused = WindowManager::IsRobloxFocused();
		static bool wasMenuUi = false;
		const bool menuUi = variables::menuOpen || variables::Misc::floatingPanelOpen;
		if (wasMenuUi && !menuUi)
			Aimbot::ReleaseCursorClip();
		wasMenuUi = menuUi;
		static bool wasGameFocused = false;
		if (wasGameFocused && !gameFocused)
			Aimbot::ReleaseCursorClip();
		wasGameFocused = gameFocused;

		if (gameFocused && ((GetAsyncKeyState(menuVk) & 1) || (GetAsyncKeyState(VK_INSERT) & 1) || (GetAsyncKeyState(VK_RCONTROL) & 1))) {
			if (!variables::Loading::active && !Telemetry::consentPending.load()) {
				if (variables::Theme::useFloatingHeader)
					variables::Misc::floatingPanelOpen = !variables::Misc::floatingPanelOpen;
				else
					variables::menuOpen = !variables::menuOpen;
			}
		}
		if (gameFocused && variables::Misc::panicKey && (GetAsyncKeyState(variables::Misc::panicVk) & 1))
			PanicDisableAll();

		overlay.BeginFrame();

		if (variables::Loading::active) {
			overlay.RenderLoading();
			overlay.EndFrame();
			continue;
		}

		if (variables::pendingMenuReveal.exchange(false)) {
			variables::Misc::menuAnim = 0.f;
			variables::Misc::tabContentAnim = 1.f;
			variables::Misc::floatingPanelAnim = 0.f;
			variables::Misc::headerIntro = 0.f;
			if (variables::Theme::useFloatingHeader) {
				variables::menuOpen = false;
				variables::Misc::floatingPanelOpen = true;
			} else {
				variables::menuOpen = true;
			}
		}

		static bool lastDiscordRpc = variables::Misc::discordRpc;
		if (variables::Misc::discordRpc != lastDiscordRpc) {
			lastDiscordRpc = variables::Misc::discordRpc;
			FrontierPresence::SyncEnabled(variables::Misc::discordRpc);
			if (variables::Misc::discordRpc)
				FrontierPresence::RequestRefresh();
		}

		if (variables::Misc::discordRpc && attachSucceeded.load()) {
			static auto lastPresence = std::chrono::steady_clock::now();
			auto nowRpc = std::chrono::steady_clock::now();
			if (std::chrono::duration<float>(nowRpc - lastPresence).count() >= 3.f) {
				lastPresence = nowRpc;
				FrontierPresence::UpdateFromSession();
			}
		}

		static bool telemetryReady = false;
		if (!telemetryReady) {
			Telemetry::OnReady();
			telemetryReady = true;
		}

		overlay.RenderMenu();
		overlay.RenderSpotifyMini();
		ImDrawList* drawList = ImGui::GetBackgroundDrawList();
		overlay.render(drawList);

		if (Globals::renderEngine.Addr != 0 && IsGameSessionReady()) {
			Globals::RefreshServices();

			PlayerCache::EnsureCacheWorker();
			PlayerCache::refreshLivePositions();

			auto viewMatrix = Globals::renderEngine.GetViewMat();
			if (Globals::renderEngine.Addr) {
				float rw = memory->read<float>(Globals::renderEngine.Addr + Offsets::VisualEngine::Dimensions);
				float rh = memory->read<float>(Globals::renderEngine.Addr + Offsets::VisualEngine::Dimensions + 0x4);
				W2S::SetRenderDimensions(rw, rh);
			}
			if (!Scanner::ViewMatrixLooksValid(viewMatrix)) {
				const auto anchors = Scanner::ResolveAnchors();
				if (anchors.success) {
					Globals::dataModel = RBX::RbxInstance(anchors.dataModel);
					Globals::renderEngine = RBX::RenderEngine(anchors.visualEngine);
					Globals::RefreshServices();
					viewMatrix = Globals::renderEngine.GetViewMat();
					DebugLog::Write("Render: view matrix re-resolved");
				}
			}

			auto players = PlayerCache::snapshotPlayers();
			static int lastLoggedCount = -1;
			if ((int)players.size() != lastLoggedCount) {
				lastLoggedCount = (int)players.size();
				DebugLog::Write("Cache: %d players  camera=%llX  lp=%llX",
					lastLoggedCount,
					(unsigned long long)Globals::camera.Addr,
					(unsigned long long)Globals::localPlayer.Addr);
			}

			Aimbot::RunAimbot(viewMatrix, players);
			Aimbot::RunSilentFireAssist(viewMatrix, players);
			Aimbot::RunMagicBulletAssist(viewMatrix, players);
			Aimbot::RunTriggerbot(viewMatrix, players);
			Aimbot::RunMeleeAura(players);

			if (variables::ESP::enabled ||
				variables::ESP::names ||
				variables::ESP::healthText ||
				variables::ESP::distance ||
				variables::ESP::equippedItem ||
				variables::ESP::skeleton ||
				variables::ESP::chamsEnabled ||
				variables::ESP::wireframePlayers ||
				variables::ESP::oofArrows)
				Visuals::RenderESP(drawList, viewMatrix, players);

			if (IsInActiveGame()) {
				CombatFx::Tick(players, viewMatrix);
				CombatFx::Draw(drawList, ImGui::GetIO().DeltaTime);
			} else {
				Aimbot::lockedPlayerAddr = 0;
			}

			// Overlay extras
			if (variables::Misc::enemyCounter) {
				int enemies = 0;
				for (auto& p : players) {
					if (p.health > 0) enemies++;
				}
				char buf[48];
				sprintf_s(buf, "Enemies %d", enemies);
				drawList->AddText(ImVec2(18, 52), IM_COL32(240, 240, 245, 230), buf);
			}
			if (variables::Misc::targetHud && Aimbot::lockedPlayerAddr) {
				for (auto& p : players) {
					if (p.playerAddr != Aimbot::lockedPlayerAddr) continue;
					char buf[96];
					sprintf_s(buf, "Target  %s  %.0f HP", p.name.c_str(), p.health);
					drawList->AddRectFilled(ImVec2(14, 72), ImVec2(14 + 220, 98), IM_COL32(10, 10, 12, 180), 6.f);
					drawList->AddText(ImVec2(22, 78), IM_COL32(240, 240, 245, 255), buf);
					break;
				}
			}
		}

		overlay.EndFrame();
	}

	Globals::running = false;
	FrontierPresence::Stop();
	if (attachThread.joinable()) attachThread.join();
	if (tpThread.joinable()) tpThread.join();
	if (locThread.joinable()) locThread.join();
	if (animThread.joinable()) animThread.join();
	overlay.Cleanup();
	return 0;
}
